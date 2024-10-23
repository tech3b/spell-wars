use serde::{Deserialize, Serialize};

#[repr(u32)] // Ensure that the enum is represented as an u32
#[derive(Serialize, Deserialize)]
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

impl Into<u32> for MessageType {
    fn into(self) -> u32 {
        self as u32
    }
}

pub struct MessageHeader {
    message_type: u32,
    length: u32,
}

impl MessageHeader {
    pub fn as_bytes(&self) -> Vec<u8> {
        let mut bytes = Vec::with_capacity(std::mem::size_of::<Self>());

        bytes.extend(&self.message_type.to_be_bytes());
        bytes.extend(&self.length.to_be_bytes());

        bytes
    }
}

pub struct Message {
    header: MessageHeader,
    data: Vec<u8>,
}

impl Message {
    pub fn new(message_type: MessageType) -> Self {
        Message {
            header: MessageHeader {
                message_type: message_type.into(),
                length: 0,
            },
            data: Vec::new(),
        }
    }

    pub fn push<T>(&mut self, data: T) -> &mut Message
    where
        T: serde::Serialize,
    {
        self.data.extend(bincode::serialize(&data).unwrap());
        self.header.length = self.data.len() as u32;
        self
    }

    pub fn pop<T>(&mut self) -> Option<T>
    where
        T: serde::de::DeserializeOwned,
    {
        let last_length = self.header.length as usize;

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
                self.header.length -= size_of_t as u32; // Update the header length

                Some(data)
            }
            Err(_) => None,
        }
    }

    pub fn as_bytes(&self) -> Vec<u8> {
        let mut bytes = self.header.as_bytes();
        bytes.extend(&self.data);
        bytes
    }
}
