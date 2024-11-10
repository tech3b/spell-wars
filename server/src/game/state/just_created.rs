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
    time_from_first_connection: Option<Duration>,
    user_to_write_about: HashSet<i32>,
    stop_accepting_users: Arc<Mutex<bool>>,
    final_call: bool,
    finished_with_final_users: bool,
}

impl JustCreatedGame {
    pub fn new(users: Arc<Mutex<HashSet<i32>>>, stop_accepting_users: Arc<Mutex<bool>>) -> Self {
        JustCreatedGame {
            users,
            time_from_first_connection: None,
            user_to_write_about: HashSet::new(),
            stop_accepting_users,
            final_call: false,
            finished_with_final_users: false,
        }
    }
}

impl GameState for JustCreatedGame {
    fn elapsed(&mut self, elapsed: Duration) -> Option<Box<dyn GameState>> {
        for user in self.user_to_write_about.iter() {
            println!("message from {user}: ConnectionRequested");
        }
        self.user_to_write_about.clear();

        if let Some(d) = self.time_from_first_connection.as_mut() {
            *d += elapsed;
        }
        let time_to_start = self
            .time_from_first_connection
            .map(|d| d > Duration::from_secs(10))
            .unwrap_or(false);

        if time_to_start {
            if self.finished_with_final_users {
                println!("moving to ReadyToStartGame");
                return Some(Box::new(ReadyToStartGame::new(
                    self.users.lock().unwrap().clone(),
                )));
            }
            self.final_call = true;
        }
        return None;
    }

    fn io_updates(
        &mut self,
        user_to_sender: &Mutex<HashMap<i32, mpsc::Sender<Message>>>,
        user_to_receiver: &Mutex<HashMap<i32, mpsc::Receiver<Message>>>,
    ) {
        if self.final_call {
            self.finished_with_final_users = true;
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
                            self.time_from_first_connection =
                                self.time_from_first_connection.or(Some(Duration::ZERO));
                        }
                        _ => continue,
                    }
                }
            });
        }
    }
}
