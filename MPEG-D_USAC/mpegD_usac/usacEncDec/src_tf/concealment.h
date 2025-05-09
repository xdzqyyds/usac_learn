/**********************************************************************
MPEG4 Audio VM Module

This software module was originally developed by

  Pierre Lauber (Fraunhofer IIS-A)

and edited by

  Reinhold Boehm (Fraunhofer IIS-A)
  Ralph Sperschneider (Fraunhofer IIS-A)

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

**********************************************************************/

#ifndef _concealment_h_
#define _concealment_h_

#include "block.h"

typedef enum {
  CODEWORD_PCW      = 2,
  CODEWORD_LONG_PCW = 4,
  CODEWORD_NON_PCW  = 6
} CODEWORD_TYPE;

typedef enum {
  NO_ERROR,
  SECTIONLENGTH__EQ__ZERO,
  NR_OF_SCF_BANDS__GT__CURRENT_MAX_SFB,
  NR_SFB__NE__TOT_SFB,
  NSECT__GT__TOTSFB,
  GROUP__NE__NUMGROUPS
} SECTIONDATA_ERRORTYPE;


/*
 * Over all initialization.
 * Analyze commandline substring, set internal status vars and allocate memory.
 */
void ConcealmentInit(HANDLE_CONCEALMENT* hConcealment,
                     const char         *decPara,
                     int                 epDebugLevel);


/*
 * Initialization before starting decoding of each channel of a frame.
 * Set the internal audio channel.
 * Setting the number of really used audio channels.
 * Note: channel is the VM internal channel.
 */
void ConcealmentInitChannel(int                channel,
                            int                numberOfChannels,
                            HANDLE_CONCEALMENT hConcealment);


/*
 * Set currently decoded audio channel.
 * Set the internal audio channel.
 * Note: channel is the VM internal channel.
 */
void ConcealmentSetChannel(int                channel,
                           HANDLE_CONCEALMENT hConcealment);


/*
 * Increment the internal spectral line counter.
 * Note: line is not different for different channels.
 */
void ConcealmentIncrementLine(int                increment,
                              HANDLE_CONCEALMENT hConcealment);


/*
 * Set the internal spectral line counter.
 * Note: line is not different for different channels.
 */
void ConcealmentSetLine(int                line,
                        HANDLE_CONCEALMENT hConcealment);


/*
 * Get the internal spectral line counter.
 * Note: line is not different for different channels.
 */
int ConcealmentGetLine(const HANDLE_CONCEALMENT hConcealment);


/*
 * Inserts one line of invocation switches in the main program.
 */
void ConcealmentHelp(FILE *helpStream);


/*
 * Evaluate errorprotection class integrity before starting any concealment and before
 * decoding align bits.
 */
void ConcealmentCheckClassBufferFullnessEP(int                            decoded_bits,
                                           const HANDLE_RESILIENCE        hResilience,
                                           const HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                           HANDLE_CONCEALMENT             hConcealment);


/*
 * Obtain the prematurely evaluated error information.
 */
HANDLE_ESC_INSTANCE_DATA ConcealmentGetEPprematurely(HANDLE_CONCEALMENT hConcealment);


/*
 * Consistency check of the real length of the decoded spectral data with
 * the length of the spectral data encoded as bitstream element. Error variables
 * are set internally for further use.
 */
void ConcealmentCheckLenOfHcrSpecData(const HANDLE_HCR   hHcrInfo,
                                      HANDLE_CONCEALMENT hConcealment);


/*
 * Consistency check of section huffman codebook. In the case of having audio mono
 * and the codebook is 14 or 15 the codebook is set/concealed to zero.
 */
void ConcealmentDetectErrorCodebook(byte               *codebook,
                                    HANDLE_CONCEALMENT hConcealment);


/*
 * Set typed error variable over all VM internal channels.
 */
void ConcealmentDetectErrorSectionData(const SECTIONDATA_ERRORTYPE sectionDataError,
                                       HANDLE_CONCEALMENT    hConcealment);


/*
 * Mark really used huffman spectral lines.
 * Note: uses internal spectral line counter and internal audio channel.
 */
