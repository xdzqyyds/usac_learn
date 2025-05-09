/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:J. Macpherson

and edited by: Jeremie lecomte (Fraunhofer IIS)

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Assurance that the originally developed software module can be used (1) in
ISO/IEC 23003 once ISO/IEC 23003 has been adopted; and (2) to develop ISO/IEC
23003:s
Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, VoiceAge Corp.
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, VoiceAge Corp.
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, VoiceAge Corp. retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>   

#include "vector_ops.h"
#include "int_dec.h"
#include "cnst.h"
#include "block.h"
#include "tf_main.h"
#include "ms.h"
#include "tns3.h"

#include "tf_mainHandle.h"
#include "proto_func.h"
#include "usac_mdct.h"


#define TRANS_FAC 8

extern const double dolby_win_1024[1024]; /* symbol already in imdct.o */
extern const double dolby_win_768[768]; /* symbol already in imdct.o */
extern const double dolby_win_128[128]; /* symbol already in imdct.o */
extern const double dolby_win_96[96]; /* symbol already in imdct.o */

static int nmax=0;
static float *cosTab=NULL;
static const double zero = 0;
static void __window(HANDLE_TFDEC hTfDec, WINDOW_SEQUENCE windowSequence,int windowShape, int prevWindowShape, int useACENext, int useACEPrev, double *data);

#ifdef INTDEC_WAVEOUT
#include "audio.h"
#endif


typedef struct T_TFDEC {

  double overlap_buffer[MAX_TIME_CHANNELS][1024];

#ifdef INTDEC_WAVEOUT
  AudioFile  *hFile; /* audio writeout */
#endif

  int nfSeed;
  int nGranuleLength;
  double *pLongWindowSine;
  double *pShortWindowSine;
  double *pShortWindowSineLpdStart;
  double *pLongStartWindowSine;
  double *pLongStopWindowSine;
  double *pLongWindowKBD;
  double *pShortWindowKBD;
  double *pShortWindowKBDLpdStart;

} TFDEC;



/*****************************************************************************

    function name: CreateIntDec
    description:  Creates the internal decoder handle

    returns:      void
    input:        Position of the Internal Decoder Handle in the main Hanlde 
                  structure
 *****************************************************************************/

