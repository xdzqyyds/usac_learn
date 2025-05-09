/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.

Initial author:

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
$Id: dec_acelp.c,v 1.2 2010-03-12 06:54:40 mul Exp $
*******************************************************************************/

/*
 *===================================================================
 *  3GPP AMR Wideband Floating-point Speech Codec
 *===================================================================
 */
#include <memory.h>
#include "typedef.h"
#include "int3gpp.h"

#define NEW_28bits     /* for amr_wbplus */
#define L_SUBFR      64    /* Subframe size              */
#define PRED_ORDER   4
#define MEAN_ENER    30    /* average innovation energy  */


/*
 * D_ACELP_add_pulse
 *
 * Parameters:
 *    pos         I: position of pulse
 *    nb_pulse    I: number of pulses
 *    track       I: track
 *    code        O: fixed codebook
 *
 * Function:
 *    Add pulses to fixed codebook
 *
 * Returns:
 *    void
 */
static void D_ACELP_add_pulse(Word32 pos[], Word32 nb_pulse,
                              Word32 track, Word16 code[])
{
   Word32 i, k;
   for(k = 0; k < nb_pulse; k++)                            
   {
      /* i = ((pos[k] & (16-1))*NB_TRACK) + track; */
      i = ((pos[k] & (16 - 1)) << 2) + track;
      if((pos[k] & 16) == 0)
      {
         code[i] = (Word16)(code[i] + 512);
      }
      else
      {
         code[i] = (Word16)(code[i] - 512);
      }
   }
   return;
}
/*
 * D_ACELP_decode_1p_N1
 *
 * Parameters:
 *    index    I: pulse index
 *    N        I: number of bits for position
 *    offset   I: offset
 *    pos      O: position of the pulse
 *
 * Function:
 *    Decode 1 pulse with N+1 bits
 *
 * Returns:
 *    void
 */
static void D_ACELP_decode_1p_N1(Word32 index, Word32 N,
                                 Word32 offset, Word32 pos[])
{
   Word32 i, pos1, mask;
   mask = ((1 << N) - 1);
   /*
    * Decode 1 pulse with N+1 bits
    */
   pos1 = ((index & mask) + offset);
   i = ((index >> N) & 1);
   if(i == 1)
   {
      pos1 += 16;
   }
   pos[0] = pos1;
   return;
}
/*
 * D_ACELP_decode_2p_2N1
 *
 * Parameters:
 *    index    I: pulse index
 *    N        I: number of bits for position
 *    offset   I: offset
 *    pos      O: position of the pulse
 *
 * Function:
 *    Decode 2 pulses with 2*N+1 bits
 *
 * Returns:
 *    void
 */
static void D_ACELP_decode_2p_2N1(Word32 index, Word32 N,
                                  Word32 offset, Word32 pos[])
{
   Word32 i, pos1, pos2;
   Word32 mask;
   mask = ((1 << N) - 1);
   /*
    * Decode 2 pulses with 2*N+1 bits
    */
   pos1 = (((index >> N) & mask) + offset);
   i = (index >> (2 * N)) & 1;
   pos2 = ((index & mask) + offset);
   if((pos2 - pos1) < 0)
   {
      if(i == 1)
      {
         pos1 += 16;
      }
      else
      {
         pos2 += 16;
      }
   }
   else
   {
      if(i == 1)
      {
         pos1 += 16;
         pos2 += 16;
      } 
   }
   pos[0] = pos1;
   pos[1] = pos2;
   return;
}
/*
 * D_ACELP_decode_3p_3N1
 *
 * Parameters:
 *    index    I: pulse index
 *    N        I: number of bits for position
 *    offset   I: offset
 *    pos      O: position of the pulse
 *
 * Function:
 *    Decode 3 pulses with 3*N+1 bits
 *
 * Returns:
 *    void
 */
