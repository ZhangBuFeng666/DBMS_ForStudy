cmake_minimum_required(VERSION 3.16)

project(Security LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core LinguistTools)

set(TS_FILES Security_zh_CN.ts)

add_library(Security SHARED
  Security_global.h
  security.cpp
  security.h
  ${TS_FILES}
)

target_link_libraries(Security PRIVATE Qt${QT_VERSION_MAJOR}::Core)

target_compile_definitions(Security PRIVATE SECURITY_LIBRARY)

if(COMMAND qt_create_translation)
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()
