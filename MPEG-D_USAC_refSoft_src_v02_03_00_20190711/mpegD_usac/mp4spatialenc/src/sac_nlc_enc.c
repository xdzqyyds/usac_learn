/*******************************************************************************
This software module was originally developed by

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips

and edited by

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

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
*******************************************************************************/
/*******************************************************************************
This software module was further modified in light of the JAME project that 
was initiated by Yonsei University and LG Electronics.

The JAME project aims at making a better MPEG Reference Software especially
for the encoder in direction to simplification, quality improvement and 
efficient implementation being still compliant with the ISO/IEC specification. 
Those intending to participate in this project can use this software module 
under consideration of above mentioned ISO/IEC copyright notice. 

Any voluntary contributions to this software module in the course of this 
project can be recognized by proudly listing up their authors here:

- Henney Oh, LG Electronics (henney.oh@lge.com)
- Sungyong Yoon, LG Electronics (sungyong.yoon@lge.com)

Copyright (c) JAME 2010.
*******************************************************************************/
                                           

#include <math.h>
#include <memory.h>
#include <assert.h>
#include <stdlib.h>

#include "sac_stream_enc.h"
#include "sac_nlc_enc.h"
#include "sac_huff_tab.h"
#include "sac_types_enc.h"


#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

static const int lavtab_cld[4] = {3, 5, 7, 9};
static const int lavtab_icc[4] = {1, 3, 5, 7};
static const int lavtab_cpc[4] = {3, 6, 9, 12};
static const int lavtab_ipd[4] = {1, 3, 5, 7};

static const HUFF_LAV_TAB huffLAVTab = 
{
  {0x0, 0x2, 0x6, 0x7},
  {  1,   2,   3,   3}
};

extern const HUFF_CLD_TABLE huffCLDTab;
extern const HUFF_ICC_TABLE huffICCTab;
extern const HUFF_CPC_TABLE huffCPCTab;
extern const HUFF_IPD_TABLE huffIPDTab;

static int write_bits(Stream* strm, unsigned int data, int nbits) {
  if (strm) writeBits(strm, data, nbits);
  return nbits;
}

static int calc_lav(int in[], int N, const int tab[], int *lavidx)
{
  int i;
  int maxval = 0;

  /* find max. */
  for (i = 0; i < N; i++) {
    if (maxval <  in[i]) maxval =  in[i];
    if (maxval < -in[i]) maxval = -in[i];
  }

  /* calc lav. */
  for (i = 0; i < 4; i++)
  {
    if (maxval <= tab[i]) {
      *lavidx = i;
      return tab[i];
    }
  }

  *lavidx = -1; 
  return maxval;
}

static void enc_sym_data(int lav, int d1, int d2, int nd[2], int SymBit[2])
{
  int temp, sum, diff;

  SymBit[0]=0;
  SymBit[1]=0;

  if (d1 == d2) SymBit[1]=-1;

  if ((d1+d2) == 0) {
    SymBit[0] = -1;
    if (d1 < 0) {
      temp = d2;
      d2 = d1;
      d1 = temp;
      SymBit[1] = 1;
    }
  }

  if (abs(d2) > abs(d1)) {
    temp = d2;
    d2 = d1;
    d1 = temp;
    SymBit[1] = 1;
  }

  if (d1 < 0) {
    d1 = -d1;
    d2 = -d2;
    SymBit[0] = 1;
  }

  sum  = d1 + d2;
  diff = d1 - d2;

  if (sum % 2) {
    nd[0] = lav + ((1 - sum )>>1);
    nd[1] = lav + ((1 - diff)>>1);
  }
  else {
    nd[0] = sum  >> 1;
    nd[1] = diff >> 1;
  }
}

static int split_lsb( int* in_data,
                      int  offset,
                      int  num_lsb,
                      int  num_val,
                      int* out_data_lsb,
                      int* out_data_msb )
{
  int i = 0, val = 0, lsb = 0, msb = 0;

  unsigned int mask = (1 << num_lsb) - 1;
  int no_lsb_bits = 0;

  for( i=0; i<num_val; i++ ) {

    val = in_data[i] + offset;

    lsb = val & mask;
    msb = val >> num_lsb;

    if( out_data_lsb != NULL) out_data_lsb[i] = lsb;
    if( out_data_msb != NULL) out_data_msb[i] = msb;

    no_lsb_bits += num_lsb;

  }

  return no_lsb_bits;
}


static void apply_lsb_coding( Stream* strm,
                              int*    in_data_lsb,
                              int     num_lsb,
                              int     num_val )
{
  int i = 0;

  for( i=0; i<num_val; i++ ) {
    writeBits( strm, in_data_lsb[i], num_lsb );
  }
}


static void calc_diff_freq( int* in_data,
                            int* out_data,
                            int  num_val,
							int  data_type)
{
  int i = 0;

  out_data[0] = in_data[0];

  for( i=1; i<num_val; i++ ) {
    out_data[i] = in_data[i] - in_data[i-1];
	if(data_type == t_IPD) out_data[i] = (out_data[i]+8)%8;
  }
}


static void calc_diff_time( int*      in_data,
                            int*      prev_data,
                            int*      out_data,
                            DIRECTION direction,
                            int       num_val,
							int       data_type)
{
  int i = 0;

  out_data[-1] = (direction == BACKWARDS) ? in_data[0] : prev_data[0];

  for( i=0; i<num_val; i++ ) {
    out_data[i] = in_data[i] - prev_data[i];
	if(data_type == t_IPD) out_data[i] = (out_data[i]+8)%8;
  }
}

