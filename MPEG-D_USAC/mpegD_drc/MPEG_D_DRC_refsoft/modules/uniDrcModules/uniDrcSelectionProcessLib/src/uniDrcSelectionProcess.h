/***********************************************************************************
 
 This software module was originally developed by
 
 Apple Inc. and Fraunhofer IIS
 
 in the course of development of the ISO/IEC 23003-4 for reference purposes and its
 performance may not have been optimized. This software module is an implementation
 of one or more tools as specified by the ISO/IEC 23003-4 standard. ISO/IEC gives
 you a royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
 and make derivative works of this software module or modifications  thereof for use
 in implementations or products claiming conformance to the ISO/IEC 23003-4 standard
 and which satisfy any specified conformance criteria. Those intending to use this
 software module in products are advised that its use may infringe existing patents.
 ISO/IEC have no liability for use of this software module or modifications thereof.
 Copyright is not released for products that do not conform to the ISO/IEC 23003-4
 standard.
 
 Apple Inc. and Fraunhofer IIS retains full right to modify and use the code for its
 own purpose, assign or donate the code to a third party and to inhibit third parties
 from using the code for products that do not conform to MPEG-related ITU Recommenda-
 tions and/or ISO/IEC International Standards.
 
 This copyright notice must be included in all copies or derivative works.
 
 Copyright (c) ISO/IEC 2014.
 
 ***********************************************************************************/

#ifndef _UNI_DRC_SELECTION_PROCESS_H_
#define _UNI_DRC_SELECTION_PROCESS_H_

#include "uniDrcCommon_api.h"
#include "uniDrcCommon.h"
#include "uniDrc.h"
#include "uniDrcInterface.h"

#include "uniDrcSelectionProcess_api.h"
#include "uniDrcSelectionProcess_init.h"
#include "uniDrcSelectionProcess_drcSetSelection.h"
#include "uniDrcSelectionProcess_loudnessControl.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    int drcInstructionsIndex;
} SubDrc;

#if MPEG_D_DRC_EXTENSION_V1
typedef struct {
    int eqInstructionsIndex;
} SubEq;
#endif

typedef struct T_UNI_DRC_SEL_PROC_STRUCT
{   
    
    /* general */
    int firstFrame;
    int uniDrcConfigHasChanged;
    int loudnessInfoSetHasChanged;
    int selectionProcessRequestHasChanged;
    int subbandDomainMode;
    
    /* selection process request parameters */
    UniDrcSelProcParams uniDrcSelProcParams;
    
    /* bitstream content */
    UniDrcConfig uniDrcConfig;
    LoudnessInfoSet loudnessInfoSet;
    
    /* user selected DRC */
    int drcInstructionsIndexSelected;
    int drcCoefficientsIndexSelected;
    int downmixInstructionsIndexSelected;
    
    /* multiple DRC instances */
    SubDrc subDrc[SUB_DRC_COUNT];                     /* DRC 0: Selected DRC */
                                                      /* DRC 1: Dependent DRC (DRC 0 depends on DRC 1) */
                                                      /* DRC 2: Fading DRC */
                                                      /* DRC 3: Ducking DRC */
#if MPEG_D_DRC_EXTENSION_V1
    /* permission based on complexity */
    int   drcSetIdIsPermitted[DRC_INSTRUCTIONS_COUNT_MAX];
    int   eqSetIdIsPermitted[EQ_INSTRUCTIONS_COUNT_MAX];
    
    /* user selected EQ */
    int eqInstructionsIndexSelected;
    SubEq subEq[SUB_EQ_COUNT];                        /* sub EQ 0: applied before downmix */
                                                      /* sub EQ 1: applied after downmix */
    int loudEqInstructionsIndexSelected;
    
    float complexityLevelSupportedTotal;
#endif
    
    /* output of selection process */
    UniDrcSelProcOutput uniDrcSelProcOutput;
    
} UNI_DRC_SEL_PROC_STRUCT;
    
/* map target config request to downmixId */
int mapTargetConfigRequestToDownmixId(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                                      HANDLE_UNI_DRC_CONFIG hUniDrcConfig);
    
/* select downmix matrix */
int selectDownmixMatrix(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                        HANDLE_UNI_DRC_CONFIG hUniDrcConfig);
    
#if DEBUG_DRC_SELECTION
/* display selected downmix matrix */
int displaySelectedDownmixMatrix(HANDLE_UNI_DRC_SEL_PROC_STRUCT hUniDrcSelProcStruct,
                                 HANDLE_UNI_DRC_CONFIG hUniDrcConfig);
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
