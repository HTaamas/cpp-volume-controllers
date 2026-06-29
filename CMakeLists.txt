cmake_minimum_required(VERSION 3.16)

if(APPLE)
    project(SpotifyVol LANGUAGES CXX OBJCXX)
else()
    project(SpotifyVol LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
    set(CMAKE_OBJCXX_STANDARD 17)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
endif()

# Include environment parsing
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ParseEnv)

# Generate config header from .env file
set(ENV_FILE "${CMAKE_CURRENT_SOURCE_DIR}/.env")
set(CONFIG_HEADER "${CMAKE_CURRENT_BINARY_DIR}/include/app_config.h")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${ENV_FILE}")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include")
generate_config_header("${ENV_FILE}" "${CONFIG_HEADER}")

if(WIN32)
    set(CMAKE_PREFIX_PATH "C:\\Qt\\6.10.2\\mingw_64\\")
endif()
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Gui)
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
endif()
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(APP_ICON_ASSET "${CMAKE_CURRENT_SOURCE_DIR}/assets/app_icon.png")
set(APP_ICON_WINDOWS_ICO "${CMAKE_CURRENT_BINARY_DIR}/app_icon.ico")
set(APP_ICON_WINDOWS_RC "${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc")
set(WINDOWS_RUNTIME_DIR "${CMAKE_CURRENT_BINARY_DIR}/runtime")
set(WINDOWS_MIN_RUNTIME_DIR "${CMAKE_CURRENT_BINARY_DIR}/runtime-min")

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
    src/implementation/audio_ducker.h
    src/implementation/audio_ducker.cpp
    src/implementation/app_settings.h
    src/implementation/app_settings.cpp
    src/implementation/settings_dialog.h
    src/implementation/settings_dialog.cpp
)

if(WIN32)
    list(APPEND SOURCES src/volume_handler_win.cpp)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows/app_icon.rc.in"
        "${APP_ICON_WINDOWS_RC}"
        @ONLY
    )
    list(APPEND SOURCES "${APP_ICON_WINDOWS_RC}")
endif()

if(APPLE)
    list(APPEND SOURCES src/app_behavior_mac.mm)
    list(APPEND SOURCES src/volume_handler_mac.mm)
    list(APPEND SOURCES src/osd_window_mac.mm)
    list(APPEND SOURCES "${APP_ICON_ASSET}")
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

# Add generated config header include directory
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt6::Widgets 
    Qt6::Network 
    Qt6::Gui
)

if(UNIX AND NOT APPLE)
    target_include_directories(${PROJECT_NAME} PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES})
endif()

