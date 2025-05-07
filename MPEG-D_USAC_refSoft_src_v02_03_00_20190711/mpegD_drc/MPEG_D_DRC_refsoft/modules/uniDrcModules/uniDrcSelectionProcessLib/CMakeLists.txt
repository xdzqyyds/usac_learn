cmake_minimum_required(VERSION 3.16.3)
#---------------------------------------------------------------------------------------------------
project(
    uniDrcSelectionProcess
    VERSION 1.0.0
    )

#---------------------------------------------------------------------------------------------------
add_library(
    ${PROJECT_NAME}
    STATIC
        include/uniDrcSelectionProcess_api.h
        src/uniDrcSelectionProcess_drcSetSelection.c
        src/uniDrcSelectionProcess_drcSetSelection.h
        src/uniDrcSelectionProcess_init.c
        src/uniDrcSelectionProcess_init.h
        src/uniDrcSelectionProcess_loudnessControl.c
        src/uniDrcSelectionProcess_loudnessControl.h
        src/uniDrcSelectionProcess.c
        src/uniDrcSelectionProcess.h
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
    )