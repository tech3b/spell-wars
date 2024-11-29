use core::str;
use std::io::{ErrorKind, Read, Write};

#[repr(u32)] // Ensure that the enum is represented as an u32
#[derive(Copy, Clone)]
pub enum MessageType {
    ConnectionRequested = 1,
    ConnectionAccepted = 2,
    ConnectionRejected = 3,
    UserStatusUpdate = 4,
    ReadyToStartChanged = 5,
    ReadyToStart = 6,
    StubMessage = 7,
    GameAboutToStart = 8,
    GameStarting = 9,
    ChatUpdate = 10,
}

impl From<u32> for MessageType {
    fn from(value: u32) -> MessageType {
        match value {
            1 => MessageType::ConnectionRequested,
            2 => MessageType::ConnectionAccepted,
            3 => MessageType::ConnectionRejected,
            4 => MessageType::UserStatusUpdate,
            5 => MessageType::ReadyToStartChanged,
            6 => MessageType::ReadyToStart,
            7 => MessageType::StubMessage,
            8 => MessageType::GameAboutToStart,
            9 => MessageType::GameStarting,
            10 => MessageType::ChatUpdate,
            _ => panic!("Unknown MessageType value: {value}!"),
        }
    }
}

#[derive(Clone)]
pub struct Message {
    message_type: MessageType,
    data: Vec<u8>,
}

impl Message {
    pub fn message_type(&self) -> MessageType {
        self.message_type
    }

    pub fn new(message_type: MessageType) -> Self {
        Message {
            message_type,
            data: Vec::new(),
        }
    }

    pub fn push<T: bytemuck::Pod>(&mut self, data: &T) -> &mut Self {
        let bytes = bytemuck::bytes_of(data);
        self.data.extend_from_slice(bytes);
        self
    }

    pub fn push_string(&mut self, data: &str) -> &mut Self {
        let bytes = data.as_bytes();
        self.data.extend(bytes);
        self.push(&(bytes.len() as u32))
    }

    pub fn pop<T: bytemuck::Pod>(&mut self) -> Option<T> {
        let size = std::mem::size_of::<T>();

        if self.data.len() < size {
            return None;
        }

        let start_index = self.data.len() - size;
        let bytes = self.data.split_off(start_index);

        bytemuck::try_from_bytes(&bytes).ok().cloned()
    }

    pub fn pop_string(&mut self) -> Option<String> {
        let string_size: u32 = self.pop()?;
        Some(
            str::from_utf8(&self.data.split_off(self.data.len() - string_size as usize))
                .ok()?
                .to_string(),
        )
    }

    pub fn write_to<T: Write>(&self, writer: &mut T) -> std::io::Result<()> {
        writer.write_all(&(self.message_type as u32).to_le_bytes())?;
        writer.write_all(&(self.data.len() as u32).to_le_bytes())?;
        writer.write_all(&self.data)
    }

    pub fn next_message<R: Read>(reader: &mut R) -> std::io::Result<Message> {
        let mut message_type_buf = [0u8; std::mem::size_of::<MessageType>()];
        reader.read_exact(&mut message_type_buf)?;
        let message_type = MessageType::from(u32::from_le_bytes(message_type_buf));

        let mut length_buf = [0u8; std::mem::size_of::<u32>()];
        reader.read_exact(&mut length_buf)?;
        let message_length = u32::from_le_bytes(length_buf) as usize;

        let mut data = vec![0u8; message_length];
        reader.read_exact(&mut data)?;

        Ok(Message { message_type, data })
    }

    pub fn iter<'a, R: std::io::Read>(reader: &'a mut R) -> MessageIterator<'a, R> {
        MessageIterator(reader)
    }
}

pub struct MessageIterator<'a, R: Read>(&'a mut R);

impl<'a, R: Read> Iterator for MessageIterator<'a, R> {
    type Item = std::io::Result<Message>;

    fn next(&mut self) -> Option<Self::Item> {
        match Message::next_message(self.0) {
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