void CreateIntDec(HANDLE_TFDEC *hTfDEC, unsigned int nChannels, unsigned int nSamplingRate, unsigned int nGranuleLength)
{

  int i;
  int error = 0;
  double  Window_long[1024];
  double  Window_short[128];
  double  Window_medium_fhg[256];
  double  Window_medium_dolby[256];

  HANDLE_TFDEC hdecoder;

  hdecoder = (HANDLE_TFDEC) calloc(1,sizeof(TFDEC));

  *hTfDEC = hdecoder; 

  for(i=0;i<MAX_TIME_CHANNELS;i++) {
    {
      unsigned int ii;
      for(ii=0; ii<nGranuleLength; ii++)
        (*hTfDEC)->overlap_buffer[i][ii] = 0.0;
    }
  }

#ifdef INTDEC_WAVEOUT
  {
    char outfilename[]="tfDec.wav";
    char audioFileFormat[]="wav";

    (*hTfDEC)->hFile=AudioOpenWrite(outfilename,
                               audioFileFormat,
                               1,
                               25600,
                               0);
  }
#endif /* #ifdef INTDEC_WAVEOUT */

  (*hTfDEC)->nGranuleLength = nGranuleLength;
  if(error == 0){
    if(NULL == ((*hTfDEC)->pShortWindowSineLpdStart = (double *) calloc((*hTfDEC)->nGranuleLength/(TRANS_FAC/2), sizeof(double)))){
      error = 1;
    }
  }

  if(error == 0){
    if(NULL == ((*hTfDEC)->pShortWindowKBDLpdStart = (double *) calloc((*hTfDEC)->nGranuleLength/(TRANS_FAC/2), sizeof(double)))){
      error = 1;
    }
  }

  if(error == 0){
      if(NULL == ((*hTfDEC)->pLongWindowSine = (double *) calloc((*hTfDEC)->nGranuleLength, sizeof(double)))){
        error = 1;
      }
    }
    
  if(error == 0){
      if(NULL == ((*hTfDEC)->pShortWindowSine = (double *) calloc((*hTfDEC)->nGranuleLength/(TRANS_FAC/2), sizeof(double)))){
        error = 1;
      }
    }

  if(error == 0){
      if(NULL == ((*hTfDEC)->pLongWindowKBD = (double *) calloc((*hTfDEC)->nGranuleLength, sizeof(double)))){
        error = 1;
      }
    }
    
  if(error == 0){
      if(NULL == ((*hTfDEC)->pShortWindowKBD = (double *) calloc((*hTfDEC)->nGranuleLength/(TRANS_FAC/2), sizeof(double)))){
        error = 1;
      }
    }


  if(error == 0){
    calc_window_db( Window_long,      	 nGranuleLength,   WS_FHG );
    calc_window_db( Window_short,     	 nGranuleLength/8, WS_FHG );
    calc_window_db( Window_medium_fhg,   nGranuleLength/4, WS_FHG );
    calc_window_db( Window_medium_dolby, nGranuleLength/4, WS_DOLBY );

    switch(nGranuleLength){

    case 1024:
      vcopy_db(Window_medium_fhg,   &(*hTfDEC)->pShortWindowSineLpdStart[0],1,1, (*hTfDEC)->nGranuleLength/(TRANS_FAC/2));
      vcopy_db(Window_medium_dolby, &(*hTfDEC)->pShortWindowKBDLpdStart[0], 1,1, (*hTfDEC)->nGranuleLength/(TRANS_FAC/2));
      vcopy_db(Window_long, 	    &(*hTfDEC)->pLongWindowSine[0],	    1,1, (*hTfDEC)->nGranuleLength);
      vcopy_db(Window_short, 	    &(*hTfDEC)->pShortWindowSine[0],	    1,1, (*hTfDEC)->nGranuleLength/(TRANS_FAC));
      vcopy_db(dolby_win_1024, 	    &(*hTfDEC)->pLongWindowKBD[0],	    1,1, (*hTfDEC)->nGranuleLength);
      vcopy_db(dolby_win_128, 	    &(*hTfDEC)->pShortWindowKBD[0],	    1,1, (*hTfDEC)->nGranuleLength/(TRANS_FAC));
      break;

    case 768:
      vcopy_db(Window_medium_fhg,   &(*hTfDEC)->pShortWindowSineLpdStart[0],1,1, (*hTfDEC)->nGranuleLength/(TRANS_FAC/2));
      vcopy_db(Window_medium_dolby, &(*hTfDEC)->pShortWindowKBDLpdStart[0], 1,1, (*hTfDEC)->nGranuleLength/(TRANS_FAC/2));
      vcopy_db(Window_long, 	    &(*hTfDEC)->pLongWindowSine[0],	    1,1, (*hTfDEC)->nGranuleLength);
      vcopy_db(Window_short, 	    &(*hTfDEC)->pShortWindowSine[0],	    1,1, (*hTfDEC)->nGranuleLength/(TRANS_FAC));
      vcopy_db(dolby_win_768, 	    &(*hTfDEC)->pLongWindowKBD[0],	    1,1, (*hTfDEC)->nGranuleLength);
      vcopy_db(dolby_win_96, 	    &(*hTfDEC)->pShortWindowKBD[0],	    1,1, (*hTfDEC)->nGranuleLength/(TRANS_FAC));
      break;

    default:
      error = 1;
      break;
    }
  }

  return;
}

/*****************************************************************************

    function name: DeleteIntDec
    description:  Deletes the internal decoder handle

    returns:      error info
    input:        The Internal Decoder Handle
 *****************************************************************************/

int DeleteIntDec(HANDLE_TFDEC *phtfDec){

#ifdef INTDEC_WAVEOUT
  if ((*phtfDec)->hFile) {
    AudioClose((*phtfDec)->hFile);
    (*phtfDec)->hFile=NULL;
  }
#endif /* #ifdef INTDEC_WAVEOUT */    

  if(phtfDec){
    if(*phtfDec){
      if((*phtfDec)->pShortWindowSineLpdStart){
        free((*phtfDec)->pShortWindowSineLpdStart);
      }
      if((*phtfDec)->pShortWindowKBDLpdStart){
        free((*phtfDec)->pShortWindowKBDLpdStart);
      }
      if((*phtfDec)->pLongWindowSine){
        free((*phtfDec)->pLongWindowSine);
      }
      if((*phtfDec)->pShortWindowSine){
        free((*phtfDec)->pShortWindowSine);
      }      
      if((*phtfDec)->pLongWindowKBD){
        free((*phtfDec)->pLongWindowKBD);
      }
      if((*phtfDec)->pShortWindowKBD){
        free((*phtfDec)->pShortWindowKBD);
      }  

      free(*phtfDec);
      *phtfDec=NULL;
    }
  }

  return(0);
}


