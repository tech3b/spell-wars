use core::str;
use rand::Rng;
use std::{
    io::{Read, Write},
    net::{TcpListener, TcpStream},
};

fn respond(mut stream: TcpStream) {
    let mut end_of_conversation = false;

    loop {
        let mut buffer = [0; 1024];
        let mut message = String::new();

        loop {
            let bytes_read = stream.read(&mut buffer).unwrap();

            if bytes_read == 0 {
                break;
            }

            message.push_str(&str::from_utf8(&buffer[..bytes_read]).unwrap());

            if message.ends_with("\n") {
                break;
            }
        }
        let request = message.trim();

        let response = match request {
            "ALLOU?" => "psssht pssssht nichego ne slyshno\n",
            "ALLOU YOBA ETO TI?" => "YOBI NET DOMA\n",
            "YOBI POZOVITE POZHALUSTA" => "YOBA ETO TI!\n",
            "KLADU TRUBKU" => {
                end_of_conversation = true;
                "NU I POZHALUSTA!\n"
            }
            _ => {
                let random = rand::thread_rng().gen_range(1..=2);

                if random % 2 == 0 {
                    "psssht JA TEBE POMOLCHY SHENOK!\n"
                } else {
                    "ESHE RAZ POZVONISH - S POLITSIEY BUDESH RAZGOVARIVAT!\n"
                }
            }
        };

        stream.write_all(response.as_bytes()).unwrap();
        stream.flush().unwrap();

        if end_of_conversation {
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
