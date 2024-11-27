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

pub struct Users {
    user_to_write_sender: HashMap<i32, mpsc::Sender<Message>>,
    user_to_read_receiver: HashMap<i32, mpsc::Receiver<Message>>,
    users: HashSet<i32>,
}

impl Users {
    pub fn new() -> Self {
        Users {
            user_to_write_sender: HashMap::new(),
            user_to_read_receiver: HashMap::new(),
            users: HashSet::new(),
        }
    }
}

fn main() {
    let address = "127.0.0.1:10101";
    let listener = TcpListener::bind(address).unwrap();
    println!("Listening on {address} for incoming connections");

    let users = Arc::new(Mutex::new(Users::new()));

    let pause_accepting_users: Arc<Mutex<bool>> = Arc::new(Mutex::new(false));
    let stop_accepting_users: Arc<Mutex<bool>> = Arc::new(Mutex::new(false));

    spawn_listening_thread(
        listener,
        users.clone(),
        pause_accepting_users.clone(),
        stop_accepting_users.clone(),
    );

    let rate = Duration::from_secs_f64(1.0 / 30.0);

    let mut game = Game::new(
        users,
        Box::new(JustCreatedGame::new(
            pause_accepting_users,
            stop_accepting_users,
        )),
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

fn spawn_listening_thread(
    listener: TcpListener,
    users: Arc<Mutex<Users>>,
    pause_accepting_users: Arc<Mutex<bool>>,
    stop_accepting_users: Arc<Mutex<bool>>,
) {
    thread::spawn(move || {
        for stream in listener.incoming() {
            while *pause_accepting_users.lock().unwrap() {
                if *stop_accepting_users.lock().unwrap() {
                    println!("stopping accepting new users");
                    break;
                }
            }

            let (write_sender, write_receiver) = mpsc::channel();
            let (read_sender, read_receiver) = mpsc::channel();

            let user_id = add_user_to_users(&users, write_sender, read_receiver);

            let actual_stream = stream.unwrap();

            spawn_write_thread(actual_stream.try_clone().unwrap(), user_id, write_receiver);
            spawn_read_thread(
                actual_stream.try_clone().unwrap(),
                user_id,
                read_sender,
                users.clone(),
            );
        }
    });
}

fn add_user_to_users(
    users: &Arc<Mutex<Users>>,
    write_sender: mpsc::Sender<Message>,
    read_receiver: mpsc::Receiver<Message>,
) -> i32 {
    let mut locked_users = users.lock().unwrap();

    let user_id = insert_user(&mut locked_users.users);

    locked_users
        .user_to_write_sender
        .insert(user_id, write_sender);
    locked_users
        .user_to_read_receiver
        .insert(user_id, read_receiver);

    user_id
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
    users: Arc<Mutex<Users>>,
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

        let mut locked_users = users.lock().unwrap();
        locked_users.user_to_write_sender.remove(&user_id).unwrap();
        locked_users.user_to_read_receiver.remove(&user_id).unwrap();
        locked_users.users.remove(&user_id);
        println!("read thread finished for {user_id}");
    })
}
