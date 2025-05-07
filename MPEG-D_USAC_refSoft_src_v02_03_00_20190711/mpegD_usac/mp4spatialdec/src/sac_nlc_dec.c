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



#include <math.h>
#include <string.h>
#include <assert.h>

#include "sac_nlc_dec.h"
#include "sac_huff_nodes.h"
#include "sac_types.h"
#include "sac_bitinput.h"


#define min(a,b) (((a) < (b)) ? (a) : (b))


extern const HUFF_CLD_NODES huffCLDNodes;
extern const HUFF_ICC_NODES huffICCNodes;
extern const HUFF_CPC_NODES huffCPCNodes;

extern const HUFF_PT0_NODES huffPart0Nodes;
extern const HUFF_RES_NODES huffReshapeNodes;

extern const HUFF_PT0_NODES huffPilotNodes;
extern const HUFF_LAV_NODES huffLavIdxNodes;

extern const HUFF_IPD_NODES huffIPDNodes;


static int pcm_decode( HANDLE_S_BITINPUT strm,
                       int*    out_data_1,
                       int*    out_data_2,
                       int     offset,
                       int     num_val,
                       int     num_levels )
{
  int i = 0, j = 0, idx = 0;
  int max_grp_len = 0, grp_len = 0, next_val = 0, grp_val = 0;
  unsigned long data = 0;
  
  float ld_nlev = 0.f;
  
  int pcm_chunk_size[7] = { 0 };
  
  
  switch( num_levels )
    {
    case  3: max_grp_len = 5; break;
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
    default: assert(0);
    }
  
  ld_nlev = (float)(log((float)num_levels)/log(2.f));
  
  for( i=1; i<=max_grp_len; i++ ) {
    pcm_chunk_size[i] = (int) ceil( (float)(i) * ld_nlev );
  }
  
  
  for( i=0; i<num_val; i+=max_grp_len ) {
    grp_len = min( max_grp_len, num_val-i );
    data = s_GetBits( strm, pcm_chunk_size[grp_len]);
    
    grp_val = data;
    
    for( j=0; j<grp_len; j++ ) {
      idx = i+(grp_len-j-1);
      next_val = grp_val%num_levels;
      
      if( out_data_2 == NULL ) {
        out_data_1[idx] = next_val - offset;
      }
      else if( out_data_1 == NULL ) {
        out_data_2[idx] = next_val - offset;
      }
      else {
        if(idx%2) {
          out_data_2[idx/2] = next_val - offset;
        }
        else {
          out_data_1[idx/2] = next_val - offset;
        }
      }
      
      grp_val = (grp_val-next_val)/num_levels;
    }
  }
  
  return 1;
}


static int huff_read( HANDLE_S_BITINPUT  strm,
                      const int        (*nodeTab)[][2],
                      int*             out_data )
{
  int node = 0;
  unsigned long next_bit = 0;
  
  do {
    next_bit = s_GetBits(strm, 1);
    node = (*nodeTab)[node][next_bit];
  } while( node > 0 );
  
  *out_data = node;
  
  return 1;
}

static int huff_read_2D( HANDLE_S_BITINPUT  strm,
                         const int        (*nodeTab)[][2],
                         int              out_data[2],
                         int*             escape )
     
{
  int huff_2D_8bit = 0;
  int node = 0;
  
  if( !huff_read(strm, nodeTab, &node) ) return 0;
  *escape = (node == 0);
  
  if( *escape ) {
    out_data[0] = 0;
    out_data[1] = 1;
  }
  else {
    huff_2D_8bit = -(node+1);
	out_data[0] = huff_2D_8bit >> 4;
    out_data[1] = huff_2D_8bit & 0xf;
  }
  
  return 1;
}

static int sym_restore( HANDLE_S_BITINPUT strm,
                        int     lav,
                        int     data[2] )
{
  int tmp = 0;
  unsigned long sym_bit = 0;
  
  int sum_val  = data[0]+data[1];
  int diff_val = data[0]-data[1];
  
  if( sum_val > lav ) {
    data[0] = - sum_val + (2*lav+1);
    data[1] = - diff_val;
  }
  else {
    data[0] = sum_val;
    data[1] = diff_val;
  }
  
  if( data[0]+data[1] != 0 ) {
    sym_bit = s_GetBits(strm, 1);
    if( sym_bit ) {
      data[0] = -data[0];
      data[1] = -data[1];
    }
  }
  
  if( data[0]-data[1] != 0 ) {
    sym_bit = s_GetBits(strm, 1);
    if( sym_bit ) {
      tmp     = data[0];
      data[0] = data[1];
      data[1] = tmp;
    }
  }
  
  return 1;
}

static int sym_restoreIPD( HANDLE_S_BITINPUT strm,
                        int     lav,
                        int     data[2] )
{
  int tmp = 0;
  unsigned long sym_bit = 0;
  
  int sum_val  = data[0]+data[1];
  int diff_val = data[0]-data[1];
  
  if( sum_val > lav ) {
    data[0] = - sum_val + (2*lav+1);
    data[1] = - diff_val;
  }
  else {
    data[0] = sum_val;
    data[1] = diff_val;
  }
  /* 2DSIGN */

  if( data[0]-data[1] != 0 ) {
    sym_bit = s_GetBits(strm, 1);
    if( sym_bit ) {
      tmp     = data[0];
      data[0] = data[1];
      data[1] = tmp;
    }
  }
  
  return 1;
}

