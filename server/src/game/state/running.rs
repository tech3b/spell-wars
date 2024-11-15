use std::{
    collections::{HashMap, HashSet},
    sync::{mpsc, Mutex},
    time::Duration,
};

use crate::message::{Message, MessageType};

use super::GameState;

enum UserState {
    WaitingForUser,
    AcceptedFromUser(String, String),
    WaitingToSend(Duration),
}

pub struct RunningGame {
    user_to_user_state: HashMap<i32, UserState>,
}

impl RunningGame {
    pub fn new(users: HashSet<i32>) -> Self {
        RunningGame {
            user_to_user_state: users
                .iter()
                .map(|user| (*user, UserState::WaitingToSend(Duration::ZERO)))
                .collect(),
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
        for (user, state) in self.user_to_user_state.iter_mut() {
            match state {
                UserState::WaitingForUser => {}
                UserState::AcceptedFromUser(s1, s2) => {
                    println!("message from {user}: s1: {s1}, s2: {s2}");
                    *state = UserState::WaitingToSend(Duration::ZERO);
                }
                UserState::WaitingToSend(duration) => {
                    *duration += elapsed;
                }
            }
        }
        None
    }

    fn io_updates(
        &mut self,
        user_to_sender: &Mutex<HashMap<i32, mpsc::Sender<Message>>>,
        user_to_receiver: &Mutex<HashMap<i32, mpsc::Receiver<Message>>>,
    ) {
        for (user, state) in self.user_to_user_state.iter_mut() {
            match state {
                UserState::WaitingForUser => {
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
                                    *state = UserState::AcceptedFromUser(s1, s2);
                                }
                                _ => continue,
                            }
                        }
                    });
                }
                UserState::AcceptedFromUser(_, _) => continue,
                UserState::WaitingToSend(duration) => {
                    if *duration > Duration::from_secs(2) {
                        user_to_sender.lock().unwrap().get(user).map(|sender| {
                            sender.send(Self::create_stub_message()).unwrap();
                        });
                        *state = UserState::WaitingForUser
                    }
                }
            }
        }
    }
}
