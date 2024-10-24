use rand::Rng;

use std::{
    io::Write,
    net::{TcpListener, TcpStream},
};

mod message;

fn respond(mut stream: TcpStream) {
    let mut writing_stream = stream.try_clone().unwrap();

    let random = rand::thread_rng().gen_range(1..=100);
    let mut everyone_is_welcome = message::Message::new(message::MessageType::ConnectionAccepted);

    writing_stream
        .write_all(
            &everyone_is_welcome
                .push(format!("Welcome! You're our {random} customer today!\n"))
                .as_bytes(),
        )
        .unwrap();
    writing_stream.flush().unwrap();

    for message_result in message_iterator(&mut stream) {
        match message_result {
            Ok(mut message) => {
                let maybe_request: Option<String> = message.pop();

                if let Some(request) = maybe_request {
                    let (response, end_of_conversation) = match request.as_str() {
                        "q" => {
                            let mut message =
                                message::Message::new(message::MessageType::ConnectionRejected);
                            message.push(format!(
                                "End of connection for the client with id={random}\n"
                            ));
                            (message, true)
                        }
                        _ => {
                            let mut message =
                                message::Message::new(message::MessageType::StubMessage);
                            message.push("Just keeping the conversation going\n");
                            (message, false)
                        }
                    };

                    writing_stream.write_all(&response.as_bytes()).unwrap();
                    writing_stream.flush().unwrap();

                    if end_of_conversation {
                        break;
                    }
                }
            }
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
