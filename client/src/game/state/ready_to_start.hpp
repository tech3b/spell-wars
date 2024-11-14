#pragma once

#include "../state.hpp"
#include "running.hpp"

class ReadyToStartGame : public GameState {
private:
    enum class State {
        JUST_CREATED,
        GAME_STARTING
    };

    bool reacted;
    State state;
    uint8_t seconds_before_start;
    bool game_starting;

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
    ReadyToStartGame() :
        reacted(true),
        state(State::JUST_CREATED),
        game_starting(false) {
    }

    virtual std::optional<std::unique_ptr<GameState>> elapsed(std::chrono::system_clock::duration& elapsed,
                                                              InputState& input_state) {
        switch(state) {
            case State::JUST_CREATED: {
                react_once([&]() {
                    std::cout << (int)seconds_before_start << " seconds before start" << std::endl;
                });
                break;
            }
            case State::GAME_STARTING: {
                std::cout << "moving to RunningGame" << std::endl;
                return std::make_optional(std::make_unique<RunningGame>());
            }
        }
        return {};
    }

    virtual void io_updates(TFQueue<Message>& read_message_queue, TFQueue<Message>& write_message_queue) {
        switch(state) {
            case State::JUST_CREATED: {
                for(Message message: read_message_queue) {
                    switch(message.type()) {
                        case MessageType::GameAboutToStart: {
                            message >> this->seconds_before_start;
                            update_state(State::JUST_CREATED);
                            return;
                        }
                        case MessageType::GameStarting: {
                            update_state(State::GAME_STARTING);
                            return;
                        }
                        default: {
                            break;
                        }
                    }
                }
                break;
            }
            case State::GAME_STARTING: {
                break;
            }
        }

    }
};