if(WIN32)
    set(QT_BIN_DIR "${CMAKE_PREFIX_PATH}/bin")
    set(QT_PLUGIN_DIR "${CMAKE_PREFIX_PATH}/plugins")
    find_program(POWERSHELL_EXECUTABLE NAMES pwsh powershell REQUIRED)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${QT_BIN_DIR}"
        REQUIRED
    )
    find_program(QTPATHS_EXECUTABLE
        NAMES qtpaths
        HINTS "${QT_BIN_DIR}"
        REQUIRED
    )
    add_custom_command(
        OUTPUT "${APP_ICON_WINDOWS_ICO}"
        COMMAND "${POWERSHELL_EXECUTABLE}"
            -NoProfile
            -ExecutionPolicy Bypass
            -File "${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows/GenerateIco.ps1"
            -InputPng "${APP_ICON_ASSET}"
            -OutputIco "${APP_ICON_WINDOWS_ICO}"
        DEPENDS
            "${APP_ICON_ASSET}"
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows/GenerateIco.ps1"
        VERBATIM
    )
    set_source_files_properties("${APP_ICON_WINDOWS_RC}" PROPERTIES OBJECT_DEPENDS "${APP_ICON_WINDOWS_ICO}")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${APP_ICON_ASSET}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/app_icon.png"
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        User32
        Gdi32
        Ole32
        Uuid
    )

    add_custom_target(runtime_bundle
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${WINDOWS_RUNTIME_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${WINDOWS_RUNTIME_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${WINDOWS_RUNTIME_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${APP_ICON_ASSET}"
            "${WINDOWS_RUNTIME_DIR}/app_icon.png"
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            --qtpaths "${QTPATHS_EXECUTABLE}"
            --release
            --compiler-runtime
            --no-translations
            --dir "${WINDOWS_RUNTIME_DIR}"
            "${WINDOWS_RUNTIME_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        DEPENDS ${PROJECT_NAME}
        VERBATIM
        COMMENT "Bundling a slim Windows runtime in ${WINDOWS_RUNTIME_DIR}"
    )

    add_custom_target(runtime_min_bundle
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${WINDOWS_MIN_RUNTIME_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${WINDOWS_MIN_RUNTIME_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${WINDOWS_MIN_RUNTIME_DIR}/platforms"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${WINDOWS_MIN_RUNTIME_DIR}/imageformats"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${WINDOWS_MIN_RUNTIME_DIR}/tls"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${WINDOWS_MIN_RUNTIME_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${APP_ICON_ASSET}"
            "${WINDOWS_MIN_RUNTIME_DIR}/app_icon.png"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_BIN_DIR}/Qt6Core.dll"
            "${QT_BIN_DIR}/Qt6Gui.dll"
            "${QT_BIN_DIR}/Qt6Network.dll"
            "${QT_BIN_DIR}/Qt6Widgets.dll"
            "${QT_BIN_DIR}/D3Dcompiler_47.dll"
            "${WINDOWS_MIN_RUNTIME_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_BIN_DIR}/libwinpthread-1.dll"
            "${WINDOWS_MIN_RUNTIME_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_PLUGIN_DIR}/platforms/qwindows.dll"
            "${WINDOWS_MIN_RUNTIME_DIR}/platforms/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_PLUGIN_DIR}/imageformats/qjpeg.dll"
            "${WINDOWS_MIN_RUNTIME_DIR}/imageformats/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_PLUGIN_DIR}/tls/qschannelbackend.dll"
            "${WINDOWS_MIN_RUNTIME_DIR}/tls/"
        DEPENDS ${PROJECT_NAME}
        VERBATIM
        COMMENT "Bundling the smallest practical Windows runtime in ${WINDOWS_MIN_RUNTIME_DIR}"
    )
endif()

if(APPLE)
    set_source_files_properties("${APP_ICON_ASSET}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    add_executable(set_bundle_icon_mac src/set_bundle_icon_mac.mm)
    target_link_libraries(set_bundle_icon_mac PRIVATE "-framework AppKit")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND $<TARGET_FILE:set_bundle_icon_mac>
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
            "${APP_ICON_ASSET}"
        VERBATIM
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "SpotifyVol"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.spotifyvol.app"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/Info.plist.in"
    )

    set(MACOS_CODESIGN_IDENTITY "$ENV{CODESIGN_IDENTITY}" CACHE STRING "macOS codesign identity used for post-build signing")
    if(MACOS_CODESIGN_IDENTITY STREQUAL "")
        execute_process(
            COMMAND security find-identity -v -p codesigning
            OUTPUT_VARIABLE MACOS_CODESIGN_IDENTITIES
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE MACOS_CODESIGN_STATUS
        )
        if(MACOS_CODESIGN_STATUS EQUAL 0)
            string(REGEX MATCH "Apple Development: [^\"]+" MACOS_CODESIGN_MATCH "${MACOS_CODESIGN_IDENTITIES}")
            if(MACOS_CODESIGN_MATCH)
                set(MACOS_CODESIGN_IDENTITY "${MACOS_CODESIGN_MATCH}" CACHE STRING "macOS codesign identity used for post-build signing" FORCE)
            endif()
        endif()
    endif()

    if(MACOS_CODESIGN_IDENTITY STREQUAL "")
        message(WARNING "No macOS codesign identity found. Build will continue unsigned and TCC prompts may reappear after rebuilds.")
    else()
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND /usr/bin/xattr -cr "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
            COMMAND /usr/bin/codesign
                --force
                --deep
                --sign "${MACOS_CODESIGN_IDENTITY}"
                "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
            VERBATIM
            COMMENT "Signing ${PROJECT_NAME}.app with ${MACOS_CODESIGN_IDENTITY}"
        )
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework AppKit"
        "-framework ApplicationServices"
        "-framework IOKit"
    )
endif()

if(UNIX AND NOT APPLE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${APP_ICON_ASSET}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/app_icon.png"
    )
endif()
