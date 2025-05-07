/**********************************************************************
MPEG-4 Audio VM
Deocoder cores (parametric, LPC-based, t/f-based)



This software module was originally developed by

Markus Werner (SEED / Software Development Karlsruhe)

and edited by

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

Copyright (c) 2000.

$Id: decifc.h,v 1.3 2011-05-28 14:25:18 mul Exp $
Decoder high level interface
**********************************************************************/
#ifndef DECIFC_H
#define DECIFC_H

#include "all.h"
#include "lpc_common.h"
#include "tf_mainStruct.h"
#include "usac_mainStruct.h"
#include "hvxc_struct.h"
#include "flex_mux.h"
#include "mp4au.h"
#include "dec_par.h"
#ifdef MPEG12
#include "dec_mpeg12.h"
#endif
#ifdef EXT2PAR
#include "SscDec.h"
#endif


typedef struct {
  char *decPara;
  char *aacDebugString;
  char *infoFileName;
  int mainDebugLevel;
  int epDebugLevel;
}DEC_DEBUG_DATA;



typedef struct {
    FRAME_DATA    *frameData;
    TF_DATA       *tfData ;
    USAC_DATA     *usacData;
    LPC_DATA      *lpcData ;
    HVXC_DATA     *hvxcData;
    PARA_DATA     *paraData;
#ifdef MPEG12
    MPEG12_DATA   *mpeg12Data;
#endif
#ifdef EXT2PAR
    SSCDEC        *sscData;
#endif
} DEC_DATA;



int frameDataInit(int  allTracks,
                  int  streamCount, /* in: number of ESs */
                  int  maxLayer,
                  DEC_CONF_DESCRIPTOR decConf [MAX_TRACKS],
                  DEC_DATA       *decData
);

int audioDecInit(HANDLE_DECODER_GENERAL   hFault,
                 DEC_DATA                *decData,
                 DEC_DEBUG_DATA          *decDebugData,
                 char                    *drc_payload_fileName,
                 char                    *drc_config_fileName,
                 char                    *loudness_config_fileName,
                 int                     *streamID,
#ifdef CT_SBR
                 int                      HEaacProfileLevel,
                 int                      bUseHQtransposer,
#endif
                 int                      tracksForDecoder
);



int audioDecFrame(DEC_DATA   *decData,            /* in  : Decoder Status(Handles) */
                  HANDLE_DECODER_GENERAL hFault,
                  float      ***outSamples,       /* out : composition Unit (outsamples) */
                  long        *numOutSamples,     /* out : number of oputput samples/ch  */
                  int         *numOutChannels     /* out : number of oputput channels */ /* SAMSUNG_2005-09-30 */
);

int audioDecFree(DEC_DATA   *decData,             /* in  : Decoder Status(Handles) */
                 HANDLE_DECODER_GENERAL hFault
);

#endif
