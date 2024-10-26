#include <iostream>
#include <vector>
#include <cstdint>
#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>

enum MessageType {
    ConnectionRequested = 1,
    ConnectionAccepted = 2,
    ConnectionRejected = 3,
    StubMessage = 4,
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
    
    Message(Message&& other) noexcept 
        : messageType(other.messageType), data(std::move(other.data)) {
    }

    MessageType type() {
        return this->messageType;
    }

    void write_async_to(boost::asio::ip::tcp::socket& socket, std::function<void(const boost::system::error_code&, std::size_t)> handler) {       
        std::array<boost::asio::const_buffer, 3> buffers;

        uint32_t messageTypeSerialized = boost::endian::native_to_big(static_cast<uint32_t>(messageType));
        buffers[0] = boost::asio::buffer(&messageTypeSerialized, sizeof(messageTypeSerialized));

        uint32_t dataLengthSerialized = boost::endian::native_to_big(static_cast<uint32_t>(data.size()));
        buffers[1] = boost::asio::buffer(&dataLengthSerialized, sizeof(dataLengthSerialized));
        buffers[2] = boost::asio::buffer(data);  

        boost::asio::async_write(socket, buffers,
            [handler](const boost::system::error_code& error, std::size_t bytes_transferred) {
                handler(error, bytes_transferred);
            });
    }

    std::size_t write_to(boost::asio::ip::tcp::socket& socket) {     
        std::array<boost::asio::const_buffer, 3> buffers;

        uint32_t messageTypeSerialized = boost::endian::native_to_big(static_cast<uint32_t>(messageType));
        buffers[0] = boost::asio::buffer(&messageTypeSerialized, sizeof(messageTypeSerialized));

        uint32_t dataLengthSerialized = boost::endian::native_to_big(static_cast<uint32_t>(data.size()));
        buffers[1] = boost::asio::buffer(&dataLengthSerialized, sizeof(dataLengthSerialized));
        buffers[2] = boost::asio::buffer(data);  

        return boost::asio::write(socket, buffers);
    }

    template<typename DataType>
    friend Message& operator << (Message& msg, const DataType& data)
    {
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

    friend Message& operator<<(Message& msg, const std::string& input) {
        msg.data.assign(input.begin(), input.end());
        return msg;
    }

    friend Message& operator>>(Message& msg, std::string& data)
    {
        data.assign(msg.data.begin(), msg.data.end());
        return msg;
    }	
};

Message read_from(boost::asio::ip::tcp::socket& socket) {
    uint32_t messageTypeSerialized = 0;
    boost::asio::read(socket, boost::asio::buffer(&messageTypeSerialized, sizeof(messageTypeSerialized)));
    MessageType messageType = static_cast<MessageType>(boost::endian::big_to_native(messageTypeSerialized));

    uint32_t dataLengthSerialized = 0;
    boost::asio::read(socket, boost::asio::buffer(&dataLengthSerialized, sizeof(dataLengthSerialized)));
    size_t dataLength = boost::endian::big_to_native(dataLengthSerialized);

    std::vector<uint8_t> data;
    data.resize(dataLength);
    boost::asio::read(socket, boost::asio::buffer(data));

    return Message(messageType, std::move(data));
}

int main() {
    // Define the server IP address and port
    const std::string serverIP = "127.0.0.1"; // Change to your target IP
    const int serverPort = 10101;                 // Change to your target port

    try {
        // Create an io_context object
        boost::asio::io_context io_context;

        // Create a resolver to resolve the server address
        boost::asio::ip::tcp::resolver resolver(io_context);
        boost::asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(serverIP, std::to_string(serverPort));

        // Create a socket
        boost::asio::ip::tcp::socket socket(io_context);

        // Connect to the server
        boost::asio::connect(socket, endpoints);

        Message connection_requested_message(MessageType::ConnectionRequested);
        connection_requested_message.write_to(socket);

        while(true) {
            auto message = read_from(socket);

            switch(message.type()) {
                case MessageType::ConnectionAccepted: {
                    std::string accepted_message;
                    message >> accepted_message;
                    std::cout << "Connection accepted: " << accepted_message << std::endl;
                    break;
                }
                case MessageType::StubMessage: {
                    std::string accepted_message;
                    message >> accepted_message;
                    std::cout << "StubMessage: " << accepted_message << std::endl;

                    Message message_back(MessageType::StubMessage);
                    std::string message_back_string("Allo, yoba?");
                    message_back << message_back_string;
                    message_back.write_to(socket);
                    break;
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