static void D_ACELP_decode_3p_3N1(Word32 index, Word32 N,
                                  Word32 offset, Word32 pos[])
{
   Word32 j, mask, idx;
   /*
    * Decode 3 pulses with 3*N+1 bits
    */
   mask = ((1 << ((2 * N) - 1)) - 1);
   idx = index & mask;
   j = offset;
   if(((index >> ((2 * N) - 1)) & 1) == 1)
   {
      j += (1 << (N - 1));
   }
   D_ACELP_decode_2p_2N1(idx, N - 1, j, pos);
   mask = ((1 << (N + 1)) - 1);
   idx = (index >> (2 * N)) & mask;
   D_ACELP_decode_1p_N1(idx, N, offset, pos + 2);
   return;
}
/*
 * D_ACELP_decode_4p_4N1
 *
 * Parameters:
 *    index    I: pulse index
 *    N        I: number of bits for position
 *    offset   I: offset
 *    pos      O: position of the pulse
 *
 * Function:
 *    Decode 4 pulses with 4*N+1 bits
 *
 * Returns:
 *    void
 */
static void D_ACELP_decode_4p_4N1(Word32 index, Word32 N,
                                  Word32 offset, Word32 pos[])
{
   Word32 j, mask, idx;
   /*
    * Decode 4 pulses with 4*N+1 bits
    */
   mask = ((1 << ((2 * N) - 1)) - 1);
   idx = index & mask;
   j = offset;
   if(((index >> ((2 * N) - 1)) & 1) == 1)
   {
      j += (1 << (N - 1));
   }
   D_ACELP_decode_2p_2N1(idx, N - 1, j, pos);
   mask = ((1 << ((2 * N) + 1)) - 1);
   idx = (index >> (2 * N)) & mask;
   D_ACELP_decode_2p_2N1(idx, N, offset, pos + 2);
   return;
}
/*
 * D_ACELP_decode_4p_4N
 *
 * Parameters:
 *    index    I: pulse index
 *    N        I: number of bits for position
 *    offset   I: offset
 *    pos      O: position of the pulse
 *
 * Function:
 *    Decode 4 pulses with 4*N bits
 *
 * Returns:
 *    void
 */
static void D_ACELP_decode_4p_4N(Word32 index, Word32 N,
                                 Word32 offset, Word32 pos[])
{
   Word32 j, n_1;
   /*
    * Decode 4 pulses with 4*N bits
    */
   n_1 = N - 1;
   j = offset + (1 << n_1);
   switch((index >> ((4 * N) - 2)) & 3)
   {
   case 0:
      if(((index >> ((4 * n_1) + 1)) & 1) == 0)
      {
         D_ACELP_decode_4p_4N1(index, n_1, offset, pos);
      }
      else
      {
         D_ACELP_decode_4p_4N1(index, n_1, j, pos);
      }
      break;
   case 1:
      D_ACELP_decode_1p_N1((index >> ((3 * n_1) + 1)), n_1, offset, pos);
      D_ACELP_decode_3p_3N1(index, n_1, j, pos + 1);
      break;
   case 2:
      D_ACELP_decode_2p_2N1((index >> ((2 * n_1) + 1)), n_1, offset, pos);
      D_ACELP_decode_2p_2N1(index, n_1, j, pos + 2);
      break;
   case 3:
      D_ACELP_decode_3p_3N1((index >> (n_1 + 1)), n_1, offset, pos);
      D_ACELP_decode_1p_N1(index, n_1, j, pos + 3);
      break;
   }
   return;
}
/*
 * D_ACELP_decode_5p_5N
 *
 * Parameters:
 *    index    I: pulse index
 *    N        I: number of bits for position
 *    offset   I: offset
 *    pos      O: position of the pulse
 *
 * Function:
 *    Decode 5 pulses with 5*N bits
 *
 * Returns:
 *    void
 */