static int apply_pcm_coding( Stream* strm,
                            int*    in_data_1,
                            int*    in_data_2,
                            int     offset,
                            int     num_val,
                            int     num_levels )
{
  int i = 0, j = 0, idx = 0;
  int max_grp_len = 0, grp_len = 0, next_val = 0, grp_val = 0;
  int bits_pcm = 0;

  float ld_nlev = 0.f;

  int pcm_chunk_size[7] = { 0 };


  switch( num_levels )
    {
    case  3: max_grp_len = 5; break;
    case  6: max_grp_len = 5; break;
    case  7: max_grp_len = 6; break;
    case 11: max_grp_len = 2; break;
    case 13: max_grp_len = 4; break;
    case 19: max_grp_len = 4; break;
    case 25: max_grp_len = 3; break;
    case 51: max_grp_len = 4; break;
    case 4:
    case 8:
    case 15:
	case 16:
    case 26:
    case 31: max_grp_len = 1; break;
    default: assert(0);max_grp_len = 1;
  }

  ld_nlev = (float)(log((float)num_levels)/log(2.f));

  if (strm) {
    for( i=1; i<=max_grp_len; i++ ) {
      pcm_chunk_size[i] = (int) ceil( (float)(i) * ld_nlev );
    }

    for( i=0; i<num_val; i+=max_grp_len ) {
      grp_len = min( max_grp_len, num_val-i );
      grp_val = 0;
      for( j=0; j<grp_len; j++ ) {
        idx = i+j;
        if( in_data_2 == NULL ) {
          next_val = in_data_1[idx];
        }
        else if( in_data_1 == NULL ) {
          next_val = in_data_2[idx];
        }
        else {
          next_val = ((idx%2) ? in_data_2[idx/2] : in_data_1[idx/2]);
        }
        next_val += offset;
        grp_val = grp_val*num_levels + next_val;
      }

      bits_pcm += write_bits( strm, grp_val, pcm_chunk_size[grp_len] );
    }
  }
  else { /* fast and simple calculation */
    int num_complete_chunks = num_val/max_grp_len;
    int rest_chunk_size     = num_val%max_grp_len;

    bits_pcm  = ((int) ceil((float)(max_grp_len)*ld_nlev)) * num_complete_chunks;
    bits_pcm += (int) ceil((float)(rest_chunk_size)*ld_nlev);
  }

  return bits_pcm;
}

static int calc_pcm_bits( int num_val,
                         int num_levels )
{
  return apply_pcm_coding(NULL, NULL, NULL, 0, num_val, num_levels);
}

static int huff_enc_cld_1D(Stream*             strm,
                       const HUFF_CLD_TAB* huffTabPt0,
                       const HUFF_CLD_TAB* huffTabDiff,
                       int*                in_data,
                       int                 num_val,
                       int                 p0_flag)
{
  int i, id, id_sign;
  int offset = 0;
  int huffBits = 0;

  if (p0_flag) {
    huffBits += write_bits(strm, huffTabPt0->value [in_data[0]], huffTabPt0->length[in_data[0]]);
    offset = 1;
  }

  for (i = offset; i < num_val; i++ ) {

    id = in_data[i];

    if (id < 0) {
      id = -id;
      id_sign = 1;
    }
    else id_sign = 0;

    huffBits += write_bits(strm, huffTabDiff->value[id], huffTabDiff->length[id]);

    if (id != 0) huffBits += write_bits(strm, id_sign, 1);
  }

  return huffBits;
}

static int huff_enc_cld_2D_FP(Stream*             strm,
                              const HUFF_CLD_TAB* huffTabPt0,
                              const HUFF_CLD_TAB* huffTabDiff,
                              const HUFF_2D_TAB   huffTab2D[4][4],
                              int*                in_data,
                              int                 num_val,
                              int                 p0_flag)
{
  int i;
  int offset = 0;
  int huffBits = 0;
  int escDataCntr = 0;

  int lavidx, lav, idx_position;
  int idx[2], SymBit[2];
  HCOD_TYPE hcod_type = p0_flag? DF_FP : DT_FP;
  unsigned int hcod2D_length, hcod2D_codeword, escape_length, escape_codeword;
  int aaEscData[MAXBANDS];

  lav = calc_lav(in_data + p0_flag, ((num_val - p0_flag) >> 1) << 1, lavtab_cld, &lavidx);
  if (lavidx == -1) return 100000; 

  escape_length   = huffCLDTab.huff2D_CLD_ESC[hcod_type].length[lavidx];
  escape_codeword = huffCLDTab.huff2D_CLD_ESC[hcod_type].value[lavidx];

  huffBits += write_bits(strm, huffLAVTab.value[lavidx], huffLAVTab.length[lavidx]);

  if (p0_flag) {
    huffBits += write_bits(strm, huffTabPt0->value [in_data[0]], huffTabPt0->length[in_data[0]]);
    offset = 1;
  }

  for (i = offset; i < num_val - 1; i += 2) {
    enc_sym_data(lav, in_data[i], in_data[i+1], idx, SymBit);
    idx_position = idx[0] * (lav + 1) + idx[1];
    hcod2D_length   = huffTab2D[lavidx][hcod_type].length[idx_position];
    hcod2D_codeword = huffTab2D[lavidx][hcod_type].value[idx_position];

    huffBits += write_bits(strm, hcod2D_codeword, hcod2D_length);
    if (hcod2D_length == escape_length && hcod2D_codeword == escape_codeword) {
      aaEscData[escDataCntr++] = in_data[i]   + lav;
      aaEscData[escDataCntr++] = in_data[i+1] + lav;
    }
    else {
      if (SymBit[0] >= 0) huffBits += write_bits(strm, SymBit[0], 1);
      if (SymBit[1] >= 0) huffBits += write_bits(strm, SymBit[1], 1);
    } 
  }

  if (escDataCntr) {
    huffBits += apply_pcm_coding(strm,
      aaEscData,
      NULL,
      0,
      escDataCntr,
      2 * lav + 1);
  }

  if ((num_val - offset) % 2)
    huffBits += huff_enc_cld_1D(strm,
      huffTabPt0,
      huffTabDiff,
      in_data + num_val - 1,
      1,
      0);

  return huffBits;
}

