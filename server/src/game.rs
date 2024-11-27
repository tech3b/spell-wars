use std::{
    sync::{Arc, Mutex},
    time::Duration,
};

use state::GameState;

use crate::Users;

pub mod state;

pub struct Game {
    users: Arc<Mutex<Users>>,
    game_state: Box<dyn GameState>,
}

impl Game {
    pub fn new(users: Arc<Mutex<Users>>, game_state: Box<dyn GameState>) -> Game {
        Game { users, game_state }
    }

    pub fn elapsed(&mut self, elapsed: Duration) {
        if let Some(new_state) = self.game_state.elapsed(elapsed) {
            self.game_state = new_state;
        }
    }

    pub fn io_updates(&mut self) {
        let locked_users = self.users.lock().unwrap();
        self.game_state.io_updates(
            &locked_users.user_to_write_sender,
            &locked_users.user_to_read_receiver,
            &locked_users.users,
        );
    }
}
