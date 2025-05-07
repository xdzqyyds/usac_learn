cmake_minimum_required(VERSION 3.16.3)
#---------------------------------------------------------------------------------------------------
project(
    mpeg_d_drc_reference_software
    VERSION 1.0.0
    )

#---------------------------------------------------------------------------------------------------
include(CMakePrintHelpers)
cmake_host_system_information(RESULT OS_NAME QUERY OS_NAME)
cmake_host_system_information(RESULT OS_PLATFORM QUERY OS_PLATFORM)
set(OS_NAME_PLATFORM ${OS_NAME}_${OS_PLATFORM})
cmake_print_variables(OS_NAME_PLATFORM)

#---------------------------------------------------------------------------------------------------
option(MPEGH "MPEG-D DRC for MPEG-H 3D Audio" Off)
option(AMD1 "Enable Amendment 1 features" On)
option(LOUDNESS_LEVELING "Enable loudness leveling" On)
option(WRITE_FRAMESIZE "" On)
option(NODE_RESERVOIR "Enable node reservoir" Off)
option(CONFORMANCE "Create targets related to conformance testing" On)

if(AMD1 AND MPEGH)
    message(FATAL_ERROR "Combination of MPEGH and AMD1 not supported!")
elseif(MPEGH AND WRITE_FRAMESIZE)
    message(FATAL_ERROR "Combination of MPEGH and WRITE_FRAMESIZE not supported!")
endif()


#---------------------------------------------------------------------------------------------------
cmake_print_variables(MPEGH)
cmake_print_variables(AMD1)
cmake_print_variables(LOUDNESS_LEVELING)
cmake_print_variables(WRITE_FRAMESIZE)
cmake_print_variables(NODE_RESERVOIR)
cmake_print_variables(CONFORMANCE)

#---------------------------------------------------------------------------------------------------
add_subdirectory(tools/wavIO)
add_subdirectory(tools/dmx/dmxCmdl)
add_subdirectory(tools/fftlib)
add_subdirectory(tools/qmflib)
add_subdirectory(tools/writeonlybitbuf)
add_subdirectory(tools/readonlybitbuf)
add_subdirectory(tools/xmlReaderLib)

#---------------------------------------------------------------------------------------------------
# Add compile defintions for all DRC targets below
add_compile_definitions(
    $<$<BOOL:${AMD1}>:AMD1>
    $<$<BOOL:${MPEGH}>:MPEG_H>
    $<$<BOOL:${WRITE_FRAMESIZE}>:DEBUG_OUTPUT_FORMAT>
    $<$<BOOL:${NODE_RESERVOIR}>:NODE_RESERVOIR>
    $<$<BOOL:${LOUDNESS_LEVELING}>:LEVELING_SUPPORT>
    )

add_subdirectory(modules/loudnessNormalizer/loudnessNormalizerCmdl)
add_subdirectory(modules/peakLimiter/peakLimiterLib)
add_subdirectory(modules/peakLimiter/peakLimiterCmdl)

add_subdirectory(modules/uniDrcModules/uniDrcCommon)
add_subdirectory(modules/uniDrcModules/uniDrcBitstreamDecoderLib)
add_subdirectory(modules/uniDrcModules/uniDrcBitstreamDecoderCmdl)
add_subdirectory(modules/uniDrcModules/uniDrcGainDecoderLib)
add_subdirectory(modules/uniDrcModules/uniDrcGainDecoderCmdl)
add_subdirectory(modules/uniDrcModules/uniDrcInterfaceDecoderLib)
add_subdirectory(modules/uniDrcModules/uniDrcInterfaceDecoderCmdl)
add_subdirectory(modules/uniDrcModules/uniDrcInterfaceEncoderLib)
add_subdirectory(modules/uniDrcModules/uniDrcInterfaceEncoderCmdl)
add_subdirectory(modules/uniDrcModules/uniDrcSelectionProcessLib)
add_subdirectory(modules/uniDrcModules/uniDrcSelectionProcessCmdl)

add_subdirectory(modules/drcTool/drcToolDecoder)
add_subdirectory(modules/drcTool/drcToolEncoder)

#---------------------------------------------------------------------------------------------------
# Test targets
#--------------------------------------------------------------------------------------------------
add_custom_target(
    encode
    COMMAND
        $<IF:$<PLATFORM_ID:Windows>,${CMAKE_SOURCE_DIR}/scripts/encode$<$<BOOL:${AMD1}>:_AMD1>.bat,${CMAKE_SOURCE_DIR}/scripts/encode$<$<BOOL:${AMD1}>:_AMD1>.sh> ${OS_NAME_PLATFORM} $<$<BOOL:${MPEGH}>:MPEGH>
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/scripts
    DEPENDS
        drcToolEncoder
    )

