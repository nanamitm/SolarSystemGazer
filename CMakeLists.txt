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

# icon_gen を実行して build/ に app_icon.ico を生成
set(ICON_ICO  "${CMAKE_CURRENT_BINARY_DIR}/app_icon.ico")
set(WINDRES   "C:/Qt/Tools/mingw1310_64/bin/windres.exe")

add_custom_command(
    OUTPUT  ${ICON_ICO}
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=C:/Qt/6.9.2/mingw_64/bin;C:/Qt/Tools/mingw1310_64/bin;$ENV{PATH}"
            $<TARGET_FILE:icon_gen> ${ICON_ICO}
    DEPENDS icon_gen
    COMMENT "Generating app_icon.ico"
)

# windres で .rc → .o に直接コンパイル
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
    ${RC_OBJ}        # windres でコンパイル済みリソースオブジェクト
)

target_link_libraries(SolarSystemGazer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

set_target_properties(SolarSystemGazer PROPERTIES
    WIN32_EXECUTABLE TRUE
)
