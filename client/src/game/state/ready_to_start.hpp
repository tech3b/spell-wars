#pragma once

#include "../state.hpp"
#include "running.hpp"
#include "reaction.hpp"
#include "overloaded.hpp"

class ReadyToStartGame : public GameState {
private:
    struct WaitingForStart {
        Reaction reaction;
        uint8_t seconds_before_start;

        WaitingForStart(uint8_t _seconds_before_start) : reaction(), seconds_before_start(_seconds_before_start) {
        }

        WaitingForStart() : reaction(true), seconds_before_start() {
        }
    };

    struct Starting {
    };

    std::variant<WaitingForStart, Starting> state;
public:
    ReadyToStartGame() :
        state(WaitingForStart()) {
    }

    virtual std::optional<std::unique_ptr<GameState>> elapsed(std::chrono::system_clock::duration& elapsed,
                                                              InputState& input_state) {

        return std::visit(overloaded{[&](WaitingForStart& waiting_for_start) -> std::optional<std::unique_ptr<GameState>> {
            waiting_for_start.reaction.react_once([&]() {
                std::cout << (int)waiting_for_start.seconds_before_start << " seconds before start" << std::endl;
            });
            return {};
        }, [&](Starting& starting) -> std::optional<std::unique_ptr<GameState>> {
            std::cout << "moving to RunningGame" << std::endl;
            return std::make_optional(std::make_unique<RunningGame>());
        }}, state);
    }

    virtual void io_updates(TFQueue<Message>& read_message_queue, TFQueue<Message>& write_message_queue) {
        std::visit(overloaded{[&](WaitingForStart& waiting_for_start) {
            for(Message message: read_message_queue) {
                switch(message.type()) {
                    case MessageType::GameAboutToStart: {
                        uint8_t seconds_before_start;
                        message >> seconds_before_start;
                        state = WaitingForStart(seconds_before_start);
                        return;
                    }
                    case MessageType::GameStarting: {
                        state = Starting();
                        return;
                    }
                    default: {
                        break;
                    }
                }
            }
        }, [&](Starting& starting) {
        }}, state);
    }
};