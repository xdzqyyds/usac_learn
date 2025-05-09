/**********************************************************************
MPEG-4 Audio VM
Decoder core (parametric)



This software module was originally developed by

Heiko Purnhagen (University of Hannover)

and edited by

Yuji Maeda (Sony Corporation)

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



Source file: dec_par_dmy.c

 

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
03-sep-96   HP    added speed change & pitch change for parametric core
05-nov-03   OK    fixed this dummy after rewrite activities
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_m4a.h"		/* common module */
#include "cmdline.h"		/* command line module */
#include "lpc_common.h"
#include "obj_descr.h"		/* AI 990616 */
#include "dec_par.h"		/* decoder cores */


/* ---------- declarations ---------- */

#define PROGVER "parametric decoder core dummy"


/* ---------- functions ---------- */


/* DecParInfo() */
/* Get info about parametric decoder core. */

char *DecParInfo (
  FILE *helpStream)		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL)
    fprintf(helpStream,PROGVER "\n\n");
  return PROGVER;
}


/* DecHvxcInit() */
/* Init HVXC decoder core for system interface(AI 990616) */

HvxcDecStatus *DecHvxcInit (
  char *decPara,		/* in: decoder parameter string */
  FRAME_DATA *fD,		/* in: system interface */
  int layer			/* in: number of layer */
  , int epFlag,			/* in: error protection flag */
  int epDebugLevel,		/* in: debug level */
  char* infoFileName,		/* in: crc information file name */
  HANDLE_ESC_INSTANCE_DATA* hEscInstanceData 	/* out: ep-information */
  )
{
  CommonExit(1,"DecHvxcInit: dummy");

  return(NULL);
}


/* DecParFree() */
/* Free memory allocated by parametric decoder core. */

void DecParFree (ParaDecStatus *paraD)
{
  CommonExit(1,"DecParFree: dummy");
}


/* DecHvxcFrame() */
/* Decode one bit stream frame into one audio frame with */
/* HVXC decoder core - system interface(AI 990616) */

void DecHvxcFrame (
  FRAME_DATA*  fD,
  HvxcDecStatus* hvxcD,		/* in: HVXC decoder status handle */
  int *usedNumBit	        /* out: num bits used for this frame         */
  , int epFlag,                 /* in: error protection flag */
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData
  )
{
  CommonExit(1,"DecHvxcFrame: dummy");
}


/* DecParInit() */
/* Init parametric decoder core for system interface   HP 20000330 */

ParaDecStatus *DecParInit (
  char *decPara,		/* in: decoder parameter string */
  FRAME_DATA *fD,		/* in: system interface */
  int layer,			/* in: number of layer */
  int epFlag,			/* in: error protection flag */
  int epDebugLevel,		/* in: debug level */
  char* infoFileName,		/* in: crc information file name */
  HANDLE_ESC_INSTANCE_DATA* hEscInstanceData 	/* out: ep-information */
  )
{
  CommonExit(-1,"DecParInit: dummy\n");
  return NULL;
}		

/* DecParFrame() */
/* Decode one bit stream frame into one audio frame with */
/* parametric decoder core - system interface   HP 20000330 */

void DecParFrame (
  FRAME_DATA*  fD,
  ParaDecStatus* paraD,		/* in: para decoder status handle */
  int *usedNumBit,	        /* out: num bits used for this frame         */
  int epFlag,                   /* in: error protection flag */
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData
  )
{
  fprintf(stderr,"DecParFrame: dummy\n");
}


/* DecHvxcFree() */
/* Free memory allocated by parametric decoder core. */

void DecHvxcFree(
HvxcDecStatus *hvxcD		/* in: HVXC decoder status handle */
) 
{
  CommonExit(1,"DecHvxcFree: dummy");
}


/* end of dec_par_dmy.c */

