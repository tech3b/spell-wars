#pragma once

#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <imgui.h>

#include <optional>
#include <random>
#include <vector>

#include "../state.hpp"

class TFEGame {
    static const int SIZE = 4;
    std::array<std::array<int, SIZE>, SIZE> board {};
    std::mt19937 rng;

public:
    TFEGame(std::mt19937&& rng)
        : rng(std::move(rng)) {
    }

    std::array<std::array<int, SIZE>, SIZE>& getBoard() {
        return board;
    }

    int getSize() {
        return SIZE;
    }

    void spawnNew() {
        std::vector<std::tuple<int, int>> zeroCells;

        for(int i = 0; i < SIZE; i++) {
            for(int j = 0; j < SIZE; j++) {
                if(board[i][j] == 0) {
                    zeroCells.push_back({i, j});
                }
            }
        }

        if(zeroCells.size() > 0) {
            std::uniform_int_distribution<size_t> dist(0, zeroCells.size() - 1);
            std::tuple<int, int> choice = zeroCells[dist(rng)];

            std::uniform_real_distribution<float> prob(0.0f, 1.0f);
            int value                                       = prob(rng) < 0.9f ? 2 : 4;
            board[std::get<0>(choice)][std::get<1>(choice)] = value;
        }
    }

    bool moveLeft() {
        std::array<std::array<int, SIZE>, SIZE> boardUpdated {};
        bool moveMade = false;
        for(int i = 0; i < SIZE; i++) {
            for(int j = 1; j < SIZE; j++) {
                if(board[i][j] == 0) {
                    continue;
                }
                int k = j - 1;
                while(board[i][k] == 0 && k > 0 && (board[i][k - 1] == 0 || board[i][k - 1] == board[i][j])) {
                    k--;
                }
                if(board[i][k] == 0) {
                    board[i][k] = board[i][j];
                    board[i][j] = 0;
                    moveMade    = true;
                } else if(board[i][k] == board[i][j] && !boardUpdated[i][k]) {
                    board[i][k]        = board[i][k] * 2;
                    board[i][j]        = 0;
                    boardUpdated[i][k] = 1;
                    moveMade           = true;
                }
            }
        }
        return moveMade;
    }

    bool moveRight() {
        std::array<std::array<int, SIZE>, SIZE> boardUpdated {};
        bool moveMade = false;
        for(int i = 0; i < SIZE; i++) {
            for(int j = SIZE - 2; j >= 0; j--) {
                if(board[i][j] == 0) {
                    continue;
                }
                int k = j + 1;
                while(board[i][k] == 0 && k < SIZE - 1 && (board[i][k + 1] == 0 || board[i][k + 1] == board[i][j])) {
                    k++;
                }
                if(board[i][k] == 0) {
                    board[i][k] = board[i][j];
                    board[i][j] = 0;
                    moveMade    = true;
                } else if(board[i][k] == board[i][j] && !boardUpdated[i][k]) {
                    board[i][k]        = board[i][k] * 2;
                    board[i][j]        = 0;
                    boardUpdated[i][k] = 1;
                    moveMade           = true;
                }
            }
        }
        return moveMade;
    }

    bool moveUp() {
        std::array<std::array<int, SIZE>, SIZE> boardUpdated {};
        bool moveMade = false;
        for(int j = 0; j < SIZE; j++) {
            for(int i = 1; i < SIZE; i++) {
                if(board[i][j] == 0) {
                    continue;
                }
                int k = i - 1;
                while(board[k][j] == 0 && k > 0 && (board[k - 1][j] == 0 || board[k - 1][j] == board[i][j])) {
                    k--;
                }
                if(board[k][j] == 0) {
                    board[k][j] = board[i][j];
                    board[i][j] = 0;
                    moveMade    = true;
                } else if(board[k][j] == board[i][j] && !boardUpdated[k][j]) {
                    board[k][j]        = board[k][j] * 2;
                    board[i][j]        = 0;
                    boardUpdated[k][j] = 1;
                    moveMade           = true;
                }
            }
        }
        return moveMade;
    }

    bool moveDown() {
        std::array<std::array<int, SIZE>, SIZE> boardUpdated {};
        bool moveMade = false;
        for(int j = 0; j < SIZE; j++) {
            for(int i = SIZE - 2; i >= 0; i--) {
                if(board[i][j] == 0) {
                    continue;
                }
                int k = i + 1;
                while(board[k][j] == 0 && k < SIZE - 1 && (board[k + 1][j] == 0 || board[k + 1][j] == board[i][j])) {
                    k++;
                }
                if(board[k][j] == 0) {
                    board[k][j] = board[i][j];
                    board[i][j] = 0;
                    moveMade    = true;
                } else if(board[k][j] == board[i][j] && !boardUpdated[k][j]) {
                    board[k][j]        = board[k][j] * 2;
                    board[i][j]        = 0;
                    boardUpdated[k][j] = 1;
                    moveMade           = true;
                }
            }
        }
        return moveMade;
    }
};

