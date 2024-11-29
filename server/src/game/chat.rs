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

    pub fn append(&mut self, user: i32, message: String) {
        self.new_messages.push((user, message));
    }

    pub fn new_messages(&self) -> &Vec<(i32, String)> {
        &self.new_messages
    }

    pub fn messages(&self) -> &Vec<(i32, String)> {
        &self.messages
    }

    pub fn commit(&mut self) {
        self.messages.reserve(self.new_messages.len());
        let messages = std::mem::replace(&mut self.new_messages, Vec::new());
        for ele in messages {
            self.messages.push(ele);
        }
    }
}
