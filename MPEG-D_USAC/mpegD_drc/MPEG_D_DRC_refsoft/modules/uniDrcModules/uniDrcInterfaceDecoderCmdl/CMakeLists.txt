cmake_minimum_required(VERSION 3.16.3)
#---------------------------------------------------------------------------------------------------
project(
    uniDrcInterfaceDecoderCmdl
    VERSION 1.0.0
    )

#---------------------------------------------------------------------------------------------------
add_executable(
    ${PROJECT_NAME}
        src/uniDrcInterfaceDecoder_main.c
    )

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        uniDrcInterfaceDecoder
    )