static int huff_dec_pilot( HANDLE_S_BITINPUT      strm,
                           const int		  (*nodeTab)[][2],
                           int*                   pilot_data )
{
  int node = 0;
  
  if( !huff_read(strm, nodeTab, &node) ) return 0;
  *pilot_data = -(node+1);
  
  return 1;
}


static int huff_dec_cld_1D( HANDLE_S_BITINPUT        strm,
                            const HUFF_CLD_NOD_1D* huffNodes,
                            int*                   out_data,
                            int                    num_val,
                            int                    p0_flag,
                            DIFF_TYPE              diff_type )
{
  int i = 0, node = 0, offset = 0;
  int od = 0, od_sign = 0;
  unsigned long data = 0;
  
  if( p0_flag ) {
    if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffPart0Nodes.cld, &node) ) return 0;
    out_data[0] = -(node+1);
    offset = 1;
  }
  
  for( i=offset; i<num_val; i++ ) {
    
    if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffNodes->nodeTab, &node) ) return 0;
    od = -(node+1);
    
    if( od != 0 ) {
      data = s_GetBits(strm, 1);
      od_sign = data;
      
      if( od_sign ) od = -od;
    }
    
    out_data[i] = od;
  }
  
  return 1;
}

static int huff_dec_ipd_1D( HANDLE_S_BITINPUT        strm,
                            const HUFF_IPD_NOD_1D* huffNodes,
                            int*                   out_data,
                            int                    num_val,
                            int                    p0_flag,
                            DIFF_TYPE              diff_type )
{
  int i = 0, node = 0, offset = 0;
  int od = 0;
    
  if( p0_flag ) {
    if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffIPDNodes.hP0.nodeTab, &node) ) return 0;
    out_data[0] = -(node+1);
    offset = 1;
  }
  
  for( i=offset; i<num_val; i++ ) {
    
    if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffNodes->nodeTab, &node) ) return 0;
	od = -(node+1);
    out_data[i] = od;
  }
  
  return 1;
}


static int huff_dec_icc_1D( HANDLE_S_BITINPUT        strm,
                            const HUFF_ICC_NOD_1D* huffNodes,
                            int*                   out_data,
                            int                    num_val,
                            int                    p0_flag,
                            DIFF_TYPE              diff_type )
{
  int i = 0, node = 0, offset = 0;
  int od = 0, od_sign = 0;
  unsigned long data = 0;
  
  if( p0_flag ) {
    if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffPart0Nodes.icc, &node) ) return 0;
    out_data[0] = -(node+1);
    offset = 1;
  }
  
  for( i=offset; i<num_val; i++ ) {
    
    if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffNodes->nodeTab, &node) ) return 0;
    od = -(node+1);
    
    if( od != 0 ) {
      data = s_GetBits(strm, 1);
      od_sign = data;
      
      if( od_sign ) od = -od;
    }
    
    out_data[i] = od;
  }
  
  return 1;
}


static int huff_dec_cpc_1D( HANDLE_S_BITINPUT        strm,
                            const HUFF_CPC_NOD_1D* huffNodes,
                            int*                   out_data,
                            int                    num_val,
                            int                    p0_flag,
                            DIFF_TYPE              diff_type )
{
  int i = 0, node = 0, offset = 0;
  int od = 0, od_sign = 0;
  unsigned long data = 0;
  
  if( p0_flag ) {
    if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffPart0Nodes.cpc, &node) ) return 0;
    out_data[0] = -(node+1);
    offset = 1;
  }
  
  for( i=offset; i<num_val; i++ ) {
    
    if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffNodes->nodeTab, &node) ) return 0;
    od = -(node+1);
    
    if( od != 0 ) {
      data = s_GetBits(strm, 1);
      od_sign = data;
      
      if( od_sign ) od = -od;
    }
    
    out_data[i] = od;
  }
  
  return 1;
}


static int huff_dec_cld_2D( HANDLE_S_BITINPUT        strm,
                            const HUFF_CLD_NOD_2D* huffNodes,
                            int                    out_data[][2],
                            int                    num_val,
                            int                    stride,
                            int*                   p0_data[2] )
{
  int i = 0, lav = 0, escape = 0, escCntr = 0;
  int node = 0;
  unsigned long data = 0;
  
  int esc_data[MAXBANDS][2] = {{0}};
  int escIdx[MAXBANDS] = {0};
  
  if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffLavIdxNodes.nodeTab, &node) ) return 0;
  data = -(node+1);
  
  lav = 2*data + 3;  
  
  
  if( p0_data[0] != NULL ) {
    if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffPart0Nodes.cld, &node) ) return 0;
    *p0_data[0] = -(node+1);
  }
  if( p0_data[1] != NULL ) {
    if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffPart0Nodes.cld, &node) ) return 0;
    *p0_data[1] = -(node+1);
  }
  
  for( i=0; i<num_val; i+=stride ) {
    
    switch( lav ) {
    case 3:
      if( !huff_read_2D(strm, (HANDLE_HUFF_NODE)&huffNodes->lav3, out_data[i], &escape) ) return 0;
      break;
    case 5:
      if( !huff_read_2D(strm, (HANDLE_HUFF_NODE)&huffNodes->lav5, out_data[i], &escape) ) return 0;
      break;
    case 7:
      if( !huff_read_2D(strm, (HANDLE_HUFF_NODE)&huffNodes->lav7, out_data[i], &escape) ) return 0;
      break;
    case 9:
      if( !huff_read_2D(strm, (HANDLE_HUFF_NODE)&huffNodes->lav9, out_data[i], &escape) ) return 0;
      break;
    default:
      break;
    }
    
    if( escape ) {
      escIdx[escCntr++] = i;
    }
    else {
      if( !sym_restore(strm, lav, out_data[i]) ) return 0;
    }
    
  }
  
  if( escCntr > 0 ) {
    if( !pcm_decode(strm, esc_data[0], esc_data[1], 0, 2*escCntr, (2*lav+1)) ) return 0;
    
    for( i=0; i<escCntr; i++ ) {
      out_data[escIdx[i]][0] = esc_data[0][i] - lav;
      out_data[escIdx[i]][1] = esc_data[1][i] - lav;
    }
  }
  
  return 1;
}


