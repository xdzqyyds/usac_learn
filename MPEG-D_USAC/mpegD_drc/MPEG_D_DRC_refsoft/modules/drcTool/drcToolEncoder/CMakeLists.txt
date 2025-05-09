cmake_minimum_required(VERSION 3.16.3)
#---------------------------------------------------------------------------------------------------
project(
    drcToolEncoder
    VERSION 1.0.0
    )

#---------------------------------------------------------------------------------------------------
add_executable(
    ${PROJECT_NAME}
        cmdlTool/drcConfig.c
        cmdlTool/drcConfig.h
        cmdlTool/main.c
        cmdlTool/userConfig.h
        cmdlTool/userConfig1_Amd1.c
        cmdlTool/userConfig1_ld.c
        cmdlTool/userConfig1.c
        cmdlTool/userConfig10_Amd1.c
        cmdlTool/userConfig11_Amd1.c
        cmdlTool/userConfig12_Amd1.c
        cmdlTool/userConfig13_Amd1.c
        cmdlTool/userConfig14_Amd1.c
        cmdlTool/userConfig2_Amd1.c
        cmdlTool/userConfig2.c
        cmdlTool/userConfig3_Amd1.c
        cmdlTool/userConfig3.c
        cmdlTool/userConfig4_Amd1.c
        cmdlTool/userConfig4.c
        cmdlTool/userConfig5_Amd1.c
        cmdlTool/userConfig5.c
        cmdlTool/userConfig6_Amd1.c
        cmdlTool/userConfig6.c
        cmdlTool/userConfig7_Amd1.c
        cmdlTool/userConfig8_Amd1.c
        cmdlTool/userConfig9_Amd1.c
        cmdlTool/userConfigMpegh1.c
        cmdlTool/userConfigMpegh2.c
        cmdlTool/userConfigMpegh3.c
        cmdlTool/userConfigMpegh4.c
        cmdlTool/userConfigMpeghDebug.c
        cmdlTool/xmlParserConfig.c
        cmdlTool/xmlParserConfig.h

        include/common.h
        include/drcToolEncoder.h
        include/uniDrc.h

        src/drcToolEncoder.c
        src/gainEnc.c
        src/gainEnc.h
        src/mux.c
        src/mux.h
        src/tables.c
        src/tables.h

        ${CMAKE_SOURCE_DIR}/modules/uniDrcModules/uniDrcGainDecoderLib/src/uniDrcFilterBank.c
        ${CMAKE_SOURCE_DIR}/modules/uniDrcModules/uniDrcGainDecoderLib/src/uniDrcEq.c
    )

# set stack size to 10 MB to prevent stack overflow for AMD1 testpoints
target_link_options(
    ${PROJECT_NAME}
    PRIVATE
        $<$<C_COMPILER_ID:MSVC>:/STACK:10000000>
    )

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_SOURCE_DIR}/modules/uniDrcModules/uniDrcGainDecoderLib/include
        ${CMAKE_SOURCE_DIR}/modules/uniDrcModules/uniDrcGainDecoderLib/src
    )

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        uniDrcCommon
        writeonlybitbuf
        xmlReader
        $<$<PLATFORM_ID:Linux,Darwin>:m>
    )

target_compile_definitions(
    ${PROJECT_NAME}
    PRIVATE
        COMPILE_FOR_DRC_ENCODER
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