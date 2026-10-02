cmake_minimum_required(VERSION 3.16)

project(GISApplication VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets WebEngineWidgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets Network)
find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets Network WebChannel)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

file(GLOB_RECURSE FUZZYLITE_SRC
    external/src/*.cpp
)

# Автоматическое копирование файлов ресурсов в директорию сборки при каждом билде
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/map.html DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/data.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

list(FILTER FUZZYLITE_SRC EXCLUDE REGEX "Console.cpp$")
list(FILTER FUZZYLITE_SRC EXCLUDE REGEX "Benchmark.cpp$")
list(FILTER FUZZYLITE_SRC EXCLUDE REGEX "main.cpp$")

add_library(fuzzylite STATIC
    ${FUZZYLITE_SRC}
)

target_include_directories(fuzzylite PUBLIC
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/fuzzylite
    ${CMAKE_SOURCE_DIR}/external/fl
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(GISApplication
        MANUAL_FINALIZATION
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        fuzzy/FuzzyEngine.cpp
        fuzzy/FuzzyEngine.h
    )
target_link_libraries(GISApplication PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::WebEngineWidgets
    fuzzylite
)
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET GISApplication APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(GISApplication SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(GISApplication
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(GISApplication PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_include_directories(GISApplication PRIVATE
    ${CMAKE_SOURCE_DIR}/external
)


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.GISApplication)
endif()
set_target_properties(GISApplication PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS GISApplication
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(GISApplication)
endif()