static int huff_dec_icc_2D( HANDLE_S_BITINPUT        strm,
                            const HUFF_ICC_NOD_2D* huffNodes,
                            int                    out_data[][2],
                            int                    num_val,
                            int                    stride,
                            int*                   p0_data[2] )
{
  int i = 0, lav = 0, escape = 0, escCntr = 0;
  int node = 0;
  unsigned long data = 0;
  
  int esc_data[2][MAXBANDS] = {{0}};
  int escIdx[MAXBANDS] = {0};
  
  if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffLavIdxNodes.nodeTab, &node) ) return 0;
  data = -(node+1);
  
  lav = 2*data + 1;  
  
  if( p0_data[0] != NULL ) {
    if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffPart0Nodes.icc, &node) ) return 0;
    *p0_data[0] = -(node+1);
  }
  if( p0_data[1] != NULL ) {
    if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffPart0Nodes.icc, &node) ) return 0;
    *p0_data[1] = -(node+1);
  }
  
  for( i=0; i<num_val; i+=stride ) {
    
    switch( lav ) {
    case 1:
      if( !huff_read_2D(strm, (HANDLE_HUFF_NODE)&huffNodes->lav1, out_data[i], &escape) ) return 0;
      break;
    case 3:
      if( !huff_read_2D(strm, (HANDLE_HUFF_NODE)&huffNodes->lav3, out_data[i], &escape) ) return 0;
      break;
    case 5:
      if( !huff_read_2D(strm, (HANDLE_HUFF_NODE)&huffNodes->lav5, out_data[i], &escape) ) return 0;
      break;
    case 7:
      if( !huff_read_2D(strm, (HANDLE_HUFF_NODE)&huffNodes->lav7, out_data[i], &escape) ) return 0;
      break;
    }
    
    if( escape ) {
      escIdx[escCntr++] = i;
    }
    else {
      if( !sym_restore(strm, lav, out_data[i]) ) return 0;
    }
    
  }
  
  if( escCntr > 0 ) {
    if( !pcm_decode(strm, esc_data[0], esc_data[1], 0, 2*escCntr, (2*lav+1)) ) return 0;
    
    for( i=0; i<escCntr; i++ ) {
      out_data[escIdx[i]][0] = esc_data[0][i] - lav;
      out_data[escIdx[i]][1] = esc_data[1][i] - lav;
    }
  }
  
  return 1;
}

static int huff_dec_ipd_2D( HANDLE_S_BITINPUT        strm,
                            const HUFF_IPD_NOD_2D* huffNodes,
                            int                    out_data[][2],
                            int                    num_val,
                            int                    stride,
                            int*                   p0_data[2] )
{
  int i = 0, lav = 0, escape = 0, escCntr = 0;
  int node = 0;
  unsigned long data = 0;
  
  int esc_data[2][MAXBANDS] = {{0}};
  int escIdx[MAXBANDS] = {0};
  
  if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffLavIdxNodes.nodeTab, &node) ) return 0;
 
  data = -(node+1);
  if(data==0)
	  data = 3;
  else
	  data--;

  lav = 2*data + 1;  
  
  if( p0_data[0] != NULL ) {
    if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffIPDNodes.hP0.nodeTab, &node) ) return 0;
    *p0_data[0] = -(node+1);
  }
  if( p0_data[1] != NULL ) {
    if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffIPDNodes.hP0.nodeTab, &node) ) return 0;
    *p0_data[1] = -(node+1);
  }

  for( i=0; i<num_val; i+=stride ) {
    
    switch( lav ) {
    case 1:
      if( !huff_read_2D(strm, (HANDLE_HUFF_NODE)&huffNodes->lav1, out_data[i], &escape ) ) return 0;
      break;
    case 3:
      if( !huff_read_2D(strm, (HANDLE_HUFF_NODE)&huffNodes->lav3, out_data[i], &escape ) ) return 0;
      break;
    case 5:
      if( !huff_read_2D(strm, (HANDLE_HUFF_NODE)&huffNodes->lav5, out_data[i], &escape ) ) return 0;
      break;
    case 7:
      if( !huff_read_2D(strm, (HANDLE_HUFF_NODE)&huffNodes->lav7, out_data[i], &escape ) ) return 0;
      break;
    }
 
    if( escape ) {
      escIdx[escCntr++] = i;
    }
    else {
      if( !sym_restoreIPD(strm, lav, out_data[i]) ) return 0;
    }
    
  }
  
  if( escCntr > 0 ) {
    if( !pcm_decode(strm, esc_data[0], esc_data[1], 0, 2*escCntr, (2*lav+1)) ) return 0;
    
    for( i=0; i<escCntr; i++ ) {
      out_data[escIdx[i]][0] = esc_data[0][i] - lav;
      out_data[escIdx[i]][1] = esc_data[1][i] - lav;
    }
  }
  
  return 1;
}