void ConcealmentTouchLine(int                step,
                          HANDLE_CONCEALMENT hConcealment);


/*
 * Mark huffman spectral line as too long if decoding length exceeds maximum codeword length.
 * Mark huffman spectral line that it error status has been set.
 * Note: uses internal spectral line counter and internal audio channel.
 */
void ConcealmentDetectErrorCodeword(unsigned short          step,
                                    unsigned short          codewordLen,
                                    unsigned short          maxCWLen,
                                    int                     lavInclEsc,
                                    unsigned short          codebook,
                                    int                     *codewordValue,
                                    CODEWORD_TYPE           codewordType,
                                    const HANDLE_RESILIENCE hResilience,
                                    const HANDLE_HCR        hHcrInfo,
                                    HANDLE_CONCEALMENT      hConcealment);


/*
 * Mark huffman spectral line as too long unconditionally.
 * Mark huffman spectral line that it error status has been set.
 * Note: uses internal spectral line counter and internal audio channel.
 */
void ConcealmentSetErrorCodeword(unsigned short     step,
                                 HANDLE_CONCEALMENT hConcealment);


/*
 * Preconcealment of faulty huffman spectral lines representing TNS prediction error.
 * Note: channel is the VM internal channel.
 */
void ConcealmentTemporalNoiseShaping(Float              **coefValue,
                                     Info               *ip,
                                     short              sbk,
                                     short              channel,
                                     HANDLE_CONCEALMENT hConcealment);


/*
 * Preconcealment of faulty huffman spectral lines representing the MS stereo sum or difference.
 * Note: left, right are the VM internal channels.
 */
void ConcealmentMiddleSideStereo(Info               *ip,
                                 byte               *group,
                                 byte               *mask,
                                 Float              **coefValue,
                                 int                left,
                                 int                right,
                                 HANDLE_CONCEALMENT hConcealment);


/*
 * Preconcealment of faulty huffman spectral lines representing the standard prediction error.
 * Note: uses internal audio channel.
 */
void Concealment0SetOfErrCWCoeff(const char         *coefficientDescription,
                                 Float              *coefValue,
                                 int                lineTotal,
                                 int                resetError,
                                 HANDLE_CONCEALMENT hConcealment);


/*
 * Over all main call.
 * The primary intension is to replace spectral lines (delayed) by concealed ones.
 * Note: different concealment techniques have different delays between input and output.
 * This is the reason for using different parameters for window sequence/shape.
 */
void ConcealmentMainEntry(const MC_Info           *mip,
                          const WINDOW_SEQUENCE   *wnd,
                          const Wnd_Shape         *wnd_shape,
                          WINDOW_SEQUENCE         *windowSequence,
                          WINDOW_SHAPE            *window_shape,
                          int                     **lpflag,
                          byte                    **group,
                          byte                    *hasmask,
                          byte                    **mask,
                          Float                   **coefValue,
                          const HANDLE_RESILIENCE hResilience,
                          HANDLE_CONCEALMENT      hConcealment);


/*
 * Deinterleaving between error detection and concealment.
 * The error detection is performed on interleaved huffman spectral lines.
 * The concealment on spectral lines works on deinterleaved lines.
 */
void ConcealmentDeinterleave(const Info*              info,
                             const HANDLE_RESILIENCE  hResilience,
                             HANDLE_CONCEALMENT       hConcealment);


/*
 * Return whether concealment is active.
 * Note: channel is the VM internal channel.
 */
unsigned char ConcealmentAvailable( int                channel,
                                    HANDLE_CONCEALMENT hConcealment );


/* #define SENSITIVITY */ /* debugging only !*/

#ifdef SENSITIVITY

void ResilienceOpenSpectralValuesFile(char*              audioFileName,
                            HANDLE_CONCEALMENT hConcealment);

void ResilienceDealWithSpectralValues(int*               quant,
                            HANDLE_CONCEALMENT hConcealment,
                            unsigned long      bno);
#endif /*SENSITIVITY*/

#endif /* _concealment_h_ */

