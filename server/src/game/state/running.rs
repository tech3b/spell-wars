use std::{
    collections::{HashMap, HashSet},
    sync::mpsc,
    time::Duration,
};

use crate::message::{Message, MessageType};

use super::{reaction::Reaction, GameState};

enum UserState {
    WaitingForStub,
    StubAccepted(Vec<String>, Duration, Reaction),
}

pub struct RunningGame {
    user_to_user_state: HashMap<i32, UserState>,
}

impl RunningGame {
    pub fn new(users: HashSet<i32>) -> Self {
        RunningGame {
            user_to_user_state: users
                .iter()
                .map(|user| {
                    (
                        *user,
                        UserState::StubAccepted(
                            Vec::new(),
                            Duration::ZERO,
                            Reaction::new_reacted(true),
                        ),
                    )
                })
                .collect(),
        }
    }

    fn create_stub_message() -> Message {
        let mut stub_message = Message::new(MessageType::StubMessage);
        stub_message.push_string("Hello from the other siiiiiiiiiide!");
        stub_message.push_string("At least I can say that I've triiiiiiiiiiied!");
        stub_message.push(&(2 as u8));
        stub_message
    }
}

impl GameState for RunningGame {
    fn elapsed(&mut self, elapsed: Duration) -> Option<Box<dyn GameState>> {
        for (user, state) in self.user_to_user_state.iter_mut() {
            match state {
                UserState::WaitingForStub => {}
                UserState::StubAccepted(strings, duration, reaction) => {
                    reaction.react_once(|| {
                        for s in strings.iter() {
                            println!("message from {user}: sx: {s}");
                        }
                    });
                    *duration += elapsed;
                }
            }
        }
        None
    }

    fn io_updates(
        &mut self,
        user_to_sender: &HashMap<i32, mpsc::Sender<Message>>,
        user_to_receiver: &HashMap<i32, mpsc::Receiver<Message>>,
        _: &HashSet<i32>,
    ) {
        for (user, state) in self.user_to_user_state.iter_mut() {
            match state {
                UserState::WaitingForStub => {
                    user_to_receiver.get(user).map(|receiver| {
                        for mut message in receiver.try_iter() {
                            match message.message_type() {
                                MessageType::StubMessage => {
                                    let number_of_strings: u8 = message.pop().unwrap();
                                    let strings: Vec<String> = (0..number_of_strings)
                                        .map(|_| message.pop_string().unwrap())
                                        .collect();

                                    *state = UserState::StubAccepted(
                                        strings,
                                        Duration::ZERO,
                                        Reaction::new(),
                                    );
                                }
                                _ => continue,
                            }
                        }
                    });
                }
                UserState::StubAccepted(_, duration, _) => {
                    if *duration > Duration::from_secs(2) {
                        user_to_sender.get(user).map(|sender| {
                            sender.send(Self::create_stub_message()).unwrap();
                        });
                        *state = UserState::WaitingForStub
                    }
                }
            }
        }
    }
}