static int huff_dec_cpc_2D( HANDLE_S_BITINPUT        strm,
                            const HUFF_CPC_NOD_2D* huffNodes,
                            int                    out_data[][2],
                            int                    num_val,
                            int                    stride,
                            int*                   p0_data[2] )
{
  int i = 0, lav = 0, escape = 0, escCntr = 0;
  int node = 0;
  unsigned long data = 0;
  
  int esc_data[2][MAXBANDS] = {{0}};
  int escIdx[MAXBANDS] = {0};
  
  if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffLavIdxNodes.nodeTab, &node) ) return 0;
  data = -(node+1);
  
  lav = 3*data + 3;
  
  if( p0_data[0] != NULL ) {
    if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffPart0Nodes.cpc, &node) ) return 0;
    *p0_data[0] = -(node+1);
  }
  if( p0_data[1] != NULL ) {
    if( !huff_read(strm, (HANDLE_HUFF_NODE)&huffPart0Nodes.cpc, &node) ) return 0;
    *p0_data[1] = -(node+1);
  }
  
  for( i=0; i<num_val; i+=stride ) {
    
    switch( lav ) {
    case 3:
      if( !huff_read_2D(strm, (HANDLE_HUFF_NODE)&huffNodes->lav3 , out_data[i], &escape) ) return 0;
      break;
    case 6:
      if( !huff_read_2D(strm, (HANDLE_HUFF_NODE)&huffNodes->lav6 , out_data[i], &escape) ) return 0;
      break;
    case 9:
      if( !huff_read_2D(strm, (HANDLE_HUFF_NODE)&huffNodes->lav9 , out_data[i], &escape) ) return 0;
      break;
    case 12:
      if( !huff_read_2D(strm, (HANDLE_HUFF_NODE)&huffNodes->lav12, out_data[i], &escape) ) return 0;
      break;
    }
    
    if( escape ) {
      escIdx[escCntr++] = i;
    }
    else {
      if( !sym_restore(strm, lav, out_data[i]) ) return 0;
    }
    
  }
  
  if( escCntr > 0 ) {
    if( !pcm_decode(strm, esc_data[0], esc_data[1], 0, 2*escCntr, (2*lav+1)) ) return 0;
    
    for( i=0; i<escCntr; i++ ) {
      out_data[escIdx[i]][0] = esc_data[0][i] - lav;
      out_data[escIdx[i]][1] = esc_data[1][i] - lav;
    }
  }
  
  return 1;
}


