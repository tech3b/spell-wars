#pragma once

#include <vector>
#include <string>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

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

    void push_message(int32_t user, std::string&& chat_message) {
        messages.push_back({user, std::move(chat_message)});
    }

    std::vector<std::string>& get_not_sent_messages() {
        return not_sent_messages;
    }

    void all_sent() {
        not_sent_messages.clear();
    }

    void render_chat(bool enter_state) {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.5f));
        ImGui::Begin("Chat", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);

        ImGui::BeginChild("ChatScroll", ImVec2(0, -30), true);
        for (const auto& msg : messages) {
            ImGui::TextWrapped("[%s]: %s", std::to_string(std::get<0>(msg)).c_str(), std::get<1>(msg).c_str());
        }
        ImGui::EndChild();

        if (is_active) {
            ImGui::Separator();
            if (ImGui::InputText("##ChatInput", &current_input, ImGuiInputTextFlags_EnterReturnsTrue)) {
                if(!current_input.empty()) {
                    not_sent_messages.push_back(std::move(current_input));
                }
            }
            ImGui::SetKeyboardFocusHere(-1);
        }

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
            if(previous_enter_state != enter_state) {
                if(enter_state) {
                    is_active = !is_active;
                }
                previous_enter_state = enter_state;
            }
        } else {
            is_active = false;
        }

        ImGui::End();
        ImGui::PopStyleColor();
    }
};