/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Takehiro Moriya (NTT)                                                   */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
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



#define   sign(x)     ( (x) >= 0.0 ? (1.0) : (-1.0) )


#define   NCD          4 /* number of candidates for delayed decision */
#define   ntt_N_PROFF      0    /* boundary offset between 2nd and 3rd VQ */
#define   MIN_GAP     0.0001
#define   L_LIMIT     -10.
#define   M_LIMIT      10.
#define   BIT_ERROR_F OFF
#define   BIT_ERROR   OFF
#define   ESP_RATE      0.01
#define   AMP_ntt_GAMMA   0.00


#define   MAXSTAGE     2        /*  Maximum number of stage  */
#define   SAMPLE_LIMIT   5000   /*   max num of vector for training input */

#ifndef M_PI
#define   M_PI       3.14159265358979323846
#endif

