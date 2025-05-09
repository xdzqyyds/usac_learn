cmake_minimum_required(VERSION 3.16.3)
#---------------------------------------------------------------------------------------------------
project(
    fft
    VERSION 1.0.0
    )

#---------------------------------------------------------------------------------------------------
add_library(
    ${PROJECT_NAME}
    STATIC
        include/fftlib.h
        src/fftlib.c
    )

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
    )

#---------------------------------------------------------------------------------------------------
add_executable(
    ${PROJECT_NAME}_example
        src/${PROJECT_NAME}lib_example.c
    )

target_link_libraries(
    ${PROJECT_NAME}_example
    PRIVATE
        ${PROJECT_NAME}
        wavio
        $<$<PLATFORM_ID:Linux,Darwin>:m>
    )