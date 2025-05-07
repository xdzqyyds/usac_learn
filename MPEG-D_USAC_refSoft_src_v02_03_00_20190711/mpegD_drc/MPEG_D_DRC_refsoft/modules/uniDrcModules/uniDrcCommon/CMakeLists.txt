cmake_minimum_required(VERSION 3.16.3)
#---------------------------------------------------------------------------------------------------
project(
    uniDrcCommon
    VERSION 1.0.0
    )

#---------------------------------------------------------------------------------------------------
add_library(
    ${PROJECT_NAME}
    INTERFACE
    )

target_include_directories(
    ${PROJECT_NAME}
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
    )

add_library(
    ${PROJECT_NAME}_tables
    STATIC
        uniDrcTables.c
    )

add_library(
    ${PROJECT_NAME}_hostparams
    STATIC
        uniDrcHostParams.c
    )

target_include_directories(
    ${PROJECT_NAME}_hostparams
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../uniDrcSelectionProcessLib/include
    )