cmake_minimum_required(VERSION 3.16.3)
#---------------------------------------------------------------------------------------------------
project(
    xmlReader
    VERSION 1.0.0
    )

#---------------------------------------------------------------------------------------------------
add_library(
    ${PROJECT_NAME}
    STATIC
        include/xmlReaderLib.h
        src/xmlReaderLib.c
    )

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
    )