use message::{Message, MessageType};
use rand::Rng;

use std::{
    collections::{HashMap, HashSet, VecDeque},
    net::{TcpListener, TcpStream},
    sync::{Arc, Mutex},
    thread::{self, JoinHandle},
    time::Duration,
};

mod message;

fn process_message(
    message: &mut Message,
    user: i32,
    write_queue_mutex: &Mutex<VecDeque<Message>>,
) -> Result<(), String> {
    match message.message_type() {
        MessageType::ConnectionRequested => {
            let some_client_number: i32 = message.pop().unwrap_or(0);

            println!("Some client connected: {some_client_number}");
            println!("Sending client number: {user}");

            let mut queue = write_queue_mutex.lock().unwrap();
            let mut accepted_message = Message::new(MessageType::ConnectionAccepted);
            accepted_message.push(&user);
            queue.push_back(accepted_message);
            queue.push_back(create_stub_message());

            Ok(())
        }
        MessageType::StubMessage => {
            let s2 = message
                .pop_string()
                .ok_or(String::from("Can't pop string"))?;
            let s1 = message
                .pop_string()
                .ok_or(String::from("Can't pop string"))?;

            println!("message from {user}: s1: {s1}, s2: {s2}");

            write_queue_mutex
                .lock()
                .unwrap()
                .push_back(create_stub_message());

            Ok(())
        }
        _ => todo!(),
    }
}

fn create_stub_message() -> Message {
    let mut stub_message = Message::new(MessageType::StubMessage);
    stub_message.push_string("Hello from the other siiiiiiiiiide!");
    stub_message.push_string("At least I can say that I've triiiiiiiiiiied!");
    stub_message
}

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
    thread::spawn(move || loop {
        println!("new iteration");
        for user in users.lock().unwrap().iter() {
            Option::zip(
                deq_by_user(&user_to_read_deq, *user),
                deq_by_user(&user_to_write_deq, *user),
            )
            .and_then(|(read_deq, write_deq)| {
                message_from_deq(&read_deq)
                    .map(|mut message| process_message(&mut message, *user, &write_deq).unwrap())
            });
        }
        thread::sleep(Duration::from_secs(2));
    });
}

fn deq_by_user(
    user_to_deq: &Mutex<HashMap<i32, Arc<Mutex<VecDeque<Message>>>>>,
    user: i32,
) -> Option<Arc<Mutex<VecDeque<Message>>>> {
    user_to_deq.lock().unwrap().get(&user).map(|a| a.clone())
}

fn message_from_deq(deq: &Mutex<VecDeque<Message>>) -> Option<Message> {
    deq.lock().unwrap().pop_front()
}
