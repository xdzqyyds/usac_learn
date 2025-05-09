/**********************************************************************
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by

Bodo Teichmann (Fraunhofer Institute of Integrated Circuits tmn@iis.fhg.de)

and edited by

Heiko Purnhagen (University of Hannover)
Olaf Kaehler (Fraunhofer IIS-A)
Manuela Schinn (Fraunhofer IIS)

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1998.




BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

**********************************************************************/

#ifndef _FLEX_MUX_H_INCLUDED
#define _FLEX_MUX_H_INCLUDED

#include "obj_descr.h"           /* typedef structs */


typedef enum {
  NULL_OBJECT	= 0,	/* NULL Object */
  AAC_MAIN 	= 1,	/* AAC Main Object */
  AAC_LC 	= 2,	/* AAC Low Complexity(LC) Object */
  AAC_SSR 	= 3,	/* AAC Scalable Sampling Rate(SSR) Object */
  AAC_LTP 	= 4,	/* AAC Long Term Predictor(LTP) Object */
#ifndef CT_SBR
  RSVD_5 	= 5,	/* (reserved) */
#else
  SBR           = 5,    /* Spectral Band Replication */
#endif
  AAC_SCAL 	= 6,	/* AAC Scalable Object */
  TWIN_VQ 	= 7,	/* TwinVQ Object */
  CELP 		= 8,	/* CELP Object */
  HVXC 		= 9,	/* HVXC Object */
  RSVD_10 	= 10,	/* (reserved) */
  RSVD_11 	= 11,	/* (reserved) */
  TTSI 		= 12,	/* TTSI Object(not supported) */
  MAIN_SYNTH 	= 13,	/* Main Synthetic Object(not supported) */
  WAV_TAB_SYNTH = 14,	/* Wavetable Synthesis Object(not supported) */
  GEN_MIDI 	= 15,	/* General MIDI Object(not supported) */
  ALG_SYNTH_AUD_FX = 16	/* Algorithmic Synthesis and Audio FX Object(not supported) */
  ,
  ER_AAC_LC	= 17,	/* Error Resilient(ER) AAC Low Complexity(LC) Object */
  RSVD_18 	= 18,	/* (reserved) */
  ER_AAC_LTP	= 19,	/* Error Resilient(ER) AAC Long Term Predictor(LTP) Object */
  ER_AAC_SCAL	= 20,	/* Error Resilient(ER) AAC Scalable Object */
  ER_TWIN_VQ	= 21,	/* Error Resilient(ER) TwinVQ Object */
  ER_BSAC	= 22,	/* Error Resilient(ER) BSAC Object */
  ER_AAC_LD	= 23,	/* Error Resilient(ER) AAC LD Object */
  ER_CELP	= 24,	/* Error Resilient(ER) CELP Object */
  ER_HVXC	= 25,	/* Error Resilient(ER) HVXC Object */
  ER_HILN	= 26,	/* Error Resilient(ER) HILN Object */
  ER_PARA	= 27,	/* Error Resilient(ER) Parametric Object */
#ifdef EXT2PAR
  SSC           = 28,	/* Parametric coding extension 2 Object */
#else
  RSVD_28 	= 28,	/* (reserved) */
#endif
#ifdef PARAMETRICSTEREO
  PS 	        = 29,	/* (reserved) */
#else
  RSVD_29 	= 29,	/* (reserved) */
#endif
  RSVD_30       = 30,	/* (reserved) */
  AOT_ESC       = 31,	/* Escape sequence for AOTs greater 31 */
#ifdef MPEG12
  LAYER_1       = 32,	/* Layer-1 Object */
  LAYER_2       = 33,	/* Layer-2 Object */
  LAYER_3       = 34,	/* Layer-3 Object */
#endif
  RSVD_35       = 35,	/* (reserved) */
  RSVD_36       = 36,	/* (reserved) */
#ifdef I2R_LOSSLESS
  SLS           = 37,   /* SLS w/ AAC core*/
  SLS_NCORE     = 38,   /* SLS w/o core */
#endif
#ifdef AAC_ELD
  ER_AAC_ELD    = 39,   /* AAC ELD object with LDFB */
#endif
  USAC          = 42,
  __dummy_aot__
} AUDIO_OBJECT_TYPE_ID;

