cmake_minimum_required(VERSION 3.16.3)
#---------------------------------------------------------------------------------------------------
project(
    uniDrcInterfaceEncoder
    VERSION 1.0.0
    )

#---------------------------------------------------------------------------------------------------
add_library(
    ${PROJECT_NAME}
    STATIC
        include/uniDrcInterfaceEncoder_api.h

        src/uniDrcInterfaceEncoder.c
        src/uniDrcInterfaceEncoder.h
        src/uniDrcInterfaceWriter.c
        src/uniDrcInterfaceWriter.h
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
        writeonlybitbuf
    PRIVATE
        $<$<PLATFORM_ID:Linux,Darwin>:m>
    )