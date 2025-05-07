/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Fraunhofer IIS

and edited by:

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

$Id: enc_prm.c,v 1.11 2011-05-02 09:11:02 mul Exp $
*******************************************************************************/

#include <float.h>
#include <stdlib.h>
#include <math.h>
#include "acelp_plus.h"
#include "re8.h"

static int unary_code(int ind, short *ptr);

static int unpack8bits(int nbits, unsigned char *prm, short *ptr);
int encode_fac(int *prm, short *ptr, int lFac);

/*-----------------------------------------------------------------*
 * Funtion  enc_prm_amrwb_plus_sq()                                *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                                   *
 * encode AMR-WB+ parameters according to selected mode            *
 *-----------------------------------------------------------------*/
void enc_prm_fac(
  int mod[],         /* (i) : frame mode (mode[4], 4 division) */
  int n_param_tcx[], /* (i) : number of parameters (freq. coeff) per tcx subframe */ 
  int codec_mode,    /* (i) : AMR-WB+ mode (see cnst.h)        */
  int param[],       /* (i) : parameters                       */
  int param_lpc[],   /* (i) : LPC parameters                   */
  int isAceStart, 
  int isBassPostFilter,
  short serial[],    /* (o) : serial bits stream               */
  HANDLE_USAC_TD_ENCODER st, /* io: quantization Analysis values      */
  int *total_nbbits,   /* (o) : number of bits per superframe    */
  int const bUsacIndependencyFlag
)
{
  int i,j, k, n, sfr, mode, nbits, sqBits, *prm, reset, nb_lpc;
  short *ptr;
  unsigned char prm_SQ[6144] = {0};
  short firstTcx = 1;
  int skip, q_type, nb_ind;
  int nb, st1, qn1, qn2, skip_avq;
  int nbits_fac, nb_bits_lpc;
  int core_mode_last = (isAceStart)?0:1;
  int fac_data_present;

  /* Initialize ptr pointer */
  ptr = serial;
  st->nb_bits = 0;
  *total_nbbits = 0;

  /* Encode mode combination (26 possibilities = 5 bits) */
  if (mod[0] == 3)
  {
    mode = 25;
  }
  else if ((mod[0] == 2) && (mod[2] == 2))
  {
    mode = 24;
  }
  else
  {
     if (mod[0] == 2)
     {
        mode = 16 + mod[2] + 2*mod[3];
     }
     else if (mod[2] == 2)
     {
        mode = 20 + mod[0] + 2*mod[1];
     }
     else
     {
        mode = mod[0] + 2*mod[1] + 4*mod[2] + 8*mod[3];
     }
  }

  nb_lpc=4;
  if (isAceStart==1) nb_lpc +=1;
  int2bin(mode, 5, &serial[0]);
  ptr = serial + 5;
  st->nb_bits = 5;
  *total_nbbits = 5;

   /* bpf_control_info */
  int2bin(isBassPostFilter, 1, ptr);
  ptr += 1;
  *total_nbbits += 1;

   /* core_mode_last */
  int2bin(core_mode_last, 1, ptr);
  ptr += 1;
  *total_nbbits += 1;

  /* fac_data_present */
  if( ((mod[0] == 0) && (mod[-1] != 0)) ||
      ((mod[0] > 0) && (mod[-1] == 0)) ){
    fac_data_present = 1;
  } else {
    fac_data_present = 0;
  }

  int2bin(fac_data_present, 1, ptr);
  ptr += 1;
  *total_nbbits += 1;

  /* remove bits for mode (2 bits per 20ms packet) */
  if ((st->lDiv)==((3*L_DIV_1024)/4))
  {
    nbits = (NBITS_CORE_768[codec_mode]/4) - 2;
  }
  else
  {
    nbits = (NBITS_CORE_1024[codec_mode]/4) - 2;
  }


  k = 0;
  while (k < NB_DIV) {
    mode=mod[k];
    /* set pointer to parameters */
    prm = param + (k*NPRM_DIV);
    j = 0;

    /* Add FAC information for transitions between TCX and ACELP */

    skip = 1;
    if ( ((mod[k-1]==0) && (mod[k]>0)) || ((mod[k-1]>0) && (mod[k]==0)) ) skip=0;

    if (!skip)
    {
      nbits_fac = encode_fac(&prm[j], ptr, (st->lDiv)/2);	   j += (st->lDiv)/2; ptr += nbits_fac;
      *total_nbbits += nbits_fac;
    }

    if ((mode == 0) || (mode == 1)) {


      if (mode == 0) {
       /*---------------------------------------------------------*
        * encode 20ms ACELP frame                                 *
        * acelp bits: 2+(9+6+9+6)+4+(4xICB_NBITS])+(4x7)          *
        *---------------------------------------------------------*/
        /* mean energy : 2 bits */
        int2bin(prm[j], 2, ptr);       ptr += 2;    j++;

        for (sfr=0; sfr<(st->nbSubfr); sfr++) {
          n = 6;
          if ((sfr==0) || (((st->lDiv)==256) && (sfr==2))) n=9;
          /* AMR-WB closed-loop pitch lag */
          int2bin(prm[j], n, ptr);       ptr += n;    j++;
          int2bin(prm[j], 1, ptr);       ptr += 1;    j++;
          if (codec_mode == MODE_8k0) {
            int2bin(prm[j], 1, ptr);       ptr += 1;     j++;
            int2bin(prm[j], 5, ptr);       ptr += 5;     j++;
            int2bin(prm[j], 1, ptr);       ptr += 1;     j++;
            int2bin(prm[j], 5, ptr);       ptr += 5;     j++;
          } else if (codec_mode == MODE_8k8) {
            int2bin(prm[j], 1, ptr);       ptr += 1;     j++;
            int2bin(prm[j], 5, ptr);       ptr += 5;     j++;
            int2bin(prm[j], 5, ptr);       ptr += 5;     j++;
            int2bin(prm[j], 5, ptr);       ptr += 5;     j++;
          } else if (codec_mode == MODE_9k6) {
            /* 20 bits AMR-WB codebook is used */
            int2bin(prm[j], 5, ptr);       ptr += 5;     j++;
            int2bin(prm[j], 5, ptr);       ptr += 5;     j++;
            int2bin(prm[j], 5, ptr);       ptr += 5;     j++;
            int2bin(prm[j], 5, ptr);       ptr += 5;     j++;
          } else if (codec_mode == MODE_11k2) {
            /* 28 bits AMR-WB codebook is used */
            int2bin(prm[j], 9, ptr);       ptr += 9;     j++;
            int2bin(prm[j], 9, ptr);       ptr += 9;     j++;
            int2bin(prm[j], 5, ptr);       ptr += 5;     j++;
            int2bin(prm[j], 5, ptr);       ptr += 5;     j++;
          } else if (codec_mode == MODE_12k8) {
            /* 36 bits AMR-WB codebook is used */
            int2bin(prm[j], 9, ptr);       ptr += 9;     j++;
            int2bin(prm[j], 9, ptr);       ptr += 9;     j++;
            int2bin(prm[j], 9, ptr);       ptr += 9;     j++;
            int2bin(prm[j], 9, ptr);       ptr += 9;     j++;
          } else if (codec_mode == MODE_14k4) {
            /* 44 bits AMR-WB codebook is used */
            int2bin(prm[j], 13, ptr);      ptr += 13;    j++;
            int2bin(prm[j], 13, ptr);      ptr += 13;    j++;
            int2bin(prm[j], 9, ptr);       ptr += 9;     j++;
            int2bin(prm[j], 9, ptr);       ptr += 9;     j++;
          } else if (codec_mode == MODE_16k) {
            /* 52 bits AMR-WB codebook is used */
            int2bin(prm[j], 13, ptr);      ptr += 13;    j++;
            int2bin(prm[j], 13, ptr);      ptr += 13;    j++;
            int2bin(prm[j], 13, ptr);      ptr += 13;    j++;
            int2bin(prm[j], 13, ptr);      ptr += 13;    j++;
          } else if (codec_mode == MODE_18k4) {
            /* 64 bits AMR-WB codebook is used */
            int2bin(prm[j], 2, ptr);       ptr += 2;     j++;
            int2bin(prm[j], 2, ptr);       ptr += 2;     j++;
            int2bin(prm[j], 2, ptr);       ptr += 2;     j++;
            int2bin(prm[j], 2, ptr);       ptr += 2;     j++;
            int2bin(prm[j], 14, ptr);      ptr += 14;    j++;
            int2bin(prm[j], 14, ptr);      ptr += 14;    j++;
            int2bin(prm[j], 14, ptr);      ptr += 14;    j++;
            int2bin(prm[j], 14, ptr);      ptr += 14;    j++;
          }
          /* AMR-WB 7 bits gains codebook */
          int2bin(prm[j], 7, ptr);       ptr += 7;    j++;
        }
        *total_nbbits += (nbits - NBITS_LPC);
	st->nb_bits += (nbits - NBITS_LPC);
      } /* end of mode 0 */
      else  /* mode 1 */
      {
        int2bin(prm[j++], 3, ptr);     ptr += 3;
        int2bin(prm[j++], 7, ptr);     ptr += 7;
	*total_nbbits += 10;
	st->nb_bits += 10;
 
	/*Arithmetic reset flag*/

        if(firstTcx){
          firstTcx = 0;
          if(bUsacIndependencyFlag){
            st->arithReset = 1;
            tcxArithReset(st->arithQ);
          } else {
            reset = st->arithReset;
            if(reset)
              tcxArithReset(st->arithQ);
            int2bin(reset, 1, ptr);
            ptr += 1;
            *total_nbbits += 1;
            st->nb_bits += 1;
          }
        }
        
        /* encode 20ms TCX */
        sqBits = tcxArithEnc(n_param_tcx[k],
                             st->lFrame,
                             prm+j,
                             st->arithQ,
                             1,
                             prm_SQ);
        
        
        
        unpack8bits(sqBits, prm_SQ, ptr);
        *total_nbbits += sqBits;
        st->nb_bits += sqBits;
        ptr += sqBits;
        

      } /* end of mode 1 */
      k++;
    } /* end of mode 0/1 */
    else if (mode == 2)
    {
      int2bin(prm[j++], 3, ptr);      ptr += 3;
      int2bin(prm[j++], 7, ptr);      ptr += 7;
      *total_nbbits += 10;
      st->nb_bits += 10;

      /*Arithmetic reset flag*/
      if(firstTcx){
        firstTcx = 0;
        if(bUsacIndependencyFlag){
	        tcxArithReset(st->arithQ);
          st->arithReset = 1;
        } else {
          reset = st->arithReset;
          if(reset)
            tcxArithReset(st->arithQ);
          int2bin(reset, 1, ptr);
          ptr += 1;
          *total_nbbits += 1;
          st->nb_bits += 1;
        }
      }

      /* encode 40ms TCX */
      sqBits = tcxArithEnc(n_param_tcx[k],
                           st->lFrame,
			   prm+j,
			   st->arithQ,
			   1,
			   prm_SQ);


      unpack8bits(sqBits, prm_SQ, ptr);
      *total_nbbits += sqBits;
      st->nb_bits += sqBits;
      ptr += sqBits;

      k+=2;

    } /* end of mode 2 */
    else if (mode == 3)
    {
      int2bin(prm[j++], 3, ptr);      ptr += 3;
      int2bin(prm[j++], 7, ptr);      ptr += 7;
      *total_nbbits += 10;
      st->nb_bits += 10;

      /*Arithmetic reset flag*/

      if(firstTcx){
        firstTcx = 0;
        if(bUsacIndependencyFlag){
          st->arithReset = 1;
          tcxArithReset(st->arithQ);
        } else {
          reset = st->arithReset;
          if(reset)
            tcxArithReset(st->arithQ);
          int2bin(reset, 1, ptr);
          ptr += 1;
          *total_nbbits += 1;
          st->nb_bits += 1;
        }
      }

      /* encode 80ms TCX */
      sqBits = tcxArithEnc(n_param_tcx[k],
                           st->lFrame,
			   prm+j,
			   st->arithQ,
			   1,
			   prm_SQ);

      unpack8bits(sqBits, prm_SQ, ptr);
      *total_nbbits += sqBits;
      st->nb_bits += sqBits;
      ptr += sqBits;

      k+=4;
    }  /* end of mode 3 */
  } /* end of while k < NB_DIV */

 /*---------------------------------------------------------*
  * encode LPC parameters                                   *
  *---------------------------------------------------------*/

  nb_bits_lpc = *total_nbbits;
  j=0;

  /* Encode remaining LPC filters */
  for (k=0; k<5; k++)            /* loop for LPC 4,0,2,1,3 */
  {
    if ((k==1) && !isAceStart) k++;  /* skip LPC0 */

     /* Retreive quantizer type */

    if (k==0) q_type = 0;            /* LPC4 always abs */
    else {
      q_type = param_lpc[j]; j++;      
    }
 
     /* Determine number of AVQ indices */

     nb_ind = 0;
     skip_avq=0;
     if ((k==3) && (q_type==1)) skip_avq=1;       /* LPC1 mid0 */

     if (!skip_avq)   /* All quantizers except LPC1 mid 0bits */
     {
        if (q_type==0)
        {
          st1 = param_lpc[j]; j++;        /* Abs 1st index */
        }
        qn1 = param_lpc[j]; j++;
        qn2 = param_lpc[j]; j++;
        nb_ind = get_num_prm(qn1, qn2) - 2;
     }

     /* Determine if this LPC filter needs to be transmitted */

     skip=1;
     if (k<2) skip=0;                    /* LPC4,LPC0 */
     if ((k==2) && (mod[0]<3)) skip=0;   /* LPC2 */
     if ((k==3) && (mod[0]<2)) skip=0;   /* LPC1 */
     if ((k==4) && (mod[2]<2)) skip=0;   /* LPC3 */

     if (skip)
     {
        j += nb_ind; /* Skip that LPC predictor */
     }
	 else
     {
        /* Encode quantizer type */
       
       if (k==0)      /* LPC4 always abs */
         {
           nb = 0;    
         }
       else if (k==3)   /* Unary code for LPC1 */
         {
           if (q_type==0)      /*  10 = abs */
             {
               ptr[0] = 1;
               ptr[1] = 0;
               nb = 2;
             }
           else if (q_type==1) /*  11 = mid0 */
             {
               ptr[0] = 1;
               ptr[1] = 1;
               nb = 2;
             }
           else                /*   0 = relR */
             {
               ptr[0] = 0;
               nb = 1;
             }
         }
       else if (k==4)   /* Unary code for LPC3 */
         {
           if (q_type==0)      /*  10 = abs */
             {
               ptr[0] = 1;
               ptr[1] = 0;
               nb = 2;
             }
           else if (q_type==1) /*   0 = mid */
             {
               ptr[0] = 0;
               nb = 1;
             }
           else if (q_type==2) /* 110 = relL */
             {
               ptr[0] = 1;
               ptr[1] = 1;
               ptr[2] = 0;
               nb = 3;
             }
           else                /* 111 = relR */
             {
               ptr[0] = 1;
               ptr[1] = 1;
               ptr[2] = 1;
               nb = 3;
             }
         }
       else        /* LPC2 or LPC0: 0=abs or 1=relR */
        {
           nb = 1;
           int2bin(q_type, nb, ptr);
        }

        ptr += nb;
        *total_nbbits += nb;
        st->nb_bits += nb;

        /* Encode quantization indices */

        if (!skip_avq)
          {
            /* Encode quantizer data */
            if (q_type==0)
              {
                /* Absolute quantizer with 1st stage stochastic codebook */
                int2bin(st1, 8, ptr);   ptr += 8;
                *total_nbbits += 8;
                st->nb_bits += 8;
              }

            if ((q_type == 1) && ((k==3) || (k==4)))
              {
                /* Unary code for mid LPC1/LPC3 */
                /* Q0=0, Q2=10, Q3=110, ... */
                nb = unary_code(qn1, ptr); ptr += nb;
                *total_nbbits += nb;
                st->nb_bits +=nb;

                nb = unary_code(qn2, ptr); ptr += nb;
                *total_nbbits += nb;
                st->nb_bits +=nb;
              }
            else {
              /* 2 bits to specify Q2,Q3,Q4,ext */
              *total_nbbits += 4;
              st->nb_bits += 4;

              i = qn1-2;
              if ((i<0) || (i>3)) i = 3;
              int2bin(i, 2, ptr);   ptr += 2;  

              i = qn2-2;
              if ((i<0) || (i>3)) i = 3;
              int2bin(i, 2, ptr);   ptr += 2;  

              if ((q_type > 1) && ((k==3) || (k==4)))
                {
                  /* Unary code for rel LPC1/LPC3 */
                  /* Q0 = 0, Q5=10, Q6=110, ... */
                  nb = qn1;
                  if (nb > 4) nb -= 3;
                  else if (nb == 0) nb = 1;
                  else nb = 0;

                  if (nb > 0) unary_code(nb, ptr);
                  ptr += nb;
                  *total_nbbits += nb;
                  st->nb_bits +=nb;

                  nb = qn2;
                  if (nb > 4) nb -= 3;
                  else if (nb == 0) nb = 1;
                  else nb = 0;

                  if (nb > 0) unary_code(nb, ptr);
                  ptr += nb;
                  *total_nbbits += nb;
                  st->nb_bits +=nb;
                }
              else
                {
                  /* Unary code for abs and rel LPC0/LPC2 */
                  /* Q5 = 0, Q6=10, Q0=110, Q7=1110, ... */
                  nb = qn1;
                  if (nb > 6) nb -= 3;
                  else if (nb > 4) nb -= 4;
                  else if (nb == 0) nb = 3;
                  else nb = 0;

                  if (nb > 0) unary_code(nb, ptr);
                  ptr += nb;
                  *total_nbbits += nb;
                  st->nb_bits +=nb;

                  nb = qn2;
                  if (nb > 6) nb -= 3;
                  else if (nb > 4) nb -= 4;
                  else if (nb == 0) nb = 3;
                  else nb = 0;

                  if (nb > 0) unary_code(nb, ptr);
                  ptr += nb;
                  *total_nbbits += nb;
                  st->nb_bits +=nb;
                }
            }

           {
             if (qn1 > 0) {
               int n, nk, i;
               
               nk = 0;
               n = qn1;
               if (qn1 > 4){
                 nk = (qn1-3)>>1;
                 n = qn1 - nk*2;
               }
               
               /* Write codebook index */
               int2bin(param_lpc[j++], 4*n, ptr); ptr += 4*n;
               /* Write voronoi indices */
               for (i=0; i<8; i++) {
                 int2bin(param_lpc[j++], nk, ptr); ptr += nk;
               }  
               *total_nbbits += 4*n + 8*nk;
               st->nb_bits   += 4*n + 8*nk;
             }
             
             if (qn2 > 0) {
               int n, nk, i;
               nk = 0;
               n = qn2;
               if (qn2 > 4){
                 nk = (qn2-3)>>1;
                 n = qn2 - nk*2;
               }
               
               /* Write codebook index */
               int2bin(param_lpc[j++], 4*n, ptr); ptr += 4*n;
               /* Write voronoi indices */
               for (i=0; i<8; i++) {
                 int2bin(param_lpc[j++], nk, ptr); ptr += nk;
               }
               *total_nbbits += 4*n + 8*nk;
               st->nb_bits   += 4*n + 8*nk;
             }
           }
        }
     }
  }

  nb_bits_lpc = *total_nbbits - nb_bits_lpc;

  if( (core_mode_last == 0) && (fac_data_present == 1) ){
    int short_fac_flag = (mod[-1] == -2)?1:0;

    /* short_fac_flag */
    int2bin(short_fac_flag, 1, ptr);
    ptr += 1;
    *total_nbbits += 1;
  }

  return;
} 