int advanceESDescr ( HANDLE_BSBITSTREAM       bitStream,
                     ES_DESCRIPTOR*    es,
                     int               WriteFlag,
                     int               SystemsFlag);
void initESDescr( ES_DESCRIPTOR **es);
void presetESDescr( ES_DESCRIPTOR *es, int numLayers);
void initObjDescr( OBJECT_DESCRIPTOR *od);
void presetObjDescr( OBJECT_DESCRIPTOR *od);
void  advanceODescr (   HANDLE_BSBITSTREAM       bitStream, OBJECT_DESCRIPTOR *od, int WriteFlag) ;

int advanceAudioSpecificConfig(HANDLE_BSBITSTREAM bs, AUDIO_SPECIFIC_CONFIG *audSpC, int WriteFlag, int SystemsFlag
#ifndef CORRIGENDUM1
                               ,int isEnhLayer
#endif
                               );
void setupDecConfigDescr(DEC_CONF_DESCRIPTOR *decConfigDescr);
void presetDecConfigDescr(DEC_CONF_DESCRIPTOR *decConfigDescr);
int advanceDecoderConfigDescriptor(HANDLE_BSBITSTREAM bs, DEC_CONF_DESCRIPTOR *decConfig, int WriteFlag, int SystemsFlag
#ifndef CORRIGENDUM1
                                   ,int isEnhLayer
#endif
                                   );
void initEpSpecConf (EP_SPECIFIC_CONFIG *esc);
int advanceEpSpecConf(HANDLE_BSBITSTREAM  bs, EP_SPECIFIC_CONFIG *epConf, int WriteFlag);

int usac_get_channel_number(USAC_CONFIG* pUsacConfig, int* nLfe);

int get_channel_number(int channelConfig,
                       HANDLE_PROG_CONFIG p,
                       int *numFC,   /* number of front channels */
                       int *fCenter, /* 1 if decoder has front center channel */
                       int *numSC,   /* number of side channels */
                       int *numBC,   /* number of back channels */
                       int *bCenter, /* 1 if decoder has back center channel */
                       int *numLFE,  /* number of LFE channels */
                       int *numIndCCE); /* number of individually switched coupling channels */

int get_channel_number_USAC(USAC_CONFIG *pUsacConfig, /* USAC configuration*/
                       int *numFC,   /* number of front channels */
                       int *fCenter, /* 1 if decoder has front center channel */
                       int *numSC,   /* number of side channels */
                       int *numBC,   /* number of back channels */
                       int *bCenter, /* 1 if decoder has back center channel */
                       int *numLFE,  /* number of LFE channels */
                       int *numIndCCE); /* number of individually switched coupling channels */

/* fixing 7.1channel BSAC error */
int get_channel_number_BSAC(int channelConfig,
                       HANDLE_PROG_CONFIG p,
                       int *numFC,   /* number of front channels */
                       int *fCenter, /* 1 if decoder has front center channel */
                       int *numSC,   /* number of side channels */
                       int *numBC,   /* number of back channels */
                       int *bCenter, /* 1 if decoder has back center channel */
                       int *numLFE,  /* number of LFE channels */
                       int *numIndCCE); /* number of individually switched coupling channels */

/* determine number of tracks per layer...
   when using this function, remember: maxLayer = numLayer - 1 */
void tracksPerLayer(DEC_CONF_DESCRIPTOR *decConf, /* array of decoder configs */
                    int *maxLayer,   /* number of layers, in: selected, out: available */
                    int *streamCount,/* number of tracks, in: total, out: decoded */
                    int *tracksInLayer,/* out: array with track count per layer */
                    int *extInLayer);/* out: array with number of extension tracks per layer */


void removeAU(HANDLE_BSBITSTREAM  stream, int decBits, FRAME_DATA*        fd,int layer);

#endif
