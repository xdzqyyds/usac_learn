/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by
  Y.B. Thomas Kim and S.H. Park (Samsung AIT)
  Y.B. Thomas Kim (Samsung AIT) on 1997-08-22
  Heiko Purnhagen (Uni Hannover)
and edited by

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1997.

**********************************************************************/

/*  

/* HP 971023 */
/* dummy version of libBSACenc.a */
/* based on sam_encode.h and sam_main.c */

#include <stdio.h>
#include "block.h"
#include "common_m4a.h"
#include "bitstream.h"
#include "tf_main.h"
#include "nok_ltp_enc.h"
#include "aac_qc.h"
#include "sam_encode.h"


int sam_encode_bsac(
			   WINDOW_SEQUENCE  windowSequence,
			   int         scale_factor[],
			   int         nr_of_sfb,
			   int         sfb_width_table[],
			   int         num_window_groups,
			   int         window_group_length[],
			   int         quant[],
			   WINDOW_SHAPE     window_shape,
			   HANDLE_BSBITSTREAM fixed_stream,
/* 20070530 SBR */
			   int         w_flag,
			   int			n_Elm,
			   int			i_el
			   )
{
  CommonExit(1,"sam_encode_bsac: dummy");
}