static int count_huff_cld(CODING_SCHEME*      coding_scheme,
                          const HUFF_CLD_TAB* huffTabPt0,
                          const HUFF_CLD_TAB* huffTabDiff,
                          int*                in_data,
                          int                 num_val,
                          int                 p0_flag )
{
  int bits_huff_1D, bits_huff_2D;

  bits_huff_1D = huff_enc_cld_1D(NULL,
    huffTabPt0,
    huffTabDiff,
    in_data,
    num_val,
    p0_flag);

  bits_huff_2D = huff_enc_cld_2D_FP(NULL,
    huffTabPt0,
    huffTabDiff,
    huffCLDTab.huff2D_CLD,
    in_data,
    num_val,
    p0_flag);

  if (bits_huff_2D < bits_huff_1D) {
    *coding_scheme = HUFF_2D;
    return bits_huff_2D;
  }
  else {
    *coding_scheme = HUFF_1D;
    return bits_huff_1D;
  }
}

static int huff_enc_icc_1D(Stream*             strm,
                           const HUFF_ICC_TAB* huffTabPt0,
                           const HUFF_ICC_TAB* huffTabDiff,
                           int*                in_data,
                           int                 num_val,
                           int                 p0_flag)
{
  int i, id, id_sign;
  int offset = 0;
  int huffBits = 0;

  if (p0_flag) {
    huffBits += write_bits(strm, huffTabPt0->value [in_data[0]], huffTabPt0->length[in_data[0]]);
    offset = 1;
  }

  for (i = offset; i < num_val; i++ ) {

    id = in_data[i];

    if (id < 0) {
      id = -id;
      id_sign = 1;
    }
    else id_sign = 0;

    huffBits += write_bits(strm, huffTabDiff->value[id], huffTabDiff->length[id]);

    if (id != 0) huffBits += write_bits(strm, id_sign, 1);
  }

  return huffBits;
}

static int huff_enc_icc_2D_FP(Stream*             strm,
                              const HUFF_ICC_TAB* huffTabPt0,
                              const HUFF_ICC_TAB* huffTabDiff,
                              const HUFF_2D_TAB   huffTab2D[4][4],
                              int*                in_data,
                              int                 num_val,
                              int                 p0_flag)
{
  int i;
  int offset = 0;
  int huffBits = 0;
  int escDataCntr = 0;

  int lavidx, lav, idx_position;
  int idx[2], SymBit[2];
  HCOD_TYPE hcod_type = p0_flag? DF_FP : DT_FP;
  unsigned int hcod2D_length, hcod2D_codeword, escape_length, escape_codeword;
  int aaEscData[MAXBANDS];

  lav = calc_lav(in_data + p0_flag, ((num_val - p0_flag) >> 1) << 1, lavtab_icc, &lavidx);
  if (lavidx == -1) return 100000; 

  escape_length   = huffICCTab.huff2D_ICC_ESC[hcod_type].length[lavidx];
  escape_codeword = huffICCTab.huff2D_ICC_ESC[hcod_type].value[lavidx];

  huffBits += write_bits(strm, huffLAVTab.value[lavidx], huffLAVTab.length[lavidx]);

  if (p0_flag) {
    huffBits += write_bits(strm, huffTabPt0->value [in_data[0]], huffTabPt0->length[in_data[0]]);
    offset = 1;
  }

  for (i = offset; i < num_val - 1; i += 2) {
    enc_sym_data(lav, in_data[i], in_data[i+1], idx, SymBit);
    idx_position = idx[0] * (lav + 1) + idx[1];
    hcod2D_length   = huffTab2D[lavidx][hcod_type].length[idx_position];
    hcod2D_codeword = huffTab2D[lavidx][hcod_type].value[idx_position];

    huffBits += write_bits(strm, hcod2D_codeword, hcod2D_length);
    if (hcod2D_length == escape_length && hcod2D_codeword == escape_codeword) {
      aaEscData[escDataCntr++] = in_data[i]   + lav;
      aaEscData[escDataCntr++] = in_data[i+1] + lav;
    }
    else {
      if (SymBit[0] >= 0) huffBits += write_bits(strm, SymBit[0], 1);
      if (SymBit[1] >= 0) huffBits += write_bits(strm, SymBit[1], 1);
    } 
  }

  if (escDataCntr) {
    huffBits += apply_pcm_coding(strm,
      aaEscData,
      NULL,
      0,
      escDataCntr,
      2 * lav + 1);
  }

  if ((num_val - offset) % 2)
    huffBits += huff_enc_icc_1D(strm,
    huffTabPt0,
    huffTabDiff,
    in_data + num_val - 1,
    1,
    0);

  return huffBits;
}

static int count_huff_icc(CODING_SCHEME*      coding_scheme,
                          const HUFF_ICC_TAB* huffTabPt0,
                          const HUFF_ICC_TAB* huffTabDiff,
                          int*                in_data,
                          int                 num_val,
                          int                 p0_flag )
{
  int bits_huff_1D, bits_huff_2D;

  bits_huff_1D = huff_enc_icc_1D(NULL,
    huffTabPt0,
    huffTabDiff,
    in_data,
    num_val,
    p0_flag);

  bits_huff_2D = huff_enc_icc_2D_FP(NULL,
    huffTabPt0,
    huffTabDiff,
    huffICCTab.huff2D_ICC,
    in_data,
    num_val,
    p0_flag);

  if (bits_huff_2D < bits_huff_1D) {
    *coding_scheme = HUFF_2D;
    return bits_huff_2D;
  }
  else {
    *coding_scheme = HUFF_1D;
    return bits_huff_1D;
  }
}