enum Direction { Up, Down, Left, Right };

class TwoFortyEight : public GameState {
private:
    TFEGame tfeGame;
    bool leftPressed;
    bool rightPressed;
    bool upPressed;
    bool downPressed;
    std::unordered_map<int, ImU32> tileColors;

public:
    TwoFortyEight(TFEGame&& tfeGame)
        : tfeGame(std::move(tfeGame)) {
        this->tfeGame.spawnNew();

        tileColors = {
            {0, IM_COL32(200, 200, 200, 255)},     // gray
            {2, IM_COL32(238, 228, 218, 255)},     // beige
            {4, IM_COL32(237, 224, 200, 255)},     // light beige
            {8, IM_COL32(255, 182, 193, 255)},     // pink
            {16, IM_COL32(135, 206, 250, 255)},    // light sky blue
            {32, IM_COL32(144, 238, 144, 255)},    // light green
            {64, IM_COL32(255, 165, 0, 255)},      // orange
            {128, IM_COL32(186, 85, 211, 255)},    // medium orchid (purple)
            {256, IM_COL32(255, 215, 0, 255)},     // gold
            {512, IM_COL32(70, 130, 180, 255)},    // steel blue
            {1024, IM_COL32(60, 179, 113, 255)},   // medium sea green
            {2048, IM_COL32(220, 20, 60, 255)},    // crimson
            {4096, IM_COL32(123, 104, 238, 255)},  // medium slate blue
            {8192, IM_COL32(255, 99, 71, 255)},    // tomato red
            {16384, IM_COL32(0, 191, 255, 255)},   // deep sky blue
            {32768, IM_COL32(199, 21, 133, 255)},  // medium violet red
        };
    }

    virtual std::optional<std::unique_ptr<GameState>> elapsed(std::chrono::system_clock::duration& elapsed,
                                                              InputState& input_state,
                                                              SDL_Renderer* renderer) {
        if(input_state.state_by_key(Key::LEFT)) {
            if(!leftPressed) {
                if(tfeGame.moveLeft()) {
                    tfeGame.spawnNew();
                }
                leftPressed = true;
            }
        } else {
            leftPressed = false;
        }
        if(input_state.state_by_key(Key::RIGHT)) {
            if(!rightPressed) {
                if(tfeGame.moveRight()) {
                    tfeGame.spawnNew();
                }
                rightPressed = true;
            }
        } else {
            rightPressed = false;
        }
        if(input_state.state_by_key(Key::UP)) {
            if(!upPressed) {
                if(tfeGame.moveUp()) {
                    tfeGame.spawnNew();
                }
                upPressed = true;
            }
        } else {
            upPressed = false;
        }
        if(input_state.state_by_key(Key::DOWN)) {
            if(!downPressed) {
                if(tfeGame.moveDown()) {
                    tfeGame.spawnNew();
                }
                downPressed = true;
            }
        } else {
            downPressed = false;
        }
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

        float tileSize = 80.0f;
        float spacing  = 5.0f;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 p             = ImGui::GetCursorScreenPos();  // where to start drawing

        for(int y = 0; y < tfeGame.getSize(); ++y) {
            for(int x = 0; x < tfeGame.getSize(); ++x) {
                // Position of this tile in screen space
                ImVec2 tileMin(p.x + x * (tileSize + spacing), p.y + y * (tileSize + spacing));
                ImVec2 tileMax(tileMin.x + tileSize, tileMin.y + tileSize);

                int value = tfeGame.getBoard()[y][x];  // your game board

                ImU32 col = tileColors[value];

                // Draw tile rectangle
                drawList->AddRectFilled(tileMin, tileMax, col, 6.0f);

                // Draw number (centered roughly)
                if(value > 0) {
                    char buf[16];
                    snprintf(buf, sizeof(buf), "%d", value);

                    // Compute text position (rough centering)
                    ImFont* defaultFont = io.Fonts->Fonts[0];
                    ImVec2 textSize     = defaultFont->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, buf);
                    ImVec2 textPos(tileMin.x + (tileSize - textSize.x) * 0.5f, tileMin.y + (tileSize - textSize.y) * 0.5f);

                    drawList->AddText(nullptr, 24, textPos, IM_COL32(0, 0, 0, 255), buf);
                }
            }
        }

        // Reserve layout space so ImGui doesn’t overlap
        ImGui::Dummy(ImVec2(tfeGame.getSize() * (tileSize + spacing), tfeGame.getSize() * (tileSize + spacing)));

        ImGui::End();

        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        return {};
    }

    virtual void io_updates(TFQueue<Message>& read_message_queue, TFQueue<Message>& write_message_queue) {
    }
};
