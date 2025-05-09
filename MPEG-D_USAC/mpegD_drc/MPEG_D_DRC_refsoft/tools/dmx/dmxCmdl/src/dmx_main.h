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
 
 Copyright (c) ISO/IEC 2014.
 
 ***********************************************************************************/

/*
 *  File dmx_main.h
 *  of dmx cmdl app.
 *  
 */   

#ifdef __cplusplus
extern "C"
{
#endif

/***********************************************************************************/

int readDmxMtxFromFile( char* filename, float **dmxMtx, int numInputChans, int numOutputChans );

/***********************************************************************************/

static float dmxMtx_2_0_to_1_0_example[1][2] = {
    { 0.7071f, 0.7071f },
};

/***********************************************************************************/

static float dmxMtx_5_1_to_2_0_example[6][2] = {
    { 1.0000f, 0.0000f },
    { 0.0000f, 1.0000f },
    { 0.7071f, 0.7071f },
    { 0.0000f, 0.0000f },
    { 0.7071f, 0.0000f },
    { 0.0000f, 0.7071f },
};
                         
/***********************************************************************************/

#ifdef __cplusplus
}
#endif

