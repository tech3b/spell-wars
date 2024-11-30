use crate::message::{Message, MessageType};

pub struct Chat {
    messages: Vec<(i32, String)>,
    new_messages: Vec<(i32, String)>,
}

impl Chat {
    pub fn new() -> Chat {
        Chat {
            messages: Vec::new(),
            new_messages: Vec::new(),
        }
    }

    pub fn append(&mut self, user: i32, mut message: Message) {
        let number_of_messages: u8 = message.pop().unwrap();
        for _ in 0..number_of_messages {
            let chat_message = message.pop_string().unwrap();
            self.new_messages.push((user, chat_message));
        }
    }

    pub fn whole_chat_state(&self) -> Message {
        let mut chat_update_message = Message::new(MessageType::ChatUpdate);
        for (user, message) in self.messages.iter().rev() {
            chat_update_message.push_string(message);
            chat_update_message.push(user);
        }
        chat_update_message.push(&(self.messages.len() as u8));
        chat_update_message
    }

    pub fn commit(&mut self) -> Option<Message> {
        if self.new_messages.len() > 0 {
            let mut chat_update_message = Message::new(MessageType::ChatUpdate);
            for (user, message) in self.new_messages.iter().rev() {
                chat_update_message.push_string(message);
                chat_update_message.push(user);
            }
            chat_update_message.push(&(self.new_messages.len() as u8));

            self.messages.reserve(self.new_messages.len());
            let messages = std::mem::replace(&mut self.new_messages, Vec::new());
            for ele in messages {
                self.messages.push(ele);
            }

            return Some(chat_update_message);
        }
        None
    }
}
