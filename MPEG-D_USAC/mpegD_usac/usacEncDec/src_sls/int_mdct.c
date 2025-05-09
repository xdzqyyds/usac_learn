/*
This software module was originally developed by 
Ralf Geiger (Fraunhofer IIS AEMT) 
in the course of development of the MPEG-4 extension 3 
audio scalable lossless (SLS) coding . This software module is an 
implementation of a part of one or more MPEG-4 Audio tools as 
specified by the MPEG-4 Audio standard. ISO/IEC  gives users of the
MPEG-4 Audio standards free license to this software module
or modifications thereof for use in hardware or software products
claiming conformance to the MPEG-4 Audio  standards. Those
intending to use this software module in hardware or software products
are advised that this use may infringe existing patents. The original
developer of this software module, the subsequent
editors and their companies, and ISO/IEC have no liability for use of
this software module or modifications thereof in an
implementation. Copyright is not released for non MPEG-4
Audio conforming products. The original developer retains full right to
use the code for the developer's own purpose, assign or donate the code to a
third party and to inhibit third party from using the code for non
MPEG-4 Audio conforming products. This copyright notice
must be included in all copies or derivative works. 

Copyright 2003.  
*/


#include <stdio.h>

#include "int_mdct.h"

static void swapInt(int* x, int* y)
{
  int dummy;

  dummy = *x;
  *x = *y;
  *y = dummy;
}


/* IntMDCT:
   Processed buffer has length N+N/2. 
   The first N/2 values contain TDA+windowed signal from last frame.
   The next N signal contain new frame of time domain input signal.
   TDA+windowing is done on this length N buffer. The right N/2 should be 
   stored for next frame.
   DCT-IV is performed on left N TDA+w samples.
   So MDCT output is stored in the left N samples.

   
   < N/2 TDA+w > <     N time samples      >

                              | 
                            TDA+w
                              |

   < N/2 TDA+w > < N/2 TDA+w > < N/2 TDA+w >

                |
       minus mirrored DCT-IV
                |

   <      N MDCT lines       > < N/2 TDA+w >

*/
void IntMDCT(int* p_in,
	     int* p_overlap,
	     int* p_out,
	     int blockType,
	     int windowShape,
	     int osf)
{
  int i;
  int j;
  int N;
  int transFac = 8;
  int NShort;
  int startOnesLength; /* number of ones in start window */
  int signal[MAX_OSF*INTMDCT_BUFFER_SIZE];

  N = 1024*osf;
  NShort = N/transFac;
  startOnesLength = N/2 - NShort/2;

  /* copy buffers */
  for (i=0; i<N/2; i++) {
    signal[i]=p_overlap[i];
  }
  for (i=0; i<N; i++) {
    signal[N/2+i]=p_in[i];
  }
  
  /* windowing and TDA */
  switch(blockType){
  case ONLY_LONG_WINDOW:
  case LONG_STOP_WINDOW:
    /* perform TDA and windowing of right N samples 
       based on Integer Rotations */ 
    windowingTDA(&signal[N/2],
		 N,
		 windowShape,
		 1);
    break;
  case LONG_START_WINDOW:
    windowingTDA(&signal[N/2+startOnesLength],
		 NShort,
		 windowShape,
		 1);
    break;
  case EIGHT_SHORT_WINDOW:
    for (j=0; j<transFac; j++) {
      windowingTDA(&signal[NShort/2+j*NShort],
		   NShort,
		   windowShape,
		   1);
    }
    break;
  default:
    printf("blockType not implemented!\n");
    break;
  }
    
  /* DCT-IV */
  switch(blockType){
  case ONLY_LONG_WINDOW:
  case LONG_START_WINDOW:
  case LONG_STOP_WINDOW:
  
    /* revert order of N samples to process */
    for (i=0; i<N/2; i++) {
      swapInt(&signal[i],&signal[N-1-i]);
    }
    
    for (i=0; i<N; i++) {
      signal[i] *= -1;
    }

    /* perform Integer DCT-IV on left N samples */
    intDCTIV(signal,
	     &signal[N/2],
	     N,
	     1);
    break;
  case EIGHT_SHORT_WINDOW:
    for (j=0; j<transFac; j++) {
      /* revert order of N samples to process */
      for (i=0; i<NShort/2; i++) {
	swapInt(&signal[j*NShort+i],&signal[j*NShort+NShort-1-i]);
      }
      for (i=0; i<NShort; i++) {
	signal[j*NShort+i] *= -1;
      }

      intDCTIV(&signal[j*NShort],
	       &signal[j*NShort+NShort/2],
	       NShort,
	       1);
    }
    break;  
  default:
    printf("blockType not implemented!\n");
    break;
  }

  /* copy buffers */
  for (i=0; i<N; i++) {
    p_out[i]=signal[i];
  }
  for (i=0; i<N/2; i++) {
    p_overlap[i]=signal[N+i];
  }

}


