use std::{
    collections::HashMap,
    sync::{mpsc, Arc, Mutex},
    time::Duration,
};

use state::GameState;

use crate::message::Message;

pub mod state;

pub struct Game {
    user_to_sender: Arc<Mutex<HashMap<i32, mpsc::Sender<Message>>>>,
    user_to_receiver: Arc<Mutex<HashMap<i32, mpsc::Receiver<Message>>>>,
    game_state: Box<dyn GameState>,
}

impl Game {
    pub fn new(
        user_to_sender: Arc<Mutex<HashMap<i32, mpsc::Sender<Message>>>>,
        user_to_receiver: Arc<Mutex<HashMap<i32, mpsc::Receiver<Message>>>>,
        game_state: Box<dyn GameState>,
    ) -> Game {
        Game {
            user_to_sender,
            user_to_receiver,
            game_state,
        }
    }

    pub fn elapsed(&mut self, elapsed: Duration) {
        if let Some(new_state) = self.game_state.elapsed(elapsed) {
            self.game_state = new_state;
        }
    }

    pub fn io_updates(&mut self) {
        self.game_state
            .io_updates(&self.user_to_sender, &self.user_to_receiver);
    }
}
