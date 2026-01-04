cmake_minimum_required(VERSION 3.20)

project(FinancialSignalProcessing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(fsp_core
        src/capture_file.cpp
        src/time.cpp
        src/udp.cpp
)

target_include_directories(fsp_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(fsp_core PUBLIC Threads::Threads)

add_executable(fsp_capture apps/capture/main.cpp)
target_link_libraries(fsp_capture PRIVATE fsp_core)

add_executable(fsp_replay apps/replay/main.cpp)
target_link_libraries(fsp_replay PRIVATE fsp_core)