/*****************************************************************************

    function name: AdvanceIntDec
    description:  Reconstructs the original time signal from the quantised
                  signal, for use for the long term prediction module.

    returns:      void
    input:        

*****************************************************************************/
void AdvanceIntDecUSAC(HANDLE_TFDEC           htfDec,
                       double                 *reconstructed_spectrum[MAX_TIME_CHANNELS],
                       WINDOW_SEQUENCE        windowSequence[MAX_TIME_CHANNELS],
                       WINDOW_SHAPE 		  windowShape[MAX_TIME_CHANNELS],
                       WINDOW_SHAPE 		  prevWindowShape[MAX_TIME_CHANNELS],
                       int                    nr_of_sfb[MAX_TIME_CHANNELS],
                       int         		      max_sfb[MAX_TIME_CHANNELS],
                       int                    block_size_samples[MAX_TIME_CHANNELS],
                       int                    sfb_offset[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS+1],
                       MSInfo                 *msInfo,
                       TNS_INFO               *tnsInfo[MAX_TIME_CHANNELS],
                       unsigned int           nr_of_chan,
                       int                    common_window,
                       USAC_CORE_MODE         coreMode[MAX_TIME_CHANNELS],
                       USAC_CORE_MODE         prev_coreMode[MAX_TIME_CHANNELS],
                       USAC_CORE_MODE         next_coreMode[MAX_TIME_CHANNELS],
                       double                 reconstructed_time_samples[MAX_TIME_CHANNELS][4096])

{

  unsigned int i_ch;
  unsigned int k;
  int nlong = block_size_samples[MONO_CHAN];
  int nshort = block_size_samples[MONO_CHAN]/NSHORT;
  int nflat_ls    = (nlong-nshort)/ 2;
  int transfak_ls =  nlong/nshort;
  unsigned int num_short_win = NSHORT;


  if (nr_of_chan== 2 &&
      coreMode[0] == CORE_MODE_FD && coreMode[1] == CORE_MODE_FD &&
      common_window == 1) {


    MSInverse(nr_of_sfb[MONO_CHAN],
              sfb_offset[MONO_CHAN],
              msInfo->ms_used,
              (windowSequence[MONO_CHAN]==EIGHT_SHORT_SEQUENCE)?NSHORT:1,
              block_size_samples[MONO_CHAN],
              reconstructed_spectrum[0],
              reconstructed_spectrum[1]);
  }

  for(i_ch=0;i_ch<nr_of_chan;i_ch++){
    if ( coreMode[i_ch] == CORE_MODE_FD ) {
      TnsEncode2(nr_of_sfb[i_ch],                /* Number of bands per window */
                 max_sfb[i_ch],                /* max_sfb */
                 windowSequence[i_ch],           /* block type */
                 sfb_offset[i_ch],               /* Scalefactor band offset table */
                 reconstructed_spectrum[i_ch], /* Spectral data array */
                 tnsInfo[i_ch],
                 1);


      /* Inverse IMDCT */
      vcopy_db(&zero,reconstructed_time_samples[i_ch],0,1,2*nlong);
      if(windowSequence[i_ch] == EIGHT_SHORT_SEQUENCE){
        double transf_buf[256];
        double *fp = reconstructed_time_samples[i_ch] + nflat_ls;
        double *p_in_data = reconstructed_spectrum[i_ch];
        for( k = 0; k<num_short_win;k++ ) {
          imdct( p_in_data, transf_buf, 2*nshort );
          if (k==0){
            __window(htfDec,
                     windowSequence[i_ch],
                     windowShape[i_ch],
                     prevWindowShape[i_ch],
                     (next_coreMode[i_ch] == CORE_MODE_TD),
                     (prev_coreMode[i_ch] == CORE_MODE_TD),
                     transf_buf
            );
          }
          else {
            __window(htfDec,
                     windowSequence[i_ch],
                     windowShape[i_ch],
                     windowShape[i_ch],
                     (next_coreMode[i_ch] == CORE_MODE_TD),
                     (prev_coreMode[i_ch] == CORE_MODE_TD),
                     transf_buf
            );
          }
          vadd_db( transf_buf, fp, fp, 1, 1, 1, nshort );
          vadd_db( transf_buf+nshort, fp+nshort, fp+nshort, 1, 1, 1, nshort );
          p_in_data += nshort;
          fp        += nshort;
        }

      }
      else {
        imdct( reconstructed_spectrum[i_ch], reconstructed_time_samples[i_ch], 2*nlong );
        __window(htfDec,
                 windowSequence[i_ch],
                 windowShape[i_ch],
                 prevWindowShape[i_ch],
                 (next_coreMode[i_ch] == CORE_MODE_TD),
                 (prev_coreMode[i_ch] == CORE_MODE_TD),
                 reconstructed_time_samples[i_ch]
        );

      }

    }
  }
#ifdef INTDEC_WAVEOUT
  for(i_ch=0;i_ch<nr_of_chan;i_ch++){
    if ( coreMode[i_ch] == CORE_MODE_FD ) {
 	 if (htfDec->hFile) {
	   int i;
	   float* outSamples[MAX_TIME_CHANNELS];
	   for(i_ch=0;i_ch<nr_of_chan;i_ch++){
		  for(i=0;i<4096;i++){
			  outSamples[i_ch][i] =reconstructed_time_samples[i_ch][i];
		  }
		  /**outSamples[i_ch]=(float)reconstructed_time_samples[i_ch][0];*/
	  }

   	AudioWriteDataTruncat (
                     htfDec->hFile,           /* in: audio file (handle) */
                     outSamples,                      /* in: data[channel][sample] */
                     /*     (range [-32768 .. 32767]) */
                     block_size_samples[MONO_CHAN]) ;           /* in: number of samples to be written */
       /*     (samples per channel!) */

      }
    }
  }
#endif /* #ifdef INTDEC_WAVEOUT */
}






