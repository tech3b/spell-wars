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
#include "game/state/just_created.hpp"


std::tuple<std::thread, std::thread, Game> init_game(boost::asio::ip::tcp::socket&& socket_value) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 100);

    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(std::move(socket_value));

    auto write_message_queue = std::make_shared<TFQueue<Message>>();
    auto read_message_queue = std::make_shared<TFQueue<Message>>();
    auto lost_connection = std::make_shared<std::atomic_flag>();

    std::thread writer([=]() {
        while(true) {
            try {
                auto could_be_message = write_message_queue->dequeue();
                if(!could_be_message.has_value()) {
                    std::cout << "no message found in queue, finishing" << std::endl;
                    break;
                }
                could_be_message.value().write_to(*socket);
            } catch (const std::exception& e) {
                std::cerr << "Error in writer thread: " << e.what() << std::endl;
                lost_connection->test_and_set();
                break;
            }
        }
    });
    std::thread reader([=]() {
        while(true) {
            try {
                auto message = read_from(*socket);
                read_message_queue->enqueue(std::move(message));
            } catch (const std::exception& e) {
                std::cerr << "Error in reader thread: " << e.what() << std::endl;
                lost_connection->test_and_set();
                write_message_queue->finish();
                break;
            }
        }
    });

    Game game(std::make_unique<JustCreatedGame>(distrib(gen)),
              write_message_queue,
              read_message_queue,
              lost_connection);

    return std::tuple(std::move(writer), std::move(reader), std::move(game));
}

void game_loop(std::chrono::duration<double> rate, Game& game) {
    auto start = std::chrono::system_clock::now();
    auto start_io = start;

    while(true) {
        auto new_start = std::chrono::system_clock::now();

        auto elapsed = new_start - start;
        auto elapsed_io = new_start - start_io;

        game.elapsed(elapsed);

        if(elapsed_io > rate) {
            if(game.is_lost_connection()) {
                break;
            }
            game.io_updates();
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

        std::cout << "Main loop is finished, please press enter" << std::endl;
        std::cin.get();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}