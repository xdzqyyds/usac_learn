/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Takehiro Moriya (NTT) on 1998-08-10,                                    */
/* in the course of development of the                                       */
/* MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.        */
/* This software module is an implementation of a part of one or more        */
/* MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio */
/* standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards   */
/* free license to this software module or modifications thereof for use in  */
/* hardware or software products claiming conformance to the MPEG-2 AAC/     */
/* MPEG-4 Audio  standards. Those intending to use this software module in   */
/* hardware or software products are advised that this use may infringe      */
/* existing patents. The original developer of this software module and      */
/* his/her company, the subsequent editors and their companies, and ISO/IEC  */
/* have no liability for use of this software module or modifications        */
/* thereof in an implementation. Copyright is not released for non           */
/* MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer       */
/* retains full right to use the code for his/her  own purpose, assign or    */
/* donate the code to a third party and to inhibit third party from using    */
/* the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.             */
/* This copyright notice must be included in all copies or derivative works. */
/* Copyright (c)1996.                                                        */
/*****************************************************************************/

#include <stdio.h>

#include "allHandles.h"
#include "common_m4a.h"

#include "block.h"

#include "all.h"                 /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "allVariables.h"        /* variables */

#include "tf_main.h"
#include "bitstream.h"
#include "ntt_conf.h"
#include "ntt_scale_conf.h"
#include "all.h"
#include "aac.h"
#include "nok_lt_prediction.h"
#include "common_m4a.h"
#include "buffers.h"
#include "huffdec2.h"

