cmake_minimum_required(VERSION 3.24)

project(HaloServerManager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

add_executable(${PROJECT_NAME} WIN32
    src/main.cpp
    src/ui/mainwindow.cpp
    src/ui/consolewidget.cpp
    src/core/servermanager.cpp
    src/core/serverprocess.cpp
    src/config/settings.cpp
    src/config/configeditor.cpp
    src/util/serverinstaller.cpp
    src/util/zipunpacker.cpp
    resources/resources.qrc
)

target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0A00)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()

set(CPACK_GENERATOR "NSIS")
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_NSIS_DISPLAY_NAME ${PROJECT_NAME})
set(CPACK_NSIS_PACKAGE_NAME ${PROJECT_NAME})

include(CPack)