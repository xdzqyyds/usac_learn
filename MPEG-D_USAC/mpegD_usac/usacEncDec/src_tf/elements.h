/**********************************************************************
MPEG-4 Audio VM
Bit stream module

This software module was originally developed by

Pierre Lauber (Fraunhofer Institut of Integrated Circuits)

and edited by

Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)
Ralph Sperschneider (Fraunhofer Gesellschaft IIS)

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


This file contains all bitstream elements not allowed for any AAC based
V2 AOT. The macro E is used to obtain an enumerated constant and a
string constant simultaneous, which was the primary intension. The
sequence is not significant to the software.
**********************************************************************/
E(ADIF_BUFFER_FULLNESS),
E(ADIF_ID),
E(ADJUST_NUM),
E(ALEVCODE),
E(ALOCCODE),
E(BITRATE),
E(BITSTREAM_TYPE),
E(CC_TARGET_IS_CPE),
E(CC_TARGET_TAG_SELECT),
E(CC_DOMAIN),
E(CC_L),
E(CC_R),
E(COMMENT_FIELD_BYTES),
E(COMMENT_FIELD_DATA),
E(COMMON_GAIN_ELEMENT_PRESENT),
E(COPYRIGHT_ID),
E(COPYRIGHT_ID_PRESENT),
E(COUNT),
E(DATA_BYTE_ALIGN_FLAG),
E(DATA_STREAM_BYTE),
E(DUMMY),
E(ESC_COUNT),
E(FILL_BYTE),
E(FRONT_SIDE_BACK_ELEMENT_IS_CPE),
E(FRONT_SIDE_BACK_LFE_DATA_CC_ELEMENT_TAG_SELECT),
E(GAIN_ELEMENT_SCALE),
E(GAIN_ELEMENT_SIGN),
E(HOME),
E(ID_SYN_ELE),
E(IND_SW_CCE_FLAG),
E(LENGTH_OF_SF_DATA),
E(MATRIX_MIXDOWN_IDX),
E(MATRIX_MIXDOWN_IDX_PRESENT),
E(MAX_BAND),
E(MONO_MIXDOWN_ELEMENT_NUMBER),
E(MONO_MIXDOWN_PRESENT),
E(NUM_ASSOC_DATA_ELEMENTS),
E(NUM_BACK_CHANNEL_ELEMENTS),
E(NUM_COUPLED_ELEMENTS),
E(NUM_FRONT_CHANNEL_ELEMENTS),
E(NUM_LFE_CHANNEL_ELEMENTS),
E(NUM_PROGRAM_CONFIG_ELEMENTS),
E(NUM_SIDE_CHANNEL_ELEMENTS),
E(NUM_VALID_CC_ELEMENTS),
E(ORIGINAL_COPY),
E(PREDICTOR_RESET),
E(PREDICTOR_RESET_GROUP_NUMBER),
E(PREDICTION_USED),
E(PROFILE),
E(PSEUDO_SURROUND_ENABLE),
E(SAMPLING_FREQUENCY_INDEX),
E(STEREO_MIXDOWN_PRESENT),
E(STEREO_MIXDOWN_ELEMENT_NUMBER),

