#pragma once

#include "../state.hpp"
#include "ready_to_start.hpp"

class JustCreatedGame : public GameState {
private:
    bool connection_request_sent;
    bool connection_request_sent_written;
    bool connection_accepted;
    bool connection_accepted_written;
    int32_t server_user_number;
    int32_t client_user_number;

public:
    JustCreatedGame(int32_t _client_user_number) : 
                        connection_request_sent(false),
                        connection_request_sent_written(false),
                        connection_accepted(false),
                        connection_accepted_written(true),
                        client_user_number(_client_user_number) {
    }

    virtual std::optional<std::unique_ptr<GameState>> elapsed(std::chrono::system_clock::duration& elapsed) {
        if(!connection_accepted_written) {
            std::cout << "Connection accepted: Welcome! You're our "<< server_user_number << " customer today!" << std::endl;
            connection_accepted_written = true;
        }
        if(connection_request_sent && !connection_request_sent_written) {
            std::cout << "Sending my number: " << client_user_number << std::endl;
            connection_request_sent_written = true;
        }
        if(connection_request_sent && connection_accepted) {
            std::cout << "moving to ReadyToStartGame" << std::endl;
            return std::make_optional(std::make_unique<ReadyToStartGame>());
        }
        return {};
    }

    virtual void io_updates(TFQueue<Message>& read_message_queue, TFQueue<Message>& write_message_queue) {
        if(!connection_request_sent) {
            Message connection_requested_message(MessageType::ConnectionRequested);
            connection_requested_message << client_user_number;
            write_message_queue.enqueue(std::move(connection_requested_message));
            connection_request_sent = true;
        }
        for(Message& message: read_message_queue) {
            switch(message.type()) {
                case MessageType::ConnectionAccepted: {
                    message >> this->server_user_number;
                    connection_accepted = true;
                    connection_accepted_written = false;
                }
                default: {
                    break;
                }
            }
        }
    }
};