void ntt_headerdec(int                  iscl,
		   HANDLE_BSBITSTREAM          stream, 
		   ntt_INDEX*           index, 
                   Info**               sfbInfo,
		   int*                 decoded_bits,
		   TNS_frame_info       tns_info[MAX_TIME_CHANNELS],
		   NOK_LT_PRED_STATUS** nok_lt_status,
		   PRED_TYPE            pred_type,
                   HANDLE_DECODER_GENERAL hFault )
{
      int g,sfb,win;
      int gwin;
      Info*  p_sfbInfo;
      unsigned long ultmp;
      int num_groups;
      short group_len[8];
      QC_MOD_SELECT qc_select = NTT_VQ;

      {
         int max_line, sb, sb_ind;
         float tmp;
         if(iscl <0 )      {
	   tmp = (float)(index->nttDataBase->bandUpper);
           if (index->w_type==EIGHT_SHORT_SEQUENCE){
	     tmp *=  (float)(index->block_size_samples/8);
	     max_line = (int)tmp;
           }
	   else{
	     tmp *=  (float)(index->block_size_samples);
	     max_line = (int)tmp;
           }
         }
         else {
	   tmp = (float)(index->nttDataScl->ac_top[iscl][0][3]);
           if (index->w_type==EIGHT_SHORT_SEQUENCE){
	     tmp *=  (float)(index->block_size_samples/8);
	     max_line = (int)tmp;
           }
	   else{
	     tmp *=  (float)(index->block_size_samples);
	     max_line = (int)tmp;
           }
         }

	 for(sb=1, sb_ind=1; 
	     (sfbInfo[index->w_type]->bk_sfb_top[sb-1]<max_line); sb++){
	   sb_ind = sb+1;
         }
         index->max_sfb[iscl+1] = sb_ind;

     }
      DebugPrintf(5, "LLLLL index->numChannel %5d %5d \n", index->numChannel, *decoded_bits);
    if(index->numChannel > 1){
      BsGetBit( stream, &ultmp, 2);
      *decoded_bits += 2;
      index->ms_mask = ultmp;
	    DebugPrintf(5,"index->ms_mask %d %d\n",
	    index->ms_mask,*decoded_bits);
    }
    if( index->numChannel==2 ){
	if (index->ms_mask==1){
          if (index->w_type== EIGHT_SHORT_SEQUENCE ){
            p_sfbInfo = &eight_short_info;
            p_sfbInfo = sfbInfo[index->w_type];
            if(iscl < 0){
	       BsGetBit( stream, &ultmp,7);        /* max_sfb */
               *decoded_bits += 7;
               index->group_code = ultmp;
	    }
	    num_groups =
             TVQ_decode_grouping
             ( index->group_code, group_len );
	  }else {
            p_sfbInfo = &only_long_info;
            p_sfbInfo = sfbInfo[index->w_type];
            num_groups = 1;
            group_len[0]= 1;

	  }
        win=0;

          for (g=0;g<num_groups;g++){
           for (sfb=index->last_max_sfb[iscl+1];sfb<index->max_sfb[iscl+1];sfb++){
             BsGetBit( stream, &ultmp,1);    /* ms_mask */
             *decoded_bits += 1;
             index->msMask[win][sfb]=ultmp;
	   }
           for (gwin=1;gwin<group_len[g]; gwin++){
             for (sfb=index->last_max_sfb[iscl+1];sfb<index->max_sfb[iscl+1];sfb++){
               index->msMask[win+gwin][sfb]=index->msMask[win][sfb];
               /* apply same ms mask to a ll subwindows of same group */
             }
           }
           win += group_len[g];
         }
       }
       else if(index->ms_mask==0){
	 for (g=0;g<8;g++){
           for (sfb=index->last_max_sfb[iscl+1];sfb<index->max_sfb[iscl+1];sfb++){
             index->msMask[g][sfb]=0;
	   }
         }
       }
       else if(index->ms_mask==2){
	    DebugPrintf (3,"max_sfb[%d]=%5d\n",iscl, index->max_sfb[iscl+1]);
	 for (g=0;g<8;g++){
           for (sfb=index->last_max_sfb[iscl+1];sfb<index->max_sfb[iscl+1];sfb++){
             index->msMask[g][sfb]=1;
	   }
         }

       }
     } /*end MS-stereo */

	  
 if(iscl <0){
   unsigned long ultmp;
   WINDOW_SEQUENCE windowSequence[ntt_N_SUP_MAX];
   int i_ch;
   byte sfb_bands;
   windowSequence[MONO_CHAN] = (WINDOW_SEQUENCE)index->w_type;

   for(i_ch =0; i_ch<index->numChannel; i_ch++){
     
     if(pred_type == NOK_LTP)
     {
     	  DebugPrintf (5, "NOKdecoded_bits  before %d %d\n",pred_type,  *decoded_bits);

        sfb_bands =  index->max_sfb[0];
       setHuffdec2BitBuffer(stream);
       *decoded_bits += 
	 nok_lt_decode((WINDOW_SEQUENCE)index->w_type, &sfb_bands,
		       nok_lt_status[i_ch]->sbk_prediction_used,
		       nok_lt_status[i_ch]->sfb_prediction_used,
		       &nok_lt_status[i_ch]->weight,
		       nok_lt_status[i_ch]->delay,
		       hFault[0].hResilience, 
                       hFault->hEscInstanceData,
		       qc_select, 
                       NTT_TVQ, 
                       1, /***i_ch***/
                       hFault[0].hVm);
       ResetReadBitCnt(hFault[0].hVm);
     }
     else
     {
       BsGetBit(stream, &ultmp, 1);
       *decoded_bits += 1;
     }
    
    BsGetBit(stream, &ultmp, 1);
    *decoded_bits += 1;
    if(ultmp ) {
	  DebugPrintf (3, "LLLLLLLLLL TNSTNS ddACTIVE \n");
       ntt_get_tns_vm( stream, sfbInfo[windowSequence[MONO_CHAN]], 
	   &tns_info[i_ch] ,decoded_bits);
    } else {
	  DebugPrintf (3, "LLLLLLLLLL TNSTNS not ACTIVE \n");
       clr_tns( sfbInfo[windowSequence[MONO_CHAN]], &tns_info[i_ch] );
    }
  }
 }
}
/*  ntt_get_tns_vm is identical to get_tns_vm in scale_dec_frame.c */
int ntt_get_tns_vm( HANDLE_BSBITSTREAM fixed_stream, Info *info, TNS_frame_info *tns_frame_info,int* decoded_bits )
{
  int                       f, t, top, res, res2, compress;
  int                       short_flag, s;
  short                     *sp, tmp, s_mask, n_mask;
  TNSfilt                   *tns_filt;
  TNSinfo                   *tns_info;
  static const short              sgn_mask[] = { 
    0x2, 0x4, 0x8     };
  static const short              neg_mask[] = { 
    (short) 0xfffc, (short)0xfff8, (short)0xfff0     };
  unsigned long ultmp, numBits;

  short_flag = (!info->islong);
  tns_frame_info->n_subblocks = info->nsbk;

  for (s=0; s<tns_frame_info->n_subblocks; s++) {
    tns_info = &tns_frame_info->info[s];
    numBits = (short_flag ? 1 : 2);
    BsGetBit( fixed_stream, &ultmp,numBits   );
    *decoded_bits += numBits; 
    if (!(tns_info->n_filt = ultmp) )
      continue;     
    BsGetBit( fixed_stream, &ultmp, 1 );
    *decoded_bits += 1; 
    tns_info -> coef_res = res = ultmp + 3;
    top = info->sfb_per_sbk[s];
    tns_filt = &tns_info->filt[ 0 ];
    for (f=tns_info->n_filt; f>0; f--)  {
      tns_filt->stop_band = top;
      numBits = ( short_flag ? 4 : 6);
      BsGetBit( fixed_stream, &ultmp,numBits );
      *decoded_bits += numBits; 
      top = tns_filt->start_band = top - ultmp;

      numBits = ( short_flag ? 3 : 5);
      BsGetBit( fixed_stream, &ultmp, numBits );
      *decoded_bits += numBits; 

      tns_filt->order = ultmp;

      if (tns_filt->order)  {
        BsGetBit( fixed_stream, &ultmp, 1 );
        *decoded_bits += 1; 

        tns_filt->direction = ultmp;
        BsGetBit( fixed_stream, &ultmp, 1 );
        *decoded_bits += 1; 

        compress = ultmp;

        res2 = res - compress;
        s_mask = sgn_mask[ res2 - 2 ];
        n_mask = neg_mask[ res2 - 2 ];

        sp = tns_filt -> coef;
        for (t=tns_filt->order; t>0; t--)  {
          BsGetBit( fixed_stream, &ultmp, res2 );
          *decoded_bits += res2; 
          tmp = (short)ultmp;
          *sp++ = (tmp & s_mask) ? (tmp | n_mask) : tmp;
        }
      }
      tns_filt++;
    }
  }   /* subblock loop */
  return 1;
}
