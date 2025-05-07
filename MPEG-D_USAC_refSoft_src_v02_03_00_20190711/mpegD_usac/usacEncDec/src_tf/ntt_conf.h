/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Naoki Iwakami (NTT) on 1996-12-06,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Naoki Iwakami (NTT) on 1997-07-17,                                      */
/*   Takehiro Moriya (NTT) on 1997-08-01,                                    */
/*   Naoki Iwakami (NTT) on 1997-08-25                                       */
/*   Akio Jin (NTT) on 1997-10-23,                                           */
/*   Takehiro Moriya (NTT) on 2000-08-11,                                    */
/*   Takeshi Mori (NTT) on 2000-11-21,                                       */
/*   Olaf Kaehler (Fraunhofer IIS-A) on 2003-11-23                           */
/* in the course of development of the                                       */
/* MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.        */
/* This software module is an implementation of a part of one or more        */
/* MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio */
/* standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards   */
/* free license to this software module or modifications thereof for use in  */
/* hardware or software products claiming conformance to the MPEG-2 AAC/     */
/* MPEG-4 Audio  standards. Those intending to use this software module in   */
/* hardware or software products are advised that this use may infringe      */
/* existing patents. The original developer of this software module and      */
/* his/her company, the subsequent editors and their companies, and ISO/IEC  */
/* have no liability for use of this software module or modifications        */
/* thereof in an implementation. Copyright is not released for non           */
/* MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer       */
/* retains full right to use the code for his/her  own purpose, assign or    */
/* donate the code to a third party and to inhibit third party from using    */
/* the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.             */
/* This copyright notice must be included in all copies or derivative works. */
/* Copyright (c)1996.                                                        */
/*****************************************************************************/

/* 06-dec-96   NI   provided 6 kbit/s support */
/* 18-apr-96   NI   minor bug fix */
/* 01-aug-96   TM   provided 6 kbit/s support for 8 kHz*/
/* 25-aug-97   NI   added bandwidth control */
/* 10-mar-99   TM   remove all global variables */

/*---------------------------------------------------------------------------*/
/* Header file of TwinVQ audio coding program                                */
/*---------------------------------------------------------------------------*/

#ifndef _ntt_conf_h
#define _ntt_conf_h

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include "tf_mainHandle.h"
#include "tf_mainStruct.h"
#include "block.h"
#include "tns.h"
#include "bitstream.h"
#include "obj_descr.h"
#include "ntt_tvq.h"

/*---------------------------------------------------------------------------*/
/* GENERAL SETTINGS                                                          */
/*---------------------------------------------------------------------------*/

#define	YES		1
#define	NO		0

#define	ON		1
#define	OFF		0



/* Interleave set mode select */
enum ntt_INTERLEAVE_SET_MODE {
  ntt_INTR_LONG,
  ntt_INTR_MEDIUM,
  ntt_INTR_SHORT,
  ntt_INTR_PITCH,
  ntt_INTR_ALL
};

/* VM perceptual weight mode select */
enum NTT_PW_SELECT {
  NTT_PW_INTERNAL,
  NTT_PW_EXTERNAL
};

/* VM variable bit mode select */
enum NTT_VARBIT{
  NTT_VARBIT_OFF,
  NTT_VARBIT_ON
};

/* Block length */
enum ntt_BLOCK_LENGTH {
  ntt_BLOCK_LONG,
  ntt_BLOCK_MEDIUM,
  ntt_BLOCK_SHORT
};

#define	ntt_max(x,y)	( (x) > (y) ? (x) : (y) )
#define	ntt_min(x,y)	( (x) < (y) ? (x) : (y) )

#ifndef PI
#define	PI	3.141592653589793238462643383279
#endif
#define ntt_SQRT1_2 0.70710678118654752440

/*---------------------------------------------------------------------------*/
/* PROGRAM MODE                                                              */
/*---------------------------------------------------------------------------*/

#define ntt_LSP_TO_WT       YES

#define ntt_N_SUP_MAX       2
#define ntt_N_SUP_MIN       1

#define ntt_SAMPF_MAX         48000
#define ntt_SAMPF_MIN          8000
#define ntt_BPS_MAX           64000
#define ntt_N_PR_MAX             20
#define ntt_N_PR_MIN             10
#define ntt_N_FR_MAX           1024
#define ntt_N_SHRT_MAX          8


