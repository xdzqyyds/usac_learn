/**********************************************************************
MPEG-4 Audio VM
Encoder core (t/f-based)



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



Header file: mp4_tf.h

 

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
12-aug-96   HP    first version
26-aug-96   HP    CVS
**********************************************************************/


#ifndef _mp4_tf_h_
#define _mp4_tf_h_

/* ---------- declarations ---------- */
void tfScaleableAUDecode ( int          numChannel, 
                           FRAME_DATA*  frameData ,/* config data , obj descr. etc. */
                           TF_DATA*     tfData,
                           LPC_DATA*    lpcData,
                           HANDLE_DECODER_GENERAL hFault,
                           enum AAC_BIT_STREAM_TYPE bitStreamType                           
                           );

void tfAUDecode ( int numChannels,
                  FRAME_DATA* frameData,
                  TF_DATA* tfData,
                  HANDLE_DECODER_GENERAL hFault,
                  QC_MOD_SELECT qc_select
                  );


#endif	/* #ifndef _mp4_tf_h_ */

/* end of mp4_tf.h */

