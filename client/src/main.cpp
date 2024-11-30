#include <boost/asio.hpp>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <imgui.h>

#include <chrono>
#include <iostream>
#include <optional>
#include <random>
#include <thread>
#include <tuple>
#include <unordered_map>

#include "game.hpp"
#include "game/input_state.hpp"
#include "game/state/just_created.hpp"
#include "message.hpp"
#include "tfqueue.hpp"

void game_loop(std::chrono::duration<double> rate,
               Game& game,
               std::unordered_map<SDL_Scancode, Key>& key_map,
               SDL_Window* window,
               ImGuiIO& io,
               SDL_Renderer* renderer);
std::tuple<std::thread, std::thread, Game> init_game(std::shared_ptr<boost::asio::ip::tcp::socket> socket, SDL_Renderer* renderer);

int select_device() {
    int numRenderDrivers = SDL_GetNumRenderDrivers();
    if(numRenderDrivers < 1) {
        std::cerr << "No render drivers available: " << SDL_GetError() << std::endl;
    }

    int device = -1;
    for(int i = 0; i < numRenderDrivers; i++) {
        SDL_RendererInfo info;
        if(SDL_GetRenderDriverInfo(i, &info) == 0) {
            std::cout << "Renderer " << i << ": " << info.name << std::endl;
        }
        if(std::string(info.name) == "direct3d12") {
            device = i;
        }
    }
    return device;
}

