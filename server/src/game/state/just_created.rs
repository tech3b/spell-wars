use std::{
    collections::{HashMap, HashSet},
    sync::{mpsc, Arc, Mutex},
    time::Duration,
};

use crate::message::{Message, MessageType};

use super::{reaction::Reaction, ready_to_start::ReadyToStartGame};

use super::GameState;

#[derive(Debug)]
enum OverallState {
    AcceptingUsers(HashMap<i32, AcceptingUserState>, bool),
    SendingReadyToStart(HashSet<i32>),
    ReadyToStartSent(HashSet<i32>),
}

#[derive(Debug)]
enum AcceptingUserState {
    Connected,
    ConnectionAccepted(Reaction),
    ReadyToStartReceived,
}

pub struct JustCreatedGame {
    state: OverallState,
    users: Arc<Mutex<HashSet<i32>>>,
    stop_accepting_users: Arc<Mutex<bool>>,
}

impl JustCreatedGame {
    pub fn new(users: Arc<Mutex<HashSet<i32>>>, stop_accepting_users: Arc<Mutex<bool>>) -> Self {
        JustCreatedGame {
            state: OverallState::AcceptingUsers(HashMap::new(), false),
            users,
            stop_accepting_users,
        }
    }
}

impl GameState for JustCreatedGame {
    fn elapsed(&mut self, _: Duration) -> Option<Box<dyn GameState>> {
        match &mut self.state {
            OverallState::AcceptingUsers(users, final_call) => {
                for (user, state) in users.iter_mut() {
                    match state {
                        AcceptingUserState::Connected => continue,
                        AcceptingUserState::ConnectionAccepted(reaction) => {
                            reaction.react_once(|| {
                                println!("message from {user}: ConnectionRequested");
                            });
                        }
                        AcceptingUserState::ReadyToStartReceived => {
                            if !*final_call {
                                *self.stop_accepting_users.lock().unwrap() = true;
                                println!("ReadyToStartReceived from {user}");
                                *final_call = true;
                            }
                        }
                    }
                }
            }
            OverallState::SendingReadyToStart(_) => (),
            OverallState::ReadyToStartSent(users) => {
                println!("moving to ReadyToStartGame");
                return Some(Box::new(ReadyToStartGame::new(std::mem::replace(
                    users,
                    HashSet::new(),
                ))));
            }
        }
        return None;
    }

    fn io_updates(
        &mut self,
        user_to_sender: &Mutex<HashMap<i32, mpsc::Sender<Message>>>,
        user_to_receiver: &Mutex<HashMap<i32, mpsc::Receiver<Message>>>,
    ) {
        match &mut self.state {
            OverallState::AcceptingUsers(users, final_call) => {
                for user in self.users.lock().unwrap().iter() {
                    let user_state = users.entry(*user).or_insert(AcceptingUserState::Connected);
                    match user_state {
                        AcceptingUserState::Connected => {
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

                                            *user_state = AcceptingUserState::ConnectionAccepted(
                                                Reaction::new(),
                                            );
                                            break;
                                        }
                                        _ => continue,
                                    }
                                }
                            });
                        }
                        AcceptingUserState::ConnectionAccepted(_) => {
                            user_to_receiver.lock().unwrap().get(user).map(|receiver| {
                                for message in receiver.try_iter() {
                                    match message.message_type() {
                                        MessageType::ReadyToStart => {
                                            *user_state = AcceptingUserState::ReadyToStartReceived;
                                            break;
                                        }
                                        _ => continue,
                                    }
                                }
                            });
                        }
                        AcceptingUserState::ReadyToStartReceived => (),
                    }
                }
                if *final_call {
                    self.state = OverallState::SendingReadyToStart(
                        users
                            .iter()
                            .filter(|(_, state)| {
                                matches!(
                                    **state,
                                    AcceptingUserState::ReadyToStartReceived
                                        | AcceptingUserState::ConnectionAccepted(_)
                                )
                            })
                            .map(|(user, _)| *user)
                            .collect(),
                    );
                }
            }
            OverallState::SendingReadyToStart(users) => {
                for user in users.iter() {
                    user_to_sender.lock().unwrap().get(user).map(|sender| {
                        let ready_to_start_message = Message::new(MessageType::ReadyToStart);
                        sender.send(ready_to_start_message).unwrap();
                    });
                }

                self.state =
                    OverallState::ReadyToStartSent(std::mem::replace(users, HashSet::new()));
            }
            OverallState::ReadyToStartSent(_) => (),
        }
    }
}
