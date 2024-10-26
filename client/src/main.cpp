#include <boost/asio.hpp>
#include <iostream>
#include "message.hpp"


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
