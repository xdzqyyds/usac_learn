/**********************************************************************
MPEG-4 Audio VM
Bit stream module
 
 
 
This software module was originally developed by
 
Bernhard Grill (University of Erlangen)
 
and edited by
 
Bodo Teichmann (Fhg)
 
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
 
*/
 
 
 
#ifndef _scal_dec_h
#define _scal_dec_h
void mdct_core ( TF_DATA*     tfData,
                 float         time_sig[], 
                 double        spectrum[],
                 int           block_len );

void CoreDecoderInit( 
  long SamplingRate,
  enum CORE_CODEC coreCodecIdx,
  int  granules,
  int  chan,
  int output_select
);

int CoreDecoder(
  HANDLE_BSBITSTREAM fixed_stream,
  float         ll_core_buffer[], /* only core signal (modulo buffer) */
  int           ll_core_buff_len,
  int           ll_core_buff_pos,
  int           ch_select,
  int           gr
);

#endif


