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

#include "int_mdct_defs.h"
#include "inv_int_mdct.h"
#include "int_win_dctiv.h"



static void swapInt(int* x, int* y)
{
  int dummy;

  dummy = *x;
  *x = *y;
  *y = dummy;

}


/* Inverse IntMDCT:
   Processed buffer has length N+N/2.
   The first N/2 values contain TDA+windowed signal from last frame.
   The next N signal contain N MDCT lines. These N lines are inverse DCT-IV
   transformed, minused and mirrored.
   Inverse TDA+windowing is done on the left N samples.
   The right N/2 should be stored for next frame.


   < N/2 TDA+w > <     N MDCT lines        >

                              |
                          inv DCT-IV
                   minus, mirror
                        |

   < N/2 TDA+w > < N/2 TDA+w > < N/2 TDA+w >

                |
      inv TDA+w
          |

   <      N time samples     > < N/2 TDA+w >

*/
void InvIntMDCT(int *p_in,
		int *p_overlap,
		int *p_out,
		byte blockType,
		byte windowShape,
		int osf)
{
  int i;
  int j;
  int N;
  int transFac = 8;
  int NShort;
  int startOnesLength;
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

  /* Inverse DCT-IV */
  switch(blockType){
  case ONLY_LONG_WINDOW:
  case LONG_START_WINDOW:
  case LONG_STOP_WINDOW:
    /* perform inverse Integer DCT-IV on right N samples */
    invIntDCTIV(&signal[N/2],
		&signal[N],
		N,
		1);

    /* revert order of right N samples */
    for (i=0; i<N/2; i++) {
      swapInt(&signal[N/2+i],&signal[N/2+N-1-i]);
    }
    for (i=0; i<N; i++) {
      signal[N/2+i] *= -1;
    }
    break;
  case EIGHT_SHORT_WINDOW:
    for (j=0; j<transFac; j++) {
      /* perform inverse Integer DCT-IV on right N samples */
      invIntDCTIV(&signal[N/2+j*NShort],
		  &signal[N/2+j*NShort+NShort/2],
		  NShort,
		  1);
      /* revert order of right N samples */
      for (i=0; i<NShort/2; i++) {
        swapInt(&signal[N/2+j*NShort+i],&signal[N/2+j*NShort+NShort-1-i]);
      }
      for (i=0; i<NShort; i++) {
        signal[N/2+j*NShort+i] *= -1;
      }
    }
    break;
  default:
    printf("blockType not implemented!\n");
    break;
  }

  /* perform inverse TDA and windowing (sine window) of left N samples
     based on Integer Rotations */
  switch(blockType){
  case ONLY_LONG_WINDOW:
  case LONG_START_WINDOW:
    windowingTDA(signal,
		 N,
		 windowShape,
		 -1);
    break;
  case LONG_STOP_WINDOW:
    windowingTDA(&signal[startOnesLength],
		 NShort,
		 windowShape,
		 -1);
    break;
  case EIGHT_SHORT_WINDOW:
    for (j=0; j<transFac; j++) {
      windowingTDA(&signal[startOnesLength+j*NShort],
		   NShort,
		   windowShape,
		   -1);
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



void InvStereoIntMDCT(int* p_in_0,
		      int* p_in_1,
		      int* p_overlap_0,
		      int* p_overlap_1,
		      int* p_out_0,
		      int* p_out_1,
		      byte blockType,
		      byte windowShape,
		      int osf)
{
  int i;
  int j;
  int N;
  int transFac = 8;
  int NShort;
  int startOnesLength;
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

  /* Inverse DCT-IV */
  switch(blockType){
  case ONLY_LONG_WINDOW:
  case LONG_START_WINDOW:
  case LONG_STOP_WINDOW:
    /* perform inverse Integer DCT-IV on right N samples */
    invIntDCTIV(&signal[0][N/2],
		&signal[1][N/2],
		2*N,
		0);
    break;
  case EIGHT_SHORT_WINDOW:
    for (j=0; j<transFac; j++) {
      /* perform inverse Integer DCT-IV on right N samples */
      invIntDCTIV(&signal[0][N/2+j*NShort],
		  &signal[1][N/2+j*NShort],
		  2*NShort,
		  0);
    }
    break;
  default:
    printf("blockType not implemented!\n");
    break;
  }

  for (ch=0; ch<2; ch++) {
    /* Inverse DCT-IV */
    switch(blockType){
    case ONLY_LONG_WINDOW:
    case LONG_START_WINDOW:
    case LONG_STOP_WINDOW:

      /* revert order of right N samples */
      for (i=0; i<N/2; i++) {
        swapInt(&signal[ch][N/2+i],&signal[ch][N/2+N-1-i]);
      }
      for (i=0; i<N; i++) {
        signal[ch][N/2+i] *= -1;
      }
      break;
    case EIGHT_SHORT_WINDOW:
      for (j=0; j<transFac; j++) {
        /* revert order of right N samples */
        for (i=0; i<NShort/2; i++) {
          swapInt(&signal[ch][N/2+j*NShort+i],&signal[ch][N/2+j*NShort+NShort-1-i]);
        }
        for (i=0; i<NShort; i++) {
          signal[ch][N/2+j*NShort+i] *= -1;
        }
      }
      break;
    default:
      printf("blockType not implemented!\n");
      break;
    }
  }

  for (ch=0; ch<2; ch++) {
    /* perform inverse TDA and windowing (sine window) of left N samples
       based on Integer Rotations */
    switch(blockType){
    case ONLY_LONG_WINDOW:
    case LONG_START_WINDOW:
      windowingTDA(signal[ch],
		   N,
		   windowShape,
		   -1);
      break;
    case LONG_STOP_WINDOW:
      windowingTDA(&signal[ch][startOnesLength],
		   NShort,
		   windowShape,
		   -1);
      break;
    case EIGHT_SHORT_WINDOW:
      for (j=0; j<transFac; j++) {
        windowingTDA(&signal[ch][startOnesLength+j*NShort],
		     NShort,
		     windowShape,
		     -1);
      }
      break;
    default:
      printf("blockType not implemented!\n");
      break;
    }
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

