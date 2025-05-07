/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:

and edited by: Jeremie Lecomte (Fraunhofer IIS)

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

Assurance that the originally developed software module can be used (1) in
ISO/IEC 23003 once ISO/IEC 23003 has been adopted; and (2) to develop ISO/IEC
23003:s
Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, VoiceAge Corp.
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, VoiceAge Corp. 
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, VoiceAge Corp. retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
$Id: proto_func.h,v 1.14 2011-05-02 09:11:02 mul Exp $
*******************************************************************************/


#ifndef proto_func_h
#define proto_func_h

#include <stdio.h>

#include "typedef.h"
#include "td_frame.h"
#include "usac_mainStruct.h"
#include "usac_tcx_mdct.h"


#ifdef __RESTRICT
#define restrict _Restrict
#else
#define restrict
#endif


/* lag window lag_wind.c */
void init_lag_wind(float bwe,   /* input : bandwidth expansion */
                   float f_samp,        /* input : sampling frequency */
                   float wnc,   /* input : white noise correction factor */
                   int m);      /* input : order of LP filter */
void lag_wind(float r[],        /* in/out: autocorrelations */
              int m);           /* input : order of LP filter */

/* Common files */


/* bit packing and unpacking functions in bits.c */
int bin2int(                     /* output: recovered integer value              */
            int   no_of_bits,    /* input : number of bits associated with value */
            short *bitstream     /* input : address where bits are read          */
);

void int2bin(int value,         /* input : value to be converted to binary */
             int no_of_bits,    /* input : number of bits associated with value */
             short *bitstream); /* output: address where bits are written */

int get_num_prm(int qn1, int qn2);

int unary_decode(
   int                      *ind,
   HANDLE_RESILIENCE        hResilience,
   HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
   HANDLE_BUFFER            hVm
);

/* High-pass filter in hp50.c */
void hp50_12k8(Float32 signal[], Word32 lg, Float32 mem[], Word32 fscale);

/* int_lpc.c */
void int_lpc_np1(float isf_old[],       /* input : LSFs from past frame */
                 float isf_new[],       /* input : LSFs from present frame */
                 float a[],     /* output: LP coefficients in both subframes */
                 int nb_subfr,  /* input: number of subframe */
                 int m);        /* input : order of LP filter */

/* pitch predictor in pit_fr4.c */
void pred_lt4(float exc[],      /* in/out: excitation buffer */
              int T0,           /* input : integer pitch lag */
              int frac,         /* input : fraction of lag */
              int L_subfr);     /* input : subframe size */


/* util_td.c */
void set_zero(float *x, int n);

void mvr2r(const float x[],           /* input : input vector  */
           float y[],           /* output: output vector */
           int n);              /* input : vector size   */
void mvs2s(short x[],           /* input : input vector */
           short y[],           /* output: output vector */
           int n);              /* input : vector size */
void mvi2i(int x[],             /* input : input vector */
           int y[],             /* output: output vector */
           int n);              /* input : vector size */
void mvr2s(float x[],           /* input : input vector */
           short y[],           /* output: output vector */
           int n);              /* input : vector size */
void mvs2r(short x[],           /* input : input vector */
           float y[],           /* output: output vector */
           int n);              /* input : vector size */

void copyFLOAT(const float *X, float *Y, int n);
void smulFLOAT(float a, const float * restrict X, float * restrict Z, int n);
void subFLOAT(const float * restrict X, const float * restrict Y, float * restrict Z, int n);

void pessimize(void);
void addr2r(
  float x[],       /* input : input vector  */
  float y[],       /* input : input vector  */
  float z[],       /* output: output vector */
  int n );           /* input : vector size   */

void multr2r(
  float x[],       /* input : input vector  */
  float y[],       /* input : input vector  */
  float z[],       /* output: output vector */
  int n );           /* input : vector size   */

void subr2r(
  float x[],       /* input : input vector  */
  float y[],       /* input : input vector  */
  float z[],       /* output: output vector */
  int n);            /* input : vector size   */

void smulr2r(
  float a,         /* input : input factor  */
  float x[],       /* input : input vector  */
  float z[],       /* output: output vector */
  int n);            /* input : vector size   */

static int cfftn(float Re[],
                  float Im[],
                  int  nTotal,
                  int  nPass,
                  int  nSpan,
                  int  iSign);