static int count_huff_cpc( const HUFF_CPC_TAB* huffTabPt0,
                           const HUFF_CPC_TAB* huffTabDiff,
                           int*                in_data,
                           int                 num_val,
                           int                 p0_flag )
{
  int i = 0, id = 0;
  int huffBits = 0;
  int offset = 0;

  if( p0_flag ) {
    huffBits += huffTabPt0->length[in_data[0]];
    offset = 1;
  }

  for( i=offset; i<num_val; i++ ) {

    id = in_data[i];

    if( id != 0 ) {
      if( id < 0 ) {
        id = -id;
      }
      huffBits += 1;
    }

    huffBits += huffTabDiff->length[id];
  }

  return huffBits;
}


static void huff_enc_cld(Stream*             strm,
                         CODING_SCHEME       coding_scheme,
                         const HUFF_CLD_TAB* huffTabPt0,
                         const HUFF_CLD_TAB* huffTabDiff,
                         int*                in_data,
                         int                 num_val,
                         int                 p0_flag )
{
  if (coding_scheme == HUFF_1D)
    huff_enc_cld_1D(strm,
    huffTabPt0,
    huffTabDiff,
    in_data,
    num_val,
    p0_flag);
  else
    huff_enc_cld_2D_FP(strm,
    huffTabPt0,
    huffTabDiff,
    huffCLDTab.huff2D_CLD,
    in_data,
    num_val,
    p0_flag);
}

static void huff_enc_icc(Stream*             strm,
                         CODING_SCHEME       coding_scheme,
                         const HUFF_ICC_TAB* huffTabPt0,
                         const HUFF_ICC_TAB* huffTabDiff,
                         int*                in_data,
                         int                 num_val,
                         int                 p0_flag )
{
  if (coding_scheme == HUFF_1D)
    huff_enc_icc_1D(strm,
    huffTabPt0,
    huffTabDiff,
    in_data,
    num_val,
    p0_flag);
  else
    huff_enc_icc_2D_FP(strm,
    huffTabPt0,
    huffTabDiff,
    huffICCTab.huff2D_ICC,
    in_data,
    num_val,
    p0_flag);
}

static void huff_enc_cpc( Stream*             strm,
                          const HUFF_CPC_TAB* huffTabPt0,
                          const HUFF_CPC_TAB* huffTabDiff,
                          int*                in_data,
                          int                 num_val,
                          int                 p0_flag )
{
  int i = 0, id = 0, id_sign = 0;
  int offset = 0;

  if( p0_flag ) {
    writeBits( strm, huffTabPt0->value [in_data[0]],
                     huffTabPt0->length[in_data[0]] );
    offset = 1;
  }

  for( i=offset; i<num_val; i++ ) {

    id = in_data[i];

    if( id != 0 ) {
      id_sign = 0;
      if( id < 0 ) {
        id = -id;
        id_sign = 1;
      }
    }

    writeBits( strm, huffTabDiff->value[id], huffTabDiff->length[id] );

    if( id != 0 ) {
      writeBits( strm, id_sign, 1 );
    }

  }
}

static int huff_enc_ipd_1D( Stream             *strm,
                          const HUFF_IPD_TAB* huffTabPt0,
                          const HUFF_IPD_TAB* huffTabDiff,
                          int*                in_data,
                          int                 num_val,
                          int                 p0_flag )
{
  int i = 0, id = 0;
  int offset = 0;
  int huffBits = 0;

  if( p0_flag ) {
    huffBits += write_bits( strm, huffTabPt0->value [in_data[0]],
                     huffTabPt0->length[in_data[0]] );
    offset = 1;
  }

  for( i=offset; i<num_val; i++ ) {
    id = in_data[i];
    huffBits += write_bits( strm, huffTabDiff->value[id], huffTabDiff->length[id] );
  }

  return huffBits;
}

static int huff_enc_ipd_2D_FP(Stream*             strm,
                              const HUFF_IPD_TAB* huffTabPt0,
                              const HUFF_IPD_TAB* huffTabDiff,
                              const HUFF_2D_TAB   huffTab2D[4][4],
                              int*                in_data,
                              int                 num_val,
                              int                 p0_flag)
{
  int i;
  int offset = 0;
  int huffBits = 0;
  int escDataCntr = 0;

  int lavidx, lav, idx_position, lavidx_code;
  int idx[2], SymBit[2];
  HCOD_TYPE hcod_type = p0_flag? DF_FP : DT_FP;
  unsigned int hcod2D_length, hcod2D_codeword, escape_length, escape_codeword;
  int aaEscData[MAXBANDS];

  lav = calc_lav(in_data + p0_flag, ((num_val - p0_flag) >> 1) << 1, lavtab_ipd, &lavidx);
  if (lavidx == -1) return 100000;

  escape_length   = huffIPDTab.huff2D_IPD_ESC[hcod_type].length[lavidx];
  escape_codeword = huffIPDTab.huff2D_IPD_ESC[hcod_type].value[lavidx];

  lavidx_code = (lavidx + 1)%4; 

  huffBits += write_bits(strm, huffLAVTab.value[lavidx_code], huffLAVTab.length[lavidx_code]);

  if (p0_flag) {
    huffBits += write_bits(strm, huffTabPt0->value[in_data[0]], huffTabPt0->length[in_data[0]]);
    offset = 1;
  }

  for (i = offset; i < num_val - 1; i += 2) {
    enc_sym_data(lav, in_data[i], in_data[i+1], idx, SymBit);
    idx_position = idx[0] * (lav + 1) + idx[1];
    hcod2D_length   = huffTab2D[lavidx][hcod_type].length[idx_position];
    hcod2D_codeword = huffTab2D[lavidx][hcod_type].value[idx_position];

    huffBits += write_bits(strm, hcod2D_codeword, hcod2D_length);
    if (hcod2D_length == escape_length && hcod2D_codeword == escape_codeword) {
      aaEscData[escDataCntr++] = in_data[i]   + lav;
      aaEscData[escDataCntr++] = in_data[i+1] + lav;
    }
    else if (SymBit[1] >= 0) huffBits += write_bits(strm, SymBit[1], 1);

  }

  if (escDataCntr) {
    huffBits += apply_pcm_coding(strm,
      aaEscData,
      NULL,
      0,
      escDataCntr,
      2 * lav + 1);
  }

  if ((num_val - offset) % 2)
    huffBits += huff_enc_ipd_1D(strm,
    huffTabPt0,
    huffTabDiff,
    in_data + num_val - 1,
    1,
    0);

  return huffBits;
}

