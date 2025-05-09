/*****************************************************************************
 *                                                                           *
"This software module was originally developed by 

Fraunhofer Gesellschaft IIS 

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
Copyright(c)1998, 1999, 2000.
 *                                                                           *
 *****************************************************************************/

/* This file has been created automatically. */
/* Any changes in this file will be lost.    */

#define D(a) {const void *dummyfilepointer; dummyfilepointer = &a;}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <assert.h>
#include "allHandles.h"
#include "block.h"
#include "buffer.h"
#include "tf_mainHandle.h"
#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */
#include "allVariables.h"        /* variables */
#include "common_m4a.h"
#include "concealment.h"
#include "bitstream.h"
#include "huffdec2.h"
#include "monopred.h"
#include "plotmtv.h"
#include "port.h"
#include "statistics_aac.h"
#include "reorderspec.h"
#include "resilience.h"
#include "tf_main.h"
#ifdef __cplusplus
#define LANG "C"
#else
#define LANG
#endif
#ifdef DEFPROFILE
#define STATIC
#else
#define STATIC static
#endif
#ifndef M_PI
#endif
#define MALLOC(DST, ARRAYSIZE)  *(void**)&DST = malloc(sizeof(*(DST))*(ARRAYSIZE))
#define CALLOC(DST, ARRAYSIZE)  *(void**)&DST = calloc(sizeof(*(DST))*(ARRAYSIZE),1)
#define MEMSET(DST, BYTE, ARRAYSIZE) memset(DST, BYTE, sizeof(*(DST))*(ARRAYSIZE))
#define MEMCPY(DST, SRC,  ARRAYSIZE) memcpy(DST, SRC,  sizeof(*(DST))*(ARRAYSIZE))
#define MEMCMP(DST, SRC,  ARRAYSIZE) memcmp(DST, SRC,  sizeof(*(DST))*(ARRAYSIZE))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#ifndef RAND_MAX
#endif
#ifdef DEBUGPLOT
#else
#endif
#ifdef DEFSBLIMIT
#else
#endif
#ifdef DEFLENPRED
#else
#endif
#ifdef DEFLENARRAYPRED
#else
#endif
#ifdef SENSITIVITY
#endif /*SENSITIVITY*/
#ifdef NOTVMCODE
#include "winmap.h"           /* Info, winmap                        */
#include "vm-imdct.h"
void ConcealmentSetModuleStaticVariables(int   epDebugLevelLocal,
                                         float energyAttenuationFactorLocal)
{
  D(epDebugLevelLocal)
  D(energyAttenuationFactorLocal)
}

#endif
#if !defined(WIN32) && !defined(__CYGWIN__)
#endif
#if !defined(WIN32) && !defined(__CYGWIN__)
#endif
void ConcealmentInitChannel(int                channel,
                            int                numberOfChannels,
                            HANDLE_CONCEALMENT hConcealment)
{
  D(channel)
  D(numberOfChannels)
  D(hConcealment)
}

void ConcealmentSetChannel(int                channel,
                           HANDLE_CONCEALMENT hConcealment)
{
  D(channel)
  D(hConcealment)
}

void ConcealmentIncrementLine(int                increment,
                              HANDLE_CONCEALMENT hConcealment)
{
  D(increment)
  D(hConcealment)
}

void ConcealmentSetLine(int                line,
                        HANDLE_CONCEALMENT hConcealment)
{
  D(line)
  D(hConcealment)
}

int ConcealmentGetLine(const HANDLE_CONCEALMENT hConcealment)
{
  D(hConcealment)
  return(0);
}

void ConcealmentHelp(FILE *helpStream)
{
  D(helpStream)
}

void ConcealmentInit(HANDLE_CONCEALMENT *hCon,
                     const char         *decPara,
                     int                epDebugLevelDeliver)
{
  D(hCon)
  D(decPara)
  D(epDebugLevelDeliver)
}

#ifdef DEBUGPLOT
#endif
#ifdef EED_FAKE
#endif
#ifdef DEFFRAMELOSSRATE
#if DEFFRAMELOSSRATE == 'f'
#else
#endif
#else
#endif
void ConcealmentCheckClassBufferFullnessEP(int                            decoded_bits,
                                           const HANDLE_RESILIENCE        hResilience,
                                           const HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                           HANDLE_CONCEALMENT             hConcealment)
{
  D(decoded_bits)
  D(hResilience)
  D(hEscInstanceData)
  D(hConcealment)
}

HANDLE_ESC_INSTANCE_DATA ConcealmentGetEPprematurely(HANDLE_CONCEALMENT hConcealment)
{
  D(hConcealment)
  return(0);
}