int CFFTN(float *afftData,int len, int isign);

int CFFTNRI(float *afftDataReal,float *afftDataImag,int len, int isign);

int CFFTN_NI(float *InRealData,
             float *InImagData,
             float *OutRealData,
             float *OutImagData,
             int len, int isign);

int RFFTN(float *afftData, const float* trigPtr, int len, int isign);

float* CreateSineTable(int len);

void DestroySineTable(float* trigPtr);

/*---------------------------------------------------------------------*
 *              main routines                                          *
 *---------------------------------------------------------------------*/
int coder_amrwb_plus_first(   /* output: number of sample processed */
  float channel_right[], /* input: used on mono and stereo       */
  int L_next,        /* input: lookahead                         */
  HANDLE_USAC_TD_ENCODER st   /* i/o : coder memory state                 */
);

int coder_amrwb_plus_mono(  /* output: number of sample processed */
    float channel_right[], /* input: used on mono and stereo       */
    short serial[],    /* output: serial parameters                */
    short serialFac[],
    int   *nBitsFac,
    HANDLE_USAC_TD_ENCODER st,    /* i/o : coder memory state                 */
    int fscale,
    int isAceStart,
    int *total_nbbits,
    int mod_out[4],      /* output: coding mode per frame */
    int const bUsacIndependencyFlag
);

void decoder_LPD(
  HANDLE_USAC_TD_DECODER    st,
  td_frame_data            *td,
  int                  pit_adj,
  float               fsynth[],
  int              bad_frame[],
  int               isAceStart,
  int           short_fac_flag,
  int         isBassPostFilter);


int  decoder_LPD_end(  HANDLE_USAC_TD_DECODER tddec,
					HANDLE_BUFFER     hVm,
                   USAC_DATA *usac_data,
                   int i_ch);

void decoder_Synth_end(
  float signal_out[],/* output: signal with LPD delay (7 subfrs) */
  HANDLE_USAC_TD_DECODER st);       /* i/o:    decoder memory state pointer     */

void decoder_LPD_BPF_end(
  int   isShort,
  float out_buffer[],/* i/o: signal with LPD delay (7 subfrs) */
  HANDLE_USAC_TD_DECODER st);       /* i/o:    decoder memory state pointer     */

/*---------------------------------------------------------------------*
 *              low freq band routines (0..6400Hz)                     *
 *---------------------------------------------------------------------*/

void init_coder_lf(HANDLE_USAC_TD_ENCODER st, int L_frame, int fscale);
void config_coder_lf(HANDLE_USAC_TD_ENCODER st, int sampling_rate, int birate);
void close_coder_lf(HANDLE_USAC_TD_ENCODER st);
void reset_coder_lf(HANDLE_USAC_TD_ENCODER st);

void coder_lf_amrwb_plus(
                         short *codec_mode,    /* (i) : AMR-WB+ mode (see cnst.h)             */
                         float speech[],         /* (i) : speech vector [-M..L_FRAME_PLUS+L_NEXT]   */
                         float synth_enc[],      /* (o) : synthesis vector [-M..L_FRAME_PLUS]       */
                         int mod[],              /* (o) : mode for each 20ms frame (mode[4]     */
                         int n_param_tcx[],      /* (o) : numer of tcx parameters per 20ms subframe */
                         float window[],         /* (i) : window for LPC analysis               */
                         int param_lpc[],        /* (o) : parameters for LPC filters            */
                         int param[],            /* (o) : parameters (NB_DIV*NPRM_DIV)          */
                         short serialFac[],
                         int *nBitsFac,
                         float ol_gain[],        /* (o) : open-loop LTP gain                    */
                         int pit_adj,            /* (i) : indicate pitch adjustment             */
                         HANDLE_USAC_TD_ENCODER st,   /* i/o : coder memory state                    */
                         int isAceStart) ;

void init_decoder_lf(HANDLE_USAC_TD_DECODER st);
void reset_decoder_lf(HANDLE_USAC_TD_DECODER st, float* pOlaBuffer, int lastWasShort, int twMdct);
void close_decoder_lf(HANDLE_USAC_TD_DECODER st);
void acelpSetModeHistory(HANDLE_USAC_TD_DECODER self, int mode);
void acelpUpdateModeHistory(HANDLE_USAC_TD_DECODER self, int useACELP);

