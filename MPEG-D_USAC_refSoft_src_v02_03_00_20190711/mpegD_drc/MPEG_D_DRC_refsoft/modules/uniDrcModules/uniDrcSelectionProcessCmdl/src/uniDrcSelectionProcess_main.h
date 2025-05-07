/***********************************************************************************
 
 This software module was originally developed by
 
 Fraunhofer IIS
 
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
 
 Fraunhofer IIS retains full right to modify and use the code for its own purpose,
 assign or donate the code to a third party and to inhibit third parties from using
 the code for products that do not conform to MPEG-related ITU Recommendations and/or
 ISO/IEC International Standards.
 
 This copyright notice must be included in all copies or derivative works.
 
 Copyright (c) ISO/IEC 2015.
 
 ***********************************************************************************/

#ifndef _UNI_DRC_SELECTION_PROCESS_MAIN_H_
#define _UNI_DRC_SELECTION_PROCESS_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
#include "uniDrc.h"

#define DEBUG_DRC_SELECTION_CMDL 0
#if DEBUG_DRC_SELECTION_CMDL

#if !MPEG_H_SYNTAX
#define NUM_TESTS 38
#else
#define NUM_TESTS 10
#endif

static int testIdx = 0; /* 1 to NUM_TESTS; set to 0 for loop over all tests */
static int filenameAndRequestIdx[][2] = /* filename idx and request idx */
{
#if !MPEG_H_SYNTAX
    /* stream 1 */ /* testIdx 1 */
    {1, 1},
    
    /* stream 2 */ /* testIdx 2 */
    {2, 1},
    
    /* stream 3 */ /* testIdx 3 */
    {3, 1},
    
    /* stream 4 */ /* testIdx 4 */
    {4, 2},

    /* stream 5 */ /* testIdx 5-24 */
    {5, 3},
    {5, 4},
    {5, 5},
    {5, 6},
    {5, 7},
    {5, 8},
    {5, 9},
    {5, 10},
    {5, 11},
    {5, 12},
    {5, 13},
    {5, 14},
    {5, 15},
    {5, 16},
    {5, 17},
    {5, 18},
    {5, 19},
    {5, 20},
    {5, 21},
    {5, 22},
    
    /* stream 6 */ /* testIdx 25- */
    {6, 23},
    {6, 24},
    {6, 25},
    {6, 26},
    {6, 27},
    {6, 28},
    {6, 29},
    {6, 30},
    {6, 31},
    {6, 32},
    {6, 33},
    {6, 34},
    {6, 35},
    {6, 36}
    
#else
    
    /* stream 1 */ /* testIdx 1-6 */
    {1, 1},
    {1, 2},
    {1, 3},
    {1, 4},
    {1, 5},
    {1, 6},
    
    /* stream 2 */ /* testIdx 7 */
    {2, 7},
    {2, 8},
    {2, 9},
    {2, 10}
    
#endif
};

static int desiredOutputDrcSets [][3] =
{
#if !MPEG_H_SYNTAX
    /* stream 1 */
    {1, -1,  -1},
    
    /* stream 2 */
    {3, 1,  2},
    
    /* stream 3 */
    {2, 1,  -1},
    
    /* stream 4 */
    {5, 4,  -1},
    
    /* stream 5 */
    {11, 8,  -1},
    {11,  1,  -1},
    {11,  9,   2},
    {11,  9,   7},
    {11, 4,   -1},
    {11, 5,  -1},
    {11, 5,  -1},
    {11, 6,  -1},
    {11,  9,   7},
    {11, 8,   -1},
    {11,  9,   7},
    {11, -1,  -1},
    {11,  9,   2},
    {11, 4,   -1},
    {11, 9,   7},
    {11, 4,   -1},
    {11,  9,   2},
    {15, 12,   -1},
    {15, 13,   -1},
    {15, 3,   14},
    
    /* stream 6 */
    {6, 2, -1},
    {6, 1, 3},
    {6, 4, -1},
    {6, 1, 3},
    {6, 5, -1},
    {6, 1, -1},
    {6, 5, -1},
    {6, 4, -1},
    {6, -1, -1},
    {6, 2, -1},
    {7, 2, -1},
    {8, -1, -1},
    {6, -1, -1},
    {6, -1, -1}
    
#else /* MPEG-H */
    
    /* stream 1 */
    {1, -1, -1},
    {2, -1, -1},
    {3, -1, -1},
    {5, -1, -1},
    {4, -1, -1},
    {4, -1, -1},
    
    /* stream 2 */
    {-1, -1, -1},
    {-1, -1, -1},
    {1, -1, -1},
    {2, -1, -1}
    
#endif
};

static float desiredOutputLevelsGains [][3] =
{
#if !MPEG_H_SYNTAX
    /* stream 1 */
    {0.f, 0.f,   1000.f},

    /* stream 2 */
    {0.f, 0.f,   1000.f},
    
    /* stream 3 */
    {0.f, 0.f,   1000.f},
    
    /* stream 4 */
    {0.f, 0.f,   1000.f},
    
    /* stream 5 */
    {-23.f, -30.f,  -32.f},
    {-1.f, 0.f, -24.f},
    {-9.f, -9.f,-24.f},
    {0.f, 0.f,   1000.f},
    {-14.f, -15.f,   -24.f},
    {0.f, 0.f,   1000.f},
    {0.f, 0.f,   1000.f},
    {-4.f, -0.f,-36.f},
    {-1.f, -1.f,  -8.f},
    {1.f, -6.f,  -8.f},
    {-1.f, -1.f,  -8.f},
    {-4.f, -3.f,-36.f},
    {-21.f, -21.f,-36.f},
    {-2.f, -3.f,-12.f},
    {-5.f, -5.f,-12.f},
    {-2.f, -3.f,-12.f},
    {-5.f, -5.f,-20.f},
    {0.f, 0.f,-8.f},
    {-3.f, -3.f,-15.f},
    {-5.f, -8.f,-20.f},
    
    /* stream 6 */
    {4.f, 0.f, -23.f},
    {0.f, 0.f, -27.f},
    {10.f, 0.f, -17.f},
    {10.f, 0.f, -17.f},
    {10.f, 0.f, -17.f},
    {2.f, 0.f, -25.f},
    {2.f, -2.91f, -25.f},
    {11.f, 1.f, -16.f},
    {4.f, 4.f, -23.f},
    {0.f, 0.f,   1000.f},
    {0.f, 0.f,   1000.f},
    {0.f, 0.f,   1000.f},
    {-3.f, -3.f, -30.f},
    {1.f, 1.f, -26.f}
    
#else /* MPEG-H */
    
    /* stream 1 */
    {4.f, 0.f, -23.f},
    {3.5f, 0.f, -23.f},
    {1.25f, 0.f, -23.f},
    {-4.5f, 0.f, -23.f},
    {-3.5, 0.f, -20.f},
    {-6.5, 0.f, -23.f},
    
    /* stream 2 */
    {-7.f, -7.f, -31.f},
    {-7.f, -7.f, -31.f},
    {0.f, 0.f, -24.f},
    {8.f, 0.f, -16.f}
    
#endif
};

#endif
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

/***********************************************************************************/
