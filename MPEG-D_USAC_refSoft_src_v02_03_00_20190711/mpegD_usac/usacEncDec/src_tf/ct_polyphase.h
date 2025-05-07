/****************************************************************************
 
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Coding Technologies

and edited by
  -

in the course of development of the ISO/IEC 13818-7 and ISO/IEC 14496-3 
standards for reference purposes and its performance may not have been 
optimized. This software module is an implementation of one or more tools as 
specified by the ISO/IEC 13818-7 and ISO/IEC 14496-3 standards.
ISO/IEC gives users free license to this software module or modifications 
thereof for use in products claiming conformance to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International 
Standards. ISO/IEC gives users the same free license to this software module or 
modifications thereof for research purposes and further ISO/IEC standardisation.
Those intending to use this software module in products are advised that its 
use may infringe existing patents. ISO/IEC have no liability for use of this 
software module or modifications thereof. Copyright is not released for 
products that do not conform to audiovisual and image-coding related ITU 
Recommendations and/or ISO/IEC International Standards.
The original developer retains full right to modify and use the code for its 
own purpose, assign or donate the code to a third party and to inhibit third 
parties from using the code for products that do not conform to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International Standards.
This copyright notice must be included in all copies or derivative works.
Copyright (c) ISO/IEC 2002.

 $Id: ct_polyphase.h,v 1.3.18.1 2012-04-19 09:15:33 frd Exp $

*******************************************************************************/
/*!
  \file
  \brief  Polyphase Filterbank $Revision: 1.3.18.1 $
*/

#ifndef __MPEG_BWE_FILTERBANK
#define __MPEG_BWE_FILTERBANK

void
#ifdef AAC_ELD
InitSbrAnaFilterbank(int ldsbr, int numBands);
#else
InitSbrAnaFilterbank(int numBands);
#endif

struct M2 {
  double real[64][128];
  double imag[64][128];
};

struct N {
  double real[128][64];
  double imag[128][64];
};

void
CalculateSbrAnaFilterbank( float * timeSig,
                           float * Sr,
                           float * Si,
                           int channel,
                           int el,
                           int maxBand,
                           int numBands
#ifdef AAC_ELD
			   ,int ldsbr
#endif			   
			   );

void
InitSbrSynFilterbank(int bDownSampleSBR
#ifdef AAC_ELD
		    ,int ldsbr
#endif			
			); 

void
CalculateSbrSynFilterbank( float * Sr,
                           float * Si,
                           float * timeSig,
                           int bDownSampledSbr,
                           int channel,
                           int el
#ifdef AAC_ELD
			   ,int ldsbr
#endif			   
			   );

void
CalculateSbrAnaFilterbank2( float * timeSig,
                           float * Sr,
                           float * Si,
                           int channel,
                           int maxBand
#ifdef AAC_ELD
			   ,int ldsbr
#endif
                            );

double *interpolPrototype(const double *origProt,
                          int   No,
                          int   Lo,
                          int   Li,
                          double phase);

void
CQMFanalysis( const float  *timeSig,
              float  *Sr,
              float  *Si,
              int     L, /* Filter bank size */
              double       *buffer,
              struct M2    *MM,
              const double *C );

void
RQMFsynthesis( const float  *Sr,
               float  *timeSig,
               int     L,
               double       *buffer,
               struct N     *NN,
               const double *C );

#endif
