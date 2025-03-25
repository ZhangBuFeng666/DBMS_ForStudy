cmake_minimum_required(VERSION 3.16)

project(Records LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core LinguistTools)

set(TS_FILES Records_zh_CN.ts)

add_library(Records SHARED
  Records_global.h
  records.cpp
  records.h
  ${TS_FILES}
)

target_link_libraries(Records PRIVATE Qt${QT_VERSION_MAJOR}::Core)

target_compile_definitions(Records PRIVATE RECORDS_LIBRARY)

if(COMMAND qt_create_translation)
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()
