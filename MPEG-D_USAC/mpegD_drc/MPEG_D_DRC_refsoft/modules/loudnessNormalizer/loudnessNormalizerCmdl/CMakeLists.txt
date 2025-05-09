cmake_minimum_required(VERSION 3.16.3)
#---------------------------------------------------------------------------------------------------
project(
    loudnessNormalizerCmdl
    VERSION 1.0.0
    )

#---------------------------------------------------------------------------------------------------
add_executable(
    ${PROJECT_NAME}
        src/loudnessNormalizer_main.c
    )

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        wavio
        $<$<PLATFORM_ID:Linux,Darwin>:m>
    )