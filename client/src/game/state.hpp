#pragma once

#include <optional>
#include <chrono>
#include "../tfqueue.hpp"
#include "../message.hpp"
#include "input_state.hpp"

class GameState {
public:
    virtual std::optional<std::unique_ptr<GameState>> elapsed(std::chrono::system_clock::duration& elapsed,
                                                              InputState& input_state) = 0;
    virtual void io_updates(TFQueue<Message>& read_message_queue, TFQueue<Message>& write_message_queue) = 0;
};