cmake_minimum_required(VERSION 3.16.3)
#---------------------------------------------------------------------------------------------------
project(
    drcToolDecoder
    VERSION 1.0.0
    )

#---------------------------------------------------------------------------------------------------
add_executable(
    ${PROJECT_NAME}
        src/drcToolDecoder.c
    )

target_link_options(
    ${PROJECT_NAME}
    PRIVATE
        $<$<C_COMPILER_ID:MSVC>:/STACK:10000000>
    )

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        uniDrcCommon_tables
        uniDrcCommon_hostparams
        peaklimiter
        qmf
        wavio
        uniDrcBitstreamDecoder
        uniDrcGainDecoder
        uniDrcInterfaceDecoder
        uniDrcSelectionProcess
    )

#---------------------------------------------------------------------------------------------------
add_custom_command(
    TARGET
        ${PROJECT_NAME}
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_LIST_DIR}/bin/${OS_NAME_PLATFORM}/$<TARGET_FILE_NAME:${PROJECT_NAME}>
    COMMAND
        ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_LIST_DIR}/outputData
    )

set_directory_properties(
    PROPERTIES
        ADDITIONAL_CLEAN_FILES
            "${CMAKE_CURRENT_LIST_DIR}/outputData;${CMAKE_CURRENT_LIST_DIR}/bin"
    )