/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Shigeki Sagayama (NTT)                                                  */
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



/***************************************************************************
 *  MiLibrary for speech coding                                            *
 *                                    Satoshi Miki (Onsho-G, NTT HI Labs.) *
 *-------------------------------------------------------------------------*
 *  ntt_movdd() - ntt_movdd fast version                                           *
 *-------------------------------------------------------------------------*
 *  Ver.1.0  1990-08-26  Written by S.Miki                                 *
 ***************************************************************************/

/*** MODULE FUNCTION PROTOTYPE ***/

#include "ntt_conf.h"

void ntt_movdd(int n, double xx[], double yy[])
{
/*
   double
        *p_xx,
        *p_yy;
   register int
         iloop_fr;

   if (n <= 0) return;
   p_xx = xx;
   p_yy = yy;
   iloop_fr = n;
   do
   {
      *(p_yy++) = *(p_xx++);
   }
   while ((--iloop_fr) > 0);
*/
   if (n <= 0) return;
   do{ *(yy++) = *(xx++); }while(--n);
}
