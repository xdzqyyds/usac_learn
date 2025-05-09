/*******************************************************************************
This software module was originally developed by

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips

and edited by

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
*******************************************************************************/



#ifndef _sac_interface_h_
#define _sac_interface_h_



#define C_LN10		2.30258509299404568402		
#define C_PI		3.14159265358979323846		
#define C_SQRT2		1.41421356237309504880		

#define MINTHR		.5
#define SF_C1		(13.33333/1.333333)


#define	PRED_ORDER	2
#define PRED_ALPHA	0.90625
#define PRED_A		0.953125
#define PRED_B		0.953125


#ifndef _interface_h_
enum
{
    
  BLOCK_LEN_LONG	= 1024,
  BLOCK_LEN_SHORT	= 128,
  
  LN			= 2048,
  SN			= 256,
  LN2			= LN/2,
  SN2			= SN/2,
  LN4			= LN/4,
  SN4			= SN/4,
  NSHORT		= LN/SN,
  MAX_SBK		= NSHORT,
  
  MAXBANDS		= 16*NSHORT,	
  MAXFAC		= 121,		
  MIDFAC		= (MAXFAC-1)/2,
  SF_OFFSET		= 100,		
  
  
  HUF1SGN		= 1, 
  HUF2SGN		= 1, 
  HUF3SGN		= 0, 
  HUF4SGN		= 0, 
  HUF5SGN		= 1, 
  HUF6SGN		= 1, 
  HUF7SGN		= 0, 
  HUF8SGN		= 0, 
  HUF9SGN		= 0, 
  HUF10SGN		= 0, 
  HUF11SGN		= 0, 
  
  ZERO_HCB		= 0,
  BY4BOOKS		= 4,
  ESCBOOK		= 11,
  NSPECBOOKS		= ESCBOOK + 1,
  BOOKSCL		= NSPECBOOKS,
  NBOOKS		= NSPECBOOKS+1,
  INTENSITY_HCB2	= 14,
  INTENSITY_HCB	= 15,
#ifdef MPEG4V1
  NOISE_HCB   	= 13,
  
  NOISE_PCM_BITS      = 9,
  NOISE_PCM_OFFSET    = (1 << (NOISE_PCM_BITS-1)),
  
  NOISE_OFFSET        = 90,
#else
  RESERVED_HCB	= 13,
#endif
  
  LONG_SECT_BITS	= 5, 
  SHORT_SECT_BITS	= 3, 
  
    
  Main_Profile	= 0, 
  LC_Profile		= 1, 
  SSR_Profile		= 2,
#ifdef MPEG4V1
    LTP_Profile         = 3,
#endif
  
  CC_DOM		= 0,	 
  CC_IND		= 1,  
  
  
  LEN_SE_ID		= 3,
  LEN_TAG		= 4,
  LEN_COM_WIN		= 1,
  LEN_ICS_RESERV	= 1, 
  LEN_WIN_SEQ		= 2,
  LEN_WIN_SH		= 1,
  LEN_MAX_SFBL	= 6, 
  LEN_MAX_SFBS	= 4, 
  LEN_CB		= 4,
  LEN_SCL_PCM		= 8,
  LEN_PRED_PRES	= 1,
  LEN_PRED_RST	= 1,
  LEN_PRED_RSTGRP	= 5,
  LEN_PRED_ENAB	= 1,
  LEN_MASK_PRES	= 2,
  LEN_MASK		= 1,
  LEN_PULSE_PRES	= 1,
  LEN_TNS_PRES	= 1,
  LEN_GAIN_PRES	= 1,

  LEN_PULSE_NPULSE	= 2, 
  LEN_PULSE_ST_SFB	= 6, 
  LEN_PULSE_POFF	= 5, 
  LEN_PULSE_PAMP	= 4, 
  NUM_PULSE_LINES	= 4, 
  PULSE_OFFSET_AMP	= 4, 
  
  LEN_IND_CCE_FLG	= 1,  
  LEN_NCC		= 3,
  LEN_IS_CPE		= 1, 
  LEN_CC_LR		= 1,
    LEN_CC_DOM		= 1,
  LEN_CC_SGN		= 1,
    LEN_CCH_GES		= 2,
    LEN_CCH_CGP		= 1,

    LEN_D_ALIGN		= 1,
    LEN_D_CNT		= 8,
    LEN_D_ESC		= 8,
    LEN_F_CNT		= 4,
    LEN_F_ESC		= 8,
    LEN_NIBBLE		= 4,
    LEN_BYTE		= 8,
    LEN_PAD_DATA	= 8,

