
#ifndef CEPSTRUM_ANALYSIS_H
#define CEPSTRUM_ANALYSIS_H

/*

====================================================================
                        Copyright Header            
====================================================================

This software module was originally developed by Ralf Funken
(Philips, Eindhoven, The Netherlands) and edited by Takehiro Moriya 
(NTT Labs. Tokyo, Japan) in the course of development of the
MPEG-2/MPEG-4 Conformance ISO/IEC 13818-4, 14496-4. This software
module is an implementation of one or more of the Audio Conformance
tools as specified by MPEG-2/MPEG-4 Conformance. ISO/IEC gives users
of the MPEG-2 AAC/MPEG-4 Audio (ISO/IEC 13818-3,7, 14496-3) free
license to this software module or modifications thereof for use in
hardware or software products. Those intending to use this software
module in hardware or software products are advised that its use may
infringe existing patents. The original developer of this software
module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Philips retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party. This copyright notice must be included in 
all copies or derivative works. Copyright 2000.


Source file: CEPSTRUM_ANALYSIS.H

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "common.h"

                                             
/*  cepstrum analysis related defines           */
                                                
#define         N_PR                            (16)          /*    LPC order                                       */
#define         BW                              (0.0125F)     /*    Bandwidth scalefactor                           */
#define         COR_FACTOR                      (0.99999)     /*    Correction factor to solve numerical            */


/***************************************************************************/

void lpc2cepstrum (int      p,             /*    in:    LPC order = 16                     */
                   double   lpc_coef[],    /*    in:    LPC coefficients (a-parameters)    */
                   int      N,             /*    in:    LPC cepstrum order = 32            */
                   double   C[]);          /*    out:   LPC cepstrum                       */

void hamwdw (float    wdw[],
             int      n);

void lagwdw (float    wdw[],
             int      n,
             float    h);
   
void calculate_lpc (double      pcm[],              /*    in:    input PCM audio data                */
                    int         segment_length,     /*    in:    analysis frame length in samples    */
                    double*     lpc_coef,           /*    out:   LPC coefficients                    */
                    float       wdw [],             /*    in:    hamming window                      */
                    float       wlag []);           /*    in:    lag window                          */


/***************************************************************************/

#endif   /*  CEPSTRUM_ANALYSIS_H   */

