/*****************************************************************************
 *                                                                           *
"This software module was originally developed by 

Ralph Sperschneider (Fraunhofer Gesellschaft IIS)

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 AAC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1996, 1999.
 *                                                                           *
 ****************************************************************************/ 
 
#ifndef _reorderspec_h_
#define _reorderspec_h_

#include "allHandles.h"
#include "bitfct.h"

/*** T_HCR ****/

#define LCW_CONCEALMENT_PATCH
#define VCB11_CONCEALMENT_PATCH
#define COMPLETION_CONCEALMENT_PATCH

HANDLE_HCR     CreateHcrInfo ( void );

void DeleteHcrInfo (HANDLE_HCR hHcrInfo) ;

unsigned short GetLenOfLongestCw ( const HANDLE_HCR hHcrInfo );

unsigned short GetLenOfSpecData ( const HANDLE_HCR hHcrInfo );

unsigned short GetReorderStatusFlag ( const HANDLE_HCR hHcrInfo );

void ReadLenOfLongestCw ( HANDLE_HCR         hHcrInfo, 
                          HANDLE_RESILIENCE  hResilience,
                          HANDLE_BUFFER      hVm,
                          HANDLE_ESC_INSTANCE_DATA     hEscInstanceData );

void ReadLenOfSpecData ( HANDLE_HCR         hHcrInfo, 
                         HANDLE_RESILIENCE  hResilience,
                         HANDLE_BUFFER      hVm, 
                         HANDLE_ESC_INSTANCE_DATA     hEscInstanceData);

void ReadEntryPoints ( HANDLE_HCR         hHcrInfo, 
                       HANDLE_RESILIENCE  hResilience,
                       HANDLE_BUFFER      hVm, 
                       HANDLE_ESC_INSTANCE_DATA     hEscInstanceData );
/*** T_HCR end ***/

void DecodedBitCnt ( HANDLE_HCR hHcrInfo, unsigned short codewordLen );

unsigned short GetDecodedBitCnt ( const HANDLE_HCR hHcrInfo );

void CheckDecodingProgress ( HANDLE_BUFFER      hSpecData,
                             HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                             HANDLE_HCR         hHcrInfo,
                             HANDLE_RESILIENCE  hResilience );

HANDLE_BUFFER GetNonPcwBufPtrHdl ( const HANDLE_HCR     hHcrInfo );

void           InitHcr ( HANDLE_BUFFER hSpecData, 
                         HANDLE_HCR    hHcrInfo );

unsigned char GetCurrentMaxCodewordLen ( unsigned short     maxCWLen,
                                         const HANDLE_HCR   hHcrInfo );

void ReadNonPcwsNew ( unsigned short*    codewordLen, 
                      int*               qp, 
                      Hcb*               hcb, 
                      Huffman*           hcw, 
                      unsigned short     step,
                      unsigned short     table,
                      unsigned short*    codewordsInSet,
                      HANDLE_RESILIENCE  hResilience,
                      HANDLE_HCR         hHcrInfo,
                      HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                      HANDLE_CONCEALMENT hConcealment );

void ReadPcws ( unsigned short*    codewordLen, 
                int*               qp,
                Hcb*               hcb, 
                Huffman*           hcw, 
                unsigned short     codebook, 
                unsigned short     maxCWLen, 
                unsigned short     step, 
                HANDLE_RESILIENCE  hResilience,
                HANDLE_BUFFER      hSpecData,
                HANDLE_HCR         hHcrInfo, 
                HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                HANDLE_CONCEALMENT hConcealment );

void ReadNonPcws ( unsigned short*    codewordLen, 
                   int*               qp, 
                   Hcb*               hcb, 
                   Huffman*           hcw, 
                   unsigned short     step,
                   unsigned short     table,
                   HANDLE_RESILIENCE  hResilience,
                   HANDLE_HCR         hHcrInfo,
                   HANDLE_ESC_INSTANCE_DATA     hEscInstanceData,
                   HANDLE_CONCEALMENT hConcealment ); 

void ReorderSpecDecPCWFinishedCheck ( unsigned short     maxCWLen,
                                      HANDLE_BUFFER      hSpecData,
                                      HANDLE_RESILIENCE  hResilience,
                                      HANDLE_HCR         hHcrInfo, 
                                      HANDLE_ESC_INSTANCE_DATA     hEPInfo );

void SetNrOfCodewords ( unsigned short    nrOfCodewords,
                        const HANDLE_HCR  hHcrInfo );

void Vcb11ConcealmentPatch ( char*             calledFrom,
                             unsigned short    codebook,
                             int               lavInclEsc,
                             int*              qp,
                             unsigned short    step,
                             HANDLE_RESILIENCE hResilience );

void LcwConcealmentPatch ( char*          calledFrom,
                           unsigned short codewordLen,
                           unsigned short maxCWLen,
                           int*           qp,
                           unsigned short step,
                           HANDLE_HCR     hHcrInfo );

#endif /* _reorderspec_h_  */