#define ntt_T_SHRT_MAX (ntt_N_SHRT_MAX*ntt_N_SUP_MAX)
#define ntt_T_SHRT_MIN (ntt_N_SHRT_MIN*ntt_N_SUP_MIN)


#define ntt_T_FR_MAX (ntt_N_FR_MAX*ntt_N_SUP_MAX) /* total input frame size */
#define ntt_T_FR_MIN (ntt_N_FR_MIN*ntt_N_SUP_MIN) /* total input frame size */
#define ntt_T_FR_S_MAX (ntt_N_FR_S_MAX*ntt_N_SUP_MAX)
#define ntt_T_FR_S_MIN (ntt_N_FR_S_MIN*ntt_N_SUP_MIN)

#define ntt_CB_INCLUDE NO

/*---------------------------------------------------------------------------*/
/* LSP QUANTIZATION                                                          */
/*---------------------------------------------------------------------------*/

#define ntt_LSP_BIT0_MAX  1  /* num of bits for prediction switch */
#define ntt_LSP_BIT1_MAX  6  /* num of bits at first stage */
#define ntt_LSP_BIT2_MAX  4  /* num of bits per split vq at second stage */ 
#define ntt_LSP_SPLIT_MAX 3  /* num of split at second stage */

#define ntt_LSP_BIT0_MIN  1  /* num of bits for prediction switch */
#define ntt_LSP_BIT1_MIN  6  /* num of bits at first stage */
#define ntt_LSP_BIT2_MIN  4  /* num of bits per split vq at second stage */ 
#define ntt_LSP_SPLIT_MIN 2  /* num of split at second stage */

#define ntt_NSP     ntt_LSP_SPLIT
#define ntt_NSP_MAX ntt_LSP_SPLIT_MAX
#define ntt_NSP_MIN ntt_LSP_SPLIT_MIN

#define ntt_N_MODE_MAX (1<<ntt_LSP_BIT0_MAX)
#define ntt_N_MODE_MIN (1<<ntt_LSP_BIT0_MIN)

#define ntt_NC0_MAX (1<<ntt_LSP_BIT1_MAX)
#define ntt_NC0_MIN (1<<ntt_LSP_BIT1_MIN)

#define ntt_NC1_MAX (1<<ntt_LSP_BIT2_MAX)
#define ntt_NC1_MIN (1<<ntt_LSP_BIT2_MIN)

#define ntt_LSP_NIDX_MAX \
 (ntt_LSP_BIT0_MAX + ntt_LSP_BIT1_MAX + ntt_LSP_BIT2_MAX*ntt_LSP_SPLIT_MAX)
#define ntt_LSP_NIDX_MIN \
 (ntt_LSP_BIT0_MIN + ntt_LSP_BIT1_MIN + ntt_LSP_BIT2_MIN*ntt_LSP_SPLIT_MIN)

#define ntt_MA_NP      1  /* MA prediction order for LSP */
#define ntt_LSP_NSTAGE 2

#define ntt_SIDE_INF_SW 4           /* side information switch w-size   */

/*---------------------------------------------------------------------------*/
/* GAIN QUANTIZATION                                                         */
/*---------------------------------------------------------------------------*/
#define	ntt_AMP_NM 1024.0        /* Normalized amplitude of resid           */
#define ntt_I_AMP_NM (1./ntt_AMP_NM)    /* Inverse normalized amplitude */
#define	ntt_POW_NORM (ntt_AMP_NM*ntt_AMP_NM)/* Normalized amplitude of resid */

#define ntt_GAIN_BITS_MIN 5
#define ntt_GAIN_BITS_MAX 14
#define ntt_SUB_GAIN_BITS_MIN 2
#define ntt_SUB_GAIN_BITS_MAX 10

#define	  ntt_SUB_AMP_NM   1024.0 /* Normalized amplitude of resid           */

/*---------------------------------------------------------------------------*/
/* FORWARD ENVELOPE QUANTIZATION                                             */
/*---------------------------------------------------------------------------*/

#define ntt_N_CRB_MAX     96
#define ntt_N_CRB_M_MAX   96
#define ntt_N_CRB_S_MAX   12

#define ntt_FW_N_DIV_MAX  16  /* Number of division of envelope codebook */
#define ntt_FW_N_DIV_MIN   4  /* Number of division of envelope codebook */

#define ntt_FW_N_DIV_S_MAX  4      /* Num. div. of env. cb (short frame) */
#define ntt_FW_N_DIV_S_MIN  1      /* Num. div. of env. cb (short frame) */

