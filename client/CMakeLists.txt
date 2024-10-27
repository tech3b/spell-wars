cmake_minimum_required(VERSION 3.10)
project(client)

set(BOOST_INCLUDE_LIBRARIES asio)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.86 COMPONENTS asio)  # Specify required components

if(NOT Boost_FOUND)
    message(WARNING "Boost not found. Attempting to fetch Boost...")

    # Fetch Boost if not found
    include(FetchContent)
    FetchContent_Declare(
        Boost
        URL https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.7z
        DOWNLOAD_EXTRACT_TIMESTAMP ON
        OVERRIDE_FIND_PACKAGE
      )
    FetchContent_MakeAvailable(boost)
endif()

find_package(Boost 1.86 REQUIRED COMPONENTS asio)

add_executable(client src/main.cpp)

target_link_libraries(client Boost::asio)
