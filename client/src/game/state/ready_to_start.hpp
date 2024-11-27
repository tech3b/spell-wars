#pragma once

#include "../state.hpp"
#include "running.hpp"
#include "overloaded.hpp"
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>

class ReadyToStartGame : public GameState {
private:
    struct WaitingForStart {
        uint8_t seconds_before_start;
        bool seconds_received;

        WaitingForStart() : seconds_before_start(), seconds_received(false) {
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
                                                              InputState& input_state,
                                                              SDL_Renderer* renderer) {

        return std::visit(overloaded{[&](WaitingForStart& waiting_for_start) -> std::optional<std::unique_ptr<GameState>> {
            ImGui_ImplSDLRenderer2_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();
            
            auto millis_elapsed = std::chrono::duration<double, std::milli>(elapsed).count();
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", millis_elapsed, 1000.0f / millis_elapsed);
            if(!waiting_for_start.seconds_received) {
                ImGui::Text("About to start: get ready");
            } else {
                ImGui::Text("About to start: %d", waiting_for_start.seconds_before_start);
            }
            
            ImGui::Render();
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

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
                        waiting_for_start.seconds_before_start = seconds_before_start;
                        waiting_for_start.seconds_received = true;
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