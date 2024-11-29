#pragma once

#include "../state.hpp"
#include "overloaded.hpp"
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <misc/cpp/imgui_stdlib.h>
#include <vector>

class RunningGame : public GameState {
    struct WaitingForStub {
        std::chrono::system_clock::duration since_last_stub;

        WaitingForStub() : since_last_stub(std::chrono::system_clock::duration::zero()) {
        }
    };
    
    struct StubAccepted {
        std::chrono::system_clock::duration elapsed;
        std::vector<std::string> received;
        std::vector<std::string> will_send;
        bool should_send;

        StubAccepted(std::vector<std::string>&& _received)
            : elapsed(std::chrono::system_clock::duration::zero()),
              received(std::move(_received)),
              will_send(),
              should_send(false) {
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

            for(auto& s : stub_accepted.received) {
                ImGui::Text("Received s: %s", s.c_str());
            }

            if (ImGui::Button("Send")) {
                stub_accepted.should_send = true;
            }
            
            if (ImGui::Button("Add Input")) {
                stub_accepted.will_send.emplace_back(std::string());
            }

            for (size_t i = 0; i < stub_accepted.will_send.size(); ++i) {
                // Unique label for each input field
                ImGui::PushID(static_cast<int>(i));
                ImGui::InputText("##input", &stub_accepted.will_send[i]); // "##" prevents label display
                ImGui::SameLine();
                if (ImGui::Button("Remove")) {
                    stub_accepted.will_send.erase(stub_accepted.will_send.begin() + i); // Remove the input field
                    --i; // Adjust index due to removal
                }
                ImGui::PopID();
            }

            ImGui::Text("elapsed: %.3fs", std::chrono::duration<double>(stub_accepted.elapsed).count());

            ImGui::Render();
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

            stub_accepted.elapsed += elapsed;
        }}, state);

        return {};
    }

    virtual void io_updates(TFQueue<Message>& read_message_queue, TFQueue<Message>& write_message_queue) {
        std::visit(overloaded{[&](WaitingForStub& waiting_for_stub) {
            for(Message message : read_message_queue) {
                switch(message.type()) {
                    case MessageType::StubMessage: {
                        uint8_t number_of_strings;
                        message >> number_of_strings;
                        std::vector<std::string> strings;

                        for(int i = 0; i < number_of_strings; i++) {
                            std::string s;
                            message >> s;
                            strings.push_back(std::move(s));
                        }

                        state = StubAccepted(std::move(strings));
                        break;
                    }
                }
            }
        }, [&](StubAccepted& stub_accepted) {
            if(stub_accepted.should_send) {
                Message message_back(MessageType::StubMessage);

                for(auto s = stub_accepted.will_send.rbegin(); s != stub_accepted.will_send.rend(); s++) {
                    message_back << *s;
                }

                message_back << static_cast<uint8_t>(stub_accepted.will_send.size());

                write_message_queue.enqueue(std::move(message_back));
                state = WaitingForStub();
            }
        }}, state);
    }
};