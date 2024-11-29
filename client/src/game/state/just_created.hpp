#pragma once

#include "../state.hpp"
#include "ready_to_start.hpp"
#include "reaction.hpp"
#include "overloaded.hpp"
#include "../chat.hpp"

#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>

class JustCreatedGame : public GameState {
private:
    struct JustCreated {
    };

    struct ConnectionRequested {
        Reaction reaction;

        ConnectionRequested() : reaction() {
        }
    };

    struct ConnectionAccepted {
        std::unordered_map<int32_t, bool> user_to_state;
        bool state_changed;
        bool is_ready;
        Chat chat;

        ConnectionAccepted(std::unordered_map<int32_t, bool>&& _user_to_state, Chat&& _chat)
            : user_to_state(std::move(_user_to_state)),
              state_changed(false),
              is_ready(false),
              chat(std::move(_chat)) {
        }
    };

    struct ReadyToStart {
        std::unordered_map<int32_t, bool> user_to_state;

        ReadyToStart(std::unordered_map<int32_t, bool>&& _user_to_state) : user_to_state(std::move(_user_to_state)){
        }
    };

    std::variant<JustCreated, ConnectionRequested, ConnectionAccepted, ReadyToStart> state;
    int32_t server_user_number;
    int32_t client_user_number;
public:
    JustCreatedGame(int32_t _client_user_number) : 
                    state(JustCreated()),
                    client_user_number(_client_user_number) {
    }

    virtual std::optional<std::unique_ptr<GameState>> elapsed(std::chrono::system_clock::duration& elapsed,
                                                              InputState& input_state,
                                                              SDL_Renderer* renderer) {
        return std::visit(overloaded{[&](JustCreated& just_created) -> std::optional<std::unique_ptr<GameState>> {
            return {};
        }, [&](ConnectionRequested& connection_requested) -> std::optional<std::unique_ptr<GameState>> {
            connection_requested.reaction.react_once([&]() {
                std::cout << "ConnectionSent with number: " << client_user_number << std::endl;
            });
            return {};
        }, [&](ConnectionAccepted& connection_accepted) -> std::optional<std::unique_ptr<GameState>> {    
            ImGui_ImplSDLRenderer2_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();
            
            auto millis_elapsed = std::chrono::duration<double, std::milli>(elapsed).count();
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", millis_elapsed, 1000.0f / millis_elapsed);
            ImGui::Text("Me %d", this->server_user_number);
            ImGui::SameLine();
            if (ImGui::Checkbox("##Checkbox", &connection_accepted.is_ready)) {
                connection_accepted.state_changed = true;
            }

            for (auto& pair : connection_accepted.user_to_state) {
                ImGui::BeginDisabled();
                bool is_checked = pair.second != 0;
                ImGui::Text("User %d", pair.first);
                ImGui::SameLine();
                ImGui::Checkbox(("##Checkbox" + std::to_string(pair.first)).c_str(), &is_checked);
                ImGui::EndDisabled();
            }

            connection_accepted.chat.render_chat(input_state.state_by_key(Key::ENTER));
            
            ImGui::Render();
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

            return {};
        }, [&](ReadyToStart& ready_to_start) -> std::optional<std::unique_ptr<GameState>> {
            std::cout << "Moving to ReadyToStartGame" << std::endl;
            return std::make_unique<ReadyToStartGame>();
        }}, state);
    }

    virtual void io_updates(TFQueue<Message>& read_message_queue, TFQueue<Message>& write_message_queue) {
        std::visit(overloaded{[&](JustCreated& just_created)  {
            Message connection_requested_message(MessageType::ConnectionRequested);
            connection_requested_message << client_user_number;
            write_message_queue.enqueue(std::move(connection_requested_message));
            state = ConnectionRequested();
        }, [&](ConnectionRequested& connection_requested) {
            for(Message message: read_message_queue) {
                switch(message.type()) {
                    case MessageType::ConnectionAccepted:
                        message >> this->server_user_number;
                        uint8_t user_states_len;
                        message >> user_states_len;

                        std::unordered_map<int32_t, bool> user_to_state;

                        for(int i = 0; i < user_states_len; i++) {
                            uint8_t is_ready;
                            message >> is_ready;
                            int32_t user;
                            message >> user;
                            user_to_state[user] = is_ready != 0;
                        }

                        state = ConnectionAccepted(std::move(user_to_state), Chat());
                        return;
                }
            }
        }, [&](ConnectionAccepted& connection_accepted) {
            if(connection_accepted.state_changed) {
                connection_accepted.state_changed = false;
                Message ready_to_start(MessageType::ReadyToStartChanged);
                ready_to_start << static_cast<uint8_t>(connection_accepted.is_ready);
                write_message_queue.enqueue(std::move(ready_to_start));
            }
            for(Message message: read_message_queue) {
                switch(message.type()) {
                    case MessageType::UserStatusUpdate: {
                        uint8_t updates_len;
                        message >> updates_len;

                        for(int i = 0; i < updates_len; i++) {
                            uint8_t user_status;
                            message >> user_status;
                            int32_t user;
                            message >> user;
                            if(user_status == 2) {
                                connection_accepted.user_to_state.erase(user);
                            } else {
                                connection_accepted.user_to_state[user] = user_status != 0;
                            }
                        }
                        break;
                    }
                    case MessageType::ChatUpdate: {
                        uint8_t message_number;
                        message >> message_number;
                        for(int i = 0; i < message_number; i++) {
                            int32_t user;
                            message >> user;
                            std::string chat_message;
                            message >> chat_message;

                            connection_accepted.chat.push_message(user, std::move(chat_message));
                        }
                        return;
                    }
                    case MessageType::ReadyToStart: {
                        state = ReadyToStart(std::move(connection_accepted.user_to_state));
                        return;
                    }
                }
            }

            if(connection_accepted.chat.get_not_sent_messages().size() > 0) {
                Message chat_update_message(MessageType::ChatUpdate);
                for (auto chat_message = connection_accepted.chat.get_not_sent_messages().rbegin(); chat_message != connection_accepted.chat.get_not_sent_messages().rend(); ++chat_message) {
                    chat_update_message << *chat_message;
                }
                chat_update_message << static_cast<uint8_t>(connection_accepted.chat.get_not_sent_messages().size());

                write_message_queue.enqueue(std::move(chat_update_message));

                connection_accepted.chat.all_sent();
            }
        }, [&](ReadyToStart& ready_to_start) {
        }}, state);
    }
};