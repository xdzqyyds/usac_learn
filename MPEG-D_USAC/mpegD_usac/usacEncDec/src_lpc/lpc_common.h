/**********************************************************************
MPEG-4 Audio VM
Decoder core (LPC-based)



This software module was originally developed by

Heiko Purnhagen   (University of Hannover)
Naoya Tanaka      (Matsushita Communication Industrial Co., Ltd.)
Rakesh Taori, Andy Gerrits (Philips Research Laboratories, Eindhoven, The Netherlands),
Toshiyuki Nomura           (NEC Corporation)

and edited by
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

$Id: lpc_common.h,v 1.1.1.1 2009-05-29 14:10:11 mul Exp $
LPC Decoder module
**********************************************************************/
#ifndef _lpc_common_h_
#define _lpc_common_h_

#include "allHandles.h"

/* see wdbxx.h and lpc_abscomp.ch */
#define MAX_N_LAG_CANDIDATES  15

#define MultiPulseExc  0
#define RegularPulseExc 1

typedef enum {
  fs8kHz=  0,
  fs16kHz= 1
} LPC_FS_MODE;

#define ScalarQuantizer 0
#define VectorQuantizer 1

#define Scalable_VQ 0
#define Optimized_VQ 1

#define OFF 0
#define ON  1

#define NO 0
#define YES 1

#define CelpObjectV1 0
#define CelpObjectV2 1

typedef struct {
  int          frameNumSample;             /* out: num samples per frame       */
  int          delayNumSample;
  HANDLE_BSBITSTREAM  p_bitstream;
  HANDLE_BSBITBUFFER  coreBitBuf;
  float**      sampleBuf;
  int          bitsPerFrame;
  
  /*  configuration / initialisation data */

  long bit_rate;
  long sampling_frequency;
  long frame_size;
  long n_subframes;
  long sbfrm_size;
  long lpc_order;
  long num_lpc_indices;
  long num_shape_cbks;
  long num_gain_cbks;
  long *org_frame_bit_allocation;    

  long SampleRateMode;  
  long QuantizationMode; 
  long FineRateControl; 
  long LosslessCodingMode; 
  long RPE_configuration;
  long Wideband_VQ;       
  long MPE_Configuration;
  long NumEnhLayers;
  long BandwidthScalabilityMode;
  long BWS_configuration;
  long PostFilterSW;
  long SilenceCompressionSW;
  long ExcitationMode;

  int outputTrackNumber;
  int outputLayerNumber;
  int dec_objectVerType;

} LPC_DATA;  


#endif  /* #ifndef _lpc_common_h_ */

/* end of lpc_common.h */
