use game::GameState;
use message::{Message, MessageType};
use rand::Rng;

use std::{
    collections::{HashMap, HashSet, VecDeque},
    net::{TcpListener, TcpStream},
    sync::{Arc, Mutex},
    thread::{self, JoinHandle},
    time::Duration,
};

mod game;
mod message;

fn main() {
    let address = "127.0.0.1:10101";
    let listener = TcpListener::bind(address).unwrap();
    println!("Listening on {address} for incoming connections");

    let user_to_write_deq = Arc::new(Mutex::new(
        HashMap::<i32, Arc<Mutex<VecDeque<Message>>>>::new(),
    ));
    let user_to_read_deq = Arc::new(Mutex::new(
        HashMap::<i32, Arc<Mutex<VecDeque<Message>>>>::new(),
    ));
    let users = Arc::new(Mutex::new(HashSet::<i32>::new()));

    spawn_main_thread(
        users.clone(),
        user_to_read_deq.clone(),
        user_to_write_deq.clone(),
    );

    for stream in listener.incoming() {
        let write_deq = Arc::new(Mutex::new(VecDeque::<Message>::new()));
        let read_deq = Arc::new(Mutex::new(VecDeque::<Message>::new()));

        let user_id = insert_user(&mut users.lock().unwrap());

        user_to_write_deq
            .lock()
            .unwrap()
            .insert(user_id, write_deq.clone());
        user_to_read_deq
            .lock()
            .unwrap()
            .insert(user_id, read_deq.clone());

        let actual_stream = stream.unwrap();

        spawn_write_thread(
            actual_stream.try_clone().unwrap(),
            user_id,
            write_deq.clone(),
            user_to_write_deq.clone(),
        );
        spawn_read_thread(
            actual_stream.try_clone().unwrap(),
            user_id,
            read_deq.clone(),
            user_to_read_deq.clone(),
            write_deq.clone(),
        );
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

fn spawn_write_thread(
    mut stream: TcpStream,
    user_id: i32,
    write_deq: Arc<Mutex<VecDeque<Message>>>,
    user_to_write_deq: Arc<Mutex<HashMap<i32, Arc<Mutex<VecDeque<Message>>>>>>,
) -> JoinHandle<()> {
    thread::spawn(move || {
        loop {
            match write_deq.lock().unwrap().pop_front() {
                Some(message) => match message.write_to(&mut stream) {
                    Ok(_) => continue,
                    Err(_) => break,
                },
                None => continue,
            };
        }
        user_to_write_deq.lock().unwrap().remove(&user_id).unwrap();
        println!("write thread finished for {user_id}");
    })
}

fn spawn_read_thread(
    mut stream: TcpStream,
    user_id: i32,
    read_deq: Arc<Mutex<VecDeque<Message>>>,
    user_to_read_deq: Arc<Mutex<HashMap<i32, Arc<Mutex<VecDeque<Message>>>>>>,
    write_deq: Arc<Mutex<VecDeque<Message>>>,
) -> JoinHandle<()> {
    thread::spawn(move || {
        for message_result in Message::iter(&mut stream) {
            match message_result {
                Ok(message) => read_deq.lock().unwrap().push_back(message),
                Err(_) => break,
            }
        }
        user_to_read_deq.lock().unwrap().remove(&user_id).unwrap();
        write_deq
            .lock()
            .unwrap()
            .push_back(Message::new(MessageType::ConnectionRejected));
        println!("read thread finished for {user_id}");
    })
}

fn spawn_main_thread(
    users: Arc<Mutex<HashSet<i32>>>,
    user_to_read_deq: Arc<Mutex<HashMap<i32, Arc<Mutex<VecDeque<Message>>>>>>,
    user_to_write_deq: Arc<Mutex<HashMap<i32, Arc<Mutex<VecDeque<Message>>>>>>,
) {
    thread::spawn(move || {
        let rate = Duration::from_secs_f64(1 as f64 / 30 as f64);

        let mut game = GameState::new(users, user_to_write_deq, user_to_read_deq);
        let mut start = std::time::Instant::now();
        let mut start_io = start;

        loop {
            let new_start = std::time::Instant::now();
            let elapsed = new_start.duration_since(start);
            game.elapsed(elapsed);

            let elapsed_io = new_start.duration_since(start_io);
            if elapsed_io > rate {
                start_io = new_start;
                game.pull_updates();
                game.publish_updates();
            }
            start = new_start;
        }
    });
}