static void D_ACELP_decode_5p_5N(Word32 index, Word32 N,
                                 Word32 offset, Word32 pos[])
{
   Word32 j, n_1;
   Word32 idx;
   /*
    * Decode 5 pulses with 5*N bits
    */
   n_1 = N - 1;
   j = offset + (1 << n_1);
   idx = (index >> ((2 * N) + 1));
   if(((index >> ((5 * N) - 1)) & 1) == 0)
   {
      D_ACELP_decode_3p_3N1(idx, n_1, offset, pos);
      D_ACELP_decode_2p_2N1(index, N, offset, pos + 3);
   }
   else
   {
      D_ACELP_decode_3p_3N1(idx, n_1, j, pos);
      D_ACELP_decode_2p_2N1(index, N, offset, pos + 3);
   }
   return;
}
/*
 * D_ACELP_decode_6p_6N_2
 *
 * Parameters:
 *    index    I: pulse index
 *    N        I: number of bits for position
 *    offset   I: offset
 *    pos      O: position of the pulse
 *
 * Function:
 *    Decode 6 pulses with 6*N-2 bits
 *
 * Returns:
 *    void
 */
static void D_ACELP_decode_6p_6N_2(Word32 index, Word32 N,
                                   Word32 offset, Word32 pos[])
{
   Word32 j, n_1, offsetA, offsetB;
   n_1 = N - 1;
   j = offset + (1 << n_1);
   offsetA = offsetB = j;
   if(((index >> ((6 * N) - 5)) & 1) == 0)                      
   {
      offsetA = offset;
   }
   else
   {
      offsetB = offset;
   }
   switch((index >> ((6 * N) - 4)) & 3)
   {
      case 0:
         D_ACELP_decode_5p_5N(index >> N, n_1, offsetA, pos);
         D_ACELP_decode_1p_N1(index, n_1, offsetA, pos + 5);
         break;
      case 1:
         D_ACELP_decode_5p_5N(index >> N, n_1, offsetA, pos);
         D_ACELP_decode_1p_N1(index, n_1, offsetB, pos + 5);
         break;
      case 2:
         D_ACELP_decode_4p_4N(index >> ((2 * n_1) + 1), n_1, offsetA, pos);
         D_ACELP_decode_2p_2N1(index, n_1, offsetB, pos + 4);
         break;
      case 3:
         D_ACELP_decode_3p_3N1(index >> ((3 * n_1) + 1), n_1, offset, pos);
         D_ACELP_decode_3p_3N1(index, n_1, j, pos + 3);
         break;
   }
   return;
}


/*
 * D_ACELP_decode_4t
 *
 * Parameters:
 *    index          I: index
 *    mode           I: speech mode
 *    code           I: (Q9) algebraic (fixed) codebook excitation
 *
 * Function:
 *    20, 36, 44, 52, 64, 72, 88 bits algebraic codebook.
 *    4 tracks x 16 positions per track = 64 samples.
 *
 *    12 bits 1+5+1+5 --> 2 pulses 
 *    16 bits 1+5+5+5 --> 3 pulses
 *    20 bits 5+5+5+5 --> 4 pulses in a frame of 64 samples.
 *    36 bits 9+9+9+9 --> 8 pulses in a frame of 64 samples.
 *    44 bits 13+9+13+9 --> 10 pulses in a frame of 64 samples.
 *    52 bits 13+13+13+13 --> 12 pulses in a frame of 64 samples.
 *    64 bits 2+2+2+2+14+14+14+14 --> 16 pulses in a frame of 64 samples.
 *    72 bits 10+2+10+2+10+14+10+14 --> 18 pulses in a frame of 64 samples.
 *    88 bits 11+11+11+11+11+11+11+11 --> 24 pulses in a frame of 64 samples.
 *
 *    All pulses can have two (2) possible amplitudes: +1 or -1.
 *    Each pulse can sixteen (16) possible positions.
 *
 *    codevector length    64
 *    number of track      4
 *    number of position   16
 *
 * Returns:
 *    void
 */