add_custom_target(
    decode
    COMMAND
        $<IF:$<PLATFORM_ID:Windows>,${CMAKE_SOURCE_DIR}/scripts/decode$<$<BOOL:${AMD1}>:_AMD1>.bat,${CMAKE_SOURCE_DIR}/scripts/decode$<$<BOOL:${AMD1}>:_AMD1>.sh> ${OS_NAME_PLATFORM} $<$<BOOL:${MPEGH}>:MPEGH>
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/scripts
    DEPENDS
        drcToolDecoder
    )

add_custom_target(
    encode_uniDrcInterface
    COMMAND
        $<IF:$<PLATFORM_ID:Windows>,${CMAKE_SOURCE_DIR}/scripts/encode_uniDrcInterface$<$<BOOL:${AMD1}>:_AMD1>.bat,${CMAKE_SOURCE_DIR}/scripts/encode_uniDrcInterface$<$<BOOL:${AMD1}>:_AMD1>.sh> ${OS_NAME_PLATFORM} $<$<BOOL:${MPEGH}>:MPEGH>
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/scripts
    DEPENDS
        uniDrcInterfaceEncoderCmdl
    )

add_custom_target(
    test_selectionProcess
    COMMAND
        $<IF:$<PLATFORM_ID:Windows>,${CMAKE_SOURCE_DIR}/scripts/test_selectionProcess$<$<BOOL:${AMD1}>:_AMD1>.bat,${CMAKE_SOURCE_DIR}/scripts/test_selectionProcess$<$<BOOL:${AMD1}>:_AMD1>.sh> ${OS_NAME_PLATFORM} $<$<BOOL:${MPEGH}>:MPEGH>
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/scripts
    DEPENDS
        uniDrcSelectionProcessCmdl
    )

add_custom_target(
    decode_cmdlToolChain
    COMMAND
        $<IF:$<PLATFORM_ID:Windows>,${CMAKE_SOURCE_DIR}/scripts/decode_cmdlToolChain.bat,${CMAKE_SOURCE_DIR}/scripts/decode_cmdlToolChain.sh> ${OS_NAME_PLATFORM} $<$<BOOL:${MPEGH}>:MPEGH>
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/scripts
    DEPENDS
        uniDrcSelectionProcessCmdl
        uniDrcGainDecoderCmdl
    )

add_custom_target(
    start_all
    COMMAND
        $<IF:$<PLATFORM_ID:Windows>,${CMAKE_SOURCE_DIR}/scripts/start_all.bat,${CMAKE_SOURCE_DIR}/scripts/start_all.sh> ${OS_NAME_PLATFORM}
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/scripts
    )

if(CONFORMANCE)
    add_custom_target(
        check_conformance
        COMMAND
            ./checkConformance_$<IF:$<BOOL:${AMD1}>,v1,$<IF:$<BOOL:${MPEGH}>,h0,v0>>$<IF:$<PLATFORM_ID:Windows>,.bat,.sh> ${OS_NAME_PLATFORM}
        WORKING_DIRECTORY
            ${CMAKE_SOURCE_DIR}/conformanceData/conformanceData/scripts
        DEPENDS
            drcToolEncoder
            drcToolDecoder
            uniDrcGainDecoderCmdl
            peakLimiterCmdl
            uniDrcSelectionProcessCmdl
        )

    add_custom_target(
        start_all_conformance
        COMMAND
            #v0
            ${CMAKE_COMMAND} -DAMD1=0 -DMPEGH=0 -DWRITE_FRAMESIZE=0 -DLOUDNESS_LEVELING=0 ${CMAKE_SOURCE_DIR}
        COMMAND
            ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target check_conformance --clean-first

            # v1
        COMMAND
            ${CMAKE_COMMAND} -DAMD1=1 -DMPEGH=0 -DLOUDNESS_LEVELING=1 ${CMAKE_SOURCE_DIR}
        COMMAND
            ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target check_conformance --clean-first

            # h0
        COMMAND
            ${CMAKE_COMMAND} -DAMD1=0 -DMPEGH=1 -DLOUDNESS_LEVELING=1 -DWRITE_FRAMESIZE=0 ${CMAKE_SOURCE_DIR}
        COMMAND
            ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target check_conformance --clean-first
        )
endif()