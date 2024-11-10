use std::collections::HashMap;
use std::sync::{mpsc, Mutex};
use std::{collections::HashSet, time::Duration};

use crate::message::{Message, MessageType};

use super::running::RunningGame;

use super::GameState;

pub struct ReadyToStartGame {
    users: HashSet<i32>,
    time_passed: Duration,
    seconds_left: u8,
    seconds_message_sent: bool,
}

impl ReadyToStartGame {
    pub fn new(users: HashSet<i32>) -> Self {
        ReadyToStartGame {
            users,
            time_passed: Duration::ZERO,
            seconds_left: 10,
            seconds_message_sent: false,
        }
    }
}

impl GameState for ReadyToStartGame {
    fn elapsed(&mut self, elapsed: Duration) -> Option<Box<dyn GameState>> {
        if self.seconds_left == 0 && self.seconds_message_sent {
            println!("moving to RunningGame");
            return Some(Box::new(RunningGame::new(std::mem::replace(
                &mut self.users,
                HashSet::new(),
            ))));
        }

        self.time_passed += elapsed;
        if self.time_passed > Duration::from_secs(1) {
            self.time_passed -= Duration::from_secs(1);
            self.seconds_left -= 1;
            self.seconds_message_sent = false;
            println!("game about to start: {}", self.seconds_left);
        }
        None
    }

    fn io_updates(
        &mut self,
        user_to_sender: &Mutex<HashMap<i32, mpsc::Sender<Message>>>,
        user_to_receiver: &Mutex<HashMap<i32, mpsc::Receiver<Message>>>,
    ) {
        for user in self.users.iter() {
            user_to_receiver.lock().unwrap().get(user).map(|receiver| {
                for _ in receiver.try_iter() {
                    // don't care about messages here
                }
            });
        }

        if !self.seconds_message_sent {
            for user in self.users.iter() {
                user_to_sender.lock().unwrap().get(user).map(|sender| {
                    let mut game_about_to_start = Message::new(MessageType::GameAboutToStart);
                    game_about_to_start.push(&self.seconds_left);
                    sender.send(game_about_to_start).unwrap();
                    if self.seconds_left == 0 {
                        let game_starting = Message::new(MessageType::GameStarting);
                        sender.send(game_starting).unwrap();
                    }
                });
            }
            self.seconds_message_sent = true;
        }
    }
}
