cmake_minimum_required(VERSION 3.16.3)
#---------------------------------------------------------------------------------------------------
project(
    dmxCmdl
    VERSION 1.0.0
    )

#---------------------------------------------------------------------------------------------------
add_executable(
    ${PROJECT_NAME}
        src/dmx_main.h
        src/dmx_main.c
    )

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        wavio
        $<$<PLATFORM_ID:Linux,Darwin>:m>
    )