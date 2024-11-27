cmake_minimum_required(VERSION 3.10)
project(client)

set(CMAKE_CXX_STANDARD 23)

set(IMGUI_SRC
    "${CMAKE_SOURCE_DIR}/external/imgui/imgui.cpp"
    "${CMAKE_SOURCE_DIR}/external/imgui/imgui_draw.cpp"
    "${CMAKE_SOURCE_DIR}/external/imgui/imgui_widgets.cpp"
    "${CMAKE_SOURCE_DIR}/external/imgui/imgui_tables.cpp"
    "${CMAKE_SOURCE_DIR}/external/imgui/imgui_demo.cpp"
    "${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_sdl2.cpp"
    "${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_sdlrenderer2.cpp"
)

add_library(imgui STATIC ${IMGUI_SRC})

# Define SDL2 paths relative to the project
set(SDL2_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/SDL2/include")

if (WIN32)
    set(SDL2_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/external/SDL2/lib/win32/SDL2.dll")
    set(SDL2_LINK_LIB "${CMAKE_SOURCE_DIR}/external/SDL2/lib/win32/SDL2.lib")
elseif (APPLE)
    set(SDL2_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/external/SDL2/lib/macos/libSDL2.dylib")
    set(SDL2_LINK_LIB "${CMAKE_SOURCE_DIR}/external/SDL2/lib/macos/libSDL2.dylib")
elseif (UNIX)
    set(SDL2_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/external/SDL2/lib/linux/libSDL2.so")
    set(SDL2_LINK_LIB "${CMAKE_SOURCE_DIR}/external/SDL2/lib/linux/libSDL2.so")
endif()

set(BOOST_ASIO_ROOT "${CMAKE_SOURCE_DIR}/external/boost_asio")

include_directories(${SDL2_INCLUDE_DIR})
include_directories(${BOOST_ASIO_ROOT})
target_include_directories(imgui PUBLIC
    "${CMAKE_SOURCE_DIR}/external/imgui"
)

add_executable(client src/main.cpp)

if(WIN32)
    target_compile_definitions(client PRIVATE _WIN32_WINNT=0x0601)
endif()

target_link_libraries(client ${SDL2_LINK_LIB} imgui)

add_custom_command(TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${SDL2_LIBRARY_PATH}"
    $<TARGET_FILE_DIR:client>)