static int count_huff_ipd(CODING_SCHEME*      coding_scheme,
                          const HUFF_IPD_TAB* huffTabPt0,
                          const HUFF_IPD_TAB* huffTabDiff,
                          int*                in_data,
                          int                 num_val,
                          int                 p0_flag )
{
  int bits_huff_1D, bits_huff_2D;

  bits_huff_1D = huff_enc_ipd_1D(NULL,
    huffTabPt0,
    huffTabDiff,
    in_data,
    num_val,
    p0_flag);

  bits_huff_2D = huff_enc_ipd_2D_FP(NULL,
    huffTabPt0,
    huffTabDiff,
    huffIPDTab.huff2D_IPD,
    in_data,
    num_val,
    p0_flag);

  if (bits_huff_2D < bits_huff_1D) {
    *coding_scheme = HUFF_2D;
    return bits_huff_2D;
  }
  else {
    *coding_scheme = HUFF_1D;
    return bits_huff_1D;
  }
}

static void huff_enc_ipd(Stream*             strm,
                         CODING_SCHEME       coding_scheme,
                         const HUFF_IPD_TAB* huffTabPt0,
                         const HUFF_IPD_TAB* huffTabDiff,
                         int*                in_data,
                         int                 num_val,
                         int                 p0_flag )
{
  if (coding_scheme == HUFF_1D)
    huff_enc_ipd_1D(strm,
    huffTabPt0,
    huffTabDiff,
    in_data,
    num_val,
    p0_flag);
  else
    huff_enc_ipd_2D_FP(strm,
    huffTabPt0,
    huffTabDiff,
    huffIPDTab.huff2D_IPD,
    in_data,
    num_val,
    p0_flag);
	
}


static int calc_huff_bits( int*           in_data_1,
                           int*           in_data_2,
                           DATA_TYPE      data_type,
                           CODING_SCHEME* coding_scheme,
                           DIFF_TYPE      diff_type_1,
                           DIFF_TYPE      diff_type_2,
                           int            num_val )
{
  int* p0_data_1[2] = {NULL, NULL};
  int* p0_data_2[2] = {NULL, NULL};

  int p0_flag[2];

  int i = 0;

  int df_rest_flag_1 = 0, df_rest_flag_2 = 0;

  int offset_1 = (diff_type_1 == DIFF_TIME) ? 1 : 0;
  int offset_2 = (diff_type_2 == DIFF_TIME) ? 1 : 0;

  int bit_count_huff    = 0;
  int bit_count_min     = 0;

  int num_val_1_int = 0;
  int num_val_2_int = 0;

  int* in_data_1_int = in_data_1 + offset_1;
  int* in_data_2_int = in_data_2 + offset_2;


  bit_count_huff = 1;

  num_val_1_int = num_val;
  num_val_2_int = num_val;

  p0_flag[0] = (diff_type_1 == DIFF_FREQ);
  p0_flag[1] = (diff_type_2 == DIFF_FREQ);

  switch( data_type ) {
  case t_CLD:
    if( in_data_1 != NULL ) bit_count_huff += count_huff_cld(coding_scheme, &huffCLDTab.huffPt0, &huffCLDTab.huffDiff[diff_type_1], in_data_1_int, num_val_1_int, p0_flag[0]);
    if( in_data_2 != NULL ) bit_count_huff += count_huff_cld(coding_scheme, &huffCLDTab.huffPt0, &huffCLDTab.huffDiff[diff_type_2], in_data_2_int, num_val_2_int, p0_flag[1]);
    break;

  case t_ICC:
    if( in_data_1 != NULL ) bit_count_huff += count_huff_icc(coding_scheme, &huffICCTab.huffPt0, &huffICCTab.huffDiff[diff_type_1], in_data_1_int, num_val_1_int, p0_flag[0] );
    if( in_data_2 != NULL ) bit_count_huff += count_huff_icc(coding_scheme, &huffICCTab.huffPt0, &huffICCTab.huffDiff[diff_type_2], in_data_2_int, num_val_2_int, p0_flag[1] );
    break;

  case t_CPC:
    if( in_data_1 != NULL ) bit_count_huff += count_huff_cpc( &huffCPCTab.huffPt0, &huffCPCTab.huffDiff[diff_type_1], in_data_1_int, num_val_1_int, p0_flag[0] );
    if( in_data_2 != NULL ) bit_count_huff += count_huff_cpc( &huffCPCTab.huffPt0, &huffCPCTab.huffDiff[diff_type_2], in_data_2_int, num_val_2_int, p0_flag[1] );
    break;

  case t_IPD:
    if( in_data_1 != NULL ) bit_count_huff += count_huff_ipd(coding_scheme, &huffIPDTab.huffPt0, &huffIPDTab.huffDiff[diff_type_1], in_data_1_int, num_val_1_int, p0_flag[0] );
    if( in_data_2 != NULL ) bit_count_huff += count_huff_ipd(coding_scheme, &huffIPDTab.huffPt0, &huffIPDTab.huffDiff[diff_type_2], in_data_2_int, num_val_2_int, p0_flag[1] );
    break;
  default:
    break;
  }


  return bit_count_huff;

}


