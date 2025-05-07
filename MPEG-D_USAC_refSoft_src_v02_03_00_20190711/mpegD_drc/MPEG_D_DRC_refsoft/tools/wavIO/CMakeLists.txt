cmake_minimum_required(VERSION 3.16.3)
#---------------------------------------------------------------------------------------------------
project(
    wavio
    VERSION 1.0.0
    )

#---------------------------------------------------------------------------------------------------
add_library(
    ${PROJECT_NAME}
    STATIC
        include/wavIO.h
        src/wavIO.c
    )

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
    )

#---------------------------------------------------------------------------------------------------
add_executable(
    ${PROJECT_NAME}_example
        src/wavIO_example.c
    )

target_link_libraries(
    ${PROJECT_NAME}_example
    PRIVATE
        ${PROJECT_NAME}
    )