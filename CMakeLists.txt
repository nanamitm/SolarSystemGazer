cmake_minimum_required(VERSION 3.22)
project(SolarSystemGazer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ===== プラットフォーム判定 =====
if(ANDROID)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Qml)
    qt_standard_project_setup(REQUIRES 6.5)
else()
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
endif()

# ===== アイコン生成ツール（デスクトップのみ） =====
if(NOT ANDROID)
    add_executable(icon_gen tools/icon_gen.cpp icon_draw.h)
    target_link_libraries(icon_gen PRIVATE Qt6::Core Qt6::Gui)

    # Android ランチャーアイコン (mipmap PNG) 生成ツール（手動実行）
    add_executable(android_icon_gen tools/android_icon_gen.cpp icon_draw.h)
    target_link_libraries(android_icon_gen PRIVATE Qt6::Core Qt6::Gui)

    set(ICON_ICO "${CMAKE_CURRENT_BINARY_DIR}/app_icon.ico")

    if(WIN32)
        find_program(QT_QMAKE qmake6 qmake REQUIRED)
        get_filename_component(QT_BIN_DIR "${QT_QMAKE}" DIRECTORY)

        add_custom_command(
            OUTPUT  ${ICON_ICO}
            COMMAND ${CMAKE_COMMAND} -E env "PATH=${QT_BIN_DIR};$ENV{PATH}"
                    $<TARGET_FILE:icon_gen> ${ICON_ICO}
            DEPENDS icon_gen
            COMMENT "Generating app_icon.ico"
        )

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
        add_custom_command(
            OUTPUT  ${ICON_ICO}
            COMMAND $<TARGET_FILE:icon_gen> ${ICON_ICO}
            DEPENDS icon_gen
            COMMENT "Generating app_icon.ico"
        )
        set(APP_EXTRA_SOURCES "")
    endif()

    add_custom_target(generate_icon DEPENDS ${ICON_ICO})
endif()

# ===== メインアプリ =====
if(ANDROID)
    # ----- Android (Qt Quick / QML) -----
    # バックエンド C++ のみ。QML_ELEMENT を持つファイルは qt_add_qml_module の SOURCES に記述する
    qt_add_executable(SolarSystemGazer
        main_android.cpp
        orbitalcalc.h
        orbitalcalc.cpp
        planetdata.h
        planetdata.cpp
        eventdetector.h
        eventdetector.cpp
    )

    qt_add_qml_module(SolarSystemGazer
        URI SolarSystemGazer
        VERSION 1.0
        QML_FILES
            qml/main.qml
            qml/EventListSheet.qml
        SOURCES
            solaritem.h   solaritem.cpp
            eventbridge.h eventbridge.cpp
    )

    target_link_libraries(SolarSystemGazer PRIVATE
        Qt6::Core Qt6::Gui Qt6::Quick Qt6::QuickControls2
    )

    # Android SDK バージョン（ローカルは新しめの既定値。CI の古い Qt 用に -D で上書き可能）
    set(SSG_ANDROID_COMPILE_SDK "36"     CACHE STRING "Android compileSdk version")
    set(SSG_ANDROID_BUILD_TOOLS "36.0.0" CACHE STRING "Android build-tools revision")

    set_target_properties(SolarSystemGazer PROPERTIES
        QT_ANDROID_PACKAGE_NAME              "net.nanamitm.solarsystemgazer"
        QT_ANDROID_APP_NAME                  "Solar System Gazer"
        QT_ANDROID_VERSION_NAME              "${PROJECT_VERSION}"
        QT_ANDROID_VERSION_CODE              1
        QT_ANDROID_PACKAGE_SOURCE_DIR        "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_MIN_SDK_VERSION           28
        QT_ANDROID_TARGET_SDK_VERSION        34
        QT_ANDROID_COMPILE_SDK_VERSION       ${SSG_ANDROID_COMPILE_SDK}
        QT_ANDROID_SDK_BUILD_TOOLS_REVISION  ${SSG_ANDROID_BUILD_TOOLS}
    )

    # Release ビルドの APK 署名。
    # build_android_release.ps1 が QT_ANDROID_KEYSTORE_* 環境変数を設定して有効化する。
    # 環境変数が無ければ未署名 Release としてビルドされる。
    if(CMAKE_BUILD_TYPE STREQUAL "Release" AND DEFINED ENV{QT_ANDROID_KEYSTORE_PATH})
        set_target_properties(SolarSystemGazer PROPERTIES
            QT_ANDROID_SIGN_APK              TRUE
            QT_ANDROID_KEYSTORE_PATH         $ENV{QT_ANDROID_KEYSTORE_PATH}
            QT_ANDROID_KEYSTORE_ALIAS        $ENV{QT_ANDROID_KEYSTORE_ALIAS}
            QT_ANDROID_KEYSTORE_STORE_PASS   $ENV{QT_ANDROID_KEYSTORE_STORE_PASS}
            QT_ANDROID_KEYSTORE_KEY_PASS     $ENV{QT_ANDROID_KEYSTORE_KEY_PASS}
        )
    endif()

else()
    # ----- Desktop (Qt Widgets) -----
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
        Qt6::Core Qt6::Gui Qt6::Widgets
    )

    set_target_properties(SolarSystemGazer PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
