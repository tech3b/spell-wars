#pragma once

#include "message.hpp"
#include "tfqueue.hpp"
#include <atomic>

class StartedGame {
private:
    std::shared_ptr<TFQueue<Message>> write_message_queue;
    std::shared_ptr<TFQueue<Message>> read_message_queue;
    std::shared_ptr<std::atomic_flag> lost_connection;

    bool is_ready_to_start;
    std::chrono::system_clock::duration since_last_stub;
    bool accepted_stub;
public:
    StartedGame(std::shared_ptr<TFQueue<Message>>&& _write_message_queue,
                std::shared_ptr<TFQueue<Message>>&& _read_message_queue,
                std::shared_ptr<std::atomic_flag>&& _lost_connection):
                    write_message_queue(std::move(_write_message_queue)),
                    read_message_queue(std::move(_read_message_queue)),
                    lost_connection(std::move(_lost_connection)),
                    is_ready_to_start(false),
                    since_last_stub(),
                    accepted_stub(false) {
    }

    StartedGame(const StartedGame& other) = delete;

    StartedGame(StartedGame&& other) = default;

    bool elapsed(std::chrono::system_clock::duration& elapsed) {
        if(lost_connection->test()) {
            return false;
        }
        if(accepted_stub) {
            since_last_stub += elapsed;
        }
        return true;
    }

    void pull_updates() {
        for(Message& message : *read_message_queue) {
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

            write_message_queue->enqueue(std::move(message_back));
        }
    }
};