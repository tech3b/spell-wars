#pragma once

#include "../state.hpp"
#include "ready_to_start.hpp"

class JustCreatedGame : public GameState {
private:
    enum class State {
        JustCreated,
        ConnectionSent,
        ConnectionAccepted,
        ReadyToStartReceived,
        ReadyToStartShouldBeSent,
        ReadyToStartSent
    };

    State state;
    bool reacted;
    int32_t server_user_number;
    int32_t client_user_number;

    void update_state(State updated_state) {
        state = updated_state;
        reacted = false;
    }

    void react_once(const std::function<void()>& f) {
        if(!reacted) {
            f();
            reacted = true;
        }
    }
public:
    JustCreatedGame(int32_t _client_user_number) : 
                        state(State::JustCreated),
                        reacted(false),
                        client_user_number(_client_user_number) {
    }

    virtual std::optional<std::unique_ptr<GameState>> elapsed(std::chrono::system_clock::duration& elapsed,
                                                              InputState& input_state) {
        switch(state) {
            case State::JustCreated:
            case State::ReadyToStartShouldBeSent: {
                break;
            }
            case State::ConnectionSent: {
                react_once([&]() {
                    std::cout << "ConnectionSent with number: " << client_user_number << std::endl;
                });
                break;
            }
            case State::ReadyToStartSent: {
                react_once([]() {
                    std::cout << "ReadyToStartSent" << std::endl;
                });
                break;
            }
            case State::ReadyToStartReceived: {
                std::cout << "moving to ReadyToStartGame" << std::endl;
                return std::make_optional(std::make_unique<ReadyToStartGame>());
            }
            case State::ConnectionAccepted: {
                react_once([&]() {
                    std::cout << "ConnectionAccepted" << std::endl;
                });
                if(input_state.state_by_key(Key::ENTER)) {
                    std::cout << "ReadyToStartShouldBeSent" << std::endl;
                    update_state(State::ReadyToStartShouldBeSent);
                }
                break;
            }
        }
        return {};
    }

    virtual void io_updates(TFQueue<Message>& read_message_queue, TFQueue<Message>& write_message_queue) {
        switch (state)
        {
            case State::JustCreated: {
                update_state(State::ConnectionSent);
                Message connection_requested_message(MessageType::ConnectionRequested);
                connection_requested_message << client_user_number;
                write_message_queue.enqueue(std::move(connection_requested_message));
                return;
            }            
            case State::ConnectionSent: {
                for(Message message: read_message_queue) {
                    switch(message.type()) {
                        case MessageType::ConnectionAccepted:
                            message >> this->server_user_number;
                            update_state(State::ConnectionAccepted);
                            return;
                    }
                }
                break;
            }
            case State::ConnectionAccepted:
            case State::ReadyToStartSent: {
                for(Message message: read_message_queue) {
                    switch(message.type()) {
                        case MessageType::ReadyToStart:
                            update_state(State::ReadyToStartReceived);
                            return;
                    }
                }
            }
            case State::ReadyToStartReceived:
                break;
            case State::ReadyToStartShouldBeSent: {
                update_state(State::ReadyToStartSent);
                Message ready_to_start(MessageType::ReadyToStart);
                write_message_queue.enqueue(std::move(ready_to_start));
                break;
            }
        }
    }
};