static int huff_decode( HANDLE_S_BITINPUT strm,
                        int*            out_data_1,
                        int*            out_data_2,
                        DATA_TYPE       data_type,
                        DIFF_TYPE       diff_type_1,
                        DIFF_TYPE       diff_type_2,
                        int		pilotCoding_flag,
                        int*		pilot_data,
                        int             num_val,
                        CODING_SCHEME   *cdg_scheme )
{
  DIFF_TYPE diff_type;
  
  int lav = 0, i = 0, node = 0;
  unsigned long data = 0;
  
  int pair_vec[MAXBANDS][2];
  
  int* p0_data_1[2] = {NULL, NULL};
  int* p0_data_2[2] = {NULL, NULL};
  
  int p0_flag[2];
  
  int num_val_1_int = num_val;
  int num_val_2_int = num_val;
  
  int* out_data_1_int = out_data_1;
  int* out_data_2_int = out_data_2;
  
  int df_rest_flag_1 = 0;
  int df_rest_flag_2 = 0;
  
  int hufYY1;
  int hufYY2;
  int hufYY;
  
  if( pilotCoding_flag ) {
    switch( data_type ) {
    case t_CLD:
      if( out_data_1 != NULL ) {
        if( !huff_dec_pilot(strm, (HANDLE_HUFF_NODE)&huffPilotNodes.cld, pilot_data) ) return 0;
      }
      break;
      
    case t_ICC:
      if( out_data_1 != NULL ) {
        if( !huff_dec_pilot(strm, (HANDLE_HUFF_NODE)&huffPilotNodes.icc, pilot_data) ) return 0;
      }
      break;
      
    case t_CPC:
      if( out_data_1 != NULL ) {
        if( !huff_dec_pilot(strm, (HANDLE_HUFF_NODE)&huffPilotNodes.cpc, pilot_data) ) return 0;
      }
      break;

    default:
      if( out_data_1 != NULL ) {
        return 0;
      }
      break;

    }
  }
  
  data = s_GetBits(strm, 1);
  *cdg_scheme = data << PAIR_SHIFT;
  
  if( *cdg_scheme >> PAIR_SHIFT == HUFF_2D ) {
    if( (out_data_1 != NULL) && (out_data_2 != NULL) ) {
      data = s_GetBits(strm, 1);
      *cdg_scheme |= data;
    }
    else {
      *cdg_scheme |= FREQ_PAIR;
    }
  }
  
  if( pilotCoding_flag ) {
    hufYY1 = PCM_PLT;
    hufYY2 = PCM_PLT;
  }
  else {
    hufYY1 = diff_type_1;
    hufYY2 = diff_type_2;
  }
  
  switch( *cdg_scheme >> PAIR_SHIFT ) {
  case HUFF_1D:
    
    p0_flag[0] = (diff_type_1 == DIFF_FREQ) && !pilotCoding_flag;
    p0_flag[1] = (diff_type_2 == DIFF_FREQ) && !pilotCoding_flag;
    
    switch( data_type ) {
    case t_CLD:
      if( out_data_1 != NULL ) {
        if( !huff_dec_cld_1D(strm, &huffCLDNodes.h1D[hufYY1], out_data_1, num_val_1_int, p0_flag[0], diff_type_1) ) return 0;
      }
      if( out_data_2 != NULL ) {
        if( !huff_dec_cld_1D(strm, &huffCLDNodes.h1D[hufYY2], out_data_2, num_val_2_int, p0_flag[1], diff_type_2) ) return 0;
      }
      
      break;
      
    case t_ICC:
      if( out_data_1 != NULL ) {
        if( !huff_dec_icc_1D(strm, &huffICCNodes.h1D[hufYY1], out_data_1, num_val_1_int, p0_flag[0], diff_type_1) ) return 0;
      }
      if( out_data_2 != NULL ) {
        if( !huff_dec_icc_1D(strm, &huffICCNodes.h1D[hufYY2], out_data_2, num_val_2_int, p0_flag[1], diff_type_2) ) return 0;
      }
      
      break;
      
    case t_CPC:
      if( out_data_1 != NULL ) {
        if( !huff_dec_cpc_1D(strm, &huffCPCNodes.h1D[hufYY1], out_data_1, num_val_1_int, p0_flag[0], diff_type_1) ) return 0;
      }
      if( out_data_2 != NULL ) {
        if( !huff_dec_cpc_1D(strm, &huffCPCNodes.h1D[hufYY2], out_data_2, num_val_2_int, p0_flag[1], diff_type_2) ) return 0;
      }
      
      break;

    case t_IPD:
      if( out_data_1 != NULL ) {
		if( !huff_dec_ipd_1D(strm, &huffIPDNodes.h1D[hufYY1], out_data_1, num_val_1_int, p0_flag[0], diff_type_1) ) return 0;
      }
      if( out_data_2 != NULL ) {
        if( !huff_dec_ipd_1D(strm, &huffIPDNodes.h1D[hufYY2], out_data_2, num_val_2_int, p0_flag[1], diff_type_2) ) return 0;
      }
      
      break;

    default:
      break;
    }
    
    break;
    
  case HUFF_2D:
    
    switch( *cdg_scheme & PAIR_MASK ) {
      
    case FREQ_PAIR:
      
      if( out_data_1 != NULL ) {
        if( !pilotCoding_flag && diff_type_1 == DIFF_FREQ ) {
          p0_data_1[0] = &out_data_1[0];
          p0_data_1[1] = NULL;
          
          num_val_1_int  -= 1;
          out_data_1_int += 1;
        }
        df_rest_flag_1 = num_val_1_int % 2;
        if( df_rest_flag_1 ) num_val_1_int -= 1;
      }
      if( out_data_2 != NULL ) {
        if( !pilotCoding_flag && diff_type_2 == DIFF_FREQ ) {
          p0_data_2[0] = NULL;
          p0_data_2[1] = &out_data_2[0];
          
          num_val_2_int  -= 1;
          out_data_2_int += 1;
        }
        df_rest_flag_2 = num_val_2_int % 2;
        if( df_rest_flag_2 ) num_val_2_int -= 1;
      }
      
      switch( data_type ) {
      case t_CLD:
        
        if( out_data_1 != NULL ) {
          if( !huff_dec_cld_2D(strm, &huffCLDNodes.h2D[hufYY1][FREQ_PAIR], pair_vec  , num_val_1_int, 2, p0_data_1) ) return 0;
          if( df_rest_flag_1 ) {
            if( !huff_dec_cld_1D(strm, &huffCLDNodes.h1D[hufYY1], out_data_1_int+num_val_1_int, 1, 0, diff_type_1) ) return 0;
          }
        }
        if( out_data_2 != NULL ) {
          if( !huff_dec_cld_2D(strm, &huffCLDNodes.h2D[hufYY2][FREQ_PAIR], pair_vec+1, num_val_2_int, 2, p0_data_2) ) return 0;
          if( df_rest_flag_2 ) {
            if( !huff_dec_cld_1D(strm, &huffCLDNodes.h1D[hufYY2], out_data_2_int+num_val_2_int, 1, 0, diff_type_2) ) return 0;
          }
        }
        break;
        
      case t_ICC:
        if( out_data_1 != NULL ) {
          if( !huff_dec_icc_2D(strm, &huffICCNodes.h2D[hufYY1][FREQ_PAIR], pair_vec  , num_val_1_int, 2, p0_data_1) ) return 0;
          if( df_rest_flag_1 ) {
            if( !huff_dec_icc_1D(strm, &huffICCNodes.h1D[hufYY1], out_data_1_int+num_val_1_int, 1, 0, diff_type_1) ) return 0;
          }
        }
        if( out_data_2 != NULL ) {
          if( !huff_dec_icc_2D(strm, &huffICCNodes.h2D[hufYY2][FREQ_PAIR], pair_vec+1, num_val_2_int, 2, p0_data_2) ) return 0;
          if( df_rest_flag_2 ) {
            if( !huff_dec_icc_1D(strm, &huffICCNodes.h1D[hufYY2], out_data_2_int+num_val_2_int, 1, 0, diff_type_2) ) return 0;
          }
        }
        break;
        
      case t_CPC:
        if( out_data_1 != NULL ) {
          if( !huff_dec_cpc_2D(strm, &huffCPCNodes.h2D[hufYY1][FREQ_PAIR], pair_vec  , num_val_1_int, 2, p0_data_1) ) return 0;
          if( df_rest_flag_1 ) {
            if( !huff_dec_cpc_1D(strm, &huffCPCNodes.h1D[hufYY1], out_data_1_int+num_val_1_int, 1, 0, diff_type_1) ) return 0;
          }
        }
        if( out_data_2 != NULL ) {
          if( !huff_dec_cpc_2D(strm, &huffCPCNodes.h2D[hufYY2][FREQ_PAIR], pair_vec+1, num_val_2_int, 2, p0_data_2) ) return 0;
          if( df_rest_flag_2 ) {
            if( !huff_dec_cpc_1D(strm, &huffCPCNodes.h1D[hufYY2], out_data_2_int+num_val_2_int, 1, 0, diff_type_2) ) return 0;
          }
        }
        break;

      case t_IPD:
        if( out_data_1 != NULL ) {
          if( !huff_dec_ipd_2D(strm, &huffIPDNodes.h2D[hufYY1][FREQ_PAIR], pair_vec  , num_val_1_int, 2, p0_data_1) ) return 0;
          if( df_rest_flag_1 ) {
            if( !huff_dec_ipd_1D(strm, &huffIPDNodes.h1D[hufYY1], out_data_1_int+num_val_1_int, 1, 0, diff_type_1) ) return 0;
          }
        }
        if( out_data_2 != NULL ) {
          if( !huff_dec_ipd_2D(strm, &huffIPDNodes.h2D[hufYY2][FREQ_PAIR], pair_vec+1, num_val_2_int, 2, p0_data_2) ) return 0;
          if( df_rest_flag_2 ) {
            if( !huff_dec_ipd_1D(strm, &huffIPDNodes.h1D[hufYY2], out_data_2_int+num_val_2_int, 1, 0, diff_type_2) ) return 0;
          }
        }
        break;

      default:
        break;
      }
      
      if( out_data_1 != NULL ) {
        for( i=0; i<num_val_1_int-1; i+=2 ) {
          out_data_1_int[i  ] = pair_vec[i][0];
          out_data_1_int[i+1] = pair_vec[i][1];
        }
      }
      if( out_data_2 != NULL ) {
        for( i=0; i<num_val_2_int-1; i+=2 ) {
          out_data_2_int[i  ] = pair_vec[i+1][0];
          out_data_2_int[i+1] = pair_vec[i+1][1];
        }
      }
            
      break;
      
    case TIME_PAIR:
      
      if( !pilotCoding_flag && ((diff_type_1 == DIFF_FREQ) || (diff_type_2 == DIFF_FREQ)) ) {
        p0_data_1[0] = &out_data_1[0];
        p0_data_1[1] = &out_data_2[0];
        
        out_data_1_int += 1;
        out_data_2_int += 1;
        
        num_val_1_int  -= 1;
      }
      
      if( (diff_type_1 == DIFF_TIME) || (diff_type_2 == DIFF_TIME) ) {
        diff_type = DIFF_TIME;
      }
      else {
        diff_type = DIFF_FREQ;
      }
      if( pilotCoding_flag ) {
        hufYY = PCM_PLT;
      }
      else {
        hufYY = diff_type;
      }
      
      switch( data_type ) {
      case t_CLD:
        if( !huff_dec_cld_2D(strm, &huffCLDNodes.h2D[hufYY][TIME_PAIR], pair_vec, num_val_1_int, 1, p0_data_1) ) return 0;
        break;
        
      case t_ICC:
        if( !huff_dec_icc_2D(strm, &huffICCNodes.h2D[hufYY][TIME_PAIR], pair_vec, num_val_1_int, 1, p0_data_1) ) return 0;
        break;
        
      case t_CPC:
        if( !huff_dec_cpc_2D(strm, &huffCPCNodes.h2D[hufYY][TIME_PAIR], pair_vec, num_val_1_int, 1, p0_data_1) ) return 0;
        break;

      case t_IPD:
        if( !huff_dec_ipd_2D(strm, &huffIPDNodes.h2D[hufYY][TIME_PAIR], pair_vec, num_val_1_int, 1, p0_data_1) ) return 0;
        break;

      default:
        break;
      }
      
      for( i=0; i<num_val_1_int; i++ ) {
        out_data_1_int[i] = pair_vec[i][0];
        out_data_2_int[i] = pair_vec[i][1];
      }
      
      break;
      
    default:
      break;
      
    }
    
    break;
    
  default:
    break;
  }
  
  return 1;
}


