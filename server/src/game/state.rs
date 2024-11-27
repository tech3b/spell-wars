use std::{
    collections::{HashMap, HashSet},
    sync::mpsc,
    time::Duration,
};

use crate::message::Message;

pub mod just_created;
pub mod reaction;
pub mod ready_to_start;
pub mod running;

pub trait GameState {
    fn elapsed(&mut self, elapsed: Duration) -> Option<Box<dyn GameState>>;

    fn io_updates(
        &mut self,
        user_to_sender: &HashMap<i32, mpsc::Sender<Message>>,
        user_to_receiver: &HashMap<i32, mpsc::Receiver<Message>>,
        users: &HashSet<i32>,
    );
}
