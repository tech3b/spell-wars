use message::Message;
use rand::Rng;

use std::{
    io::Write,
    net::{TcpListener, TcpStream},
    thread,
    time::Duration,
};

mod message;

fn respond(mut stream: TcpStream) -> std::io::Result<()> {
    let mut writing_stream = stream.try_clone()?;

    for message_result in message_iterator(&mut stream) {
        match message_result
            .and_then(|mut message| process_message(&mut message, &mut writing_stream))
        {
            Ok(_) => continue,
            err => return err,
        }
    }
    Ok(())
}

fn process_message(message: &mut Message, writing_stream: &mut TcpStream) -> std::io::Result<()> {
    match message.message_type() {
        message::MessageType::ConnectionRequested => {
            let some_client_number: i32 = message.pop().unwrap_or(0);

            println!("Some client connected: {some_client_number}");

            let random = rand::thread_rng().gen_range(1..=100);

            println!("Sending client number: {random}");

            message::Message::new(message::MessageType::ConnectionAccepted)
                .push(&random)
                .write_to(writing_stream)?;
            create_stub_message().write_to(writing_stream)?;

            writing_stream.flush()?;
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

            thread::sleep(Duration::from_secs(2));

            create_stub_message().write_to(writing_stream)?;

            writing_stream.flush()?;
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
