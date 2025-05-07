/**********************************************************************
 MPEG-4 Audio VM

 This software module was originally developed by
 
 Manuela Schinn (Fraunhofer IIS)
  
 and edited by

 
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
 
 Copyright (c) 2006.
 
 $Id: dec_mpeg12.h,v 1.1.1.1 2009-05-29 14:10:13 mul Exp $
 MPEG12 Decoder module
**********************************************************************/

#ifndef DEC_MPEG12_H
#define DEC_MPEG12_H

#include <stdio.h>          /* typedef FILE */
#include "obj_descr.h"
#include "debug.h"
#include "decLayer123.h"    /* MPEG12 decoder core */
#include "pcmBuffer.h"
#include "bitBuffer.h"


#define MAX_CHANNEL_ELEMENTS    5
#define MAX_CHANNEL_CONF        7


/* ---------- structures ---------- */

typedef struct {
  HANDLE_decLayer123    dec[MAX_CHANNEL_ELEMENTS];        /* decoder instances */
  float*                sampleBuf[MAX_CHANNEL_CONF];      /* MPEG4 conform output */  
  int                   numChannelElements;  
  int                   numChannels;  
  int                   frameNumSample;     
  HANDLE_bitBuffer      bitbuf;                           /* MPEG12 decoder core input bitstream */
  HANDLE_pcmBuffer      pcmbuf;                           /* MPEG12 decoder core output */
  HANDLE_debug          debug;                            /* DEBUG output stream */
}MPEG12_DATA;



/* ---------- functions ---------- */


/* DecMPEG12Info() */
/* Get info about MPEG12-based decoder core. */
char *DecMPEG12Info (FILE *helpStream);



/* DecMPEG12Init() */
/* Init MPEG12-based decoder core. */
void DecMPEG12Init (char *decPara, MPEG12_DATA *mpeg12Data, int stream, FRAME_DATA *fD);



/* DecMPEG12Frame() */
/* Decode one bit stream frame into one audio frame with */
/* MPEG12-based decoder core. */
void DecMPEG12Frame (FRAME_DATA *frameData, MPEG12_DATA *mpeg12Data);



/* DecMPEG12Free() */
/* Free memory allocated by MPEG12-based decoder core. */
void DecMPEG12Free (MPEG12_DATA *mpeg12Data);



#endif /* #ifndef DEC_MPEG12_H */


