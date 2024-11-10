use std::{
    collections::{HashMap, HashSet},
    sync::{mpsc, Mutex},
    time::Duration,
};

use crate::message::{Message, MessageType};

use super::GameState;

pub struct RunningGame {
    users: HashSet<i32>,
    user_to_ready_to_send: HashMap<i32, bool>,
    user_to_duration: HashMap<i32, Duration>,
    user_to_need_to_write: HashMap<i32, (String, String)>,
}

impl RunningGame {
    pub fn new(users: HashSet<i32>) -> Self {
        let user_to_duration: HashMap<i32, Duration> =
            users.iter().map(|u| (*u, Duration::ZERO)).collect();
        let user_to_ready_to_send: HashMap<i32, bool> = users.iter().map(|u| (*u, true)).collect();
        RunningGame {
            users,
            user_to_ready_to_send,
            user_to_duration,
            user_to_need_to_write: HashMap::new(),
        }
    }

    fn create_stub_message() -> Message {
        let mut stub_message = Message::new(MessageType::StubMessage);
        stub_message.push_string("Hello from the other siiiiiiiiiide!");
        stub_message.push_string("At least I can say that I've triiiiiiiiiiied!");
        stub_message
    }
}

impl GameState for RunningGame {
    fn elapsed(&mut self, elapsed: Duration) -> Option<Box<dyn GameState>> {
        for (user, duration) in self.user_to_duration.iter_mut() {
            self.user_to_ready_to_send.get(user).map(|ready_to_send| {
                if *ready_to_send {
                    *duration += elapsed;
                }
            });
        }
        for (user, (s1, s2)) in self.user_to_need_to_write.iter() {
            println!("message from {user}: s1: {s1}, s2: {s2}");
        }
        self.user_to_need_to_write.clear();
        None
    }

    fn io_updates(
        &mut self,
        user_to_sender: &Mutex<HashMap<i32, mpsc::Sender<Message>>>,
        user_to_receiver: &Mutex<HashMap<i32, mpsc::Receiver<Message>>>,
    ) {
        for user in self.users.iter() {
            user_to_receiver.lock().unwrap().get(user).map(|receiver| {
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

                            self.user_to_need_to_write.entry(*user).or_insert((s1, s2));

                            self.user_to_ready_to_send
                                .entry(*user)
                                .and_modify(|v| *v = true);
                        }
                        _ => continue,
                    }
                }
            });
        }
        for (user, duration) in self.user_to_duration.iter_mut() {
            self.user_to_ready_to_send
                .get_mut(&user)
                .map(|ready_to_send| {
                    if *ready_to_send && duration > &mut Duration::from_secs(2) {
                        user_to_sender.lock().unwrap().get(user).map(|sender| {
                            sender.send(Self::create_stub_message()).unwrap();
                        });
                        *ready_to_send = false;
                        *duration -= Duration::from_secs(2);
                    }
                });
        }
    }
}
