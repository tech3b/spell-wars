#pragma once

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <string>
#include <vector>

#include "../message.hpp"

class Chat {
    bool is_active;
    bool previous_enter_state;
    std::vector<std::tuple<int32_t, std::string>> messages;
    std::string current_input;
    std::vector<std::string> not_sent_messages;

public:
    Chat()
        : is_active(false),
          messages(),
          current_input(),
          previous_enter_state(false),
          not_sent_messages() {
    }

    void push_message(Message& message) {
        uint8_t message_number;
        message >> message_number;
        for(int i = 0; i < message_number; i++) {
            int32_t user;
            message >> user;
            std::string chat_message;
            message >> chat_message;

            messages.push_back({user, std::move(chat_message)});
        }
    }

    void commit(TFQueue<Message>& write_message_queue) {
        if(not_sent_messages.size() > 0) {
            Message chat_update_message(MessageType::ChatUpdate);
            for(auto chat_message = not_sent_messages.rbegin(); chat_message != not_sent_messages.rend(); ++chat_message) {
                chat_update_message << *chat_message;
            }
            chat_update_message << static_cast<uint8_t>(not_sent_messages.size());

            write_message_queue.enqueue(std::move(chat_update_message));

            not_sent_messages.clear();
        }
    }

    void render_chat(bool enter_state) {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.5f));
        ImGui::Begin("Chat", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

        ImGui::BeginChild("ChatScroll", ImVec2(0, -30), true);
        for(const auto& msg : messages) {
            ImGui::TextWrapped("[%s]: %s", std::to_string(std::get<0>(msg)).c_str(), std::get<1>(msg).c_str());
        }
        ImGui::EndChild();

        if(!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
            is_active = false;
        }

        if(is_active) {
            ImGui::Separator();
            if(ImGui::InputText("##ChatInput", &current_input, ImGuiInputTextFlags_EnterReturnsTrue)) {
                if(!current_input.empty()) {
                    not_sent_messages.push_back(std::move(current_input));
                }
            }
            ImGui::SetKeyboardFocusHere(-1);
        }

        if(previous_enter_state != enter_state) {
            if(enter_state) {
                is_active = !is_active;
                if(is_active) {
                    ImGui::SetKeyboardFocusHere(-1);
                }
            }
            previous_enter_state = enter_state;
        }

        ImGui::End();
        ImGui::PopStyleColor();
    }
};