#define ntt_FW_N_DIV_M_MAX  16      /* Num. div. of env. cb (short frame) */
#define ntt_FW_N_DIV_M_MIN  1      /* Num. div. of env. cb (short frame) */

#define ntt_FW_N_DIV_MX (ntt_max(ntt_FW_N_DIV_MAX,ntt_FW_N_DIV_S_MAX*ntt_N_SHRT_MAX))
                                  /* Envelope index array size per ch. */
#define ntt_FW_N_BIT_MAX  10      /* Envelope codebook bits */
#define ntt_FW_N_BIT_MIN  6       /* Envelope codebook bits */
#define ntt_FW_CB_SIZE_MAX (1<<ntt_FW_N_BIT_MAX)   /* Envelope codebook size */
#define ntt_FW_CB_SIZE_MIN (1<<ntt_FW_N_BIT_MAX)   /* Envelope codebook size */
#define ntt_FW_CB_LEN_MAX (ntt_N_CRB_MAX/ntt_FW_N_DIV_MIN)/* codebook length */

#define ntt_FW_N_BIT_S_MAX  8    /* Envelope codebook bits */
#define ntt_FW_N_BIT_S_MIN  4    /* Envelope codebook bits */
#define ntt_FW_CB_SIZE_S_MAX (1<<ntt_FW_N_BIT_S_MAX) /* codebook size */
#define ntt_FW_CB_SIZE_S_MIN (1<<ntt_FW_N_BIT_S_MAX) /* codebook size */
#define ntt_FW_CB_LEN_S_MAX (ntt_N_CRB_S_MAX/ntt_FW_N_DIV_S_MIN) /* length */

#define ntt_FW_N_BIT_M_MAX  10 /* Envelope codebook bits */
#define ntt_FW_N_BIT_M_MIN  4  /* Envelope codebook bits */
#define ntt_FW_CB_SIZE_M_MAX (1<<ntt_FW_N_BIT_M_MAX) /* codebook size */
#define ntt_FW_CB_SIZE_M_MIN (1<<ntt_FW_N_BIT_M_MAX) /* codebook size */
#define ntt_FW_CB_LEN_M_MAX (ntt_N_CRB_M_MAX/ntt_FW_N_DIV_M_MIN) /* length */

#define ntt_FW_CB_LEN_MX \
(ntt_max(ntt_FW_CB_LEN_MAX, ntt_max(ntt_FW_CB_LEN_M_MAX, ntt_FW_CB_LEN_S_MAX)))

#define ntt_FW_ARQ_NBIT   1 /* prediction coefficient quant. bits */
#define ntt_FW_ARQ_NSTEP  (1<<ntt_FW_ARQ_NBIT)
#define ntt_FW_ALF_MAX  0.5 /* Maximum value of quantized coefficient */
#define ntt_FW_ALF_STEP (ntt_FW_ALF_MAX/(ntt_FW_ARQ_NSTEP-1))
                                  /* Coefficient quantization step width */

#define ntt_FW_LLIM     1.e-1         /* Lower limit of decoded envelope */

#define ntt_FW_TBIT_MAX   (ntt_FW_N_BIT_MAX*ntt_FW_N_DIV_MAX+ntt_FW_ARQ_NBIT)
#define ntt_FW_TBIT_S_MAX \
(ntt_FW_N_BIT_S_MAX*ntt_FW_N_DIV_S_MAX+ntt_FW_ARQ_NBIT)
#define ntt_FW_TBIT_M_MAX \
(ntt_FW_N_BIT_M_MAX*ntt_FW_N_DIV_M_MAX+ntt_FW_ARQ_NBIT)
#define ntt_FW_TBIT_MIN    (ntt_FW_N_BIT_MIN*ntt_FW_N_DIV_MIN+ntt_FW_ARQ_NBIT)
#define ntt_FW_TBIT_S_MIN \
(ntt_FW_N_BIT_S_MIN*ntt_FW_N_DIV_S_MIN+ntt_FW_ARQ_NBIT)
#define ntt_FW_TBIT_M_MIN \
(ntt_FW_N_BIT_M_MIN*ntt_FW_N_DIV_M_MIN+ntt_FW_ARQ_NBIT)

/*---------------------------------------------------------------------------*/
/* PITCH FILTER                                                              */
/*---------------------------------------------------------------------------*/
#define ntt_N_FR_FFT   512

#define ntt_N_PULSE_MAX 50

#define ntt_PGAIN_MAX    20000.

