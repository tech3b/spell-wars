#pragma once

#include <iostream>
#include <vector>
#include <cstdint>
#include <tuple>
#include <boost/asio.hpp>

enum MessageType {
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
};

class Message {
private:
    MessageType messageType;
    std::vector<uint8_t> data;

public:
    Message(MessageType _messageType): messageType(_messageType) {
    }

    Message(MessageType _messageType, std::vector<uint8_t>&& _data) noexcept :
        messageType(_messageType),
        data(std::move(_data)) {
    }
    
    Message(Message&& other) noexcept = default;
    
    Message(const Message&) noexcept = delete;

    MessageType type() {
        return this->messageType;
    }

    void write_async_to(boost::asio::ip::tcp::socket& socket, std::function<void(const boost::system::error_code&, std::size_t)> handler) {       
        auto inputs = prepare_inputs();

        boost::asio::async_write(socket, std::array<boost::asio::const_buffer, 3> {
            boost::asio::buffer(&std::get<0>(inputs), sizeof(std::get<0>(inputs))),
            boost::asio::buffer(&std::get<1>(inputs), sizeof(std::get<1>(inputs))),
            boost::asio::buffer(data.data(), data.size())
        },[handler](const boost::system::error_code& error, std::size_t bytes_transferred) {
            handler(error, bytes_transferred);
        });
    }

    std::size_t write_to(boost::asio::ip::tcp::socket& socket) {     
        auto inputs = prepare_inputs();

        return boost::asio::write(socket, std::array<boost::asio::const_buffer, 3> {
            boost::asio::buffer(&std::get<0>(inputs), sizeof(std::get<0>(inputs))),
            boost::asio::buffer(&std::get<1>(inputs), sizeof(std::get<1>(inputs))),
            boost::asio::buffer(data.data(), data.size())
        });
    }

    std::tuple<uint32_t, uint32_t> prepare_inputs() {
        return { static_cast<uint32_t>(messageType), static_cast<uint32_t>(data.size()) };
    }

    friend Message& operator<<(Message& msg, const std::string& input) {
        msg.data.insert(msg.data.end(), input.begin(), input.end());
        return msg << static_cast<uint32_t>(input.length());
    }

    template<typename DataType>
    friend Message& operator << (Message& msg, const DataType& data) {
        // Check that the type of the data being pushed is trivially copyable
        static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

        // Cache current size of vector, as this will be the point we insert the data
        size_t i = msg.data.size();

        // Resize the vector by the size of the data being pushed
        msg.data.resize(msg.data.size() + sizeof(DataType));

        // Physically copy the data into the newly allocated vector space
        std::memcpy(msg.data.data() + i, &data, sizeof(DataType));

        // Return the target message so it can be "chained"
        return msg;
    }

    friend Message& operator>>(Message& msg, std::string& data) {
        uint32_t numBytes;
        msg >> numBytes;

        data.assign(msg.data.end() - numBytes, msg.data.end());

        msg.data.resize(msg.data.size() - numBytes);

        return msg;
    }

    // Pulls any POD-like data form the message buffer
    template<typename DataType>
    friend Message& operator >> (Message& msg, DataType& data) {
        // Check that the type of the data being pushed is trivially copyable
        static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pulled from vector");

        // Cache the location towards the end of the vector where the pulled data starts
        size_t i = msg.data.size() - sizeof(DataType);

        // Physically copy the data from the vector into the user variable
        std::memcpy(&data, msg.data.data() + i, sizeof(DataType));

        // Shrink the vector to remove read bytes, and reset end position
        msg.data.resize(i);

        // Return the target message so it can be "chained"
        return msg;
    }
};

Message read_from(boost::asio::ip::tcp::socket& socket) {
    uint32_t messageTypeSerialized = 0;
    boost::asio::read(socket, boost::asio::buffer(&messageTypeSerialized, sizeof(messageTypeSerialized)));
    MessageType messageType = static_cast<MessageType>(messageTypeSerialized);

    uint32_t dataLengthSerialized = 0;
    boost::asio::read(socket, boost::asio::buffer(&dataLengthSerialized, sizeof(dataLengthSerialized)));
    size_t dataLength = dataLengthSerialized;

    std::vector<uint8_t> data;
    data.resize(dataLength);
    boost::asio::read(socket, boost::asio::buffer(data));

    return Message(messageType, std::move(data));
}