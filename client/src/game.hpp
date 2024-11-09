#pragma once

#include <chrono>
#include <random>
#include "message.hpp"
#include "tfqueue.hpp"
#include "started_game.hpp"

class Game {
private:
    std::shared_ptr<TFQueue<Message>> write_message_queue;
    std::shared_ptr<TFQueue<Message>> read_message_queue;

    std::uniform_int_distribution<> distrib;
    std::mt19937 gen;

public:
    Game(std::shared_ptr<TFQueue<Message>>& _write_message_queue,
         std::shared_ptr<TFQueue<Message>>& _read_message_queue,
         std::uniform_int_distribution<>& _distrib,
         std::mt19937& _gen) :
            write_message_queue(_write_message_queue),
            read_message_queue(_read_message_queue),
            distrib(std::move(_distrib)),
            gen(std::move(_gen)) {
    }

    Game(const Game& other) = delete;

    Game(Game&& other) = default;

    StartedGame start() {
        int32_t some_number = distrib(gen);
        Message connection_requested_message(MessageType::ConnectionRequested);
        connection_requested_message << some_number;

        std::cout << "Sending my number: " << some_number << std::endl;

        write_message_queue->enqueue(std::move(connection_requested_message));

        return StartedGame(std::move(write_message_queue), std::move(read_message_queue));
    }
};
