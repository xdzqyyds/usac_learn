/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Akio Jin (NTT)                                                          */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
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
/* Copyright (c)1997.                                                        */
/*****************************************************************************/

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"
#include "ntt_scale_conf.h"

void ntt_scale_cp_mdtbl(ntt_MODE_TABLE_SCL ltbl, int iscl)
{
    ntt_GAIN_BITS_SCL[iscl]     = ltbl.gain_bits;
    ntt_AMP_MAX_SCL[iscl]       = ltbl.amp_max;
    ntt_SUB_GAIN_BITS_SCL[iscl] = ltbl.sub_gain_bits;
    ntt_SUB_AMP_MAX_SCL[iscl]   = ltbl.sub_amp_max;
    ntt_MU_SCL[iscl]            = ltbl.mu;
    ntt_N_PR_SCL[iscl]          = ltbl.n_pr;
    ntt_LSPCODEBOOK_SCL[iscl]   = ltbl.lspcodebook;
    ntt_LSP_BIT0_SCL[iscl]      = ltbl.lsp_bit0;
    ntt_LSP_BIT1_SCL[iscl]      = ltbl.lsp_bit1;
    ntt_LSP_BIT2_SCL[iscl]      = ltbl.lsp_bit2;
    ntt_LSP_SPLIT_SCL[iscl]     = ltbl.lsp_split;
    ntt_FW_CB_NAME_SCL[iscl]    = ltbl.fw_cb_name;
    ntt_FW_N_DIV_SCL[iscl]      = ltbl.fw_n_div;
    ntt_FW_N_BIT_SCL[iscl]      = ltbl.fw_n_bit;
    ntt_CB_NAME_SCL0[iscl]      = ltbl.cb_name0;
    ntt_CB_NAME_SCL1[iscl]      = ltbl.cb_name1;
    ntt_CB_NAME_SCL0s[iscl]     = ltbl.cb_name2;
    ntt_CB_NAME_SCL1s[iscl]     = ltbl.cb_name3;
    ntt_N_CAN_SCL[iscl]         = ltbl.n_can;
    ntt_N_CAN_S_SCL[iscl]       = ltbl.n_can_s;
}
