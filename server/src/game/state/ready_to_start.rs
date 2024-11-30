use std::collections::HashMap;
use std::sync::mpsc;
use std::{collections::HashSet, time::Duration};

use crate::game::chat::Chat;
use crate::message::{Message, MessageType};

use super::running::RunningGame;

use super::GameState;

enum OverallState {
    SecondsLeft(u8, Duration, bool),
    Starting(bool),
}

pub struct ReadyToStartGame {
    state: OverallState,
    users: HashSet<i32>,
    chat: Chat,
}

impl ReadyToStartGame {
    pub fn new(users: HashSet<i32>, chat: Chat) -> Self {
        ReadyToStartGame {
            state: OverallState::SecondsLeft(10, Duration::ZERO, false),
            users,
            chat,
        }
    }
}

impl GameState for ReadyToStartGame {
    fn elapsed(&mut self, elapsed: Duration) -> Option<Box<dyn GameState>> {
        match &mut self.state {
            OverallState::SecondsLeft(seconds_left, duration, sent) => {
                *duration += elapsed;
                if *duration > Duration::from_secs(1) {
                    *duration -= Duration::from_secs(1);
                    *seconds_left -= 1;
                    *sent = false;
                    println!("game about to start: {}", *seconds_left);
                }
            }
            OverallState::Starting(sent) => {
                if *sent {
                    println!("moving to RunningGame");
                    return Some(Box::new(RunningGame::new(
                        std::mem::replace(&mut self.users, HashSet::new()),
                        std::mem::replace(&mut self.chat, Chat::new()),
                    )));
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
        match &mut self.state {
            OverallState::SecondsLeft(seconds_left, _, sent) => {
                if !*sent {
                    for user in self.users.iter() {
                        user_to_sender.get(user).map(|sender| {
                            let mut game_about_to_start =
                                Message::new(MessageType::GameAboutToStart);
                            game_about_to_start.push(seconds_left);
                            sender.send(game_about_to_start).unwrap();
                        });
                    }
                    *sent = true;
                    if *seconds_left == 0 {
                        self.state = OverallState::Starting(false);
                    }
                }
            }
            OverallState::Starting(sent) => {
                if !*sent {
                    for user in self.users.iter() {
                        user_to_sender.get(user).map(|sender| {
                            let game_starting = Message::new(MessageType::GameStarting);
                            sender.send(game_starting).unwrap();
                        });
                    }
                    *sent = true;
                }
            }
        }
        for user in self.users.iter() {
            user_to_receiver.get(user).map(|receiver| {
                for _ in receiver.try_iter() {
                    // don't care about messages here
                }
            });
        }
    }
}
