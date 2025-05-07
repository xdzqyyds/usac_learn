cmake_minimum_required(VERSION 3.16.3)
#---------------------------------------------------------------------------------------------------
project(
    uniDrcInterfaceDecoder
    VERSION 1.0.0
    )

#---------------------------------------------------------------------------------------------------
add_library(
    ${PROJECT_NAME}
    STATIC
        include/uniDrcInterfaceDecoder_api.h
        src/uniDrcInterfaceDecoder.c
        src/uniDrcInterfaceDecoder.h
        src/uniDrcInterfaceParser.c
        src/uniDrcInterfaceParser.h
    )

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
    )

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
        uniDrcCommon
        readonlybitbuf
    )