/*---------------------------------------------------------------------*
 *             Parameters encoding/decoding routines                   *
 *---------------------------------------------------------------------*/

void enc_prm_fac(
   int mod[],         /* (i) : frame mode (mode[4], 4 division) */
   int n_param_tcx[], /* (i) : number of parameters (freq. coeff) per tcx subframe */ 
   int codec_mode,    /* (i) : AMR-WB+ mode (see cnst.h)        */
   int param[],       /* (i) : parameters                       */
   int param_lpc[],   /* (i) : LPC parameters                   */
   int isAceStart, 
   int isBassPostFilter,
   short serial[],    /* (o) : serial bits stream               */
   HANDLE_USAC_TD_ENCODER st, /* io: quantization Analysis values    */
   int *total_nbbits, /* (o) : number of bits per superframe   */
   int const bUsacIndependencyFlag);

/*---------------------------------------------------------------------*
 *              ACELP routines                                         *
 *---------------------------------------------------------------------*/

void coder_acelp_fac(
   float A[],                  /* input: coefficients 4xAz[M+1]   */
   float Aq[],                 /* input: coefficients 4xAz_q[M+1] */
   float speech[],             /* input: speech[-M..lg]           */
   float wsig[],               /* input: wsig[-1..lg]             */
   float synth[],              /* out:   synth[-128..lg]          */
   float wsyn[],               /* out:   synth[-128..lg]          */
   short codec_mode,           /* input: AMR_WB+ mode (see cnst.h)*/
   LPD_state *mem,
   int lDiv,                   /* input: length of ACELP frame    */
   float norm_corr,
   float norm_corr2,
   int T_op,                   /* input: open-loop LTP            */  
   int T_op2,                  /* input: open-loop LTP            */  
   int	pit_adj,
   int *prm                    /* output: acelp parameters        */
);

void decoder_acelp_fac(
   td_frame_data *td,
   int             k,          /* input: frame number (within superframe) */
   float A[],                  /* input: coefficients NxAz[M+1]   */
   int codec_mode,             /* input: AMR-WB+ mode (see cnst.h)*/
   int bfi,                    /* input: 1=bad frame              */
   float exc[],                /* i/o:   exc[-(PIT_MAX+L_INTERPOL)..L_DIV] */
   float synth[],              /* i/o:   synth[-M..L_DIV]            */
   int *pT,                    /* out:   pitch for all subframe   */
   float *pgainT,              /* out:   pitch gain for all subfr */
   float stab_fac,             /* input: stability of isf         */
   HANDLE_USAC_TD_DECODER st   /* i/o :  coder memory state       */
);

/*---------------------------------------------------------------------*
 *              TCX routines                                           *
 *---------------------------------------------------------------------*/

void coder_tcx_fac(
   float A[],                  /* input: coefficients NxAz[M+1]   */
   float Aq[],                 /* input: coefficients NxAz_q[M+1] */
   float speech[],             /* input: speech[-M..lg]           */
   float wsig[],               /* input: wsig[-1..lg]             */
   float synth[],              /* out:   synth[-128..lg]          */
   float wsyn[],               /* out:   synth[-128..lg]          */
   int lg,                     /* input: frame length             */
   int lDiv,                   /* input: length of ACELP frame    */
   int nb_bits,                /* input: number of bits allowed   */
   LPD_state *mem,
   int prm[],                  /* output: tcx parameters */
   int *n_param,               /* output: number of tcx parameters to code */
   int lfacPrev,
   int lfacNext,
   HANDLE_TCX_MDCT hTcxMdct    /* in/out: handle for MDCT transform */
);

void decoder_tcx_fac(
   td_frame_data *td,
   int frame_index,            /* input: index of the presemt frame to decode*/
   float A[],                  /* input: coefficients NxAz[M+1]  */
   int L_frame,                /* input: frame length            */
   float exc[],                /* output: exc[-lg..lg]            */
   float synth[],              /* in/out: synth[-M..lg]           */
   HANDLE_USAC_TD_DECODER st   /* i/o :  coder memory state       */
);

void coder_fdfac(
   float *orig,
   int lDiv,
   int lfac,
   int lowpassLine,
   int targetBitrate,
   float *synth,
   float *Aq,
   short serial[],
   int *nBitsFac
);