#define ntt_N_FR_P_MAX   40
#define ntt_N_DIV_P_MAX  20
#define ntt_MAXBIT_SHAPE_P    6 /* maxbit for pitchVQ per channel */
#define ntt_PIT_N_BIT_MAX (ntt_MAXBIT_SHAPE_P+1)
#define ntt_PIT_CB_SIZE_MAX (1<<ntt_PIT_N_BIT_MAX)
#define ntt_POLBITS_P   1
#define ntt_MAXBIT_P    (ntt_MAXBIT_SHAPE_P+ntt_POLBITS_P)

#define ntt_CB_LEN_P_MAX 24


/*---------------------------------------------------------------------------*/
/* POST FILTER                                                               */
/*---------------------------------------------------------------------------*/
#define ntt_POSTFILT   YES
#if ntt_POSTFILT
#    define ntt_PF_SW_BIT  1
#else
#    define ntt_PF_SW_BIT  0
#endif
#define ntt_PF_OFF     0      /* Post Filtering off */
#define ntt_PF_LOW     1
#define ntt_PF_MID     2
#define ntt_PF_HIGH    3

/*---------------------------------------------------------------------------*/
/* BIRATE CONTROL                                                           */
/*---------------------------------------------------------------------------*/
#define ntt_NTITLE_BITS   16
#define ntt_NCOMMENT_BITS 16
#define ntt_CHAR_BITS      8
#define ntt_ISAMPF_BITS    6
#define ntt_IBPS_BITS      8

#define ntt_NBITS_FR_MAX \
((int)(ntt_N_FR_MAX*ntt_BPS_MAX/ntt_SAMPF_MIN)) /* Number of bits per frame */
#define ntt_NBITS_FR_MIN \
((int)(ntt_N_FR_MIN*ntt_BPS_MIN/ntt_SAMPF_MAX)) /* Number of bits per frame */

#define ntt_SIDE_INF_MAX \
(GAIN_BIT_MAX+ntt_LSP_SPLIT_MAX*ntt_LSP_BIT2_MAX+ntt_LSP_BIT1_MAX+ntt_LSP_BIT0_MAX+ntt_BASF_BIT)
#define ntt_SIDE_INF_MIN \
(ntt_GAIN_BITS_MIN+ntt_LSP_SPLIT_MIN*ntt_LSP_BIT2_MIN+ntt_LSP_BIT1_MIN+ntt_LSP_BIT0_MIN)

#define ntt_SUB_FRAME_LSP  NO
#    define ntt_SIDE_INF_FR_MAX \
(ntt_SIDE_INF_SW+ntt_LSP_SPLIT_MAX*ntt_LSP_BIT2_MAX+ntt_LSP_BIT1_MAX+ntt_LSP_BIT0_MAX+GAIN_BIT_MAX)
#    define ntt_SIDE_INF_S_MIN (ntt_GAIN_BITS_MIN+ntt_SUB_GAIN_BITS_MIN*ntt_N_SHRT_MIN)
#    define ntt_SIDE_INF_FR_MIN \
(ntt_LSP_SPLIT_MIN*ntt_LSP_BIT2_MIN+ntt_LSP_BIT1_MIN+ntt_LSP_BIT0_MIN+ntt_SIDE_INF_S_MIN)

#define ntt_TBIT_MAX \
(ntt_NBITS_FR_MAX-ntt_SIDE_INF_MIN-ntt_FW_TBIT_MIN-ntt_SIDE_INF_SW)
#define ntt_TBIT_MIN \
(ntt_NBITS_FR_MIN-ntt_SIDE_INF_MAX-ntt_FW_TBIT_MAX-ntt_SIDE_INF_SW)

#define ntt_TBIT_S_MAX \
(ntt_NBITS_FR_MAX-ntt_FW_TBIT_S_MIN*ntt_T_SHRT_MIN-ntt_SIDE_INF_FR_MIN*ntt_N_SUP_MIN-ntt_SIDE_INF_SW)

#define ntt_TBIT_M_MAX \
(ntt_NBITS_FR_MAX-ntt_FW_TBIT_M_MIN*ntt_T_MID_MIN-ntt_SIDE_INF_FR_MIN*ntt_N_SUP_MIN-ntt_SIDE_INF_SW)

