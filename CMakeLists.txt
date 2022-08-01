project(OneDriveTray LANGUAGES CXX)
cmake_minimum_required(VERSION 3.14)

set(CMAKE_AUTOMOC On)
set(CMAKE_AUTOUIC On)
set(CMAKE_AUTORCC On)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

add_executable(
        onedrive-tray
        src/main.cpp
        src/MessagesWindow.cpp
        resources/systray.qrc
        src/Application.cpp src/Application.h
)

target_link_libraries(
        onedrive-tray
        Qt5::Core
        Qt5::Widgets
)

target_compile_features(onedrive-tray PRIVATE cxx_std_17)

add_custom_target(
        translations
        ALL DEPENDS
        translations/onedrive_tray_en.qm
        translations/onedrive_tray_es.qm
        translations/onedrive_tray_fr.qm
        translations/onedrive_tray_nl.qm
)

add_dependencies(onedrive-tray translations)

install(
        TARGETS onedrive-tray
        DESTINATION /usr/local/bin/
)

install(
        FILES onedrive-tray.desktop
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        DESTINATION /usr/share/applications/onedrive-tray.desktop
)

install(
        FILES images/OneDrive.svg
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
        DESTINATION /usr/share/icons/hicolor/scalable/apps/OneDrive.svg
)
