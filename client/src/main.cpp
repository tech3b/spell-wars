#include <boost/asio.hpp>
#include <iostream>
#include <random>
#include "message.hpp"


int main() {
    // Define the server IP address and port
    const std::string serverIP = "127.0.0.1"; // Change to your target IP
    const int serverPort = 10101;                 // Change to your target port

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 100);

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
        int32_t some_number = distrib(gen);

        std::cout << "Sending my number: " << some_number << std::endl;

        connection_requested_message << some_number;
        connection_requested_message.write_to(socket);

        while(true) {
            auto message = read_from(socket);

            switch(message.type()) {
                case MessageType::ConnectionAccepted: {
                    int32_t number_of_user;
                    message >> number_of_user;
                    std::cout << "Connection accepted: Welcome! You're our "<< number_of_user << " customer today!" << std::endl;
                    break;
                }
                case MessageType::StubMessage: {
                    std::string s1;
                    std::string s2;
                    message >> s2 >> s1;

                    std::cout << "s1: " << s1 << std::endl << "s2: " << s2 << std::endl;

                    Message message_back(MessageType::StubMessage);
                    std::string s1_back("To tell you I'm sorry for everything that I've done");
                    std::string s2_back("But when I call, you never seem to be home");

                    message_back << s1_back << s2_back;

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