static void diff_freq_decode( int* diff_data,
                              int* out_data,
                              int  num_val )
{
  int i = 0;
  
  out_data[0] = diff_data[0];
  
  for( i=1; i<num_val; i++ ) {
    out_data[i] = out_data[i-1] + diff_data[i];
  }
}


static void diff_time_decode_backwards( int* prev_data,
                                        int* diff_data,
                                        int* out_data,
                                        int  mixed_diff_type,
                                        int  num_val )
{
  int i = 0;
  
  if( mixed_diff_type ) {
    out_data[0] = diff_data[0];
    for( i=1; i<num_val; i++ ) {
      out_data[i] = prev_data[i] + diff_data[i];
    }
  }
  else {
    for( i=0; i<num_val; i++ ) {
      out_data[i] = prev_data[i] + diff_data[i];
    }
  }
}


static void diff_time_decode_forwards( int* prev_data,
                                       int* diff_data,
                                       int* out_data,
                                       int  mixed_diff_type,
                                       int  num_val )
{
  int i = 0;
  
  if( mixed_diff_type ) {
    out_data[0] = diff_data[0];
    for( i=1; i<num_val; i++ ) {
      out_data[i] = prev_data[i] - diff_data[i];
    }
  }
  else {
    for( i=0; i<num_val; i++ ) {
      out_data[i] = prev_data[i] - diff_data[i];
    }
  }
}


