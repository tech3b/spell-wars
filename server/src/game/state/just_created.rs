use std::{
    collections::{HashMap, HashSet},
    sync::{mpsc, Arc, Mutex},
    time::Duration,
};

use crate::message::{Message, MessageType};

use super::ready_to_start::ReadyToStartGame;

use super::GameState;

enum UserState {
    Connected,
    ConnectionAccepted(Reaction),
    ReadyToStartReceived,
    ReadyToStartSent(Reaction),
}

enum OverallState {
    AcceptingUsers,
    SendingReadyToStart,
    ReadyToStartSent,
}

struct Reaction(bool);

impl Reaction {
    pub fn new() -> Self {
        Reaction(false)
    }

    pub fn react_once<F: FnOnce() -> ()>(&mut self, f: F) {
        if !self.0 {
            f();
            self.0 = true;
        }
    }
}

pub struct JustCreatedGame {
    state: OverallState,
    user_to_state: HashMap<i32, UserState>,
    users: Arc<Mutex<HashSet<i32>>>,
    stop_accepting_users: Arc<Mutex<bool>>,
}

impl JustCreatedGame {
    pub fn new(users: Arc<Mutex<HashSet<i32>>>, stop_accepting_users: Arc<Mutex<bool>>) -> Self {
        JustCreatedGame {
            state: OverallState::AcceptingUsers,
            user_to_state: HashMap::new(),
            users,
            stop_accepting_users,
        }
    }
}

impl GameState for JustCreatedGame {
    fn elapsed(&mut self, _: Duration) -> Option<Box<dyn GameState>> {
        match self.state {
            OverallState::AcceptingUsers => {
                for (user, state) in self.user_to_state.iter_mut() {
                    match state {
                        UserState::Connected => continue,
                        UserState::ConnectionAccepted(reaction) => {
                            reaction.react_once(|| {
                                println!("message from {user}: ConnectionRequested");
                            });
                        }
                        UserState::ReadyToStartReceived => {
                            *self.stop_accepting_users.lock().unwrap() = true;
                            println!("ReadyToStartReceived from {user}");
                            self.state = OverallState::SendingReadyToStart;
                        }
                        UserState::ReadyToStartSent(reaction) => {
                            reaction.react_once(|| {
                                println!("ReadyToStartSent to {user}");
                            });
                        }
                    }
                }
            }
            OverallState::SendingReadyToStart => (),
            OverallState::ReadyToStartSent => {
                println!("moving to ReadyToStartGame");
                return Some(Box::new(ReadyToStartGame::new(
                    self.user_to_state
                        .iter()
                        .filter(|(_, state)| matches!(state, UserState::ReadyToStartSent(_)))
                        .map(|(user, _)| *user)
                        .collect(),
                )));
            }
        }

        return None;
    }

    fn io_updates(
        &mut self,
        user_to_sender: &Mutex<HashMap<i32, mpsc::Sender<Message>>>,
        user_to_receiver: &Mutex<HashMap<i32, mpsc::Receiver<Message>>>,
    ) {
        match self.state {
            OverallState::AcceptingUsers => {
                for user in self.users.lock().unwrap().iter() {
                    let user_state = self
                        .user_to_state
                        .entry(*user)
                        .or_insert(UserState::Connected);
                    match user_state {
                        UserState::Connected => {
                            user_to_receiver.lock().unwrap().get(user).map(|receiver| {
                                for message in receiver.try_iter() {
                                    match message.message_type() {
                                        MessageType::ConnectionRequested => {
                                            let mut accepted_message =
                                                Message::new(MessageType::ConnectionAccepted);
                                            accepted_message.push(user);
                                            user_to_sender.lock().unwrap().get(user).map(
                                                |sender| sender.send(accepted_message).unwrap(),
                                            );

                                            *user_state =
                                                UserState::ConnectionAccepted(Reaction::new());
                                            break;
                                        }
                                        _ => continue,
                                    }
                                }
                            });
                        }
                        UserState::ConnectionAccepted(_) => {
                            user_to_receiver.lock().unwrap().get(user).map(|receiver| {
                                for message in receiver.try_iter() {
                                    match message.message_type() {
                                        MessageType::ReadyToStart => {
                                            *user_state = UserState::ReadyToStartReceived;
                                            break;
                                        }
                                        _ => continue,
                                    }
                                }
                            });
                        }
                        UserState::ReadyToStartReceived | UserState::ReadyToStartSent(_) => {
                            continue
                        }
                    }
                }
            }
            OverallState::SendingReadyToStart => {
                for (user, state) in self.user_to_state.iter_mut() {
                    match state {
                        UserState::ConnectionAccepted(_) | UserState::ReadyToStartReceived => {
                            user_to_sender.lock().unwrap().get(user).map(|sender| {
                                let ready_to_start_message =
                                    Message::new(MessageType::ReadyToStart);
                                sender.send(ready_to_start_message).unwrap();
                                *state = UserState::ReadyToStartSent(Reaction::new());
                            });
                        }
                        UserState::ReadyToStartSent(_) | UserState::Connected => continue,
                    }
                }
                self.state = OverallState::ReadyToStartSent;
            }
            OverallState::ReadyToStartSent => (),
        }
    }
}