/*----------------------------------------------*
 * LPC routines.                                *
 *----------------------------------------------*/

void cos_window(float *fh, int n1, int n2);


int vlpc_1st_cod( /* output: codebook index                  */
  float *lsf,     /* input:  vector to quantize              */
  float *lsfq);   /* i/o:    i:prediction   o:quantized lsf  */

int vlpc_2st_cod( /* output: number of allocated bits        */
  float *lsf,     /* input:  normalized vector to quantize   */
  float *lsfq,    /* i/o:    i:1st stage   o:1st+2nd stage   */
  int *indx,      /* output: index[] (4 bits per words)      */
  int mode);      /* input:  0=abs, >0=rel                   */

void reorder_lsf(float *lsf, float min_dist, int n);

void lsf_weight_2st(float *lsfq, float *w, int mode);

void E_LPC_isf_reorderPlus(float *isf, float min_dist, int n);

void vlpc_1st_dec(
  int index,      /* input:  codebook index                  */
  float *lsfq);   /* i/o:    i:prediction   o:quantized lsf  */

void vlpc_2st_dec(
  float *lsfq,    /* i/o:    i:1st stage   o:1st+2nd stage   */
  int *indx,      /* input:  index[] (4 bits per words)      */
  int mode);      /* input:  0=abs, >0=rel                   */

void qlpc_avq(
   float *LSF,       /* (i) Input LSF vectors              */
   float *LSF_Q,     /* (i) Quantized LFS vectors          */
   int lpc0,         /* (i) LPC0 vector is present         */
   int *index,       /* (o) Quantization indices           */
   int *nb_indices,  /* (o) Number of quantization indices */
   int *nbbits       /* (o) Number of quantization bits    */
);

void dlpc_avq(
   td_frame_data *td,
   int lpc0,         /* (i)   LPC0 vector is present         */
   float *LSF_Q,     /* (o)   Quantized LSF vectors                      */
   float *past_lsfq, /* (i/o) Past quantized LSFs for bad frame handling */
   int mod[],        /* (i)   amr-wb+ coding mode (acelp/tcx20,40 or 80) */
   int bfi
);

void E_LPC_lsp_lsf_conversion(
  float lsp[],    /* output: lsp[m] (range: -1<=val<1)                */
  float lsf[],    /* input : lsf[m] normalized (range: 0<=val<=6400)  */
  long m
);

void E_LPC_lsf_lsp_conversion(
  float lsf[],    /* input : lsf[m] normalized (range: 0<=val<=6400)  */
  float lsp[],    /* output: lsp[m] (range: -1<=val<1)                */
  int   m         /* input : LPC order                                */
);


void SAVQ_dec(
  int *indx,    /* input:  index[] (4 bits per words)      */
  int *nvecq,   /* output: vector quantized                */
  int Nsv       /* input:  number of subvectors (lg=Nsv*8) */ 
);

void SAVQ_cod(  
  float *nvec,  /* input:  vector to quantize (normalized) */
  int *nvecq,   /* output: quantized vector                */
  int *indx,    /* output: index[] (4 bits per words)      */
  int Nsv       /* input:  number of subvectors (lg=Nsv*8) */  
  );

/*---------------------------------------------------------------------*
 *                          TCX.H                                      *
 *---------------------------------------------------------------------*
 *             Prototypes of signal processing routines                *
 *---------------------------------------------------------------------*/

int q_gain2_plus(               /* (o) : index of quantizer */
                    float code[],       /* (i) : Innovative code vector */
                    int lcode,  /* (i) : Subframe size */
                    float *gain_pit,    /* (i/o): Pitch gain / Quantized pitch gain */
                    float *gain_code,   /* (i/o): code gain / Quantized codebook gain */
                    float *coeff,       /* (i) : correlations <y1,y1>, -2<xn,y1>, */
                                        /*       <y2,y2>, -2<xn,y2> and 2<y1,y2> */
                    float mean_ener);   /* (i) : mean_ener defined in open-loop (2 bits) */


float d_gain2_plus(             /* (o) : 'correction factor' */
				  int index,    /* (i) : index of quantizer */
                  float code[], /* (i) : Innovative code vector */
                  int lcode,    /* (i) : Subframe size */
                  float *gain_pit,      /* (o) : Quantized pitch gain */
                  float *gain_code,     /* (o) : Quantized codebook gain */
                  int bfi,      /* (i) : Bad frame indicato */
                  float mean_ener,      /* (i) : mean_ener defined in open-loop (2 bits) */
                  float *past_gpit,     /* (i) : past gain of pitch */
                  float *past_gcode);   /* (i/o): past gain of code */