static int unary_code(int ind, short *ptr)
{

int nb_bits;

  nb_bits = 1;

  /* Index bits */

  ind -= 1;
  while (ind-- > 0) {
    *ptr++ = 1;
    nb_bits++;
  }

  /* Stop bit */
  *ptr = 0;

  return(nb_bits);
}

/*Write byte by byte*/
static int unpack8bits(int nbits, unsigned char *prm, short *ptr)
{
  int i = 0;
  int temp;

  /*Write out bytes*/
  while (nbits > 8) {
    temp = (int) prm[i];
    int2bin(temp, 8, ptr);
    ptr += 8;
    nbits -= 8;
    i++;
  }
  /* take care of remaining bits... */
  temp = (int) prm[i]>>(8-nbits);
  int2bin(temp, nbits, ptr);

  /* to be on the safe side */
  for(i=nbits;i<8;i++) {
    ptr[i] = 0;
  }

  return(i);
}


/*Encode FAC information*/
int encode_fac(int *prm, short *ptr, int lFac)
{
  int i, iii, n, nb, qn, kv[8], nk, nbits_fac;
  long I;
  
  nbits_fac=0;

   /* Unary code for codebook numbers */
   for (i=0; i<lFac; i+=8)
   {
      RE8_cod(&prm[i], &qn, &I, kv);

      nb = unary_code(qn, ptr); ptr += nb;

      nbits_fac += nb;

      nk = 0;
      n = qn;
      if (qn > 4)
      {
         nk = (qn-3)>>1;
         n = qn - nk*2;
      }

      /* write 4*n bits for base codebook index (I) */
      int2bin(I, 4*n, ptr); ptr += 4*n;
      /* write 8*nk bits for Voronoi indices (kv[0...7]) */
      for (iii=0; iii<8; iii++) {
        int2bin(kv[iii], nk, ptr); ptr += nk;
      }

      nbits_fac += 4*qn;
   }

   return nbits_fac;
}