void D_ACELP_decode_4t(Word16 index[], Word16 nbbits, Word16 code[])
{
  Word32 i,  k, L_index, pos[6], offset;
   memset(code, 0, 64 * sizeof(Word16)); 
   /* decode the positions and signs of pulses and build the codeword */


   if(nbbits == 12)
   {
      for(k = 0; k < 4; k+=2)
      {
        offset = index[2*(k/2)];
        L_index = index[2*(k/2)+1];
        D_ACELP_decode_1p_N1(L_index, 4, 0, pos);
        D_ACELP_add_pulse(pos, 1, 2*offset+k/2, code);
      }
   }

   else if(nbbits == 16)
   {
     i=0;
     offset=index[i++];
     offset = (offset==0) ? 1:3;
     for(k = 0; k < 4; k++)
     {
       if(k!=offset){
         L_index = index[i++];
         D_ACELP_decode_1p_N1(L_index, 4, 0, pos);
         D_ACELP_add_pulse(pos, 1, k, code);
       }
     }
   }
   else if(nbbits == 20)
   {
      for(k = 0; k < 4; k++)
      {
         L_index = index[k];
         D_ACELP_decode_1p_N1(L_index, 4, 0, pos);
         D_ACELP_add_pulse(pos, 1, k, code);
      }
   }
#ifdef NEW_28bits
   else if(nbbits == 28)
   {
      for(k = 0; k < 4 - 2; k++)
      {
         L_index = index[k];
         D_ACELP_decode_2p_2N1(L_index, 4, 0, pos);
         D_ACELP_add_pulse(pos, 2, k, code);
      }
      for(k = 2; k < 4; k++)
      {
         L_index = index[k];
         D_ACELP_decode_1p_N1(L_index, 4, 0, pos);
         D_ACELP_add_pulse(pos, 1, k, code);
      }
   }
#endif
   else if(nbbits == 36)
   {
      for(k = 0; k < 4; k++)
      {
         L_index = index[k];
         D_ACELP_decode_2p_2N1(L_index, 4, 0, pos);
         D_ACELP_add_pulse(pos, 2, k, code);
      }
   }
   else if(nbbits == 44)
   {
      for(k = 0; k < 4 - 2; k++)
      {
         L_index = index[k];
         D_ACELP_decode_3p_3N1(L_index, 4, 0, pos);
         D_ACELP_add_pulse(pos, 3, k, code);
      }
      for(k = 2; k < 4; k++)
      {
         L_index = index[k];
         D_ACELP_decode_2p_2N1(L_index, 4, 0, pos);
         D_ACELP_add_pulse(pos, 2, k, code);
      }
   }
   else if(nbbits == 52)
   {
      for(k = 0; k < 4; k++)
      {
         L_index = index[k];
         D_ACELP_decode_3p_3N1(L_index, 4, 0, pos);
         D_ACELP_add_pulse(pos, 3, k, code);
      }
   }
   else if(nbbits == 64)
   {
      for(k = 0; k < 4; k++)
      {
         L_index = ((index[k] << 14) + index[k + 4]);
         D_ACELP_decode_4p_4N(L_index, 4, 0, pos);
         D_ACELP_add_pulse(pos, 4, k, code);
      }
   }
   else if(nbbits == 72)
   {
      for(k = 0; k < 4 - 2; k++)
      {
         L_index = ((index[k] << 10) + index[k + 4]);
         D_ACELP_decode_5p_5N(L_index, 4, 0, pos);
         D_ACELP_add_pulse(pos, 5, k, code);
      }
      for(k = 2; k < 4; k++)
      {
         L_index = ((index[k] << 14) + index[k + 4]);
         D_ACELP_decode_4p_4N(L_index, 4, 0, pos);
         D_ACELP_add_pulse(pos, 4, k, code);
      }
   }
   else if(nbbits == 88)
   {
      for(k = 0; k < 4; k++)
      {
         L_index = ((index[k] << 11) + index[k + 4]);
         D_ACELP_decode_6p_6N_2(L_index, 4, 0, pos);
         D_ACELP_add_pulse(pos, 6, k, code);
      }
   }
   return;
}
