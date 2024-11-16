#pragma once

#include "../state.hpp"
#include "ready_to_start.hpp"
#include "reaction.hpp"
#include "overloaded.hpp"

class JustCreatedGame : public GameState {
private:
    struct JustCreated {
    };

    struct ConnectionRequested {
        Reaction reaction;

        ConnectionRequested() : reaction() {
        }
    };

    struct ConnectionAccepted {
        Reaction connection_accepted;
        Reaction ready_to_start_sent_reaction;
        bool ready_to_start_should_be_sent;
        bool ready_to_start_sent;

        ConnectionAccepted()
            : connection_accepted(),
              ready_to_start_sent_reaction(),
              ready_to_start_sent(false),
              ready_to_start_should_be_sent(false) {
        }
    };

    struct ReadyToStart {
    };

    std::variant<JustCreated, ConnectionRequested, ConnectionAccepted, ReadyToStart> state;
    int32_t server_user_number;
    int32_t client_user_number;
public:
    JustCreatedGame(int32_t _client_user_number) : 
                    state(JustCreated()),
                    client_user_number(_client_user_number) {
    }

    virtual std::optional<std::unique_ptr<GameState>> elapsed(std::chrono::system_clock::duration& elapsed,
                                                              InputState& input_state) {
        return std::visit(overloaded{[&](JustCreated& just_created) -> std::optional<std::unique_ptr<GameState>> {
            return {};
        }, [&](ConnectionRequested& connection_requested) -> std::optional<std::unique_ptr<GameState>> {
            connection_requested.reaction.react_once([&]() {
                std::cout << "ConnectionSent with number: " << client_user_number << std::endl;
            });
            return {};
        }, [&](ConnectionAccepted& connection_accepted) -> std::optional<std::unique_ptr<GameState>> {
            connection_accepted.connection_accepted.react_once([&]() {
                std::cout << "ConnectionAccepted with number: " << server_user_number << std::endl;
            });
            if(connection_accepted.ready_to_start_sent){
                connection_accepted.ready_to_start_sent_reaction.react_once([&]() {
                    std::cout << "ReadyToStartSent" << std::endl;
                });
            } else {
                if(!connection_accepted.ready_to_start_should_be_sent && input_state.state_by_key(Key::ENTER)) {
                    std::cout << "ReadyToStartShouldBeSent" << std::endl;
                    connection_accepted.ready_to_start_should_be_sent = true;
                }
            }
            return {};
        }, [&](ReadyToStart& ready_to_start) -> std::optional<std::unique_ptr<GameState>> {
            std::cout << "Moving to ReadyToStartGame" << std::endl;
            return std::make_unique<ReadyToStartGame>();
        }}, state);
    }

    virtual void io_updates(TFQueue<Message>& read_message_queue, TFQueue<Message>& write_message_queue) {
        std::visit(overloaded{[&](JustCreated& just_created)  {
            Message connection_requested_message(MessageType::ConnectionRequested);
            connection_requested_message << client_user_number;
            write_message_queue.enqueue(std::move(connection_requested_message));
            state = ConnectionRequested();
        }, [&](ConnectionRequested& connection_requested) {
            for(Message message: read_message_queue) {
                switch(message.type()) {
                    case MessageType::ConnectionAccepted:
                        message >> this->server_user_number;
                        state = ConnectionAccepted();
                        return;
                }
            }
        }, [&](ConnectionAccepted& connection_accepted) {
            if(connection_accepted.ready_to_start_should_be_sent) {
                connection_accepted.ready_to_start_should_be_sent = false;
                connection_accepted.ready_to_start_sent = true;
                Message ready_to_start(MessageType::ReadyToStart);
                write_message_queue.enqueue(std::move(ready_to_start));
            }
            for(Message message: read_message_queue) {
                switch(message.type()) {
                    case MessageType::ReadyToStart:
                        state = ReadyToStart();
                        return;
                }
            }
        }, [&](ReadyToStart& ready_to_start) {
        }}, state);
    }
};