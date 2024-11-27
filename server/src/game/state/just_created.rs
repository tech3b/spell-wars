use std::{
    collections::{HashMap, HashSet},
    sync::{
        mpsc::{self, TryIter},
        Arc, Mutex,
    },
    time::Duration,
};

use crate::message::{Message, MessageType};

use super::{reaction::Reaction, ready_to_start::ReadyToStartGame};

use super::GameState;

enum UserUpdateStatus {
    Ready,
    NotReady,
    Disconnected
}

impl UserUpdateStatus {
    pub fn value(&self) -> u8 {
        match self {
            UserUpdateStatus::Ready => 1,
            UserUpdateStatus::NotReady => 0,
            UserUpdateStatus::Disconnected => 2,
        }
    }
}

#[derive(Debug)]
enum FinalCall {
    NotYet,
    AllReady,
    Processed,
}

#[derive(Debug)]
enum OverallState {
    AcceptingUsers(HashMap<i32, AcceptingUserState>, FinalCall),
    AllReady(HashSet<i32>, bool),
}

#[derive(Debug)]
enum AcceptingUserState {
    Connected,
    AboutToAccept,
    ConnectionAccepted(Reaction, bool),
}

pub struct JustCreatedGame {
    state: OverallState,
    pause_accepting_users: Arc<Mutex<bool>>,
    stop_accepting_users: Arc<Mutex<bool>>,
}

impl JustCreatedGame {
    pub fn new(
        pause_accepting_users: Arc<Mutex<bool>>,
        stop_accepting_users: Arc<Mutex<bool>>,
    ) -> Self {
        JustCreatedGame {
            state: OverallState::AcceptingUsers(HashMap::new(), FinalCall::NotYet),
            pause_accepting_users,
            stop_accepting_users,
        }
    }

    pub fn receiver<'a>(
        users: &'a HashMap<i32, mpsc::Receiver<Message>>,
        user: &i32,
    ) -> OptTryIterator<'a> {
        OptTryIterator {
            iter: users.get(user).map(|receiver| receiver.try_iter()),
        }
    }

    fn send_connection_accepted(
        users: &mut HashMap<i32, AcceptingUserState>,
        user_to_sender: &HashMap<i32, mpsc::Sender<Message>>,
    ) -> Vec<i32> {
        let need_to_send_connection_accepted: Vec<i32> = users
            .iter_mut()
            .filter_map(|(user, user_state)| {
                if let AcceptingUserState::AboutToAccept = user_state {
                    *user_state = AcceptingUserState::ConnectionAccepted(Reaction::new(), false);
                    Some(*user)
                } else {
                    None
                }
            })
            .collect();

        if need_to_send_connection_accepted.len() > 0 {
            let users_state: Vec<(i32, bool)> = users
                .iter()
                .filter_map(|(user, state)| {
                    if let AcceptingUserState::ConnectionAccepted(_, is_ready) = state {
                        Some((*user, *is_ready))
                    } else {
                        None
                    }
                })
                .collect();

            for user in need_to_send_connection_accepted.iter() {
                let mut accepted_message = Message::new(MessageType::ConnectionAccepted);

                for (user, is_ready) in users_state.iter() {
                    accepted_message.push(user);
                    accepted_message.push(&(*is_ready as u8));
                }

                accepted_message.push(&(users_state.len() as u8));
                accepted_message.push(user);

                user_to_sender
                    .get(&user)
                    .map(|sender| sender.send(accepted_message).unwrap());
            }
        }
        need_to_send_connection_accepted
    }
}

impl GameState for JustCreatedGame {
    fn elapsed(&mut self, _: Duration) -> Option<Box<dyn GameState>> {
        match &mut self.state {
            OverallState::AcceptingUsers(users, final_call) => {
                for (user, state) in users.iter_mut() {
                    match state {
                        AcceptingUserState::Connected | AcceptingUserState::AboutToAccept => {
                            continue
                        }
                        AcceptingUserState::ConnectionAccepted(reaction, _) => {
                            reaction.react_once(|| {
                                println!("message from {user}: ConnectionRequested");
                            });
                        }
                    }
                }
                if users.len() > 0 && users.iter().all(|(_, state)| matches!(state, AcceptingUserState::ConnectionAccepted(_, is_ready)  if *is_ready)) {
                    match final_call {
                        FinalCall::NotYet => {
                            *self.pause_accepting_users.lock().unwrap() = true;
                            *final_call = FinalCall::AllReady;
                        },
                        FinalCall::AllReady => {},
                        FinalCall::Processed => {
                            *self.stop_accepting_users.lock().unwrap() = true;
                            self.state = OverallState::AllReady(users.iter().map(|(user, _)| *user).collect(), false);
                        },
                    }
                } else {
                    *self.pause_accepting_users.lock().unwrap() = false;
                    *final_call = FinalCall::NotYet;
                }
            }
            OverallState::AllReady(users, ready_sent) => {
                if *ready_sent {
                    println!("moving to ReadyToStartGame");
                    return Some(Box::new(ReadyToStartGame::new(std::mem::replace(
                        users,
                        HashSet::new(),
                    ))));
                }
            }
        }
        return None;
    }

