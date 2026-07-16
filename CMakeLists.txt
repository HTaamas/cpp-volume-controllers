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

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Gui WebSockets)
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_PROTOBUF REQUIRED protobuf)
find_package(ZLIB REQUIRED)
set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/proto")
set(PROTO_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/proto_gen")
file(MAKE_DIRECTORY ${PROTO_BIN_DIR})
file(GLOB_RECURSE PROTO_FILES "${PROTO_SRC_DIR}/*.proto")

set(PROTO_SRCS)
set(PROTO_HDRS)

foreach(PROTO_FILE IN LISTS PROTO_FILES)
    file(RELATIVE_PATH REL_PROTO_FILE ${PROTO_SRC_DIR} ${PROTO_FILE})
    get_filename_component(REL_DIR ${REL_PROTO_FILE} DIRECTORY)
    get_filename_component(BASENAME ${REL_PROTO_FILE} NAME_WE)
    
    set(GENERATED_CC "${PROTO_BIN_DIR}/${REL_DIR}/${BASENAME}.pb.cc")
    set(GENERATED_H "${PROTO_BIN_DIR}/${REL_DIR}/${BASENAME}.pb.h")
    
    list(APPEND PROTO_SRCS ${GENERATED_CC})
    list(APPEND PROTO_HDRS ${GENERATED_H})
    
    add_custom_command(
        OUTPUT ${GENERATED_CC} ${GENERATED_H}
        COMMAND protoc
        ARGS --cpp_out=${PROTO_BIN_DIR} -I${PROTO_SRC_DIR} ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Compiling ${REL_PROTO_FILE}..."
        VERBATIM
    )
endforeach()
set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS} PROPERTIES GENERATED TRUE)

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
    ${PROTO_SRCS}
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

    # Build a real .icns from the PNG so the bundle shows an icon in Finder, the
    # Dock and the app switcher. (A 1024px square master gives the crispest icon.)
    set(APP_ICNS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/app_icon.original.png")
    if(NOT EXISTS "${APP_ICNS_SOURCE}")
        set(APP_ICNS_SOURCE "${APP_ICON_ASSET}")
    endif()
    set(APP_ICNS "${CMAKE_CURRENT_BINARY_DIR}/app_icon.icns")
    set(APP_ICONSET_DIR "${CMAKE_CURRENT_BINARY_DIR}/app_icon.iconset")
    add_custom_command(
        OUTPUT "${APP_ICNS}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${APP_ICONSET_DIR}"
        COMMAND sips -z 16 16     "${APP_ICNS_SOURCE}" --out "${APP_ICONSET_DIR}/icon_16x16.png"
        COMMAND sips -z 32 32     "${APP_ICNS_SOURCE}" --out "${APP_ICONSET_DIR}/icon_16x16@2x.png"
        COMMAND sips -z 32 32     "${APP_ICNS_SOURCE}" --out "${APP_ICONSET_DIR}/icon_32x32.png"
        COMMAND sips -z 64 64     "${APP_ICNS_SOURCE}" --out "${APP_ICONSET_DIR}/icon_32x32@2x.png"
        COMMAND sips -z 128 128   "${APP_ICNS_SOURCE}" --out "${APP_ICONSET_DIR}/icon_128x128.png"
        COMMAND sips -z 256 256   "${APP_ICNS_SOURCE}" --out "${APP_ICONSET_DIR}/icon_128x128@2x.png"
        COMMAND sips -z 256 256   "${APP_ICNS_SOURCE}" --out "${APP_ICONSET_DIR}/icon_256x256.png"
        COMMAND sips -z 512 512   "${APP_ICNS_SOURCE}" --out "${APP_ICONSET_DIR}/icon_256x256@2x.png"
        COMMAND sips -z 512 512   "${APP_ICNS_SOURCE}" --out "${APP_ICONSET_DIR}/icon_512x512.png"
        COMMAND sips -z 1024 1024 "${APP_ICNS_SOURCE}" --out "${APP_ICONSET_DIR}/icon_512x512@2x.png"
        COMMAND iconutil -c icns "${APP_ICONSET_DIR}" -o "${APP_ICNS}"
        DEPENDS "${APP_ICNS_SOURCE}"
        COMMENT "Generating macOS app icon (.icns)"
        VERBATIM
    )
    set_source_files_properties("${APP_ICNS}" PROPERTIES GENERATED TRUE MACOSX_PACKAGE_LOCATION "Resources")
    list(APPEND SOURCES "${APP_ICNS}")
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

# Add generated config header include directory
target_include_directories(${PROJECT_NAME} PRIVATE
    "${PROTO_BIN_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    ${PC_PROTOBUF_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt6::Widgets
    Qt6::Network
    Qt6::Gui
    Qt6::WebSockets
    ZLIB::ZLIB
)
target_link_options(${PROJECT_NAME} PRIVATE ${PC_PROTOBUF_LDFLAGS})

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
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "SpotifyVol"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.spotifyvol.app"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_ICON_FILE "app_icon.icns"
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
