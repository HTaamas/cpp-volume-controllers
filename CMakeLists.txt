cmake_minimum_required(VERSION 3.16)
project(SpotifyVol CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Gui)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(SOURCES
    src/main.cpp
    src/spotify_client.cpp
    src/spotify_client.h
    src/osd_window.cpp
    src/osd_window.h
    src/tray_manager.cpp
    src/tray_manager.h
    src/volume_handler.h
    src/volume_handler.cpp
    src/volume_handler_factory.h
)

if(APPLE)
    list(APPEND SOURCES 
        src/volume_handler_mac.mm
        src/volume_handler_mac.h
    )
elseif(WIN32)
    list(APPEND SOURCES
        src/volume_handler_win.cpp
        src/volume_handler_win.h
    )
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt6::Widgets 
    Qt6::Network 
    Qt6::Gui
)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework CoreGraphics"
        "-framework CoreFoundation"
        "-framework AppKit"
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.spotifyvol.app"
    )
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        User32
        Gdi32
    )
endif()
