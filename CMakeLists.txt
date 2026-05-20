cmake_minimum_required(VERSION 3.20)
project(SolarSystemGazer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# ===== アイコン生成ツール =====
add_executable(icon_gen tools/icon_gen.cpp icon_draw.h)
target_link_libraries(icon_gen PRIVATE Qt6::Core Qt6::Gui)

set(ICON_ICO "${CMAKE_CURRENT_BINARY_DIR}/app_icon.ico")

if(WIN32)
    # Windows: Qt の DLL が PATH にないと icon_gen が実行できないため
    # qmake の場所から Qt バイナリディレクトリを自動検出する
    find_program(QT_QMAKE qmake6 qmake REQUIRED)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE}" DIRECTORY)

    add_custom_command(
        OUTPUT  ${ICON_ICO}
        COMMAND ${CMAKE_COMMAND} -E env "PATH=${QT_BIN_DIR};$ENV{PATH}"
                $<TARGET_FILE:icon_gen> ${ICON_ICO}
        DEPENDS icon_gen
        COMMENT "Generating app_icon.ico"
    )

    # windres で .rc → .obj にコンパイル
    find_program(WINDRES windres REQUIRED)
    set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
    set(RC_OBJ  "${CMAKE_CURRENT_BINARY_DIR}/app_icon_rc.obj")

    file(GENERATE OUTPUT ${RC_FILE}
        CONTENT "IDI_ICON1 ICON \"${ICON_ICO}\"\n")

    add_custom_command(
        OUTPUT  ${RC_OBJ}
        COMMAND ${WINDRES} -i ${RC_FILE} -o ${RC_OBJ}
        DEPENDS ${ICON_ICO} ${RC_FILE}
        COMMENT "Compiling Windows resource (app_icon.ico -> .obj)"
    )
    set(APP_EXTRA_SOURCES ${RC_OBJ})
else()
    # Linux / macOS: システムの Qt ライブラリで icon_gen を直接実行
    add_custom_command(
        OUTPUT  ${ICON_ICO}
        COMMAND $<TARGET_FILE:icon_gen> ${ICON_ICO}
        DEPENDS icon_gen
        COMMENT "Generating app_icon.ico"
    )
    set(APP_EXTRA_SOURCES "")
endif()

# ICO 生成を SolarSystemGazer ビルド前に完了させる
add_custom_target(generate_icon DEPENDS ${ICON_ICO})

# ===== メインアプリ =====
add_executable(SolarSystemGazer
    main.cpp
    mainwindow.h
    mainwindow.cpp
    solarwidget.h
    solarwidget.cpp
    orbitalcalc.h
    orbitalcalc.cpp
    planetdata.h
    planetdata.cpp
    eventdetector.h
    eventdetector.cpp
    icon_draw.h
    ${APP_EXTRA_SOURCES}
)
add_dependencies(SolarSystemGazer generate_icon)

target_link_libraries(SolarSystemGazer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

set_target_properties(SolarSystemGazer PROPERTIES
    WIN32_EXECUTABLE TRUE
)
