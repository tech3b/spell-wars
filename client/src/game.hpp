#pragma once

#include <chrono>
#include <random>

#include "game/input_state.hpp"
#include "game/state.hpp"
#include "message.hpp"
#include "tfqueue.hpp"

class Game {
private:
    std::unique_ptr<GameState> game_state;
    std::shared_ptr<TFQueue<Message>> write_message_queue;
    std::shared_ptr<TFQueue<Message>> read_message_queue;
    std::shared_ptr<std::atomic_flag> lost_connection;

public:
    Game(std::unique_ptr<GameState>&& _game_state,
         std::shared_ptr<TFQueue<Message>>& _write_message_queue,
         std::shared_ptr<TFQueue<Message>>& _read_message_queue,
         std::shared_ptr<std::atomic_flag>& _lost_connection)
        : game_state(std::move(_game_state)),
          write_message_queue(_write_message_queue),
          read_message_queue(_read_message_queue),
          lost_connection(_lost_connection) {
    }

    Game(const Game& other) = delete;

    Game(Game&& other) = default;

    bool is_lost_connection() {
        return lost_connection->test();
    }

    void elapsed(std::chrono::system_clock::duration& elapsed, InputState& input_state, SDL_Renderer* renderer) {
        auto updated_state = game_state->elapsed(elapsed, input_state, renderer);

        if(updated_state.has_value()) {
            game_state = std::move(updated_state.value());
        }
    }

    void io_updates() {
        game_state->io_updates(*read_message_queue, *write_message_queue);
    }
};
