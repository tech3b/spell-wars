use std::{
    io::{ErrorKind, Read},
    u8, usize,
};

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
            message_type,
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

pub struct MessageIterator<'a, R: Read> {
    pub reader: &'a mut R,
}

impl<'a, R: Read> MessageIterator<'a, R> {
    fn next_message(&mut self) -> std::io::Result<Message> {
        let mut message_type_buf = [0u8; std::mem::size_of::<MessageType>()];
        self.reader.read_exact(&mut message_type_buf)?;
        let message_type = MessageType::from(u32::from_be_bytes(message_type_buf));

        let mut length_buf = [0u8; std::mem::size_of::<u32>()];
        self.reader.read_exact(&mut length_buf)?;
        let message_length = u32::from_be_bytes(length_buf) as usize;

        let mut data = vec![0u8; message_length];
        self.reader.read_exact(&mut data)?;

        Ok(Message { message_type, data })
    }
}

impl<'a, R: Read> Iterator for MessageIterator<'a, R> {
    type Item = std::io::Result<Message>;

    fn next(&mut self) -> Option<Self::Item> {
        match self.next_message() {
            Ok(message) => Some(Ok(message)),
            Err(e) => {
                if e.kind() == ErrorKind::UnexpectedEof {
                    None
                } else {
                    Some(Err(e))
                }
            }
        }
    }
}
