use rand::Rng;

use std::{
    io::Write,
    net::{TcpListener, TcpStream}, thread, time::Duration,
};

mod message;

fn respond(mut stream: TcpStream) {
    let mut writing_stream = stream.try_clone().unwrap();

    for message_result in message_iterator(&mut stream) {
        match message_result {
            Ok(mut message) => match message.message_type() {
                message::MessageType::ConnectionRequested => {
                    let random = rand::thread_rng().gen_range(1..=100);
                    let mut everyone_is_welcome =
                        message::Message::new(message::MessageType::ConnectionAccepted);

                    everyone_is_welcome
                        .push_string(&format!("Welcome! You're our {random} customer today!"))
                        .write_to(&mut writing_stream)
                        .unwrap();
                    writing_stream.flush().unwrap();

                    let mut stub_message = message::Message::new(message::MessageType::StubMessage);

                    stub_message
                        .push_string(&format!("Yoba, eto ti?"))
                        .write_to(&mut writing_stream)
                        .unwrap();
                    writing_stream.flush().unwrap();
                }
                message::MessageType::StubMessage => {
                    let maybe_request: String = message.pop_string();
                    println!("stub message got: {maybe_request}");

                    thread::sleep(Duration::from_secs(2));

                    let mut stub_message = message::Message::new(message::MessageType::StubMessage);

                    stub_message
                        .push_string(&format!("Yoba, eto ti?"))
                        .write_to(&mut writing_stream)
                        .unwrap();
                    writing_stream.flush().unwrap();
                }
                _ => todo!(),
            },
            Err(err) => {
                panic!("{err}")
            }
        }
    }
}

fn message_iterator<'a>(reader: &'a mut TcpStream) -> message::MessageIterator<'a, TcpStream> {
    message::MessageIterator { reader }
}

fn main() {
    let address = "127.0.0.1:10101";
    let listener = TcpListener::bind(address).unwrap();
    println!("Listening on {address} for incoming connections");

    for stream in listener.incoming() {
        let stream = stream.unwrap();
        respond(stream);
    }
}
