/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed in the course of 
development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 NBC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/
/*******************************************************************************************
 *
 * MS stereo coding module
 *
 * Authors:
 * CL    Chuck Lueck, TI <lueck@ti.com>
 * RSY   Rongshan Yu, I2R
 *
 * Changes:
 * 22-jan-98   CL   Initial revision.  Lacks psychoacoustics for MS stereo.
 * 06-sep-98   CL   Disable MS stereo for bands which use PNS (MPEG4 only).
 * Oct-2003	   RSY  Modified for Scalable Lossless coding 
 ***************************************************************************************/

#ifndef LIT_MS_ENC
#define LIT_MS_ENC

#ifdef I2R_LOSSLESS
#include "all.h"
#include "tf_main.h"
#include "aac_qc.h"

void lit_MSEncode(int *spectrum[MAX_TIME_CHANNELS],   /* array of pointers to spectral data */
                  Ch_Info *channelInfo,                  /* Pointer to Ch_Info */
		  MSInfo      *msInfo,
                  int sfb_offset_table[][MAX_SCFAC_BANDS+1],
                  WINDOW_TYPE block_type[MAX_TIME_CHANNELS], /* Block type */
                  AACQuantInfo* quantInfo,
                  int numberOfChannels,                /* Number of channels */
                  short msenable);                 

void lit_MSReconstruct(int *spectrum[MAX_TIME_CHANNELS],   /* array of pointers to spectral data */
                       Ch_Info *channelInfo,                  /* Pointer to Ch_Info */
                       int sfb_offset_table[][MAX_SCFAC_BANDS+1],
                       WINDOW_TYPE block_type[MAX_TIME_CHANNELS], /* Block type */
                       AACQuantInfo* quantInfo,               /* Quant info */
                       int numberOfChannels);                 /* Number of channels */
#endif
#endif

