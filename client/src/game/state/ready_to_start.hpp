#pragma once

#include "../state.hpp"
#include "running.hpp"

class ReadyToStartGame : public GameState {
private:
    uint8_t seconds_before_start;
    bool seconds_before_start_written;
    bool game_starting;
public:
    ReadyToStartGame() :
        seconds_before_start_written(true),
        game_starting(false) {
    }

    virtual std::optional<std::unique_ptr<GameState>> elapsed(std::chrono::system_clock::duration& elapsed) {
        if(!seconds_before_start_written) {
            std::cout << (int)seconds_before_start << " seconds before start" << std::endl;
            seconds_before_start_written = true;
        }
        if(game_starting) {
            std::cout << "moving to RunningGame" << std::endl;
            return std::make_optional(std::make_unique<RunningGame>());
        }
        return {};
    }

    virtual void io_updates(TFQueue<Message>& read_message_queue, TFQueue<Message>& write_message_queue) {
        for(Message& message: read_message_queue) {
            switch(message.type()) {
                case MessageType::GameAboutToStart: {
                    message >> this->seconds_before_start;
                    seconds_before_start_written = false;
                    break;
                }
                case MessageType::GameStarting: {
                    game_starting = true;
                    return;
                }
                default: {
                    break;
                }
            }
        }
    }
};