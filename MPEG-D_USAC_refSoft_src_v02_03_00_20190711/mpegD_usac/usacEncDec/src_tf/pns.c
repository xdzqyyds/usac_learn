/************************* MPEG-4 AAC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of 
development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
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
non MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/
#ifdef PNS

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#ifdef PNS_NONDETERMINISTIC_RANDOM
#include <time.h>
#endif

#include "allHandles.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "allVariables.h"        /* variables */

#include "port.h"

static long
random2( long *seed )
{
  *seed = (1664525L * *seed) + 1013904223L;  /* Numerical recipes */
  return(long)(*seed);
}

#ifndef RAND_MAX
#define RAND_MAX 32767
#endif
void gen_rand_vector( Float *spec, int size, long *state )  
/* Noise generator, generating vector with unity energy */
{
  int i;
  int mode = 3;
  
  switch (mode) {
  case 0:      /* Dummy 0:  zero vector */
    for (i=0; i<size; i++)
      spec[i] = 0.0;
    break;
  case 1:      /* Dummy 1:  zero vector with one spectral component */
    for (i=0; i<size; i++)
      spec[i] = 0.0;
    spec[3] = 1.0;
    break;
  case 2:      /* Dummy 2:  random vector */
    {
      Float scale, nrg=0.0;
      
      for (i=0; i<size; i++) {
        spec[i] = scale = (Float)rand()/RAND_MAX - 0.5;
        nrg += scale * scale;
      }
      scale = 1.0 / sqrt( nrg );
      for (i=0; i<size; i++) {
        spec[i] *= scale;
        /*printf("\nRAND[%d] = %f", i, spec[i]);*/
      }
    }
    break;
  case 3:      /* Mode 3:  real random number generator */
    {
      Float scale, nrg= 0.0;
      
      for (i=0; i<size; i++) {
        spec[i] = (Float)(random2( state ) );
        nrg += spec[i] * spec[i];
      }
      scale = 1.0 / sqrt (nrg);
      for (i=0; i<size; i++) {
        spec[i] *= scale; 
      }
    }
  }
}


/*
 * if (noise correlated) {
 *   restore saved left channel random generator state
 *   generate random values
 * } else {
 *   save current random generator state
 *   generate random values
 * }
 * scale according to scalefactor
 *
 * Important: needs to be called left channel, then right channel
 *            for each channel pair
 */

void pns ( MC_Info* mip, 
           Info*    info, 
           int      ch,
           byte*    group, 
           byte*    cb_map, 
           short*   factors, 
           int*     lpflag, 
           Float*   coef[Chans] )
{
  Ch_Info *cip = &mip->ch_info[ch];
  Float   *spec, *fp, scale;
  int     cb, corr_flag, sfb, n, nn, b, bb, nband;
  const short   *band;
  long    *nsp;

  static long    cur_noise_state = 0;
  static long    noise_state_save[ MAXBANDS ];
  static int     lp_store[ MAXBANDS ];

#ifdef PNS_NONDETERMINISTIC_RANDOM
  static time_t  curtime=(time_t)0;

  if (curtime==(time_t)0) {
    time(&curtime);
    cur_noise_state=(int)curtime;
  }
#endif


  /* store original predictor flags when left channel of a channel pair */
  if ((cip->cpe  && cip->common_window && cip->ch_is_left  &&  info->islong))
    for (sfb=0; sfb<info->sfb_per_sbk[0]; sfb++)
      lp_store[sfb+1] = lpflag[sfb+1];

  /* restore original predictor flags when right channel of a channel pair */
  if ((cip->cpe  && cip->common_window &&  !cip->ch_is_left  &&  info->islong))
    for (sfb=0; sfb<info->sfb_per_sbk[0]; sfb++)
      lpflag[sfb+1] = lp_store[sfb+1];


  spec = coef[ ch ];
  nsp = noise_state_save;

  /* PNS goes by group */
  bb = 0;
  for (b = 0; b < info->nsbk; ) {
    nband = info->sfb_per_sbk[b];
    band = info->sbk_sfb_top[b];

    b = *group++;		/* b = index of last sbk in group */
    for (; bb < b; bb++) {	/* bb = sbk index */
      n = 0;
      for (sfb = 0; sfb < nband; sfb++){
        nn = band[sfb];	/* band is offset table, nn is last coef in band */
        cb = cb_map[sfb];
        if (cb == NOISE_HCB  ||  cb == NOISE_HCB+100) {
          /* found noise  substitution code book */

          /* disable prediction (only important for long blocks) */
          if (info->islong)  lpflag[1+sfb] = 0;

          /* determine left/right correlation */
          corr_flag = (cb != NOISE_HCB);

          /* reconstruct noise substituted values */
          if (debug['P'])
            fprintf(stderr,"applying PNS coding on ch %d at sfb %3d %d\n",
                    ch, sfb, corr_flag);

          /* generate random noise */
          fp = spec + n;
          if (corr_flag)  {
            /* Start with stored state */
            gen_rand_vector( fp, nn-n, nsp+sfb );
          } else {
            /* Store current state and go */
            nsp[sfb] = cur_noise_state;
            gen_rand_vector( fp, nn-n, &cur_noise_state );
          }

          /* scale to target energy */
          scale = pow( 2.0, 0.25*(factors[sfb]) );
          for (; n < nn; n++) {	/* n is coef index */
            *fp++ *= scale;
          }
        }
        n = nn;
      }
      spec += info->bins_per_sbk[bb];
      factors += nband;
      nsp     += nband;
    }
    cb_map += info->sfb_per_sbk[bb-1];
  }
}



/********************************************************************************
 *** FUNCTION: predict_pns_reset()                                              *
 ***										*
 ***    carry out predictor reset for PNS scalefactor bands (long blocks)       *
 ***										*
 ********************************************************************************/
void
predict_pns_reset(Info* info, PRED_STATUS *psp, byte *cb_map)
{
    int    nband, sfb, i, top;
    const short  *band;

    if (info->islong) {

        nband = info->sfb_per_sbk[0];
        band = info->sbk_sfb_top[0];

        for (i=0,sfb=0; sfb<nband; sfb++)  {

            top = band[sfb];
            if (cb_map[sfb] == NOISE_HCB  ||  cb_map[sfb] == NOISE_HCB+100) {
                for (; i<top; i++)
                    reset_pred_state(&psp[i]);
            }
            i = top;
        }

    } /* end islong */
}

#endif /*PNS*/
