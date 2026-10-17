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
    src/webgui.c
    src/bots.c
    
    src/protocol/login.c
    src/protocol/chat.c
    src/protocol/cargo_ship.c
    src/protocol/resource.c
    
    src/protocol/heartbeat.c
    src/protocol/alliance.c
    
    src/protocol/hospital.c
    
    src/protocol/social.c
    src/protocol/mail.c
    
    src/protocol/building.c
    
    src/protocol/barrack.c
    
    src/bot/tick.c
    src/bot/command.c
    src/bot/transfer.c
    src/bot/bank.c
    # src/bot/http_server.c
)

target_compile_definitions(client PRIVATE
    VERSION="${PROJECT_VERSION}"
)

option(DEBUG_LOG "Enable debug logs" OFF)

if(DEBUG_LOG)
    target_compile_definitions(client PRIVATE _DEBUG_)
endif()

# Embed webgui/index.html into the binary (served for "/")
file(READ ${CMAKE_SOURCE_DIR}/webgui/index.html WEBGUI_HEX HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," WEBGUI_BYTES "${WEBGUI_HEX}")
file(WRITE ${CMAKE_BINARY_DIR}/webgui_page.h
     "static const unsigned char webgui_page[] = {${WEBGUI_BYTES}};\n")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS webgui/index.html)

target_include_directories(client PRIVATE include ${CMAKE_BINARY_DIR})

find_package(Threads REQUIRED)
target_link_libraries(client PRIVATE m Threads::Threads)

