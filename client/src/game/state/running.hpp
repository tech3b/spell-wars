#pragma once

#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <vector>

#include "../chat.hpp"
#include "../state.hpp"
#include "overloaded.hpp"

class RunningGame : public GameState {
    struct WaitingForStub {
        std::chrono::system_clock::duration since_last_stub;

        WaitingForStub()
            : since_last_stub(std::chrono::system_clock::duration::zero()) {
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
    Chat chat;

public:
    RunningGame(Chat&& _chat)
        : state(WaitingForStub()),
          chat(std::move(_chat)) {
    }

    virtual std::optional<std::unique_ptr<GameState>> elapsed(std::chrono::system_clock::duration& elapsed,
                                                              InputState& input_state,
                                                              SDL_Renderer* renderer) {
        std::visit(overloaded {[&](WaitingForStub& waiting_for_stub) {
            ImGui_ImplSDLRenderer2_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            ImGuiIO& io         = ImGui::GetIO();
            float screen_width  = io.DisplaySize.x;
            float screen_height = io.DisplaySize.y;

            float main_window_height = 400;

            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(screen_width, main_window_height), ImGuiCond_Always);

            ImGui::Begin("main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

            auto millis_elapsed = std::chrono::duration<double, std::milli>(elapsed).count();
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", millis_elapsed, 1000.0f / millis_elapsed);

            ImGui::Text("Waiting for stub for %.3fs", std::chrono::duration<double>(waiting_for_stub.since_last_stub).count());

            ImGui::End();

            float chat_height = screen_height - main_window_height;
            float chat_pos_y  = screen_height - chat_height;

            ImGui::SetNextWindowPos(ImVec2(0, chat_pos_y), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(screen_width, chat_height), ImGuiCond_Always);

            chat.render_chat(input_state.state_by_key(Key::ENTER));

            ImGui::Render();
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

            waiting_for_stub.since_last_stub += elapsed;
        },
                               [&](StubAccepted& stub_accepted) {
            ImGui_ImplSDLRenderer2_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            ImGuiIO& io         = ImGui::GetIO();
            float screen_width  = io.DisplaySize.x;
            float screen_height = io.DisplaySize.y;

            float main_window_height = 400;

            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(screen_width, main_window_height), ImGuiCond_Always);

            ImGui::Begin("main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

            auto millis_elapsed = std::chrono::duration<double, std::milli>(elapsed).count();
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", millis_elapsed, 1000.0f / millis_elapsed);

            for(auto& s : stub_accepted.received) {
                ImGui::Text("Received s: %s", s.c_str());
            }

            if(ImGui::Button("Send")) {
                stub_accepted.should_send = true;
            }

            if(ImGui::Button("Add Input")) {
                stub_accepted.will_send.emplace_back(std::string());
            }

            for(size_t i = 0; i < stub_accepted.will_send.size(); ++i) {
                // Unique label for each input field
                ImGui::PushID(static_cast<int>(i));
                ImGui::InputText("##input",
                                 &stub_accepted.will_send[i]);  // "##" prevents label display
                ImGui::SameLine();
                if(ImGui::Button("Remove")) {
                    stub_accepted.will_send.erase(stub_accepted.will_send.begin() + i);  // Remove the input field
                    --i;                                                                 // Adjust index due to removal
                }
                ImGui::PopID();
            }

            ImGui::Text("elapsed: %.3fs", std::chrono::duration<double>(stub_accepted.elapsed).count());

            ImGui::End();

            float chat_height = screen_height - main_window_height;
            float chat_pos_y  = screen_height - chat_height;

            ImGui::SetNextWindowPos(ImVec2(0, chat_pos_y), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(screen_width, chat_height), ImGuiCond_Always);

            chat.render_chat(input_state.state_by_key(Key::ENTER));

            ImGui::Render();
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

            stub_accepted.elapsed += elapsed;
        }},
                   state);

        return {};
    }

    virtual void io_updates(TFQueue<Message>& read_message_queue, TFQueue<Message>& write_message_queue) {
        std::visit(overloaded {[&](WaitingForStub& waiting_for_stub) {
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
                case MessageType::ChatUpdate: {
                    chat.push_message(message);
                    break;
                }
                }
            }
        },
                               [&](StubAccepted& stub_accepted) {
            for(Message message : read_message_queue) {
                switch(message.type()) {
                case MessageType::ChatUpdate: {
                    chat.push_message(message);
                    break;
                }
                }
            }
            if(stub_accepted.should_send) {
                Message message_back(MessageType::StubMessage);

                for(auto s = stub_accepted.will_send.rbegin(); s != stub_accepted.will_send.rend(); s++) {
                    message_back << *s;
                }

                message_back << static_cast<uint8_t>(stub_accepted.will_send.size());

                write_message_queue.enqueue(std::move(message_back));
                state = WaitingForStub();
            }
        }},
                   state);

        chat.commit(write_message_queue);
    }
};