void find_wsp(float A[], float speech[],        /* speech[-M..lg] */
              float wsp[],      /* wsp[0..lg] */
              float *mem_wsp,   /* memory */
              int lg);



int get_nb_bits(short extension, short mode, short st_mode);

/*---------------------------------------------------------------------*
 *                          FAC+FDNS                                   *
 *---------------------------------------------------------------------*/

int FDFac(int   sfbOffsets[],
          int sfbActive,
          double *origTimeSig,
          WINDOW_SEQUENCE windowSequence,
          double *synthTime,
          HANDLE_USAC_TD_ENCODER     hAcelp,
          int              lastSubFrameWasAcelp,
          int              nextFrameIsLPD,
          short           *facPrmOut,
          int             *nBitsFac);


int fac_decoding( int                      lFac,
                  int                      k,
                  int                     *facPrm,
                  HANDLE_RESILIENCE        hResilience,
                  HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                  HANDLE_BUFFER            hVm );


float segsnr(                   /* return: segmential signal-to-noise ratio in dB */
                float x[],      /* input : input sequence of length n samples */
                float xe[],     /* input : estimate of x */
                short n,        /* input : signal length */
                short nseg      /* input : segment length */
    );

float get_gain(                 /* output: codebook gain (adaptive or fixed) */
                  float x[],    /* input : target signal */
                  float y[],    /* input : filtered codebook excitation */
                  int L_subfr   /* input : subframe size */
    );

float SQ_gain(   /* output: SQ gain                   */ 
  float x[],     /* input:  vector to quantize        */
  int nbitsSQ,   /* input:  number of bits targeted   */ 
  int lg);       /* input:  vector size (2048 max)    */ 

int* getRe8Prm(HANDLE_USAC_TD_ENCODER st);

void AdaptLowFreqEmph(float x[], int lg);
void AdaptLowFreqDeemph(float x[], int lg, float gains[]);
void lpc2mdct(float *lpcCoeffs, int lpcOrder, float *mdct_gains, int lg);
void mdct_IntPreShaping(float x[], int lg, int FDNS_NPTS, float old_gains[], float new_gains[]);
void mdct_IntNoiseShaping(float x[], int lg, int FDNS_NPTS, float old_gains[], float new_gains[]);

void int_lpc_acelp(float lsf_old[],  /* input : LSFs from past frame              */
                   float lsf_new[],  /* input : LSFs from present frame           */
                   float a[],        /* output: LP coefficients in both subframes */
                   int   nb_subfr,   /* input: number of subframe                 */
                   int   m);         /* input : order of LP filter                */
void int_lpc_tcx(float lsf_old[],  /* input : LSFs from past frame              */
                 float lsf_new[],  /* input : LSFs from present frame           */
                 float a[],        /* output: LP coefficients in both subframes */
                 int   nb_subfr,   /* input: number of subframe                 */
                 int   m);         /* input : order of LP filter                */
int encode_fac(int *prm, short *ptr, int lFac);

void decode_fdfac(int *facPrm, int lDiv, int lfac, float *Aq, float *zir, float *xn2);

float *getLastLpc(HANDLE_USAC_TD_DECODER st);
float *getAcelpZir(HANDLE_USAC_TD_DECODER st);

float * acelpGetZir(HANDLE_USAC_TD_ENCODER hAcelp);
float * acelpGetLastLPC(HANDLE_USAC_TD_ENCODER hAcelp);
int acelpLastSubFrameWasAcelp(HANDLE_USAC_TD_ENCODER hAcelp) ;
void acelpUpdatePastFDSynthesis(HANDLE_USAC_TD_ENCODER hAcelp, double* pPastFDSynthesis, double* pPastFDOrig, int lowpassLine);

int lsf_mid_side(   /* output: 0=old_lpc, 1=new_lpc              */
  float lsf_old[],  /* input : LSFs from past frame              */
  float lsf_mid[],  /* input : LSFs from mid frame               */
  float lsf_new[]); /* input : LSFs from present frame           */ 

#endif
