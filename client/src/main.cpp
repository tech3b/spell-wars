#include <boost/asio.hpp>
#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <optional>
#include <tuple>
#include "message.hpp"
#include "game.hpp"
#include "tfqueue.hpp"


std::tuple<std::thread, std::thread, Game> init_game(boost::asio::ip::tcp::socket&& socket_value) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 100);

    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(std::move(socket_value));

    auto write_message_queue = std::make_shared<TFQueue<Message>>();
    auto read_message_queue = std::make_shared<TFQueue<Message>>();

    std::thread writer([=]() {
        while(true) {
            auto could_be_message = write_message_queue->dequeue();
            could_be_message.transform([&](Message& message) {
                return message.write_to(*socket);
            });
        }
    });
    std::thread reader([=]() {
        while(true) {
            auto message = read_from(*socket);

            read_message_queue->enqueue(std::move(message));
        }
    });

    Game game(write_message_queue, read_message_queue, distrib, gen);

    return std::tuple(std::move(writer), std::move(reader), std::move(game));
}

void game_loop(std::chrono::duration<double> rate, Game& game) {
    auto started_game = game.start();
    auto start = std::chrono::system_clock::now();
    auto start_io = start;

    while(true) {
        auto new_start = std::chrono::system_clock::now();

        auto elapsed = new_start - start;
        auto elapsed_io = new_start - start_io;

        started_game.elapsed(elapsed);

        if(elapsed_io > rate) {
            started_game.pull_updates();
            started_game.publish_updates();
            start_io = new_start;
        }
        start = new_start;
    }
}

int main() {
    auto rate = std::chrono::duration<double>(1.0 / 30);
    // Define the server IP address and port
    const std::string serverIP = "127.0.0.1"; // Change to your target IP
    const int serverPort = 10101;                 // Change to your target port

    try {
        boost::asio::io_context io_context;

        boost::asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(serverIP, std::to_string(serverPort));

        auto socket = boost::asio::ip::tcp::socket(io_context);

        boost::asio::connect(socket, endpoints);

        auto init_result = init_game(std::move(socket));

        game_loop(rate, std::get<2>(init_result));

        std::get<0>(init_result).join();
        std::get<1>(init_result).join();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}