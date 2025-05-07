/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by

Pierre Lauber       (Fraunhofer Gesellschaft IIS)

and edited by

Markus Werner       (SEED / Software Development Karlsruhe)

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

$Id: allHandles.h,v 1.1.1.1 2009-05-29 14:10:10 mul Exp $
Handle definitions
**********************************************************************/
#ifndef _allHandle_h_
#define _allHandle_h_

typedef struct tagAACDecoder *HANDLE_AACDECODER;
typedef struct tagUsacDecoder *HANDLE_USAC_DECODER;
typedef struct tagProgConfig *HANDLE_PROG_CONFIG;

#ifdef I2R_LOSSLESS
typedef struct  tagSLSDecoder *HANDLE_SLSDECODER;
#endif

/*
  VERSION 1 handle type definitions
*/
typedef struct tagBsBitBufferStruct *HANDLE_BSBITBUFFER;
typedef struct tagBsBitStreamStruct *HANDLE_BSBITSTREAM;
typedef struct tag_buffer           *HANDLE_BUFFER;
typedef struct tag_modulo_buffer    *HANDLE_MODULO_BUF_VM;

/*
  VERSION 2 handle type definitions
*/
typedef struct tag_resilience       *HANDLE_RESILIENCE;
typedef struct tag_hcr              *HANDLE_HCR;
typedef struct tagEscInstanceData   *HANDLE_ESC_INSTANCE_DATA;
typedef struct tag_concealment      *HANDLE_CONCEALMENT;

typedef struct tagEpConverter       *HANDLE_EPCONVERTER;


/*
  the overall handler
*/
typedef struct tagDecoderGeneral {
  HANDLE_BUFFER            hVm;
  HANDLE_BUFFER            hHcrSpecData;
  HANDLE_RESILIENCE        hResilience;
  HANDLE_HCR               hHcrInfo;
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData;
  HANDLE_CONCEALMENT       hConcealment;
} T_DECODERGENERAL;
typedef struct tagDecoderGeneral *HANDLE_DECODER_GENERAL;

#ifdef DRC
typedef struct tagDRC *HANDLE_DRC;
#endif

#endif /* _allHandle_h_ */
