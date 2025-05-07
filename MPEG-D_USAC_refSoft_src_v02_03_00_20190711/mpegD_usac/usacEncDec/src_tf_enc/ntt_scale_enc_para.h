/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-07-17,                                      */
/*   Akio Jin (NTT) on 1997-10-23,                                           */
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
#define   ntt_PERCEPTW2     YES     /* exponential weighting  */
#define   ntt_GAMMA_W2       0.3 /*0.5*/ 
#define   ntt_PERCEPTW_MIC2 YES     /* exponential weighting  */
#define	  ntt_GAMMA_W_MIC2  0.5 

#define   ntt_PERCEPTW_S2   YES     /* exponential weighting on short frame */
#define   ntt_GAMMA_W_S2    0.3 /*0.5 */
#define   ntt_PERCEPTW_S_T2 YES     /* exp. weight. in time domain on short frame */
#define   ntt_GAMMA_W_S_T2  0.25
#define   ntt_GAMMA_AMP2    1.0
#define   ntt_GAMMA_CH      0.75


#define ntt_BAND_EXPN      0.02 
#define ntt_BAND_EXPN_SCL  0.02  /*normalized band expand for LPC */ 
#define ntt_N_CAN_SCL         6
#define ntt_N_CAN_S_SCL       6