    LEN_PC_COMM		= 8, 
    
    
    LEN_EX_TYPE		= 4,	 
    EX_FILL		= 0, 
    EX_FILL_DATA	= 1, 
    EX_DRC		= 11, 
    
    
    MAX_DBYTES	= ((1<<LEN_D_CNT) + (1<<LEN_D_ESC)),

    
    LEN_DRC_PL		= 7, 
    LEN_DRC_PL_RESV	= 1, 
    LEN_DRC_PCE_RESV	= (8 - LEN_TAG),
    LEN_DRC_BAND_INCR	= 4, 

#ifdef CT_SBR
    LEN_DRC_BAND_INTERP	= 4, 
#else
    LEN_DRC_BAND_RESV	= 4, 
#endif

    LEN_DRC_BAND_TOP	= 8, 
    LEN_DRC_SGN		= 1, 
    LEN_DRC_MAG		= 7, 
    DRC_BAND_MULT	= 4,	

    
     MAX_PRED_SFB	= 40,	
     MAX_PRED_BINS	= 672,

    ID_SCE 		= 0,
    ID_CPE,
    ID_CCE,
    ID_LFE,
    ID_DSE,
    ID_PCE,
    ID_FIL,
    ID_END,

    ID_HDR,   
    ID_NUL,
    ID_2ND,
    LOG_FIN,
#ifdef NEW_ADTS
    END_HDR,
#endif

    
    FILL_VALUE		= 0x55,

    
    LEN_PROFILE		= 2, 
    LEN_SAMP_IDX	= 4, 
    LEN_NUM_ELE		= 4, 
    LEN_NUM_LFE		= 2, 
    LEN_NUM_DAT		= 3, 
    LEN_NUM_CCE		= 4, 
    LEN_MIX_PRES	= 1, 
    LEN_MMIX_IDX	= 2, 
    LEN_PSUR_ENAB	= 1, 
    LEN_ELE_IS_CPE	= 1,
    LEN_IND_SW_CCE	= 1,  
    LEN_COMMENT_BYTES	= 8, 


    
    
    LEN_SYNCWORD	= 12,
    LEN_ID		= 1,
    LEN_LAYER		= 2,
    LEN_PROTECT_ABS	= 1,
    LEN_PRIVTE_BIT	= 1,
    LEN_CHANNEL_CONF	= 3,
	 
    
    LEN_COPYRT_ID_ADTS		= 1,
    LEN_COPYRT_START		= 1,
    LEN_FRAME_LEN		= 13,
    LEN_ADTS_BUF_FULLNESS	= 11,
    LEN_NUM_OF_RDB		= 2,
#if ((defined NEW_ADTS)||(defined WRITE_NEW_ADTS))
    LEN_RDB_POSITION		=16,
#endif
    MAX_RDB_PER_FRAME		=4,

    
    LEN_CRC	= 16,

         
    
    LEN_ADIF_ID		= (32/8), 
    LEN_COPYRT_PRES	= 1, 
    LEN_COPYRT_ID	= (72/8), 
    LEN_ORIG		= 1, 
    LEN_HOME		= 1, 
    LEN_BS_TYPE		= 1, 
    LEN_BIT_RATE	= 23, 
    LEN_NUM_PCE		= 4, 
    LEN_ADIF_BF		= 20, 

#ifdef MPEG4V1
#ifdef USELIB_ISOMP4
    
    lenAudioObjectType		= 5,
    lenSamplingFrequencyIndex	= 4,
    lenSamplingFrequency	= 24,
    lenChannelConfiguration	= 4,
    lenFrameLengthFlag		= 1,
    lenDependsOnCoreCoder	= 1,
    lenExtensionFlag		= 1,

    samplingFrequencyEscape	= (1<<lenSamplingFrequencyIndex)-1,
#endif
#endif

    CRC_LEN_HDR		= 56,
    CRC_LEN_ELE		= 192,
    CRC_LEN_2ND		= 128,
    CRC_LEN_ALL		= -1,

    XXX
};


#endif  

typedef enum {
  ONLYLONG_WINDOW	= 0,
  LONGSTART_WINDOW,
  EIGHTSHORT_WINDOW,
  LONGSTOP_WINDOW,
  NUMWIN_SEQ
} BLOCKTYPE;



#define aacFrameLength			1024	    
#define	bufferSizePerAudioChn		6144
#ifdef	BIGDECODER
#define MAX_FRAME_LEN			(64*1024)   
#else
#define MAX_FRAME_LEN			8192	    
#endif

#endif   
