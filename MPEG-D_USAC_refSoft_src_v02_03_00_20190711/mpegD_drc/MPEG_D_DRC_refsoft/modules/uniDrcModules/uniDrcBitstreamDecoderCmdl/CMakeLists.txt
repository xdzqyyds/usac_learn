cmake_minimum_required(VERSION 3.16.3)
#---------------------------------------------------------------------------------------------------
project(
    uniDrcBitstreamDecoderCmdl
    VERSION 1.0.0
    )

#---------------------------------------------------------------------------------------------------
add_executable(
    ${PROJECT_NAME}
        src/uniDrcBitstreamDecoder_main.c
    )

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        uniDrcBitstreamDecoder
        $<$<PLATFORM_ID:Linux,Darwin>:m>
    )