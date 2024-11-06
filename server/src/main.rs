use message::Message;
use rand::Rng;

use std::{
    collections::{HashMap, HashSet, VecDeque},
    net::TcpListener,
    sync::{Arc, Mutex},
    thread,
    time::Duration,
};

mod message;

fn process_message(
    message: &mut Message,
    user: i32,
    write_queue_mutex: &Arc<Mutex<VecDeque<Message>>>,
) -> std::io::Result<()> {
    match message.message_type() {
        message::MessageType::ConnectionRequested => {
            let some_client_number: i32 = message.pop().unwrap_or(0);

            println!("Some client connected: {some_client_number}");
            println!("Sending client number: {user}");

            let mut queue = write_queue_mutex.lock().unwrap();
            let mut accepted_message =
                message::Message::new(message::MessageType::ConnectionAccepted);
            accepted_message.push(&user);
            queue.push_back(accepted_message);
            queue.push_back(create_stub_message());

            Ok(())
        }
        message::MessageType::StubMessage => {
            let s2 = message.pop_string().ok_or(std::io::Error::new(
                std::io::ErrorKind::NotFound,
                "Can't pop string",
            ))?;
            let s1 = message.pop_string().ok_or(std::io::Error::new(
                std::io::ErrorKind::NotFound,
                "Can't pop string",
            ))?;

            println!("s1: {s1}");
            println!("s2: {s2}");

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
    let mut stub_message = message::Message::new(message::MessageType::StubMessage);
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

    let users_in_main = users.clone();
    let user_to_write_deq_in_main = user_to_write_deq.clone();
    let user_to_read_deq_in_main = user_to_read_deq.clone();
    thread::spawn(move || loop {
        println!("new iteration");
        for user in users_in_main.lock().unwrap().iter() {
            Option::zip(
                user_to_read_deq_in_main.lock().unwrap().get(user),
                user_to_write_deq_in_main.lock().unwrap().get(user),
            )
            .and_then(|(read_deq, write_deq)| {
                read_deq
                    .lock()
                    .unwrap()
                    .pop_front()
                    .map(|mut message| process_message(&mut message, *user, write_deq).unwrap())
            });
        }
        thread::sleep(Duration::from_secs(2));
    });

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
        let mut write_stream = actual_stream.try_clone().unwrap();
        let mut read_stream = actual_stream.try_clone().unwrap();

        let local_write_deq = write_deq.clone();
        let local_read_deq = read_deq.clone();
        let user_to_write_deq_in_write = user_to_write_deq.clone();
        let user_to_read_deq_in_read = user_to_read_deq.clone();

        let local_write_in_read_deq = write_deq.clone();
        thread::spawn(move || {
            loop {
                match local_write_deq.lock().unwrap().pop_front() {
                    Some(message) => match message.write_to(&mut write_stream) {
                        Ok(_) => continue,
                        Err(_) => break,
                    },
                    None => continue,
                };
            }
            user_to_write_deq_in_write
                .lock()
                .unwrap()
                .remove(&user_id)
                .unwrap();
            println!("write thread finished for {user_id}");
        });

        thread::spawn(move || {
            for message_result in Message::iter(&mut read_stream) {
                match message_result {
                    Ok(message) => local_read_deq.lock().unwrap().push_back(message),
                    Err(_) => break,
                }
            }
            user_to_read_deq_in_read
                .lock()
                .unwrap()
                .remove(&user_id)
                .unwrap();
            local_write_in_read_deq
                .lock()
                .unwrap()
                .push_back(Message::new(message::MessageType::ConnectionRejected));
            println!("read thread finished for {user_id}");
        });
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
