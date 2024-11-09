#pragma once

#include <chrono>
#include <queue>
#include <random>
#include "message.hpp"


class Game {
private:
    bool is_ready_to_start;
    std::shared_ptr<std::queue<Message>> write_message_queue;
    std::shared_ptr<std::mutex> write_queue_mutex;

    std::shared_ptr<std::queue<Message>> read_message_queue;
    std::shared_ptr<std::mutex> read_queue_mutex;

    std::chrono::system_clock::duration since_last_stub;
    bool accepted_stub;
    std::uniform_int_distribution<> distrib;
    std::mt19937 gen;

public:
    Game(std::shared_ptr<std::queue<Message>>& _write_message_queue,
         std::shared_ptr<std::mutex>& _write_queue_mutex,
         std::shared_ptr<std::queue<Message>>& _read_message_queue,
         std::shared_ptr<std::mutex>& _read_queue_mutex,
         std::uniform_int_distribution<>& _distrib,
         std::mt19937& _gen) :
            write_message_queue(_write_message_queue),
            write_queue_mutex(_write_queue_mutex),
            read_message_queue(_read_message_queue),
            read_queue_mutex(_read_queue_mutex),
            is_ready_to_start(false),
            since_last_stub(),
            accepted_stub(false),
            distrib(std::move(_distrib)),
            gen(std::move(_gen)) {
    }

    Game(const Game& other) = delete;

    Game(Game&& other) = default;

    void start() {
        int32_t some_number = distrib(gen);
        Message connection_requested_message(MessageType::ConnectionRequested);
        connection_requested_message << some_number;

        std::cout << "Sending my number: " << some_number << std::endl;

        {
            std::unique_lock<std::mutex> lock(*write_queue_mutex);
            (*write_message_queue).push(std::move(connection_requested_message));
        }
    }

    void elapsed(std::chrono::system_clock::duration& elapsed) {
        if(accepted_stub) {
            since_last_stub += elapsed;
        }
    }

    void pull_updates() {
        std::unique_lock<std::mutex> lock(*read_queue_mutex);
        while (!(*read_message_queue).empty()) {
            auto message = std::move((*read_message_queue).front());
            (*read_message_queue).pop();

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

                    accepted_stub = true;
                    break;
                }
            }
        }
    }

    void publish_updates() {
        if(accepted_stub && since_last_stub > std::chrono::seconds(2)) {
            accepted_stub = false;
            since_last_stub -= std::chrono::seconds(2);

            Message message_back(MessageType::StubMessage);
            std::string s1_back("To tell you I'm sorry for everything that I've done");
            std::string s2_back("But when I call, you never seem to be home");

            message_back << s1_back << s2_back;

            std::unique_lock<std::mutex> lock(*write_queue_mutex);
            (*write_message_queue).push(std::move(message_back));
        }
    }
};