/**********************************************************************
MPEG-4 Audio VM
Bit stream module

This software module was originally developed by

Pierre Lauber (Fraunhofer Institut of Integrated Circuits)

and edited by

Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)
Daniel Homm (Fraunhofer Gesellschaft IIS)

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


This file contains all permitted bitstream elements. The macro E is
used to obtain an enumerated constant and a string constant 
simultaneous, which was the primary intension. The sequence is not
significant to the software.
**********************************************************************/
E(COEF),
E(COEF_COMPRESS),
E(COEF_RES),
E(COMMON_WINDOW),
E(DIFF_CONTROL),
E(DIFF_CONTROL_LR),
E(DIRECTION),
E(DPCM_NOISE_LAST_POSITION),
E(DPCM_NOISE_NRG),
E(ELEMENT_INSTANCE_TAG),
E(GAIN_CONTROL_DATA_PRESENT),
E(GLOBAL_GAIN),
E(HCOD),
E(HCOD_ESC),
E(HCOD_SF),
E(ICS_RESERVED),
E(LENGTH),
E(LENGTH_OF_LONGEST_CODEWORD),
E(LENGTH_OF_REORDERED_SPECTRAL_DATA),
E(LENGTH_OF_RVLC_ESCAPES),
E(LENGTH_OF_RVLC_SF),
E(LTP_COEF),
E(LTP_DATA_PRESENT),
E(LTP_LAG),
E(LTP_LAG_UPDATE),
E(LTP_LONG_USED),
E(LTP_SHORT_LAG),
E(LTP_SHORT_LAG_PRESENT),
E(LTP_SHORT_USED),
E(MAX_SFB),
E(MS_MASK_PRESENT),
E(MS_USED),
E(N_FILT),
E(NUMBER_PULSE),
E(ORDER),
E(PREDICTOR_DATA_PRESENT),
E(PULSE_AMP),
E(PULSE_DATA_PRESENT),
E(PULSE_OFFSET),
E(PULSE_START_SFB),
E(REORDERED_SPECTRAL_DATA),
E(REV_GLOBAL_GAIN),
E(RVLC_CODE_SF),
E(RVLC_ESC_SF),
E(SCALE_FACTOR_GROUPING),
E(SECT_CB),
E(SECT_LEN_INCR),
E(SF_CONCEALMENT),
E(SF_ESCAPES_PRESENT),
E(SIGN_BITS),
E(TNS_CHANNEL_MONO),
E(TNS_DATA_PRESENT),
E(TNS_ACTIVE),
E(COMMON_MAX_SFB),
E(COMMON_TNS),
E(TNS_ON_LR),
E(TNS_PRESENT_BOTH),
E(TNS_DATA_PRESENT1),
E(CPLX_PRED_ALL),
E(CPLX_PRED_USED),
E(PRED_DIR),
E(COMPLEX_COEF),
E(USE_PREV_FRAME),
E(DELTA_CODE_TIME),
E(DPRED_COEF),

E(WINDOW_SEQ),
E(WINDOW_SHAPE_CODE),
E(ALIGN_BITS),
#ifdef I2R_LOSSLESS
E(SLS_BITS),
#endif
