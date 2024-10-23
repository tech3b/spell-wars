use serde::{Deserialize, Serialize};

#[repr(u32)] // Ensure that the enum is represented as an u32
#[derive(Serialize, Deserialize, Copy, Clone)]
pub enum MessageType {
    ConnectionRequested = 1,
    ConnectionAccepted = 2,
    ConnectionRejected = 3,
    StubMessage = 4,
}

impl From<u32> for MessageType {
    fn from(value: u32) -> MessageType {
        match value {
            1 => MessageType::ConnectionRequested,
            2 => MessageType::ConnectionAccepted,
            3 => MessageType::ConnectionRejected,
            4 => MessageType::StubMessage,
            _ => panic!("Unknown MessageType value: {value}!"),
        }
    }
}

pub struct Message {
    message_type: MessageType,
    data: Vec<u8>,
}

impl Message {
    pub fn new(message_type: MessageType) -> Self {
        Message {
            message_type: message_type,
            data: Vec::new(),
        }
    }

    pub fn push<T>(&mut self, data: T) -> &mut Message
    where
        T: serde::Serialize,
    {
        self.data.extend(bincode::serialize(&data).unwrap());
        self
    }

    pub fn pop<T>(&mut self) -> Option<T>
    where
        T: serde::de::DeserializeOwned,
    {
        let last_length = self.data.len();

        let size_of_t = std::mem::size_of::<T>();
        if last_length < size_of_t {
            return None;
        }

        // Create a slice of the last serialized object
        let start_index = last_length - size_of_t;
        let temp_data = &self.data[start_index..last_length];

        // Attempt to deserialize into the type T
        match bincode::deserialize(temp_data) {
            Ok(data) => {
                self.data.truncate(start_index); // Remove the last T-sized portion

                Some(data)
            }
            Err(_) => None,
        }
    }

    pub fn as_bytes(&self) -> Vec<u8> {
        let int_length = self.data.len() as u32;
        let mut bytes = Vec::with_capacity(
            std::mem::size_of_val(&self.message_type)
                + std::mem::size_of_val(&int_length)
                + self.data.len(),
        );
        bytes.extend(&(self.message_type as u32).to_be_bytes());
        bytes.extend(&int_length.to_be_bytes());

        bytes.extend(&self.data);
        bytes
    }
}
