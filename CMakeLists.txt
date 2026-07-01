cmake_minimum_required(VERSION 3.22)
project(tuna-installer-kde VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

set(SOURCES
    src/main.cpp
    src/wizard.cpp
    src/recipe.cpp
    src/pages/welcome.cpp
    src/pages/diskselection.cpp
    src/pages/confirm.cpp
    src/pages/progress.cpp
    src/pages/done.cpp
)

set(HEADERS
    src/wizard.h
    src/recipe.h
    src/pages/welcome.h
    src/pages/diskselection.h
    src/pages/confirm.h
    src/pages/progress.h
    src/pages/done.h
)

add_executable(tuna-installer-kde ${SOURCES} ${HEADERS})

target_link_libraries(tuna-installer-kde PRIVATE
    Qt6::Core
    Qt6::Widgets
)

target_include_directories(tuna-installer-kde PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Install
install(TARGETS tuna-installer-kde RUNTIME DESTINATION bin)
install(FILES fisherman-recipe.schema.json DESTINATION share/tuna-installer-kde)
