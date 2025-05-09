/**********************************************************************
 MPEG-4 Audio VM

 This software module was originally developed by
 
 Takehiro Moriya (NTT) on 1999-04-01, 
  
 and edited by
 Takeshi Mori (NTT) on 2000-11-21 
 Markus Werner       (SEED / Software Development Karlsruhe) 
 
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
 
 Copyright (c) 2000.
 
 $Id: ntt_tvq.h,v 1.1.1.1 2009-05-29 14:10:17 mul Exp $ 
 Twin VQ Decoder module
**********************************************************************/

#ifndef _nttNew_h
#define  _nttNew_h


typedef struct{
   double *lsp_code_scl;
   double *lsp_fgcode_scl;
   double *ntt_fwcodev_scl;
   double   *ntt_codev0_scl;
   double   *ntt_codev1_scl;
   double   *ntt_codev0s_scl;
   double   *ntt_codev1s_scl;
   double   ac_top[8][2][4];
   double   ac_btm[8][2][4];

   int      ntt_NBITS_FR_SCL[8];
   int      ntt_VQTOOL_BITS_SCL[8];
   int      ntt_VQTOOL_BITS_S_SCL[8];
   int      ntt_NSclLay;
   int      ntt_NSclLayDec;
   int      *prev_fw_code[8];
   int      *prev_lsp_code[8];
   int      prev_w_type;
   int      ma_np;
   int	    epFlag;
   }ntt_DATA_scl;

typedef struct{
   double *lsp_code_base;
   double *lsp_fgcode_base;
   int    *ntt_crb_tbl;
   double *ntt_fwcodev;
   double *ntt_codev0;
   double *ntt_codev1;
   double *ntt_codev0s;
   double *ntt_codev1s;
   double *ntt_codevp0;
   short  *pleave0;
   short  *pleave1;
   double  bandUpper;
   double  bandLower;
   int      ntt_NBITS_FR;
   int      ntt_VQTOOL_BITS;
   int      ntt_VQTOOL_BITS_S;
   double   *cos_TT;
   int      *prev_fw_code;
   int      *prev_lsp_code;
   int      prev_w_type;
   int      ma_np;
   int	    epFlag;
}ntt_DATA_base;

typedef struct{
   ntt_DATA_scl* nttDataScl;
   ntt_DATA_base* nttDataBase;
}TVQ_DECODER, ntt_DATA; 

typedef TVQ_DECODER *HANDLE_TVQDECODER;

#endif /* _nttNew_h */
