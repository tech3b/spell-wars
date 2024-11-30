#pragma once

#include <SDL.h>

#include <chrono>
#include <optional>

#include "../message.hpp"
#include "../tfqueue.hpp"
#include "input_state.hpp"

class GameState {
public:
    virtual std::optional<std::unique_ptr<GameState>> elapsed(std::chrono::system_clock::duration& elapsed,
                                                              InputState& input_state,
                                                              SDL_Renderer* renderer)                    = 0;
    virtual void io_updates(TFQueue<Message>& read_message_queue, TFQueue<Message>& write_message_queue) = 0;
};
