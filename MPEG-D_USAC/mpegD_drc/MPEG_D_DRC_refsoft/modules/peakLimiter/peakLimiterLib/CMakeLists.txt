cmake_minimum_required(VERSION 3.16.3)
#---------------------------------------------------------------------------------------------------
project(
    peaklimiter
    VERSION 1.0.0
    )

#---------------------------------------------------------------------------------------------------
add_library(
    ${PROJECT_NAME}
    STATIC
        include/peakLimiterlib.h
        include/peakLimiterlib_errorcodes.h

        src/peakLimiter.c
    )

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
    )