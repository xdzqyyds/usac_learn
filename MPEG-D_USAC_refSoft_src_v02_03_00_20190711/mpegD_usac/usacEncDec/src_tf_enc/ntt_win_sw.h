/*****************************************************************************/
/* This software module was originally developed by                          */
/*   NTT                                                                     */
/* and edited by                                                             */
/*   Olaf Kaehler (Fraunhofer IIS-A) on 2003-11-04                           */
/*                                                                           */
/* in the course of development of the                                       */
/* MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14486-1,2 and 3.        */
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
/* Copyright (c)2003.                                                        */
/*****************************************************************************/
/* header to ne new, non-static window switching module, originally          */
/* developed by NTT as part of the TwinVQ preprocessing submodule            */

#ifndef _NTT_WIN_SW_H_
#define _NTT_WIN_SW_H_

#include "tf_mainStruct.h"

typedef struct tag_ntt_winSwitchModule *HANDLE_NTT_WINSW;

WINDOW_SEQUENCE ntt_win_sw(double *sig[],    /* Input: input signal */
                           int num_ch,
                           int samp_rate,
                           int block_size,
                           WINDOW_SEQUENCE window_prev,
                           HANDLE_NTT_WINSW win_sw);

HANDLE_NTT_WINSW ntt_win_sw_init(int max_ch, int block_size_samples);
   
#endif
