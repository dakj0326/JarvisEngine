cmake_minimum_required(VERSION 3.25)

project(JarvisWorkspace LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------
# Fetch standalone Asio
# -------------------------------------------------
include(FetchContent)

FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-28-0
)

FetchContent_MakeAvailable(asio)

# -------------------------------------------------
# Create reusable INTERFACE target for Asio
# -------------------------------------------------
add_library(asio_interface INTERFACE)

target_include_directories(asio_interface INTERFACE
        ${asio_SOURCE_DIR}/asio/include
)

target_compile_definitions(asio_interface INTERFACE
        ASIO_STANDALONE
)

# Windows requires Winsock
if (WIN32)
    target_link_libraries(asio_interface INTERFACE ws2_32)
endif()

# -------------------------------------------------
# jarvisCore executable
# -------------------------------------------------
add_executable(jarvisCore
        jarvisCore/main.cpp
        jarvisCore/logger/logger.cpp
)

target_include_directories(jarvisCore PRIVATE
        jarvisCore
)

target_link_libraries(jarvisCore PRIVATE asio_interface)

# -------------------------------------------------
# ESPR executable (ESP listener / recorder)
# -------------------------------------------------
add_executable(ESPR
        JDM/ESPR/main.cpp
        JDM/ESPR/UDPHandler.cpp
)

target_include_directories(ESPR PRIVATE
        JDM/ESPR
)

target_link_libraries(ESPR PRIVATE asio_interface)