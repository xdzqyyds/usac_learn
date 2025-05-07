/**********************************************************************
MPEG-4 Audio VM
Decoder core (LPC-based)



This software module was originally developed by

Heiko Purnhagen (University of Hannover)

and edited by

Ali Nowbakht-Irani (FhG-IIS)

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



Source file: dec_lpc_dmy.c

 

Required modules:
common.o		common module
cmdline.o		command line module
bitstream.o		bits stream module

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

#include "lpc_common.h"          /* defines, structs */
#include "obj_descr.h"           /* structs */
#include "nok_ltp_common.h"      /* structs*/
#include "tf_mainStruct.h"       /* structs */

#include "common_m4a.h"		/* common module */
#include "cmdline.h"		/* command line module */
#include "bitstream.h"		/* bit stream module */

#include "dec_lpc.h"		/* decoder cores */


/* ---------- declarations ---------- */

#define PROGVER "LPC-based decoder core dummy"


/* ---------- functions ---------- */


/* DecLpcInfo() */
/* Get info about LPC-based decoder core. */

char *DecLpcInfo (
  FILE *helpStream)		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL)
    fprintf(helpStream,PROGVER "\n\n");
  return PROGVER;
}


/* DeLpcrFree() */
/* Free memory allocated by LPC-based decoder core. */

void DecLpcFree (LPC_DATA *lpcData)
{
  CommonExit(1,"DecLpcFree: dummy");
}


void DecLpcInit (
  char *decPara,                   /* in: decoder parameter string     */
  FRAME_DATA*  frameData,
  LPC_DATA*    lpcData
  ) 
{
  CommonExit(1,"DecLpcInitNew: dummy");
}


void DecLpcFrame (
  FRAME_DATA*  frameData,
  LPC_DATA*    lpcData,
  int *usedNumBit)        /* out: num bits used for this frame         */
{
  CommonExit(1,"DecLpcFrameNew: dummy");
}

int lpcframelength( CELP_SPECIFIC_CONFIG *celpConf, int layer )
{
  CommonExit(1,"lpcframelength: dummy");
  return ( 0 );
}


int erlpcframelength( CELP_SPECIFIC_CONFIG *erCelpConf, int layer )
{
  CommonExit(1,"erlpcframelength: dummy");
  return ( 0 );
}


/* end of dec_lpc_dmy.c */