void ConcealmentCheckLenOfHcrSpecData(const HANDLE_HCR   hHcrInfo,
                                      HANDLE_CONCEALMENT hConcealment)
{
  D(hHcrInfo)
  D(hConcealment)
}

void ConcealmentDetectErrorCodebook(byte               *codebook,
                                    HANDLE_CONCEALMENT hConcealment)
{
  D(codebook)
  D(hConcealment)
}

void ConcealmentDetectErrorSectionData(const SECTIONDATA_ERRORTYPE sectionDataError,
                                       HANDLE_CONCEALMENT          hConcealment)
{
  D(sectionDataError)
  D(hConcealment)
}

void ConcealmentTouchLine(int                step,
                          HANDLE_CONCEALMENT hConcealment)
{
  D(step)
  D(hConcealment)
}

void ConcealmentDetectErrorCodeword(unsigned short          step,
                                    unsigned short          codewordLen,
                                    unsigned short          maxCWLen, 
                                    int                     lavInclEsc,
                                    unsigned short          codebook,
                                    int                     *codewordValue,
                                    CODEWORD_TYPE           codewordType,
                                    const HANDLE_RESILIENCE hResilience,
                                    const HANDLE_HCR        hHcrInfo,
                                    HANDLE_CONCEALMENT      hConcealment)
{
  D(step)
  D(codewordLen)
  D(maxCWLen)
  D(lavInclEsc)
  D(codebook)
  D(codewordValue)
  D(codewordType)
  D(hResilience)
  D(hHcrInfo)
  D(hConcealment)
}

void ConcealmentSetErrorCodeword(unsigned short     step,
                                 HANDLE_CONCEALMENT hConcealment)
{
  D(step)
  D(hConcealment)
}

void ConcealmentTemporalNoiseShaping(Float             **coefValue,
                                     Info               *ip,
                                     short              sbk,
                                     short              channel,
                                     HANDLE_CONCEALMENT hConcealment)
{
  D(*coefValue)
  D(ip)
  D(sbk)
  D(channel)
  D(hConcealment)
}

void ConcealmentMiddleSideStereo(Info               *ip,
                                 byte               *group,
                                 byte               *mask,
                                 Float              **coefValue,
                                 int                left,
                                 int                right,
                                 HANDLE_CONCEALMENT hConcealment)
{
  D(ip)
  D(group)
  D(mask)
  D(*coefValue)
  D(left)
  D(right)
  D(hConcealment)
}

void Concealment0SetOfErrCWCoeff(const char         *coefficientDescription,
                                 Float             *coefValue,
                                 int                lineTotal,
                                 int                resetError,
                                 HANDLE_CONCEALMENT hConcealment)
{
  D(coefficientDescription)
  D(coefValue)
  D(lineTotal)
  D(resetError)
  D(hConcealment)
}

#ifdef RAND_OLD
#else
#endif
void ConcealmentMainEntry(const MC_Info           *mip,
                          const WINDOW_SEQUENCE   *wnd,
                          const Wnd_Shape         *wnd_shape,
                          WINDOW_SEQUENCE         *windowSequence,
                          WINDOW_SHAPE            *window_shape,
                          int                     **lpflag,
                          byte                    **group,
                          byte                    *hasmask,
                          byte                    **mask,
                          Float                  **coefValue,
                          const HANDLE_RESILIENCE hResilience,
                          HANDLE_CONCEALMENT      hConcealment)
{
  D(mip)
  D(wnd)
  D(wnd_shape)
  D(windowSequence)
  D(window_shape)
  D(*lpflag)
  D(*group)
  D(hasmask)
  D(*mask)
  D(*coefValue)
  D(hResilience)
  D(hConcealment)
}

void ConcealmentDeinterleave(const Info*              info,
                             const HANDLE_RESILIENCE  hResilience,
                             HANDLE_CONCEALMENT       hConcealment)
{
  D(info)
  D(hResilience)
  D(hConcealment)
}

unsigned char ConcealmentAvailable(int                channel,
                                   HANDLE_CONCEALMENT hConcealment)
{
  D(channel)
  D(hConcealment)
  return(0);
}

#ifdef SENSITIVITY
#include "interface.h"
void ResilienceOpenSpectralValuesFile(char*              audioFileName,
                            HANDLE_CONCEALMENT hConcealment)
{
  D(audioFileName)
  D(hConcealment)
}

void ResilienceDealWithSpectralValues(int*               quant,
                            HANDLE_CONCEALMENT hConcealment,
                            unsigned long      bno )
{
  D(quant)
  D(hConcealment)
  D(bno)
}

#endif /*SENSITIVITY*/
#undef D