    fn io_updates(
        &mut self,
        user_to_sender: &HashMap<i32, mpsc::Sender<Message>>,
        user_to_receiver: &HashMap<i32, mpsc::Receiver<Message>>,
        users: &HashSet<i32>,
    ) {
        match &mut self.state {
            OverallState::AcceptingUsers(current_users, final_call) => {
                let mut updated_users = Vec::new();

                for user in users.iter() {
                    current_users
                        .entry(*user)
                        .or_insert(AcceptingUserState::Connected);
                }

                let disconnected_users: Vec<i32> = current_users.iter().map(|(user, _)| *user).filter(|u| !users.contains(&u)).collect();

                for user in disconnected_users {
                    current_users.remove(&user);
                    updated_users.push((user, UserUpdateStatus::Disconnected));
                };


                for (user, user_state) in current_users.iter_mut() {
                    match user_state {
                        AcceptingUserState::Connected => {
                            for message in JustCreatedGame::receiver(&user_to_receiver, user) {
                                match message.message_type() {
                                    MessageType::ConnectionRequested => {
                                        *user_state = AcceptingUserState::AboutToAccept;
                                        break;
                                    }
                                    _ => continue,
                                }
                            }
                        }
                        AcceptingUserState::ConnectionAccepted(_, user_ready) => {
                            let mut was_changed = false;
                            for mut message in JustCreatedGame::receiver(&user_to_receiver, user) {
                                match message.message_type() {
                                    MessageType::ReadyToStartChanged => {
                                        let is_ready: u8 = message.pop().unwrap_or(0);
                                        *user_ready = is_ready != 0;
                                        was_changed = true;
                                    }
                                    _ => continue,
                                }
                            }
                            if was_changed {
                                updated_users.push((*user, if *user_ready {UserUpdateStatus::Ready} else {UserUpdateStatus::NotReady}));
                            }
                        }
                        AcceptingUserState::AboutToAccept => {}
                    }
                }

                let connection_accepted_sent_to =
                    JustCreatedGame::send_connection_accepted(current_users, user_to_sender);

                for user in connection_accepted_sent_to {
                    updated_users.push((user, UserUpdateStatus::NotReady));
                }

                if !updated_users.is_empty() {
                    for (user, user_state) in current_users.iter() {
                        match user_state {
                            AcceptingUserState::Connected | AcceptingUserState::AboutToAccept => {
                                continue
                            }
                            AcceptingUserState::ConnectionAccepted(_, _) => {
                                let mut update_message =
                                    Message::new(MessageType::UserStatusUpdate);
                                for (user_to_update, update_status) in updated_users.iter() {
                                    update_message.push(user_to_update);
                                    update_message.push(&update_status.value());
                                }
                                update_message.push(&(updated_users.len() as u8));
                                user_to_sender.get(user).map(|sender| {
                                    sender.send(update_message).unwrap();
                                });
                            }
                        }
                    }
                }
                *final_call = FinalCall::Processed;
            }
            OverallState::AllReady(users, ready_sent) => {
                if !*ready_sent {
                    for user in users.iter() {
                        let ready_to_start = Message::new(MessageType::ReadyToStart);
                        user_to_sender.get(user).map(|sender| {
                            sender.send(ready_to_start).unwrap();
                        });
                    }
                    *ready_sent = true;
                }
            }
        }
    }
}

pub struct OptTryIterator<'a> {
    iter: Option<TryIter<'a, Message>>,
}

impl<'a> Iterator for OptTryIterator<'a> {
    type Item = Message;

    fn next(&mut self) -> Option<Self::Item> {
        self.iter.as_mut()?.next()
    }
}
