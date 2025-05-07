/**********************************************************************
MPEG-4 Audio VM
Encoder core (parametric)



This software module was originally developed by

Heiko Purnhagen (University of Hannover)

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



Header file: mp4_par.h

 

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>

Changes:
13-aug-96   HP    first version
26-aug-96   HP    CVS
07-may-97   BT    added _MName0 ...
15-may-97   HP    clean up
06-jul-97   HP    switch/mix
11-nov-97   HP    mode hvx=0 il=1 mix=2 swit=3 undef=4
**********************************************************************/


#ifndef _mp4_par_h_
#define _mp4_par_h_


/* ---------- declarations ---------- */

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

/* parametric core mode */

enum MP4ModePar {MODEPAR_HVX = 0, MODEPAR_IL = 1, MODEPAR_SWIT = 2, MODEPAR_MIX = 3, MODEPAR_UNDEF = 4, MODEPAR_NUM};

#define MODEPARNAME_HVX "hvxc"
#define MODEPARNAME_IL "hiln"
#define MODEPARNAME_SWIT "swit"
#define MODEPARNAME_MIX "mix"
#define MODEPARNAME_UNDEF "undefined"

static char *MP4ModeParName[] = {MODEPARNAME_HVX, MODEPARNAME_IL,
				 MODEPARNAME_SWIT, MODEPARNAME_MIX,
				 MODEPARNAME_UNDEF
				 };

#define MODEPARNAME_LIST MODEPARNAME_HVX ", " MODEPARNAME_IL ", " MODEPARNAME_SWIT ", " MODEPARNAME_MIX


#endif	/* #ifndef _mp4_par_h_ */

/* end of mp4_par.h */
