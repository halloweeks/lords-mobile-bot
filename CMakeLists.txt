cmake_minimum_required(VERSION 3.10)

project(LordsMobileBot VERSION 1.1.1 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(client
    src/main.c
    src/connection.c
    src/log.c
    src/des.c
    src/map_point.c
    
    src/config.c
    src/dispatcher.c
    src/utility.c
    
    # src/command.c
    
    src/protocol/login.c
    src/protocol/chat.c
    src/protocol/cargo_ship.c
    src/protocol/resource.c
    
    src/protocol/heartbeat.c
    src/protocol/alliance.c
    
    src/protocol/hospital.c
    
    src/protocol/social.c
    
    src/bot/tick.c
    
    # src/bot/http_server.c
)

target_compile_definitions(client PRIVATE
    VERSION="${PROJECT_VERSION}"
)

option(DEBUG_LOG "Enable debug logs" OFF)

if(DEBUG_LOG)
    target_compile_definitions(client PRIVATE _DEBUG_)
endif()

target_include_directories(client PRIVATE include)