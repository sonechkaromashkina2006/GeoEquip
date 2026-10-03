cmake_minimum_required(VERSION 3.16)

project(GeoEquipClient VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск необходимых компонентов Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets Network WebChannel)

# Исходные файлы проекта
set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
    src/model/GeoObject.cpp
    src/model/GeoObject.h
)

# Автоматическое копирование ресурсов в директорию сборки
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/map.html DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/data.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Сборка встроенной библиотеки fuzzylite
file(GLOB_RECURSE FUZZYLITE_SRC
    external/src/*.cpp
)
list(FILTER FUZZYLITE_SRC EXCLUDE REGEX "Console.cpp$")
list(FILTER FUZZYLITE_SRC EXCLUDE REGEX "Benchmark.cpp$")
list(FILTER FUZZYLITE_SRC EXCLUDE REGEX "main.cpp$")

if(FUZZYLITE_SRC)
    add_library(fuzzylite STATIC ${FUZZYLITE_SRC})
    target_include_directories(fuzzylite PUBLIC
        ${CMAKE_SOURCE_DIR}/external
        ${CMAKE_SOURCE_DIR}/external/fuzzylite
        ${CMAKE_SOURCE_DIR}/external/fl
    )
endif()

# Создание исполняемого файла приложения
qt_add_executable(GeoEquipClient
    ${PROJECT_SOURCES}
)

# Подключение библиотек Qt и внешних модулей
target_link_libraries(GeoEquipClient PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::WebEngineWidgets
    Qt6::WebChannel
)

if(TARGET fuzzylite)
    target_link_libraries(GeoEquipClient PRIVATE fuzzylite)
endif()

# Пути к заголовочным файлам (включая external и src)
target_include_directories(GeoEquipClient PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/nlohmann
)

# Настройки свойств таргета
set_target_properties(GeoEquipClient PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS GeoEquipClient
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(GeoEquipClient)