/*---------------------------------------------------------------------------*/
/* RESIDUAL SIGNAL QUANTIZATION                                              */
/*---------------------------------------------------------------------------*/
#define ntt_MAXBIT_SHAPE     5
#define ntt_POLBITS          1    /* num bits for polarity per channel */
#define ntt_MAXBIT (ntt_MAXBIT_SHAPE+ntt_POLBITS)     /* max bit per channel */
#define	ntt_CB_SIZE (0x1 << ntt_MAXBIT_SHAPE)
#define	ntt_N_DIV_MAX  500
#define ntt_CB_LEN_MAX 256
#define ntt_CB_LEN_MGN 40

#define	ntt_N_DIV_S_MAX	500
#define ntt_CB_LEN_S_MAX   256

#define	ntt_N_DIV_M_MAX	500
#define ntt_CB_LEN_M_MAX   256


typedef struct {
    int         wvq[ntt_max(ntt_N_DIV_MAX*2,ntt_N_DIV_S_MAX*2)];
    int         fw[ntt_FW_N_DIV_MX*ntt_N_SUP_MAX];
    int         fw_alf[ntt_N_SHRT_MAX*ntt_N_SUP_MAX];
    int         pow[(ntt_N_SHRT_MAX+1)*ntt_N_SUP_MAX];
    int         lsp[ntt_N_SUP_MAX][ntt_LSP_NIDX_MAX];
    int         pit[ntt_N_SUP_MAX];
    int         pls[ntt_N_DIV_P_MAX*2];
    int         pgain[ntt_N_SUP_MAX];
    int         blim_h[ntt_N_SUP_MAX];
    int         blim_l[ntt_N_SUP_MAX];
    int         pf;
    int         pitch_q;
    int         bandlimit;
    int         postprocess;
    int         ms_mask;
    WINDOW_SEQUENCE w_type;
    /*NOK_LT_PRED_STATUS (*lt_status)[MAX_TIME_CHANNELS];*/
    int         nokia_LPTOOL_BITS;
    int         max_sfb[8];
    int         msMask[8][60];
    int         group_code;
    int         last_max_sfb[8];
    int		restore_flag_tns;
    int         tvq_tns_enable;
    int         tvq_ppc_enable;
    ntt_DATA_scl* nttDataScl;
    ntt_DATA_base* nttDataBase;
    int            numChannel;
    int            block_size_samples;
    int            isampf;
    int            er_tvq; /* object type ER_TWIN_VQ (1)or TWIN_VQ (0) */
    int            mainDebugLevel;
} ntt_INDEX;

typedef struct{
    int fine_sw;
    int speech_sw;
    int ntt_param_set_flag;  /* used for VM execution control */
} ntt_PARAM;

#define    ntt_N_SHRT        8
#define    ntt_GAIN_BITS     9
#define    ntt_AMP_MAX       16000.
#define    ntt_SUB_GAIN_BITS 4
#define    ntt_SUB_AMP_MAX   4700.
#define    ntt_MU            100.
#define    ntt_N_PR          20
#define    ntt_LSPCODEBOOK   "./tables/tf_vq_tbls/20b19s48bs"
#define    ntt_LSP_BIT0      1
#define    ntt_LSP_BIT1      6
#define    ntt_LSP_BIT2      4
#define    ntt_LSP_SPLIT     3
#define    ntt_FW_CB_NAME    "./tables/tf_vq_tbls/fcdl"
#define    ntt_FW_N_DIV      7
#define    ntt_FW_N_BIT      6
#define    ntt_PIT_CB_NAME   "./tables/tf_vq_tbls/pcdl"
#define    ntt_BASF_BIT      8
#define    ntt_PIT_N_BIT     28
#define    ntt_PIT_CB_LEN    20
#define    ntt_PGAIN_BIT     7
#define    ntt_PGAIN_MU      200
#define    ntt_CB_NAME0       "./tables/tf_vq_tbls/cdmdct_0"
#define    ntt_CB_NAME1       "./tables/tf_vq_tbls/cdmdct_1"
#define    ntt_CB_NAME0s      "./tables/tf_vq_tbls/cdmdct_2"
#define    ntt_CB_NAME1s      "./tables/tf_vq_tbls/cdmdct_3"
#define    ntt_BLIM_BITS_H    2
#define    ntt_BLIM_BITS_L    1
#define    ntt_CUT_M_H        0.7
#define    ntt_CUT_M_L        0.0025

#define    ntt_CB_LEN_READ    20
#define    ntt_CB_LEN_READ_S  17
#define    ntt_N_MODE        (1<<ntt_LSP_BIT0)
#define    ntt_NC0           (1<<ntt_LSP_BIT1)
#define    ntt_NC1           (1<<ntt_LSP_BIT2)

