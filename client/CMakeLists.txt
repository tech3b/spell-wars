cmake_minimum_required(VERSION 3.10)
project(client)

set(CMAKE_CXX_STANDARD 23)

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

add_executable(client src/main.cpp)

target_link_libraries(client ${SDL2_LINK_LIB})

add_custom_command(TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${SDL2_LIBRARY_PATH}"
    $<TARGET_FILE_DIR:client>)