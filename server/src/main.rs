use rand::Rng;

use std::{
    io::{BufRead, BufReader, Write},
    net::{TcpListener, TcpStream},
};

mod message;

fn respond(mut stream: TcpStream) {
    let mut reader = BufReader::new(stream.try_clone().unwrap());

    let random = rand::thread_rng().gen_range(1..=100);
    let mut everyone_is_welcome = message::Message::new(message::MessageType::ConnectionAccepted);

    stream
        .write_all(
            &everyone_is_welcome
                .push(format!("Welcome! You're our {random} customer today!\n"))
                .as_bytes(),
        )
        .unwrap();
    stream.flush().unwrap();

    loop {
        let mut message = String::new();
        let bytes_read = reader.read_line(&mut message).unwrap();

        if bytes_read > 0 {
            let request = message.trim();

            let (response, end_of_conversation) = match request {
                "q" => {
                    let mut message =
                        message::Message::new(message::MessageType::ConnectionRejected);
                    message.push(format!(
                        "End of connection for the client with id={random}\n"
                    ));
                    (message, true)
                }
                _ => {
                    let mut message = message::Message::new(message::MessageType::StubMessage);
                    message.push("Just keeping the conversation going\n");
                    (message, false)
                }
            };

            stream.write_all(&response.as_bytes()).unwrap();
            stream.flush().unwrap();

            if end_of_conversation {
                break;
            }
        } else {
            break;
        }
    }
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