static void apply_huff_coding( Stream*          strm,
                               int*             in_data_1,
                               int*             in_data_2,
                               DATA_TYPE        data_type,
                               CODING_SCHEME    coding_scheme,
                               DIFF_TYPE        diff_type_1,
                               DIFF_TYPE        diff_type_2,
                               int              num_val )
{
  int i = 0;

  int* p0_data_1[2] = {NULL, NULL};
  int* p0_data_2[2] = {NULL, NULL};

  int p0_flag[2];

  int df_rest_flag_1 = 0, df_rest_flag_2 = 0;

  int num_val_1_int = num_val;
  int num_val_2_int = num_val;

  int* in_data_1_int = in_data_1;
  int* in_data_2_int = in_data_2;

  if( diff_type_1 == DIFF_TIME ) in_data_1_int += 1;
  if( diff_type_2 == DIFF_TIME ) in_data_2_int += 1;


  writeBits(strm, coding_scheme, 1);

  p0_flag[0] = (diff_type_1 == DIFF_FREQ);
  p0_flag[1] = (diff_type_2 == DIFF_FREQ);

  switch( data_type ) {
  case t_CLD:
    if( in_data_1 != NULL ) huff_enc_cld( strm, coding_scheme, &huffCLDTab.huffPt0, &huffCLDTab.huffDiff[diff_type_1], in_data_1_int, num_val_1_int, p0_flag[0] );
    if( in_data_2 != NULL ) huff_enc_cld( strm, coding_scheme, &huffCLDTab.huffPt0, &huffCLDTab.huffDiff[diff_type_2], in_data_2_int, num_val_2_int, p0_flag[1] );
    break;

  case t_ICC:
    if( in_data_1 != NULL ) huff_enc_icc( strm, coding_scheme, &huffICCTab.huffPt0, &huffICCTab.huffDiff[diff_type_1], in_data_1_int, num_val_1_int, p0_flag[0] );
    if( in_data_2 != NULL ) huff_enc_icc( strm, coding_scheme, &huffICCTab.huffPt0, &huffICCTab.huffDiff[diff_type_2], in_data_2_int, num_val_2_int, p0_flag[1] );
    break;

  case t_CPC:
    if( in_data_1 != NULL ) huff_enc_cpc( strm, &huffCPCTab.huffPt0, &huffCPCTab.huffDiff[diff_type_1], in_data_1_int, num_val_1_int, p0_flag[0] );
    if( in_data_2 != NULL ) huff_enc_cpc( strm, &huffCPCTab.huffPt0, &huffCPCTab.huffDiff[diff_type_2], in_data_2_int, num_val_2_int, p0_flag[1] );
    break;

  case t_IPD:
    if( in_data_1 != NULL ) huff_enc_ipd( strm, coding_scheme, &huffIPDTab.huffPt0, &huffIPDTab.huffDiff[diff_type_1], in_data_1_int, num_val_1_int, p0_flag[0] );
    if( in_data_2 != NULL ) huff_enc_ipd( strm, coding_scheme, &huffIPDTab.huffPt0, &huffIPDTab.huffDiff[diff_type_2], in_data_2_int, num_val_2_int, p0_flag[1] );
    break;
  default:
    break;
  }

}


