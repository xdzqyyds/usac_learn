/**********************************************************************
MPEG-4 Audio VM
psychoacoustic model for "individual lines" by UHD



This software module was originally developed by

Charalampos Ferekidis (University of Hannover / Deutsche Telekom Berkom)
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

Copyright (c) 1997.



Library header file: libuhd_psy.h

 

Corresponding library:
libuhd_psy.a		indiline psychoacoustic model

Required modules:
(none)

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
11-sep-96   HP    adapted existing code
13-nov-97   HP    removed extern
30-jan-01   BE    float -> double
**********************************************************************/


#ifndef _libuhd_psy_h_
#define _libuhd_psy_h_


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif


/* UHD_init_psycho_acoustic() */
/* Init UHD psychoacoustic model. */
int UHD_init_psycho_acoustic (
  double samplerate,
  double fmax,
  int fftlength,
  int blockoffset,
  int anz_SMR,
  int norm_value);

/* UHD_psycho_acoustic() */
/* UHD psychoacoustic model. */
double *UHD_psycho_acoustic (
  double *re,
  double *im,
  int chan);


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _libuhd_psy_h_ */

/* end of libuhd_psy.h */