void StereoIntMDCT(int* p_in_0,
		   int* p_in_1,
		   int* p_overlap_0,
		   int* p_overlap_1,
		   int* p_out_0,
		   int* p_out_1,
		   int blockType,
		   int windowShape,
		   int osf)
{
  int i;
  int j;
  int N;
  int transFac = 8;
  int NShort;
  int startOnesLength; /* number of ones in start window */
  int signal[2][MAX_OSF*INTMDCT_BUFFER_SIZE];
  int ch;

  N = 1024*osf;
  NShort = N/transFac;
  startOnesLength = N/2 - NShort/2;

  /* copy buffers */
  for (i=0; i<N/2; i++) {
    signal[0][i]=p_overlap_0[i];
    signal[1][i]=p_overlap_1[i];
  } 
  for (i=0; i<N; i++) {
    signal[0][N/2+i]=p_in_0[i];
    signal[1][N/2+i]=p_in_1[i];
  }

  for (ch=0; ch<2; ch++) {
    /* windowing and TDA */
    switch(blockType){
    case ONLY_LONG_WINDOW:
    case LONG_STOP_WINDOW:
      /* perform TDA and windowing of right N samples 
	 based on Integer Rotations */ 
      windowingTDA(&signal[ch][N/2],
		   N,
		   windowShape,
		   1);
      break;
    case LONG_START_WINDOW:
      windowingTDA(&signal[ch][N/2+startOnesLength],
		   NShort,
		   windowShape,
		   1);
      break;
    case EIGHT_SHORT_WINDOW:
      for (j=0; j<transFac; j++) {
	windowingTDA(&signal[ch][NShort/2+j*NShort],
		     NShort,
		     windowShape,
		     1);
      }
      break;
    default:
      printf("blockType not implemented!\n");
      break;
    }
  }

  for (ch=0; ch<2; ch++) {
    /* DCT-IV */
    switch(blockType){
    case ONLY_LONG_WINDOW:
    case LONG_START_WINDOW:
    case LONG_STOP_WINDOW:
  
      /* revert order of N samples to process */
      for (i=0; i<N/2; i++) {
	swapInt(&signal[ch][i],&signal[ch][N-1-i]);
      }
    
      for (i=0; i<N; i++) {
	signal[ch][i] *= -1;
      }
      break;
    case EIGHT_SHORT_WINDOW:
      for (j=0; j<transFac; j++) {
	/* revert order of N samples to process */
	for (i=0; i<NShort/2; i++) {
	  swapInt(&signal[ch][j*NShort+i],&signal[ch][j*NShort+NShort-1-i]);
	}
	for (i=0; i<NShort; i++) {
	  signal[ch][j*NShort+i] *= -1;
	}
      }
      break;  
    default:
      printf("blockType not implemented!\n");
      break;
    }
  }

  /* DCT-IV */
  switch(blockType){
  case ONLY_LONG_WINDOW:
  case LONG_START_WINDOW:
  case LONG_STOP_WINDOW:
    
    /* perform Integer DCT-IV on left N samples */
    intDCTIV(signal[0],
	     signal[1],
	     2*N,
	     0);
    break;
  case EIGHT_SHORT_WINDOW:
    for (j=0; j<transFac; j++) {
      intDCTIV(&signal[0][j*NShort],
	       &signal[1][j*NShort],
	       2*NShort,
	       0);
    }
    break;  
  default:
    printf("blockType not implemented!\n");
    break;
  }

  /* copy buffers */
  for (i=0; i<N; i++) {
    p_out_0[i]=signal[0][i];
    p_out_1[i]=signal[1][i];
  }
  for (i=0; i<N/2; i++) {
    p_overlap_0[i]=signal[0][N+i];
    p_overlap_1[i]=signal[1][N+i];
  }
}









