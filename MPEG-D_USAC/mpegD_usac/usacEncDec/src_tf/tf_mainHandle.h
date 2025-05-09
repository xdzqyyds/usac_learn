/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
 "This software module was originally developed by
 Fraunhofer Gesellschaft IIS / University of Erlangen (UER)
 and edited by
 in the course of
 development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7,
 14496-1,2 and 3. This software module is an implementation of a part of one or more
 MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4
 Audio standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio
 standards free license to this software module or modifications thereof for use in
 hardware or software products claiming conformance to the MPEG-2 AAC/MPEG-4
 Audio  standards. Those intending to use this software module in hardware or
 software products are advised that this use may infringe existing patents.
 The original developer of this software module and his/her company, the subsequent
 editors and their companies, and ISO/IEC have no liability for use of this software
 module or modifications thereof in an implementation. Copyright is not released for
 non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
 retains full right to use the code for his/her  own purpose, assign or donate the
 code to a third party and to inhibit third party from using the code for non
 MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
 be included in all copies or derivative works."
 Copyright(c)1999.
 *                                                                           *
 ****************************************************************************/

#ifndef _TF_MAIN_HANDLE_H_INCLUDED
#define _TF_MAIN_HANDLE_H_INCLUDED

#include <stdlib.h> /* might contain min and max definitions. RS 2001-10-18 */
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

enum DC_FLAG   {
  DC_DIFF,
  DC_SIMUL,
  DC_INVALID = -1
};

enum JS_MASK {
  JS_MASK_OFF     =  0,
  JS_MASK_MS      =  1,
  JS_MASK_IS      =  2,
  JS_MASK_INVALID = -1
};

/* select different pre-/post- processing modules TK */

enum PP_MOD_SELECT {
  NONE   = 0x0,
  AAC_PP = 0x1 };

/* select different T/F modules */

enum TF_MOD_SELECT {
  VM_TF_SOURCE  = 0x1,
  MDCT_AAC      = 0x2,
  MDCT_UER      = 0x4,
  QMF_MDCT_SONY = 0x8,
  LOW_DELAY_UNH = 0x10
};

/* select different Q&C modules */ /* YB : 970825 */
typedef enum _QC_MOD_SELECT {
  VM_QC_SOURCE      = 0x1,
  AAC_QC            = 0x2,
  MDCT_VALUES_16BIT = 0x4,
  UER_QC            = 0x8,
  NTT_VQ            = 0x10,
  AAC_SCALABLE      = 0x20,
  AAC_SYS           = 0x30,  /* NTT_VQ | AAC_SCALABLE */
  AAC_BSAC          = 0x40,
  AAC_SCA_BSAC      = 0x80,
  AAC_SYS_BSAC      = 0x90,   /* NTT_VQ | AAC_SCA_BSAC */

  AAC_LD            = 0x100,
  USAC_AC           = 0x110
} QC_MOD_SELECT;

typedef enum _TNS_COMPLEXITY {
  NO_TNS,
  TNS_MAIN,
  TNS_LC,
  TNS_SSR,
  TNS_LD_512,
  TNS_LD_480,
  TNS_SCAL,
  TNS_BSAC,
  TNS_USAC
} TNS_COMPLEXITY;        /* enum type for TNS complexity parameters */


/* name the audio channels */

enum CHANN_ASS {
  MONO_CHAN    = 0,
  LEFT_CHAN    = 0,
  RIGHT_CHAN   = 1,
  MAX_CHANNELS = 48
};

enum TF_LAYER {
  MAX_MED_LAYER = 10,
  HIGH_LAYER    = 10,
  MAX_TF_LAYER  = 10
};

/* audio channel configuration coding */

enum CH_CONFIG {
  CHC_MONO,
  CHC_DUAL,
  CHC_JOINT_DUAL,
  CHC_5CHAN,
  CHC_MODES
};

/* transport layer type */ /* added "NO_SYNCWORD" by NI (28 Aug. 1996) */

enum TRANSPORT_STREAM {
  NO_TSTREAM,
  AAC_RAWDATA_TSTREAM,
  LENINFO_TSTREAM,
  NO_SYNCWORD
};

enum SR_CODING {
  SR8000,
  SR11025,
  SR12000,
  SR16000,
  SR22050,
  SR24000,
  SR32000,
  SR44100,
  SR48000,
  SR96000,
  MAX_SAMPLING_RATES
};

typedef enum {
    WT_LONG,
    WT_SHORT,
    WT_FLAT,
    WT_ADV,			/* Advanced flat window */
    N_WINDOW_TYPES
} WINDOW_TYPE;

typedef enum  {
  ONLY_LONG_SEQUENCE,
  LONG_START_SEQUENCE,
  EIGHT_SHORT_SEQUENCE,
  LONG_STOP_SEQUENCE,
  STOP_START_SEQUENCE,
  NUM_WIN_SEQ
} WINDOW_SEQUENCE;

enum AAC_BIT_STREAM_TYPE {
  SCALABLE,
  SCALABLE_ER,
  MULTICHANNEL,
  MULTICHANNEL_ER,
  MULTICHANNEL_ELD
};

enum CORE_CODEC{
  CC_FS1016,
  CC_CELP_MPEG4,
  CC_CELP_MPEG4_50,
  CC_CELP_MPEG4_60,
  CC_CELP_MPEG4_80,
  CC_PAR_MPEG4,
  MAX_CC_CORE_CODEC,
  NTT_TVQ,
  NO_CORE
};

enum _MAX {
  MAX_GRANULES            =  3,
  NSFB_LONG               = 53,
  NSFB_SHORT              = 16,
  MAX_SHORT_IN_LONG_BLOCK =  8,
  MAX_SHORT_WINDOWS       =  8
};

/* if static memory allocation is used, this value tells the max. nr of
   audio channels to be supported */

#define MAX_TIME_CHANNELS (MAX_CHANNELS + 2)   /* saftey margin */

/* max. number of scale factor bands */

#define MAX_SCFAC_BANDS ((NSFB_SHORT+1)*MAX_SHORT_IN_LONG_BLOCK)

#define SFB_NUM_MAX MAX_SCFAC_BANDS

#define	MAX_CHANNEL_ELEMENT	10  /* SAMSUNG_2005-09-30 : maximum channel elements for BSAC Multichannel */



#endif  /* #ifndef _TF_MAIN_HANDLE_H_INCLUDED */