int main(int argc, char* argv[]) {
    std::unordered_map<SDL_Scancode, Key> key_map;
    key_map[SDL_Scancode::SDL_SCANCODE_UNKNOWN] = Key::NONE;
    key_map[SDL_Scancode::SDL_SCANCODE_A]       = Key::A;
    key_map[SDL_Scancode::SDL_SCANCODE_B]       = Key::B;
    key_map[SDL_Scancode::SDL_SCANCODE_C]       = Key::C;
    key_map[SDL_Scancode::SDL_SCANCODE_D]       = Key::D;
    key_map[SDL_Scancode::SDL_SCANCODE_E]       = Key::E;
    key_map[SDL_Scancode::SDL_SCANCODE_F]       = Key::F;
    key_map[SDL_Scancode::SDL_SCANCODE_G]       = Key::G;
    key_map[SDL_Scancode::SDL_SCANCODE_H]       = Key::H;
    key_map[SDL_Scancode::SDL_SCANCODE_I]       = Key::I;
    key_map[SDL_Scancode::SDL_SCANCODE_J]       = Key::J;
    key_map[SDL_Scancode::SDL_SCANCODE_K]       = Key::K;
    key_map[SDL_Scancode::SDL_SCANCODE_L]       = Key::L;
    key_map[SDL_Scancode::SDL_SCANCODE_M]       = Key::M;
    key_map[SDL_Scancode::SDL_SCANCODE_N]       = Key::N;
    key_map[SDL_Scancode::SDL_SCANCODE_O]       = Key::O;
    key_map[SDL_Scancode::SDL_SCANCODE_P]       = Key::P;
    key_map[SDL_Scancode::SDL_SCANCODE_Q]       = Key::Q;
    key_map[SDL_Scancode::SDL_SCANCODE_R]       = Key::R;
    key_map[SDL_Scancode::SDL_SCANCODE_S]       = Key::S;
    key_map[SDL_Scancode::SDL_SCANCODE_T]       = Key::T;
    key_map[SDL_Scancode::SDL_SCANCODE_U]       = Key::U;
    key_map[SDL_Scancode::SDL_SCANCODE_V]       = Key::V;
    key_map[SDL_Scancode::SDL_SCANCODE_W]       = Key::W;
    key_map[SDL_Scancode::SDL_SCANCODE_X]       = Key::X;
    key_map[SDL_Scancode::SDL_SCANCODE_Y]       = Key::Y;
    key_map[SDL_Scancode::SDL_SCANCODE_Z]       = Key::Z;

    key_map[SDL_Scancode::SDL_SCANCODE_0] = Key::N0;
    key_map[SDL_Scancode::SDL_SCANCODE_1] = Key::N1;
    key_map[SDL_Scancode::SDL_SCANCODE_2] = Key::N2;
    key_map[SDL_Scancode::SDL_SCANCODE_3] = Key::N3;
    key_map[SDL_Scancode::SDL_SCANCODE_4] = Key::N4;
    key_map[SDL_Scancode::SDL_SCANCODE_5] = Key::N5;
    key_map[SDL_Scancode::SDL_SCANCODE_6] = Key::N6;
    key_map[SDL_Scancode::SDL_SCANCODE_7] = Key::N7;
    key_map[SDL_Scancode::SDL_SCANCODE_8] = Key::N8;
    key_map[SDL_Scancode::SDL_SCANCODE_9] = Key::N9;

    key_map[SDL_Scancode::SDL_SCANCODE_RETURN] = Key::ENTER;

    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window =
        SDL_CreateWindow("SDL2 Keyboard Input", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);

    if(window == nullptr) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, select_device(), SDL_RENDERER_ACCELERATED);

    SDL_RendererInfo renderer_info;
    SDL_GetRendererInfo(renderer, &renderer_info);
    std::cout << "Renderer: " << renderer_info.name << std::endl;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    auto rate = std::chrono::duration<double>(1.0 / 30);
    // Define the server IP address and port
    const std::string serverIP = "127.0.0.1";  // Change to your target IP
    const int serverPort       = 10101;        // Change to your target port

    try {
        boost::asio::io_context io_context;

        boost::asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(serverIP, std::to_string(serverPort));

        auto socket = boost::asio::ip::tcp::socket(io_context);

        boost::asio::connect(socket, endpoints);

        auto socket_ptr = std::make_shared<boost::asio::ip::tcp::socket>(std::move(socket));

        auto init_result = init_game(socket_ptr, renderer);

        game_loop(rate, std::get<2>(init_result), key_map, window, io, renderer);

        socket_ptr->close();

        std::get<0>(init_result).join();
        std::get<1>(init_result).join();
    } catch(const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

std::tuple<std::thread, std::thread, Game> init_game(std::shared_ptr<boost::asio::ip::tcp::socket> socket, SDL_Renderer* renderer) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 100);

    auto write_message_queue = std::make_shared<TFQueue<Message>>();
    auto read_message_queue  = std::make_shared<TFQueue<Message>>();
    auto lost_connection     = std::make_shared<std::atomic_flag>();

    std::thread writer([=]() {
        while(true) {
            try {
                auto could_be_message = write_message_queue->dequeue();
                if(!could_be_message.has_value()) {
                    std::cout << "no message found in queue, finishing" << std::endl;
                    break;
                }
                could_be_message.value().write_to(*socket);
            } catch(const std::exception& e) {
                std::cerr << "Error in writer thread: " << e.what() << std::endl;
                lost_connection->test_and_set();
                break;
            }
        }
    });
    std::thread reader([=]() {
        while(true) {
            try {
                auto message = read_from(*socket);
                read_message_queue->enqueue(std::move(message));
            } catch(const std::exception& e) {
                std::cerr << "Error in reader thread: " << e.what() << std::endl;
                lost_connection->test_and_set();
                write_message_queue->finish();
                break;
            }
        }
    });

    Game game(std::make_unique<JustCreatedGame>(distrib(gen)), write_message_queue, read_message_queue, lost_connection);

    return {std::move(writer), std::move(reader), std::move(game)};
}

void game_loop(std::chrono::duration<double> rate,
               Game& game,
               std::unordered_map<SDL_Scancode, Key>& key_map,
               SDL_Window* window,
               ImGuiIO& io,
               SDL_Renderer* renderer) {
    auto start    = std::chrono::system_clock::now();
    auto start_io = start;

    InputState inputState;
    bool running = true;

    while(running) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if(event.type == SDL_KEYDOWN) {
                auto it = key_map.find(event.key.keysym.scancode);
                if(it != key_map.end()) {
                    inputState.update_state(it->second, true);
                }
            }
            if(event.type == SDL_KEYUP) {
                auto it = key_map.find(event.key.keysym.scancode);
                if(it != key_map.end()) {
                    inputState.update_state(it->second, false);
                }
            }
            if(event.type == SDL_QUIT) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
        SDL_RenderClear(renderer);

        auto new_start = std::chrono::system_clock::now();

        auto elapsed    = new_start - start;
        auto elapsed_io = new_start - start_io;

        game.elapsed(elapsed, inputState, renderer);

        SDL_RenderPresent(renderer);

        if(elapsed_io > rate) {
            if(game.is_lost_connection()) {
                break;
            }
            game.io_updates();
            start_io = new_start;
        }
        start = new_start;
    }
}