static int attach_lsb( HANDLE_S_BITINPUT               strm,
                       int*                  in_data_msb,
                       int                   offset,
                       int                   num_lsb,
                       int                   num_val,
                       int*                  out_data )
{
  int i = 0, lsb = 0, msb = 0;
  unsigned long data = 0;
  
  for( i=0; i<num_val; i++ ) {
    
    msb = in_data_msb[i];
    
    if( num_lsb > 0 ) {
      data = s_GetBits(strm, num_lsb);
      lsb = data;
      
      out_data[i] = ((msb << num_lsb) | lsb) - offset;
    }
    else out_data[i] = msb - offset;
    
  }
  
  return 0;
}


int EcDataPairDec( HANDLE_S_BITINPUT strm,
                   int        aaOutData[][MAXBANDS],
                   int        aHistory[MAXBANDS],
                   DATA_TYPE  data_type,
                   int        setIdx,
                   int        startBand,
                   int        dataBands,
                   int        pair_flag,
                   int        coarse_flag,
                   int        independency_flag )
     
{
  int allowDiffTimeBack_flag = !independency_flag || (setIdx > 0);
  int attachLsb_flag = 0;
  int pcmCoding_flag = 0;
  int pilotCoding_flag = 0;
  int pilotData[2] = {0,0};
  int mixed_time_pair = 0, freqResStride = 0, numValPcm = 0;
  int quant_levels = 0, quant_offset = 0;
  unsigned long data = 0;
    
  int aaDataPair[2][MAXBANDS] = {{0}};
  int aaDataDiff[2][MAXBANDS] = {{0}};
  
  int aHistoryMsb[MAXBANDS] = {0};
  
  int* pDataVec[2] = { NULL, NULL };
  
  
  DIFF_TYPE     diff_type[2] = { DIFF_FREQ, DIFF_FREQ };
  CODING_SCHEME cdg_scheme   = HUFF_1D;
  DIRECTION     direction    = BACKWARDS;
  
  
  switch( data_type ) {
  case t_CLD:
    if( coarse_flag ) {
      attachLsb_flag   =  0;
      quant_levels     = 15;
      quant_offset     =  7;
    }
    else {
      attachLsb_flag   =  0;
      quant_levels     = 31;
      quant_offset     = 15;
    }
    
    break;
    
  case t_ICC:
    if( coarse_flag ) {
      attachLsb_flag   =  0;
      quant_levels     =  4;
      quant_offset     =  0;
    }
    else {
      attachLsb_flag   =  0;
      quant_levels     =  8;
      quant_offset     =  0;
    }
    
    break;
    
  case t_CPC:
    if( coarse_flag ) {
      attachLsb_flag   =  0;
      quant_levels     = 26;
      quant_offset     = 10;
    }
    else {
      attachLsb_flag   =  1;
      quant_levels     = 51;
      quant_offset     = 20;
    }
    
    break;

  case t_IPD:
    if( coarse_flag ) {
      attachLsb_flag   =  0;
      quant_levels     =  8;
      quant_offset     =  0;
    }
    else {
      attachLsb_flag   =  1;
      quant_levels     =  16;
      quant_offset     =  0;
    }
    break;

  default:
    fprintf( stderr, "Unknown type of data!\n" );
    return 0;
  }
  
  data = s_GetBits(strm, 1);
  pcmCoding_flag = data;
  
  pilotCoding_flag = 0;
  
  if( pcmCoding_flag && !pilotCoding_flag ) {
    
    if( pair_flag ) {
      pDataVec[0] = aaDataPair[0];
      pDataVec[1] = aaDataPair[1];
      numValPcm   = 2*dataBands;
    }
    else {
      pDataVec[0] = aaDataPair[0];
      pDataVec[1] = NULL;
      numValPcm   = dataBands;
    }
    
    if( !pcm_decode( strm,
		     pDataVec[0],
		     pDataVec[1],
		     quant_offset,
		     numValPcm,
		     quant_levels ) ) return 0;	
    
  }
  else {
    
    if( pair_flag ) {
      pDataVec[0] = aaDataDiff[0];
      pDataVec[1] = aaDataDiff[1];
    }
    else {
      pDataVec[0] = aaDataDiff[0];
      pDataVec[1] = NULL;
    }
    
    diff_type[0] = DIFF_FREQ;
    diff_type[1] = DIFF_FREQ;
    
    direction = BACKWARDS;
    
    if( !pilotCoding_flag )
      {
        if( pair_flag || allowDiffTimeBack_flag ) {
          data = s_GetBits(strm, 1);
          diff_type[0] = data;
        }
        
        if( pair_flag && ((diff_type[0] == DIFF_FREQ) || allowDiffTimeBack_flag) ) {
          data = s_GetBits(strm, 1);
          diff_type[1] = data;
        }
      }
    
    if( !huff_decode( strm,
		      pDataVec[0],
		      pDataVec[1],
		      data_type,
		      diff_type[0],
		      diff_type[1],
                      pilotCoding_flag,
                      pilotData,
		      dataBands,
		      &cdg_scheme )
        ) {
      return 0;
    }
    
    if( pilotCoding_flag ) {
      
      int i;
      for( i=0; i<dataBands; i++ ) {
        aaDataPair[0][i] = aaDataDiff[0][i] + pilotData[0];
      }
      
      if( pair_flag ) {
        for( i=0; i<dataBands; i++ ) {
          aaDataPair[1][i] = aaDataDiff[1][i] + pilotData[0];
        }
      }
    }
    else {
      if( (diff_type[0] == DIFF_TIME) || (diff_type[1] == DIFF_TIME) ) {
        if( pair_flag ) {
          if( (diff_type[0] == DIFF_TIME) && !allowDiffTimeBack_flag ) {
            direction = FORWARDS;
          }
          else if( diff_type[1] == DIFF_TIME ) {
            direction = BACKWARDS;
          }
          else {
            data = s_GetBits(strm, 1);
            direction = data;
          }
        }
        else {
          direction = BACKWARDS;
        }
      }
      
      mixed_time_pair = (diff_type[0] != diff_type[1]) && ((cdg_scheme & PAIR_MASK) == TIME_PAIR);
      
      if( direction == BACKWARDS ) {
        if( diff_type[0] == DIFF_FREQ ) {
          diff_freq_decode( aaDataDiff[0], aaDataPair[0], dataBands );
        }
        else {
          int i;
          for( i=0; i<dataBands; i++ ) {
            aHistoryMsb[i] = aHistory[i+startBand] + quant_offset;
            if( attachLsb_flag ) {
              aHistoryMsb[i] >>= 1;
            }
          }
          diff_time_decode_backwards( aHistoryMsb, aaDataDiff[0], aaDataPair[0], mixed_time_pair, dataBands );
        }
        if( diff_type[1] == DIFF_FREQ ) {
          diff_freq_decode( aaDataDiff[1], aaDataPair[1], dataBands );
        }
        else {
          diff_time_decode_backwards( aaDataPair[0], aaDataDiff[1], aaDataPair[1], mixed_time_pair, dataBands );
        }
      }
      else {
        diff_freq_decode( aaDataDiff[1], aaDataPair[1], dataBands );
        
        if( diff_type[0] == DIFF_FREQ ) {
          diff_freq_decode( aaDataDiff[0], aaDataPair[0], dataBands );
        }
        else {
          diff_time_decode_forwards( aaDataPair[1], aaDataDiff[0], aaDataPair[0], mixed_time_pair, dataBands );
        }
      }
      
    }
    
    attach_lsb( strm, aaDataPair[0], quant_offset, attachLsb_flag ? 1 : 0, dataBands, aaDataPair[0] );
    if( pair_flag ) {
      attach_lsb( strm, aaDataPair[1], quant_offset, attachLsb_flag ? 1 : 0, dataBands, aaDataPair[1] );
    }
    
  } 
  
  memcpy( aaOutData[setIdx]+startBand, aaDataPair[0], sizeof(int)*dataBands );
  if( pair_flag ) {
    memcpy( aaOutData[setIdx+1]+startBand, aaDataPair[1], sizeof(int)*dataBands );
  }

  return 1;
}

int huff_dec_reshape( HANDLE_S_BITINPUT strm,
                      int*            out_data,
                      int             num_val )
{
  int val_rcvd = 0, dummy = 0, i = 0, val = 0, len = 0;
  int rl_data[2] = {0};
  
  while( val_rcvd < num_val ) {
    if( !huff_read_2D(strm, (HANDLE_HUFF_NODE)&huffReshapeNodes, rl_data, &dummy) ) return 0;
    val = rl_data[0];
    len = rl_data[1]+1;
    for( i=val_rcvd; i<val_rcvd+len; i++) {
      out_data[i] = val;
    }
    val_rcvd += len;
  }
  
  return 1;
  
}
