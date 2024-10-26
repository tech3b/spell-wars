use message::Message;
use rand::Rng;

use std::{
    io::Write,
    net::{TcpListener, TcpStream}, thread, time::Duration,
};

mod message;

fn respond(mut stream: TcpStream) -> std::io::Result<()> {
    let mut writing_stream = stream.try_clone()?;

    for message_result in message_iterator(&mut stream) {

        match message_result.and_then(|mut message| process_message(&mut message, &mut writing_stream)) {
            Ok(_) => continue,
            err => return err,
        }
    }
    Ok(())
}

fn process_message(message: &mut Message, writing_stream: &mut TcpStream) -> std::io::Result<()> {
    match message.message_type() {
        message::MessageType::ConnectionRequested => {
            let random = rand::thread_rng().gen_range(1..=100);
            let mut everyone_is_welcome =
                message::Message::new(message::MessageType::ConnectionAccepted);

            everyone_is_welcome
                .push_string(&format!("Welcome! You're our {random} customer today!"))
                .write_to(writing_stream)?;
            writing_stream.flush()?;

            let mut stub_message = message::Message::new(message::MessageType::StubMessage);

            stub_message
                .push_string(&format!("Yoba, eto ti?"))
                .write_to(writing_stream)?;
            writing_stream.flush()?;
            Ok(())
        }
        message::MessageType::StubMessage => {
            let maybe_request: String = message.pop_string();
            println!("stub message got: {maybe_request}");

            thread::sleep(Duration::from_secs(2));

            let mut stub_message = message::Message::new(message::MessageType::StubMessage);

            stub_message
                .push_string(&format!("Yoba, eto ti?"))
                .write_to(writing_stream)?;
            writing_stream.flush()?;
            Ok(())
        }
        _ => todo!(),
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
        match respond(stream) {
            Err(err) => println!("{err:?}"),
            _ => continue,
        }
    }
}
