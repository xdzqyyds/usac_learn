cmake_minimum_required(VERSION 3.16.3)
#---------------------------------------------------------------------------------------------------
project(
    uniDrcGainDecoder
    VERSION 1.0.0
    )

#---------------------------------------------------------------------------------------------------
add_library(
    ${PROJECT_NAME}
    STATIC
        include/uniDrcGainDecoder_api.h
        src/parametricDrcDecoder.c
        src/parametricDrcDecoder.h
        src/uniDrcDec.c
        src/uniDrcDec.h
        src/uniDrcEq.c
        src/uniDrcEq.h
        src/uniDrcFilterBank.c
        src/uniDrcFilterBank.h
        src/uniDrcGainDec.c
        src/uniDrcGainDec.h
        src/uniDrcGainDecoder.c
        src/uniDrcGainDecoder.h
        src/uniDrcLoudEq.c
        src/uniDrcLoudEq.h
        src/uniDrcMultiBand.c
        src/uniDrcMultiBand.h
        src/uniDrcProcessAudio.c
        src/uniDrcProcessAudio.h
        src/uniDrcShapeFilter.c
        src/uniDrcShapeFilter.h
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
    )