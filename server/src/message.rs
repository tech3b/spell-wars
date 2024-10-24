use std::{u8, usize};

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

    // TODO: read a list of messages until Vec is exhausted
    pub fn from_bytes(message_bytes: Vec<u8>) -> Option<Message> {
        let length_field_length = std::mem::size_of::<u32>();
        let message_type_length = std::mem::size_of::<MessageType>();
        let start_of_data = message_type_length + length_field_length;

        if message_bytes.len() < start_of_data {
            return None;
        }

        let message_type = MessageType::from(u32::from_be_bytes(
            message_bytes[..message_type_length]
                .try_into()
                .expect("Expected an array slice of size 4 with the message type"),
        ));
        let data_length = u32::from_be_bytes(
            message_bytes[message_type_length..length_field_length]
                .try_into()
                .expect("Expected an array slice of size 4 with the data length"),
        ) as usize;

        let mut message = Message::new(message_type);
        message
            .data
            .copy_from_slice(&message_bytes[start_of_data..start_of_data + data_length]);
        Some(message)
    }
}
