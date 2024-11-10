use game::{state::just_created::JustCreatedGame, Game};
use message::Message;
use rand::Rng;

use std::{
    collections::{HashMap, HashSet},
    net::{TcpListener, TcpStream},
    sync::{Arc, Mutex},
    thread::{self, JoinHandle},
    time::Duration,
};

use std::sync::mpsc;

pub mod game;
pub mod message;

fn main() {
    let address = "127.0.0.1:10101";
    let listener = TcpListener::bind(address).unwrap();
    println!("Listening on {address} for incoming connections");

    let user_to_write_sender = Arc::new(Mutex::new(HashMap::<i32, mpsc::Sender<Message>>::new()));
    let user_to_read_receiver =
        Arc::new(Mutex::new(HashMap::<i32, mpsc::Receiver<Message>>::new()));
    let users = Arc::new(Mutex::new(HashSet::<i32>::new()));

    let stop_accepting_users: Arc<Mutex<bool>> = Arc::new(Mutex::new(false));

    spawn_listening_thread(
        listener,
        users.clone(),
        user_to_write_sender.clone(),
        user_to_read_receiver.clone(),
        stop_accepting_users.clone(),
    );

    let rate = Duration::from_secs_f64(1.0 / 30.0);

    let mut game = Game::new(
        user_to_write_sender,
        user_to_read_receiver,
        Box::new(JustCreatedGame::new(users, stop_accepting_users)),
    );
    let mut start = std::time::Instant::now();
    let mut start_io = start;

    loop {
        let new_start = std::time::Instant::now();
        let elapsed = new_start.duration_since(start);
        game.elapsed(elapsed);

        let elapsed_io = new_start.duration_since(start_io);
        if elapsed_io > rate {
            start_io = new_start;
            game.io_updates();
        }
        start = new_start;
    }
}

fn insert_user(users: &mut HashSet<i32>) -> i32 {
    loop {
        let random = rand::thread_rng().gen_range(1..=1000);
        if users.contains(&random) {
            continue;
        } else {
            users.insert(random);
            return random;
        }
    }
}

fn spawn_listening_thread(
    listener: TcpListener,
    users: Arc<Mutex<HashSet<i32>>>,
    user_to_write_sender: Arc<Mutex<HashMap<i32, mpsc::Sender<Message>>>>,
    user_to_read_receiver: Arc<Mutex<HashMap<i32, mpsc::Receiver<Message>>>>,
    stop_accepting_users: Arc<Mutex<bool>>,
) {
    thread::spawn(move || {
        for stream in listener.incoming() {
            let should_stop = stop_accepting_users.lock().unwrap();

            if *should_stop {
                println!("stopping accepting new users");
                break;
            }

            let (write_sender, write_receiver) = mpsc::channel();
            let (read_sender, read_receiver) = mpsc::channel();

            let user_id = insert_user(&mut users.lock().unwrap());

            user_to_write_sender
                .lock()
                .unwrap()
                .insert(user_id, write_sender);
            user_to_read_receiver
                .lock()
                .unwrap()
                .insert(user_id, read_receiver);

            let actual_stream = stream.unwrap();

            spawn_write_thread(actual_stream.try_clone().unwrap(), user_id, write_receiver);
            spawn_read_thread(
                actual_stream.try_clone().unwrap(),
                user_id,
                read_sender,
                user_to_write_sender.clone(),
                user_to_read_receiver.clone(),
            );
        }
    });
}

fn spawn_write_thread(
    mut stream: TcpStream,
    user_id: i32,
    write_receiver: mpsc::Receiver<Message>,
) -> JoinHandle<()> {
    thread::spawn(move || {
        for message in write_receiver.iter() {
            match message.write_to(&mut stream) {
                Ok(_) => continue,
                Err(_) => break,
            };
        }
        println!("write thread finished for {user_id}");
    })
}

fn spawn_read_thread(
    mut stream: TcpStream,
    user_id: i32,
    read_sender: mpsc::Sender<Message>,
    user_to_write_sender: Arc<Mutex<HashMap<i32, mpsc::Sender<Message>>>>,
    user_to_read_receiver: Arc<Mutex<HashMap<i32, mpsc::Receiver<Message>>>>,
) -> JoinHandle<()> {
    thread::spawn(move || {
        for message_result in Message::iter(&mut stream) {
            match message_result {
                Ok(message) => {
                    read_sender.send(message).unwrap();
                }
                Err(_) => break,
            }
        }
        user_to_write_sender
            .lock()
            .unwrap()
            .remove(&user_id)
            .unwrap();
        user_to_read_receiver
            .lock()
            .unwrap()
            .remove(&user_id)
            .unwrap();
        println!("read thread finished for {user_id}");
    })
}
