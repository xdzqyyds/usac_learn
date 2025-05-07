/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-07-17,                                      */
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


/*---------------------------------------------------------------------------*/
/* PERCEPTUAL CONTRL                                                         */
/*---------------------------------------------------------------------------*/
#define   ntt_PERCEPTW     YES     /* exponential weighting  */
#define   ntt_GAMMA_W      0.35 /*0.75*/ /*0.5*/
#define   ntt_PERCEPTW_MIC YES     /* exponential weighting  */
#define	  ntt_GAMMA_W_MIC  0.35 /*0.75*/ /*0.5*/

#define   ntt_PERCEPTW_S   YES     /* exponential weighting on short frame */
#define   ntt_GAMMA_W_S    0.35 /*0.6*/ /*0.6*/
#define   ntt_PERCEPTW_S_T YES     /* exp. weight. in time domain on short frame */
#define	  ntt_GAMMA_W_S_T  0.35 /* 0.6*/ /*0.2*/ /*0.3*/
#define   ntt_GAMMA_AMP    1.0

#define   ntt_PERCEPTW_M   YES     /* exponential weighting on short frame */
#define   ntt_GAMMA_W_M    0.6
#define   ntt_PERCEPTW_M_T YES     /* exp. weight. in time domain on short frame */
#define	  ntt_GAMMA_W_M_T  0.3
#define   ntt_GAMMA_AMP    1.0
#define   ntt_GAMMA_CH         0.75   /* inter-channel weight */

#define ntt_BAND_EXPN      0.02 
#define ntt_BAND_EXPN_SCL  0.02  /*normalized band expand for LPC */ 
#define ntt_N_CAN          20
#define ntt_N_CAN_S        20
#define ntt_N_CAN_MAX      32
#define ntt_N_CAN_P        8

