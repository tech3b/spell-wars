use std::{
    collections::{HashMap, HashSet},
    sync::{mpsc, Arc, Mutex},
    time::Duration,
};

use crate::message::{Message, MessageType};

use super::ready_to_start::ReadyToStartGame;

use super::GameState;

pub struct JustCreatedGame {
    users: Arc<Mutex<HashSet<i32>>>,
    user_to_write_about: HashSet<i32>,
    stop_accepting_users: Arc<Mutex<bool>>,
    final_call: bool,
    ready_to_start_received: bool,
    need_to_send_ready_for_start: bool,
    ready_to_start_sent: bool,
}

impl JustCreatedGame {
    pub fn new(users: Arc<Mutex<HashSet<i32>>>, stop_accepting_users: Arc<Mutex<bool>>) -> Self {
        JustCreatedGame {
            users,
            user_to_write_about: HashSet::new(),
            stop_accepting_users,
            final_call: false,
            ready_to_start_received: false,
            need_to_send_ready_for_start: false,
            ready_to_start_sent: false,
        }
    }
}

impl GameState for JustCreatedGame {
    fn elapsed(&mut self, elapsed: Duration) -> Option<Box<dyn GameState>> {
        for user in self.user_to_write_about.iter() {
            println!("message from {user}: ConnectionRequested");
        }
        self.user_to_write_about.clear();

        if self.ready_to_start_received {
            self.need_to_send_ready_for_start = true;
            self.final_call = true;
        }

        if self.ready_to_start_sent {
            println!("moving to ReadyToStartGame");
            return Some(Box::new(ReadyToStartGame::new(
                self.users.lock().unwrap().clone(),
            )));
        }
        return None;
    }

    fn io_updates(
        &mut self,
        user_to_sender: &Mutex<HashMap<i32, mpsc::Sender<Message>>>,
        user_to_receiver: &Mutex<HashMap<i32, mpsc::Receiver<Message>>>,
    ) {
        if self.final_call {
            *self.stop_accepting_users.lock().unwrap() = true;
        }
        for user in self.users.lock().unwrap().iter() {
            user_to_receiver.lock().unwrap().get(user).map(|receiver| {
                for message in receiver.try_iter() {
                    match message.message_type() {
                        MessageType::ConnectionRequested => {
                            let mut accepted_message =
                                Message::new(MessageType::ConnectionAccepted);
                            accepted_message.push(user);
                            user_to_sender
                                .lock()
                                .unwrap()
                                .get(user)
                                .map(|sender| sender.send(accepted_message).unwrap());

                            self.user_to_write_about.insert(*user);
                        }
                        MessageType::ReadyToStart => {
                            self.ready_to_start_received = true;
                        }
                        _ => continue,
                    }
                }
            });
        }
        if self.need_to_send_ready_for_start {
            for (_, sender) in user_to_sender.lock().unwrap().iter() {
                let ready_to_start_message = Message::new(MessageType::ReadyToStart);
                sender.send(ready_to_start_message).unwrap();
                self.ready_to_start_sent = true;
            }
        }
    }
}