static void __window(HANDLE_TFDEC hTfDec, WINDOW_SEQUENCE windowSequence,int windowShape, int prevWindowShape, int useACENext, int useACEPrev, double *data) {


  int i;
  const double *leftWindowPart, *rightWindowPart;
  int nGranuleLength = hTfDec->nGranuleLength;
  int lsTrans = (nGranuleLength - (nGranuleLength/TRANS_FAC))/2;
  int lsExt = nGranuleLength/TRANS_FAC;
  int lfac = nGranuleLength/(2*NB_DIV);


  switch ( windowSequence ) {
  case ONLY_LONG_SEQUENCE:
    if(prevWindowShape == WS_FHG){
      leftWindowPart = &hTfDec->pLongWindowSine[0];
    } else { /* default: KBD window */
      leftWindowPart = &hTfDec->pLongWindowKBD[0];
    }

    if(windowShape == WS_FHG){
      rightWindowPart = &hTfDec->pLongWindowSine[0];
    } else { /* default: KBD window */
      rightWindowPart = &hTfDec->pLongWindowKBD[0];
    }

    for(i=0;i<nGranuleLength;i++){
      data[i]                *= leftWindowPart[i];
      data[nGranuleLength+i] *= rightWindowPart[nGranuleLength-i-1];
    }
    break;

  case LONG_START_SEQUENCE:
    if(prevWindowShape == WS_FHG){
      leftWindowPart = &hTfDec->pLongWindowSine[0];
    } else { /* default: KBD window */
      leftWindowPart = &hTfDec->pLongWindowKBD[0];
    }

    if(!useACENext){
      if(windowShape == WS_DOLBY){
        rightWindowPart = &hTfDec->pShortWindowKBD[0];
      } else {
        rightWindowPart = &hTfDec->pShortWindowSine[0];
      }
    } else {
      if(windowShape == WS_DOLBY){
        rightWindowPart = &hTfDec->pShortWindowKBDLpdStart[0];
      } else { /* default: sine window */
        rightWindowPart = &hTfDec->pShortWindowSineLpdStart[0];
      }
    }

    for(i=0;i<nGranuleLength/2;i++){
      data[i] *= leftWindowPart[i];
    }

    if ( useACENext ){
      int foldingPoint = nGranuleLength/2;
      for ( i = 0 ; i < foldingPoint - lfac ; i++ ) {
        data[(2*nGranuleLength-1-i)] = 0.0f;
      }

      for ( i = 0 ; i < 2*lfac ; i++ ) {
        data[(nGranuleLength+foldingPoint - lfac + i)] *=rightWindowPart[2*lfac-i-1];
      }
    }
    else
      {
        for(i=0;i<lsTrans;i++){
          data[(2*nGranuleLength-1-i)]=0.0f;
        }

        for(i=0;i<nGranuleLength/(TRANS_FAC);i++){
          data[(nGranuleLength+i+lsTrans)]*=rightWindowPart[nGranuleLength/TRANS_FAC-i-1];
        }
      }
    break;

  case LONG_STOP_SEQUENCE:
    if ( useACEPrev ) {
      if(prevWindowShape == WS_DOLBY){
        leftWindowPart = &hTfDec->pShortWindowKBDLpdStart[0];
      } else {
        leftWindowPart = &hTfDec->pShortWindowSineLpdStart[0];
      }
    }
    else
      {
        if(prevWindowShape == WS_DOLBY){
          leftWindowPart = &hTfDec->pShortWindowKBD[0];
        } else {
          leftWindowPart = &hTfDec->pShortWindowSine[0];
        }
      }

    if(windowShape == WS_FHG){
      rightWindowPart = &hTfDec->pLongWindowSine[0];
    } else {
      rightWindowPart = &hTfDec->pLongWindowKBD[0];
    }

    if ( useACEPrev ){
      int foldingPoint = nGranuleLength/2;
      for ( i = 0 ; i < foldingPoint - lfac ; i++ ) {
        data[i] = 0.0f;
      }

      for ( i = 0 ; i < 2*lfac ; i++ ) {
        data[(i+foldingPoint-lfac)]*=leftWindowPart[i];
      }
    }
    else
      {
        for(i=0;i<lsTrans;i++){
          data[i] = 0.0f;
        }
        for(i=0;i<nGranuleLength/TRANS_FAC;i++){
          data[(i+lsTrans)]*=leftWindowPart[i];
        }
      }

    for(i=0;i<nGranuleLength;i++){
      data[(nGranuleLength+i)] *= rightWindowPart[nGranuleLength-i-1];
    }

    break;


  case STOP_START_SEQUENCE:
    if ( useACEPrev ) {
      if(prevWindowShape == WS_DOLBY){
        leftWindowPart = &hTfDec->pShortWindowKBDLpdStart[0];
      } else {
        leftWindowPart = &hTfDec->pShortWindowSineLpdStart[0];
      }
    }
    else
      {
        if(prevWindowShape == WS_DOLBY){
          leftWindowPart = &hTfDec->pShortWindowKBD[0];
        } else {
          leftWindowPart = &hTfDec->pShortWindowSine[0];
        }
      }

    if(!useACENext){
      if(windowShape == WS_DOLBY){
        rightWindowPart = &hTfDec->pShortWindowKBD[0];
      } else {
        rightWindowPart = &hTfDec->pShortWindowSine[0];
      }
    } else {
      if(windowShape == WS_DOLBY){
        rightWindowPart = &hTfDec->pShortWindowKBDLpdStart[0];
      } else { /* default: sine window */
        rightWindowPart = &hTfDec->pShortWindowSineLpdStart[0];
      }
    }


    if ( useACEPrev ){
      int foldingPoint = nGranuleLength/2;
      for ( i = 0 ; i < foldingPoint - lfac ; i++ ) {
        data[i] = 0.0f;
      }

      for ( i = 0 ; i < 2*lfac ; i++ ) {
        data[(i+foldingPoint-lfac)]*=leftWindowPart[i];
      }
    }
    else
      {
        for(i=0;i<lsTrans;i++){
          data[i] = 0.0f;
        }
        for(i=0;i<nGranuleLength/TRANS_FAC;i++){
          data[(i+lsTrans)]*=leftWindowPart[i];
        }
      }

    if ( useACENext ){
      int foldingPoint = nGranuleLength/2;
      for ( i = 0 ; i < foldingPoint - lfac ; i++ ) {
        data[(2*nGranuleLength-1-i)] = 0.0f;
      }

      for ( i = 0 ; i < 2*lfac ; i++ ) {
        data[(nGranuleLength+foldingPoint - lfac + i)] *=rightWindowPart[2*lfac-i-1];
      }
    }
    else
      {
        for(i=0;i<lsTrans;i++){
          data[(2*nGranuleLength-1-i)]=0.0f;
        }

        for(i=0;i<nGranuleLength/(TRANS_FAC);i++){
          data[(nGranuleLength+i+lsTrans)]*=rightWindowPart[nGranuleLength/TRANS_FAC-i-1];
        }
      }
    break;

  case EIGHT_SHORT_SEQUENCE:
    if(prevWindowShape == WS_FHG){
      leftWindowPart = &hTfDec->pShortWindowSine[0];
    } else { /* default: KBD window */
      leftWindowPart = &hTfDec->pShortWindowKBD[0];
    }

    if(windowShape == WS_FHG){
      rightWindowPart = &hTfDec->pShortWindowSine[0];
    } else { /* default: KBD window */
      rightWindowPart = &hTfDec->pShortWindowKBD[0];
    }

    for(i=0;i<lsTrans;i++){
      data[i]                *= leftWindowPart[i];
      data[lsTrans+i] *= rightWindowPart[lsTrans-i-1];
    }
    break;

  default:
    break;

  }
}








