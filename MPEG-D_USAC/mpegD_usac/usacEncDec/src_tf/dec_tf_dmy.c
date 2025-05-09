/**********************************************************************
MPEG-4 Audio VM
Decoder core (t/f-based)



This software module was originally developed by

Heiko Purnhagen (University of Hannover)

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

Copyright (c) 1996.



Source file: dec_tf_dmy.c

 

Required modules:
common.o                common module
cmdline.o               command line module
bitstream.o             bits stream module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
24-jun-96   HP    dummy core
15-aug-96   HP    adapted to new dec.h
26-aug-96   HP    CVS
05-nov-03   OK    fixed this dummy after rewrite activities
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allHandles.h"

#include "lpc_common.h"          /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "common_m4a.h"             /* common module */
#include "cmdline.h"            /* command line module */
#include "bitstream.h"          /* bit stream module */

#include "dec_tf.h"                /* decoder cores */


/* ---------- declarations ---------- */

#define PROGVER "t/f-based decoder core dummy"


/* ---------- functions ---------- */


/* DecTfInfo() */
/* Get info about t/f-based decoder core. */

char *DecTfInfo (
  FILE *helpStream)             /* in: print decPara help text to helpStream */
                                /*     if helpStream not NULL */
                                /* returns: core version string */
{
  if (helpStream != NULL)
    fprintf(helpStream,PROGVER "\n\n");
  return PROGVER;
}


void DecTfInit (
                   char*               decPara,          /* in: decoder parameter string */    
                   char*               aacDebugStr,
                   int                 epDebugLevel,
                   HANDLE_DECODER_GENERAL hFault,
                   FRAME_DATA*         frameData,
                   TF_DATA*            tfData,
                   LPC_DATA*           lpcData,
                   int                 layer,
                   char*               infoFileName
                 )
{
  CommonExit(1,"DecTfInit: dummy");
}

/* DecTfFrame() */
/* Decode one bit stream frame into one audio frame with */
/* t/f-based decoder core. */

void DecTfFrame(
                   int          numChannels, 
                   FRAME_DATA*  fd, /* config data ,one bitstream buffer for each layer, obj descr. etc. */
                   TF_DATA*     tfData,
                   LPC_DATA*    lpcData,
                   HANDLE_DECODER_GENERAL hFault
								int *numOutChannels /* SAMSUNG_2005-09-30 */
								  ,int	*flagBSAC_SBR	/* 20060107 */
)
{
  CommonExit(1,"DecTfFrame: dummy");
}


/* DecTfFree() */
/* Free memory allocated by t/f-based decoder core. */

void DecTfFree ( 
                TF_DATA *tfData,
                HANDLE_DECODER_GENERAL hFault
                )
{
  CommonExit(1,"DecTfFree: dummy");
}


/* end of dec_tf_dmy.c */