#define  ntt_NUM_STEP      (1<<ntt_GAIN_BITS)
#define  ntt_STEP          (ntt_AMP_MAX / (ntt_NUM_STEP - 1))
#define  ntt_SUB_NUM_STEP  (1<<ntt_SUB_GAIN_BITS)
#define  ntt_SUB_STEP      (ntt_SUB_AMP_MAX / (ntt_SUB_NUM_STEP - 1))

#define  ntt_N_CRB      42

#define ntt_FW_CB_SIZE   (1<<ntt_FW_N_BIT)
#define ntt_FW_CB_LEN    (ntt_N_CRB/ntt_FW_N_DIV)
#define ntt_FW_CB_SIZE_S 0
#define ntt_FW_CB_LEN_S  0
#define ntt_BASF_STEP    ((1<<ntt_BASF_BIT)-1)
#define ntt_PIT_CB_SIZE  (1<<ntt_MAXBIT_SHAPE_P)
#define ntt_PGAIN_NSTEP  ((1<<ntt_PGAIN_BIT) - 1)
#define ntt_N_FR_P       ntt_PIT_CB_LEN

#define ntt_CB_LEN_P     10
#define ntt_BWID_BITS    (ntt_BLIM_BITS_H + ntt_BLIM_BITS_L)
#define ntt_BLIM_STEP_H  ((1 << ntt_BLIM_BITS_H) - 1)

#define ntt_PIT_TBITperCH (ntt_PIT_N_BIT+ntt_BASF_BIT+ntt_PGAIN_BIT)
#define ntt_N_DIV_PperCH  (ntt_PIT_N_BIT/ntt_MAXBIT_P/2)
#define ntt_TBIT_PperCH   ntt_PIT_N_BIT

