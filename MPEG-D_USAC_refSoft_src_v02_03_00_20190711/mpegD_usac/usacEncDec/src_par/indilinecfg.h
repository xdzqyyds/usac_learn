/**********************************************************************
MPEG4 Audio VM Module
parameter based codec - individual lines: quantiser configuration



This software module was originally developed by

Heiko Purnhagen (University of Hannover / Deutsche Telekom Berkom)
Bernd Edler (University of Hannover / Deutsche Telekom Berkom)

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

Copyright (c) 1996.



Header file: indilinecfg.h

 

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BE    Bernd Edler, Uni Hannover <edler@tnt.uni-hannover.de>

Changes:
10-sep-96   BE
11-sep-96   HP
**********************************************************************/

#ifndef _indilinecfg_h_
#define _indilinecfg_h_


/* ---------- declarations ---------- */

/* BASIC BITSTREAM */

/* envelope */
#define ILQ_TMBITS	4
#define ILQ_ATKBITS	4
#define ILQ_DECBITS	4

#define ILQ_ENV45	0.2	/* length (rel. to frame length) for */
				/* attack/decay at 45 deg */

/* new line */
#define ILQ_FBITS_8	10	/* for         fSampleQuant <=  8kHz */
#define ILQ_FBITS_16	11	/* for  8kHz < fSampleQuant <= 16kHz */
#define ILQ_FBITS_48	12	/* for 16kHz < fSampleQuant <= 48kHz */
#define ILQ_ABITS_8	4
#define ILQ_ABITS_16	5
#define ILQ_ABITS_48	5

#define ILQ_FMIN_8	30.
#define ILQ_FMAX_8	4000.
#define ILQ_FMIN_16	20.
#define ILQ_FMAX_16	8000.
#define ILQ_FMIN_48	20.
#define ILQ_FMAX_48	20000.
#define ILQ_AMIN	(1./32768.)
#define ILQ_AMAX	1.

/* continued line */
#define ILQ_DFBITS	6
#define ILQ_DABITS	4

#define ILQ_DFMAX	0.15
#define ILQ_DAMAX	4.

/* ENHANCEMENT BITSTREAM */

/* envelope */
#define ILQ_TMBITSE	3
#define ILQ_ATKBITSE	2
#define ILQ_DECBITSE	2

/* line */
#define ILQ_MAXPHASEDEV	0.1	/* max phase deviation (in periods) */
				/* for frame length due to freq quant */
#define ILQ_PBITSE	5


#endif	/* #ifndef _indilinecfg_h_ */

/* end of indilinecfg.h */
