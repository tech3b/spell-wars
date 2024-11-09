use std::{
    collections::{HashMap, HashSet},
    sync::{mpsc, Arc, Mutex},
    time::Duration,
};

use crate::message::{Message, MessageType};

pub struct GameState {
    users: Arc<Mutex<HashSet<i32>>>,
    user_to_sender: Arc<Mutex<HashMap<i32, mpsc::Sender<Message>>>>,
    user_to_receiver: Arc<Mutex<HashMap<i32, mpsc::Receiver<Message>>>>,
    user_to_ready_to_send: HashMap<i32, bool>,
    user_to_duration: HashMap<i32, Duration>,
    new_users: Vec<i32>,
}

impl GameState {
    pub fn new(
        users: Arc<Mutex<HashSet<i32>>>,
        user_to_sender: Arc<Mutex<HashMap<i32, mpsc::Sender<Message>>>>,
        user_to_receiver: Arc<Mutex<HashMap<i32, mpsc::Receiver<Message>>>>,
    ) -> GameState {
        GameState {
            users,
            user_to_sender,
            user_to_receiver,
            user_to_ready_to_send: HashMap::new(),
            user_to_duration: HashMap::new(),
            new_users: Vec::new(),
        }
    }

    pub fn elapsed(&mut self, elapsed: Duration) {
        for (user, duration) in self.user_to_duration.iter_mut() {
            self.user_to_ready_to_send.get(user).map(|ready_to_send| {
                if *ready_to_send {
                    *duration += elapsed;
                }
            });
        }
    }

    pub fn pull_updates(&mut self) {
        for user in self.users.lock().unwrap().iter() {
            self.user_to_receiver
                .lock()
                .unwrap()
                .get(user)
                .map(|receiver| {
                    for mut message in receiver.try_iter() {
                        match message.message_type() {
                            MessageType::StubMessage => {
                                let s2 = message
                                    .pop_string()
                                    .ok_or(String::from("Can't pop string"))
                                    .unwrap();
                                let s1 = message
                                    .pop_string()
                                    .ok_or(String::from("Can't pop string"))
                                    .unwrap();

                                println!("message from {user}: s1: {s1}, s2: {s2}");

                                self.user_to_ready_to_send
                                    .entry(*user)
                                    .and_modify(|v| *v = true);
                            }
                            MessageType::ConnectionRequested => {
                                println!("message from {user}: ConnectionRequested");
                                self.user_to_ready_to_send.entry(*user).or_insert(true);
                                self.user_to_duration.entry(*user).or_insert(Duration::ZERO);
                                self.new_users.push(*user);
                            }
                            _ => continue,
                        }
                    }
                });
        }
    }

    pub fn publish_updates(&mut self) {
        for user in &self.new_users {
            self.user_to_sender.lock().unwrap().get(user).map(|sender| {
                let mut accepted_message = Message::new(MessageType::ConnectionAccepted);
                accepted_message.push(user);
                sender.send(accepted_message).unwrap();
            });
        }
        self.new_users.clear();

        for (user, duration) in self.user_to_duration.iter_mut() {
            self.user_to_ready_to_send
                .get_mut(&user)
                .map(|ready_to_send| {
                    if *ready_to_send && duration > &mut Duration::from_secs(2) {
                        self.user_to_sender.lock().unwrap().get(user).map(|sender| {
                            sender.send(Self::create_stub_message()).unwrap();
                        });
                        *ready_to_send = false;
                        *duration -= Duration::from_secs(2);
                    }
                });
        }
    }

    fn create_stub_message() -> Message {
        let mut stub_message = Message::new(MessageType::StubMessage);
        stub_message.push_string("Hello from the other siiiiiiiiiide!");
        stub_message.push_string("At least I can say that I've triiiiiiiiiiied!");
        stub_message
    }
}