int EcDataPairEnc( Stream*    strm,
                   int        aaInData[][MAXBANDS],
                   int        aHistory[MAXBANDS],
                   DATA_TYPE  data_type,
                   int        setIdx,
                   int        startBand,
                   int        dataBands,
                   int        pair_flag,
                   int        coarse_flag,
                   int        independency_flag)
{
  int dummy = 0, reset = 0;
  int quant_levels = 0, quant_offset = 0, num_pcm_val = 0;

  int splitLsb_flag  = 0;
  int pcmCoding_flag = 0;

  int allowDiffTimeBack_flag = !independency_flag || (setIdx > 0);
  CODING_SCHEME coding_scheme, coding_scheme_tmp;

  int num_lsb_bits[2] = { 0, 0 };
  int num_pcm_bits    = 0;

  int aDataHist[MAXBANDS] = {0};
  int aaDataPair[2][MAXBANDS] = {{0}};

  int quant_data_lsb[2][MAXBANDS] = {{0}};
  int quant_data_msb[2][MAXBANDS] = {{0}};

  int quant_data_hist_lsb[MAXBANDS] = {0};
  int quant_data_hist_msb[MAXBANDS] = {0};

  int data_diff_freq   [2][MAXBANDS  ] = {{0}};
  int data_diff_time_bw[2][MAXBANDS+1] = {{0}};
  int data_diff_time_fw   [MAXBANDS+1] =  {0} ;

  int* p_data_pcm         [2] = {NULL};
  int* p_data_diff_freq   [2] = {NULL};
  int* p_data_diff_time_bw[2] = {NULL};
  int* p_data_diff_time_fw    =  NULL ;

  int min_bits_all = 0;
  int min_found    = 0;

  int min_bits_df_df   = -1;
  int min_bits_df_dt   = -1;
  int min_bits_dtbw_df = -1;
  int min_bits_dtfw_df = -1;
  int min_bits_dt_dt   = -1;
  


  switch( data_type ) {
  case t_CLD:
    if( coarse_flag ) {
      splitLsb_flag   =  0;
      quant_levels    = 15;
      quant_offset    =  7;
    }
    else {
      splitLsb_flag   =  0;
      quant_levels    = 31;
      quant_offset    = 15;
    }

    break;

  case t_ICC:
    if( coarse_flag ) {
      splitLsb_flag   =  0;
      quant_levels    =  4;
      quant_offset    =  0;
    }
    else {
      splitLsb_flag   =  0;
      quant_levels    =  8;
      quant_offset    =  0;
    }

    break;

  case t_CPC:
    if( coarse_flag ) {
      splitLsb_flag   =  0;
      quant_levels    = 26;
      quant_offset    =  0;
    }
    else {
      splitLsb_flag   =  1;
      quant_levels    = 51;
      quant_offset    =  0;
    }

    break;

  case t_IPD:
    if(coarse_flag)
    {
      splitLsb_flag   =  0;
      quant_levels    =  8;
      quant_offset    =  0;	  	  
    }
    else
    {
      splitLsb_flag   =  1;
      quant_levels    =  16;
      quant_offset    =  0;	  	  
    }

    break;

  default:
    fprintf( stderr, "Unknown type of data!\n" );
    return 0;
  }



  memcpy( aDataHist, aHistory+startBand, sizeof(int)*dataBands );

  memcpy( aaDataPair[0], aaInData[setIdx]+startBand, sizeof(int)*dataBands );
  p_data_pcm[0] = aaDataPair[0];
  if( pair_flag ) {
    memcpy( aaDataPair[1], aaInData[setIdx+1]+startBand, sizeof(int)*dataBands );
    p_data_pcm[1] = aaDataPair[1];
  }



  num_lsb_bits[0] = split_lsb( aaDataPair[0],
                               quant_offset,
                               splitLsb_flag ? 1 : 0,
                               dataBands,
                               quant_data_lsb[0],
                               quant_data_msb[0] );

  if( pair_flag ) {
    num_lsb_bits[1] = split_lsb( aaDataPair[1],
                                 quant_offset,
                                 splitLsb_flag ? 1 : 0,
                                 dataBands,
                                 quant_data_lsb[1],
                                 quant_data_msb[1] );
  }

  if( allowDiffTimeBack_flag ) {
    dummy = split_lsb( aDataHist,
                       quant_offset,
                       splitLsb_flag ? 1 : 0,
                       dataBands,
                       quant_data_hist_lsb,
                       quant_data_hist_msb );
  }



  calc_diff_freq( quant_data_msb[0],
                  data_diff_freq[0],
                  dataBands,
				  data_type );
  p_data_diff_freq[0] = data_diff_freq[0];

  if( pair_flag ) {
    calc_diff_freq( quant_data_msb[1],
                    data_diff_freq[1],
                    dataBands,
					data_type );
    p_data_diff_freq[1] = data_diff_freq[1];
  }



  if( allowDiffTimeBack_flag ) {
    calc_diff_time( quant_data_msb[0],
                    quant_data_hist_msb,
                    data_diff_time_bw[0]+1,
                    BACKWARDS,
                    dataBands,
					data_type );
    p_data_diff_time_bw[0] = data_diff_time_bw[0];
  }

  if( pair_flag ) {
    calc_diff_time( quant_data_msb[1],
                    quant_data_msb[0],
                    data_diff_time_bw[1]+1,
                    BACKWARDS,
                    dataBands,
					data_type );
    p_data_diff_time_bw[1] = data_diff_time_bw[1];

    calc_diff_time( quant_data_msb[1],
                    quant_data_msb[0],
                    data_diff_time_fw+1,
                    FORWARDS,
                    dataBands,
					data_type );
    p_data_diff_time_fw = data_diff_time_fw;
  }





  if( pair_flag ) {
    num_pcm_bits = calc_pcm_bits( 2*dataBands, quant_levels );
    num_pcm_val   = 2*dataBands;
  }
  else {
    num_pcm_bits = calc_pcm_bits( dataBands, quant_levels );
    num_pcm_val   = dataBands;
  }


  min_bits_all = num_pcm_bits;


  if( (p_data_diff_freq[0] != NULL) || (p_data_diff_freq[1] != NULL) ) {
    min_bits_df_df = calc_huff_bits( p_data_diff_freq[0],
                                     p_data_diff_freq[1],
                                     data_type,
                                     &coding_scheme_tmp,
                                     DIFF_FREQ, DIFF_FREQ,
                                     dataBands );

    if( pair_flag || allowDiffTimeBack_flag ) min_bits_df_df += 1;
    if( pair_flag                           ) min_bits_df_df += 1;

    if( min_bits_df_df < min_bits_all ) {
      min_bits_all = min_bits_df_df;
      coding_scheme = coding_scheme_tmp;
    }
  }



  if( (p_data_diff_freq[0] != NULL) || (p_data_diff_time_bw[1] != NULL) ) {
    min_bits_df_dt = calc_huff_bits( p_data_diff_freq[0],
                                     p_data_diff_time_bw[1],
                                     data_type,
                                     &coding_scheme_tmp,
                                     DIFF_FREQ, DIFF_TIME,
                                     dataBands );

    if( pair_flag || allowDiffTimeBack_flag ) min_bits_df_dt += 1;
    if( pair_flag                           ) min_bits_df_dt += 1;

    if( min_bits_df_dt < min_bits_all ) {
      min_bits_all = min_bits_df_dt;
      coding_scheme = coding_scheme_tmp;
    }
  }



  if( (p_data_diff_time_fw != NULL) || (p_data_diff_freq[1] != NULL) ) {
    min_bits_dtfw_df = calc_huff_bits( p_data_diff_time_fw,
                                       p_data_diff_freq[1],
                                       data_type,
                                       &coding_scheme_tmp,
                                       DIFF_TIME, DIFF_FREQ,
                                       dataBands );

    if( pair_flag || allowDiffTimeBack_flag ) min_bits_dtfw_df += 1;
    if( pair_flag && allowDiffTimeBack_flag ) min_bits_dtfw_df += 1;

    if( pair_flag && allowDiffTimeBack_flag ) {
      min_bits_dtfw_df += 1;
    }

    if( min_bits_dtfw_df < min_bits_all ) {
      min_bits_all = min_bits_dtfw_df;
      coding_scheme = coding_scheme_tmp;
    }
  }


  if( allowDiffTimeBack_flag ) {


    if( (p_data_diff_time_bw[0] != NULL) || (p_data_diff_freq[1] != NULL) ) {
      min_bits_dtbw_df = calc_huff_bits( p_data_diff_time_bw[0],
                                         p_data_diff_freq[1],
                                         data_type,
                                         &coding_scheme_tmp,
                                         DIFF_TIME, DIFF_FREQ,
                                         dataBands );

      min_bits_dtbw_df += 1;
      if( pair_flag ) {
        min_bits_dtbw_df += 2;
      }

      if( min_bits_dtbw_df < min_bits_all ) {
        min_bits_all = min_bits_dtbw_df;
        coding_scheme = coding_scheme_tmp;
      }
    }



    if( (p_data_diff_time_bw[0] != NULL) || (p_data_diff_time_bw[1] != NULL) ) {
      min_bits_dt_dt = calc_huff_bits( p_data_diff_time_bw[0],
                                       p_data_diff_time_bw[1],
                                       data_type,
                                       &coding_scheme_tmp,
                                       DIFF_TIME, DIFF_TIME,
                                       dataBands );

      min_bits_dt_dt += 1;
      if( pair_flag ) min_bits_dt_dt += 1;

      if( min_bits_dt_dt < min_bits_all ) {
        min_bits_all = min_bits_dt_dt;
        coding_scheme = coding_scheme_tmp;
      }
    }


  }









  pcmCoding_flag = (min_bits_all == num_pcm_bits);

  writeBits( strm, pcmCoding_flag, 1 );


  if( pcmCoding_flag ) {

/* 
    if( dataBands >= PBC_MIN_BANDS ) {
      writeBits( strm, 0, 1 );
    }
*/

    apply_pcm_coding( strm,
                      p_data_pcm[0],
                      p_data_pcm[1],
                      quant_offset,
                      num_pcm_val,
                      quant_levels );
  }
  else {



    min_found = 0;


    if( min_bits_all == min_bits_df_df ) {

      if( pair_flag || allowDiffTimeBack_flag ) {
        writeBits( strm, DIFF_FREQ, 1 );
      }

      if( pair_flag ) {
        writeBits( strm, DIFF_FREQ, 1 );
      }

      apply_huff_coding( strm,
                         p_data_diff_freq[0],
                         p_data_diff_freq[1],
                         data_type,
                         coding_scheme,
                         DIFF_FREQ, DIFF_FREQ,
                         dataBands );

      min_found = 1;
    }



    if( !min_found && (min_bits_all == min_bits_df_dt) ) {

      if( pair_flag || allowDiffTimeBack_flag ) {
        writeBits( strm, DIFF_FREQ, 1 );
      }

      if( pair_flag ) {
        writeBits( strm, DIFF_TIME, 1 );
      }

      apply_huff_coding( strm,
                         p_data_diff_freq[0],
                         p_data_diff_time_bw[1],
                         data_type,
                         coding_scheme,
                         DIFF_FREQ, DIFF_TIME,
                         dataBands );

      min_found = 1;
    }



    if( !min_found && (min_bits_all == min_bits_dtfw_df) ) {

      if( pair_flag || allowDiffTimeBack_flag ) {
        writeBits( strm, DIFF_TIME, 1 );
      }

      if( pair_flag && allowDiffTimeBack_flag ) {
        writeBits( strm, DIFF_FREQ, 1 );
      }

      apply_huff_coding( strm,
                         p_data_diff_time_fw,
                         p_data_diff_freq[1],
                         data_type,
                         coding_scheme,
                         DIFF_TIME, DIFF_FREQ,
                         dataBands );

      if( pair_flag && allowDiffTimeBack_flag ) {
        writeBits( strm, FORWARDS, 1 );
      }

      min_found = 1;
    }


    if( allowDiffTimeBack_flag ) {


      if( !min_found && (min_bits_all == min_bits_dtbw_df) ) {

        writeBits( strm, DIFF_TIME, 1 );

        if( pair_flag ) {
          writeBits( strm, DIFF_FREQ, 1 );
        }

        apply_huff_coding( strm,
                           p_data_diff_time_bw[0],
                           p_data_diff_freq[1],
                           data_type,
                           coding_scheme,
                           DIFF_TIME, DIFF_FREQ,
                           dataBands );

        if( pair_flag ) {
          writeBits( strm, BACKWARDS, 1 );
        }

        min_found = 1;
      }



      if( !min_found && (min_bits_all == min_bits_dt_dt) ) {

        writeBits( strm, DIFF_TIME, 1 );

        if( pair_flag ) {
          writeBits( strm, DIFF_TIME, 1 );
        }

        apply_huff_coding( strm,
                           p_data_diff_time_bw[0],
                           p_data_diff_time_bw[1],
                           data_type,
                           coding_scheme,
                           DIFF_TIME, DIFF_TIME,
                           dataBands );
      }


    }





    if( splitLsb_flag ) {

      apply_lsb_coding( strm,
                        quant_data_lsb[0],
                        1,
                        dataBands );

      if( pair_flag ) {
        apply_lsb_coding( strm,
                          quant_data_lsb[1],
                          1,
                          dataBands );
      }

    }


  }


  return reset;
}