#define LSP_TBITperCH  (ntt_LSP_BIT0+ntt_LSP_BIT1+ntt_LSP_BIT2*ntt_LSP_SPLIT)
#define FW_TBITperCH   (ntt_FW_N_BIT*ntt_FW_N_DIV+ntt_FW_ARQ_NBIT)
#define FW_TBIT_S       0
#define GAIN_TBITperCH   ntt_GAIN_BITS
#define GAIN_TBIT_SperCH (ntt_GAIN_BITS+ntt_SUB_GAIN_BITS*ntt_N_SHRT)
#define ntt_NMTOOL_BITSperCH (LSP_TBITperCH+GAIN_TBITperCH+FW_TBITperCH)
#define ntt_NMTOOL_BITS_SperCH (LSP_TBITperCH+GAIN_TBIT_SperCH)


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/****************************  FUNCTIONS  ************************************/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

  /*--- VM socket modules ---*/
  /* encoder */
  void ntt_vq_coder
    (double      *spectral_line_vector[MAX_TIME_CHANNELS],
     /* Input : frequency-domain signal array    */
     double      external_pw[], /* Input : external perceptual weight       */
     int         pw_select,	/* Input : perceptual weight mode selector  */
     WINDOW_SEQUENCE windowSequence,/* Input : window block type            */
     ntt_INDEX   *index,	/* Output: quantization indexes             */
     ntt_PARAM   *param_ntt,	/* Input : encoder parameters               */
     int     sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
     int         nr_of_sfb[MAX_TIME_CHANNELS],
     int         available_bits,/* Input : number of available bits         */
     double      lpc_spectrum[], /* Output: LPC spectrum                     */
     double      *reconstructed_spectrum[MAX_TIME_CHANNELS],
                                /* Output: reconstructed spectrum */
     int         debug_level);

  
  /* decoder */
  void ntt_vq_decoder
    (ntt_INDEX  *index,
     double     *spectral_line_vector[MAX_TIME_CHANNELS],
     Info       **sfbInfo);
  
  void ntt_scale_vq_decoder(ntt_INDEX  index_scl[],
			    ntt_INDEX  *index_base,
			    int    mat_shift[],
			    double *spectral_line_vector[MAX_TIME_CHANNELS],
			    int    iscl,
                            Info   **sfbInfo);

  void ntt_tf_local_decode
    (/* Input */
     ntt_INDEX *index,
     double    lpc_spectrum[],
     double    bark_env[],
     double    pitch_sequence[],
     double    gain[],
     double    spectrum_gain,
     /* Output */
     double    *reconstructed_spectrum[MAX_TIME_CHANNELS]);

  void ntt_scale_vq_coder(double    *spectral_line_vector[MAX_TIME_CHANNELS],
			  double    lpc_spectrum[],
			  ntt_INDEX *index,
			  ntt_INDEX *index_scl,
			  ntt_PARAM *param_ntt,
			  int       mat_shift[],
			  int       sfb_width_table
				   [MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
			  int       nr_of_sfb[MAX_TIME_CHANNELS],
		          int       ntt_available_bits,
			  double    *reconstructed_spectrum[MAX_TIME_CHANNELS],
			  int       iscl);

  int ntt_SclBitUnPack(HANDLE_BSBITSTREAM fixed_stream,
		       ntt_INDEX   index[],
		       int         mat_shift[],
                       int decoded_bits,
                       FRAME_DATA* fd,
		       int        *nextLayer,
		       int         available_bits,
		       int         iscl);
  
  void ntt_vec_lenp(int numchannel,
		    int bits[],
		    int length[]);

  void ntt_init(void);

  void ntt_fwex(int    index[],
		int    ndiv,
		int    cv_len,
		double *codebook,
		int    cv_len_max,
		double env[]);

  void   ntt_movdd(int n, double xx[], double yy[]);

  double ntt_mulawinv(double y, double xmax, double mu);

  void ntt_lsp_decw(/* Parameters */
                  int    n_pr,
                  int    nsp,
                  double code[][ntt_N_PR_MAX],
                  double fgcode[][ntt_MA_NP][ntt_N_PR_MAX],
                  int    *csize,
                  int    prev_lsp_code[],
		  /*
		  double buf_prev[ntt_MA_NP][ntt_N_PR_MAX+1],
                  */
                  int    ma_np,
		  /* Input */
                  int    index[],
                  /* Output */
                  double freq[]);

  void ntt_lsptowt_int(int nfr,
		       int  block_size_samples,
		       int n_pr,
		       double lsp[],
		       double wt[],
		       double cos_TT[]);
/*
  void ntt_cp_mdtbl(ntt_MODE_TABLE ltbl);
*/  
  void ntt_cnst_chk(int  calcv,
		    int  defv,
		    char *defname);

  void ntt_get_code(char *fname,
		    int nstage,
		    int csize[],
		    int cdim[],
		    double code[][ntt_N_PR_MAX],
		    double fgcode[ntt_N_MODE_MAX][ntt_MA_NP][ntt_N_PR_MAX]);

  void ntt_get_cdbk(char *name,
		    double *codev_l,
		    int cb_size,
		    int cb_len,
		    int cb_len_mx);

  void ntt_set_interleave(enum ntt_INTERLEAVE_SET_MODE set_mode);

  void ntt_set_isp(int nsp, int n_pr, int *ntt_isp);

  void ntt_redec(int n_pr,
               int index[],
               int csize1[],
               int nsp,
               double code[][ntt_N_PR_MAX],
               double fg_sum[ntt_N_PR_MAX],
               double pred_vec[ntt_N_PR_MAX],
               double lspq[],
               double out_vec[]);

  void ntt_dist_lsp(int n, double x[], double y[], double w[], double *dist);

  void ntt_check_lsp(int n, double buf[], double min_gap);

  void ntt_check_lsp_sort(int n, double buf[]);

  void   ntt_zerod(int n,double xx[]);

  void ntt_dec_bark_env(/* Parameters */
			int    nfr,
			int    nsf,
			int    n_ch,
			double *codebook,
			int    ndiv,
			int    cv_len,
			int    cv_len_max,
			int    *bark_tbl,
			int    n_crb,
			double alf_step,
			int    prev_fw_code[],
			/* Input */
			int    index_fw[],
			int    index_fw_alf[],
			int    pf_switch,
			/* Output */
			double bark_env[]);

  void ntt_dec_gain(/* Parameters */
		    int    nsf,
		    /* Input */
		    int    index_pow[],
		    int    numChannel,
		    /* Output */
		    double gain[]);

  void ntt_dec_lpc_spectrum_inv
  (
   /* Parameters */
   int    nfr,
   int    block_size_samples,
   int    n_ch,
   int    n_pr,
   int    nsp,
   double *lsp_code,
   double *lsp_fgcode,
   int    *csize,
   int    prev_lsp_code[],
   /*
   double prev_buf[ntt_N_SUP_MAX][ntt_MA_NP][ntt_N_PR_MAX+1],
   */
   int    ma_np,
   /* Input */
   int    index_lsp[ntt_N_SUP_MAX][ntt_LSP_NIDX_MAX],
   /* Output */
   double inv_lpc_spec[],
   double *cos_TT
   );

  void ntt_denormalizer_spectrum(/* Parameters */
				 int    nfr,
				 int    nsf,
				 int    n_ch,
				 /* Input */
				 double flat_spectrum[],
				 double gain[],
				 /*****/
				 double pit_sec[],
				 double pgain[],
				 /*****/
				 double bark_env[],
				 double inv_lpc_spec[], /**** inv */
				 /* Output */
				 double spectrum[]);

  void ntt_post_process(/* Parameters */
			int    nfr,
			int    nsf,
			double band_lower,
			double band_upper,
			/* Input */
			double spectrum[],
			/* Output */
			double out_spectrum[]);

  void ntt_vex_pn(int    *index,    /* Input : VQ indices */
		  double *sp_cv0,    /* Input : shape codebook (conj. ch. 0) */
		  double *sp_cv1,    /* Input : shape codebook (conj. ch. 1) */
		  int    cv_len_max, /* Input : memory length of codevectors */
		  int    n_sf,    /* Input : number of subframes in a frame */
		  int    block_size, /* Input : total block size */
		  int    available_bits, /* Input : available bits */
		  double *sig);       /* Output: Reconstructed coefficients */

  void ntt_tf_requantize_spectrum(/* Input */
				  ntt_INDEX *indexp,
				  /* Output */
				  double flat_spectrum[]);

  void ntt_extend_pitch(/* Input */
			int     index_pit[],
			double  pit_pack[],
			int     numChannel,
			int     block_size_samples,
			int     isampf,
			double  bandUpper,
			/* Output */
			double  pit_seq[]);

  void ntt_base_init(float sampling_rate,
		  float bit_rate,
		  float t_bit_rate_scl,
		  long num_channel,
		  int  samples,
		  ntt_INDEX* ntt_index
		  );
  
  void ntt_base_free(ntt_DATA_base *nttDataBase);
  
  int ntt_BitUnPack ( HANDLE_BSBITSTREAM   stream,
                      int           available_bits,
                      int           decoded_bits,
                      FRAME_DATA*    fd,
                      WINDOW_SEQUENCE   block_type,
                      ntt_INDEX*    index);

  int ntt_BitPack(ntt_INDEX   *index,
                  HANDLE_BSBITSTREAM *stream_for_esc,
		  int mat_shift[ntt_N_SUP_MAX],
                  int iscl);


  void ntt_dec_pgain(/* Input */
		     int    index,
		     /* Output */
		     double *pgain);

  void ntt_dec_pit_seq(/* Input */
		       int    index_pit[],
		       int    index_pls[],
		       int    numChannel,
		       int    block_size_samples,
		       int    isampf,
		       double bandUpper,
		       double *codevp0,
		       short *pleave1,
		       /* Output */
		       double pit_seq[]);

  void ntt_dec_pitch(/* Input */
		     int    index_pit[],
		     int    index_pls[],
		     int    index_pgain[],
		     int    numChannel,
		     int    block_size_sample,
		     int    isampf,
		     /* Output */
		     double pit_seq[],
		     double pgain[],
		     double bandUpper,
		     double *ptr,
		     short *pleave);

  void ntt_tf_proc_spectrum_d(/* Input */
			      ntt_INDEX  *indexp,
			      double flat_spectrum[],
			      /* Output */
			      double spectrum[]);

  int TVQ_decode_grouping( int grouping, short region_len[] );

  void ntt_headerdec(int iscl,
		     HANDLE_BSBITSTREAM  stream, 
                     ntt_INDEX* index, Info** sfbInfo ,
                     int* decoded_bits,
		     TNS_frame_info tns_info[MAX_TIME_CHANNELS],
		     NOK_LT_PRED_STATUS **nok_lt_status,
		     PRED_TYPE pred_type,
                     HANDLE_DECODER_GENERAL hFault );

  int ntt_get_tns_vm( HANDLE_BSBITSTREAM fixed_stream, 
		       Info *info, 
		       TNS_frame_info *tns_frame_info,
		       int* decoded_bits );
  void ntt_weight(int npr, double costbl[], double costablv, double *wtv);

#define BYTE_ALIGN  YES /* T.Ishikawa 980727 */
#ifdef __cplusplus
}
#endif

#endif

