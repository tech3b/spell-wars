#pragma once

#include "../state.hpp"
#include "overloaded.hpp"
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>

class RunningGame : public GameState {
    struct WaitingForStub {
        std::chrono::system_clock::duration since_last_stub;

        WaitingForStub() : since_last_stub(std::chrono::system_clock::duration::zero()) {
        }
    };
    
    struct StubAccepted {
        std::chrono::system_clock::duration before_send;
        std::string received1;
        std::string received2;
        std::string will_send1;
        std::string will_send2;

        StubAccepted(std::chrono::system_clock::duration _before_send, std::string&& _received1, std::string&& _received2, std::string&& _will_send1, std::string&& _will_send2)
            : before_send(_before_send),
              received1(std::move(_received1)),
              received2(std::move(_received2)),
              will_send1(std::move(_will_send1)),
              will_send2(std::move(_will_send2)) {
        }
    };

    std::variant<WaitingForStub, StubAccepted> state;
public:
    RunningGame()
        : state(WaitingForStub()) {
    }

    virtual std::optional<std::unique_ptr<GameState>> elapsed(std::chrono::system_clock::duration& elapsed,
                                                              InputState& input_state,
                                                              SDL_Renderer* renderer) {

        std::visit(overloaded{[&](WaitingForStub& waiting_for_stub) {
            ImGui_ImplSDLRenderer2_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();
            
            auto millis_elapsed = std::chrono::duration<double, std::milli>(elapsed).count();
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", millis_elapsed, 1000.0f / millis_elapsed);

            ImGui::Text("Waiting for stub for %.3fs", std::chrono::duration<double>(waiting_for_stub.since_last_stub).count());

            ImGui::Render();
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

            waiting_for_stub.since_last_stub += elapsed;

        }, [&](StubAccepted& stub_accepted) {
            ImGui_ImplSDLRenderer2_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();
            
            auto millis_elapsed = std::chrono::duration<double, std::milli>(elapsed).count();
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", millis_elapsed, 1000.0f / millis_elapsed);

            ImGui::Text("Received s1: %s", stub_accepted.received1.c_str());
            ImGui::Text("Received s2: %s", stub_accepted.received2.c_str());

            ImGui::Text("About to send s1: %s", stub_accepted.will_send1.c_str());
            ImGui::Text("About to send s2: %s", stub_accepted.will_send2.c_str());
            ImGui::Text("before send: %.3fs", std::chrono::duration<double>(stub_accepted.before_send).count());

            ImGui::Render();
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

            stub_accepted.before_send -= elapsed;
        }}, state);

        return {};
    }

    virtual void io_updates(TFQueue<Message>& read_message_queue, TFQueue<Message>& write_message_queue) {
        std::visit(overloaded{[&](WaitingForStub& waiting_for_stub) {
            for(Message message : read_message_queue) {
                switch(message.type()) {
                    case MessageType::StubMessage: {
                        std::string s1;
                        std::string s2;
                        message >> s2 >> s1;
                        state = StubAccepted(std::chrono::seconds(10),
                                             std::move(s1),
                                             std::move(s2),
                                             "To tell you I'm sorry for everything that I've done",
                                             "But when I call, you never seem to be home");
                        break;
                    }
                }
            }
        }, [&](StubAccepted& stub_accepted) {
            if(stub_accepted.before_send <= std::chrono::system_clock::duration::zero()) {

                Message message_back(MessageType::StubMessage);

                message_back << stub_accepted.will_send1 << stub_accepted.will_send2;

                write_message_queue.enqueue(std::move(message_back));
                state = WaitingForStub();
            }
        }}, state);
    }
};