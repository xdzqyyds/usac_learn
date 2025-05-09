/*


This software module was originally developed by

    Kazuyuki Iijima  (Sony Corporation)

    and edited by

    Akira Inoue (Sony Corporation)

    in the course of development of the MPEG-4 Audio standard (ISO/IEC 14496-3).
    This software module is an implementation of a part of one or more
    MPEG-4 Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
    standard (ISO/IEC 14496-3).
    ISO/IEC gives users of the MPEG-4 Audio standards (ISO/IEC 14496-3)
    free license to this software module or modifications thereof for use
    in hardware or software products claiming conformance to the MPEG-4
    Audio standards (ISO/IEC 14496-3).
    Those intending to use this software module in hardware or software
    products are advised that this use may infringe existing patents.
    The original developer of this software module and his/her company,
    the subsequent editors and their companies, and ISO/IEC have no
    liability for use of this software module or modifications thereof in
    an implementation.
    Copyright is not released for non MPEG-4 Audio (ISO/IEC 14496-3)
    conforming products. The original developer retains full right to use
    the code for his/her own purpose, assign or donate the code to a third
    party and to inhibit third party from using the code for non MPEG-4
    Audio (ISO/IEC 14496-3) conforming products.
    This copyright notice must be included in all copies or derivative works.

    Copyright (c)1996.

                                                                  
*/


#ifndef _hvxcEnc_h_
#define _hvxcEnc_h_

#include "phi_priv.h"   /* 98/05/06 PRIV */	

#ifdef LINUX
# ifndef MINFLOAT
#  define	EPS	((float)1.40129846432481707e-45)
# else
#  define	EPS	MINFLOAT
# endif
#else
# define	EPS	0.0
#endif /* LINUX */

/* -------- Definition of functions -------- */

#ifdef __cplusplus
extern "C" {
#endif

void IPC_HVXCInit(void);

void IPC_HVXCEncParFrm(
short int	*frmBuf,
IdLsp		*idLsp,
int		*idVUV,
IdCelp		*idCelp,
float		*mfdpch,
IdAm		*idAm);

void IPC_PackPrm2Bit(
IdLsp		*idLsp,
int		idVUV,
IdCelp		*idCelp,
float		pitch,
IdAm		*idAm,
unsigned char	*encBit);

void IPC_lp_lpc_lsp(
float		*cma,
float	 	*lpw);

void IPC_pt_lsp_lpc(
float		*ptw,
float		*cma);

void vuv_decision(
float		*arys,
float		*mfdpch,
float		(*am)[3],
float		*rms,
float		*r0r,
int		*vuvFlg,
float		r0h);

int HvxcVUVDecisionVR(
float   *arys,
float   pitch,
float   *r0r,
float   r0h,
int     *idVUV);

void IPC_hp_filter(
float		*arys,
int		h);

void IPC_hp_filterp(
float		*arys,
int		h);

void IPC_hp_filterp2(
float		*arys,
int		h);

void IPC_hp_filter3(
float		*arys);

void IPC_hp_filter4(
float		*arys,
int		h);

void IPC_bpf(
float		*bufsf);

void IPC_make_bss(void);

void harm_srew(
float		frmPwr,
float		*arysres,
int		winSize,
int		offset,
float		pitchOL,
float		*mfdpch,
float		(*am)[3],
float		*rms,
float		*dummyHarm,
int		*normMode);

void pitch_estimation(
float		*arysRaw,
float		*r0r,
int		idVUV,
float		*pchOL,
float		*r0h);

void pitch_estimation1FA(
float		*arysRaw,
float		*r0r,
int		idVUV,
float		*pchOL,
float		*r0h);

float IPC_calc_alpha256(
float		*ary,
float		*alpha,
int		p);

float IPC_calc_alpha(
float		*ary,
float		*alpha,
int		p);

void IPC_alpha_lsp(
float		*alpha,
float		*lsp);

void IPC_calc_residue256(
float		*arys,
float		alphaq[P + 1],
float		*resi);

void IPC_calc_residue_cont(
float		*arys,
float		(*alphaip)[P + 1],
float		*resi);

void percep_weight(
float		qLsp[10],
float		*per_wt);

void percep_weight_alpha(
float		alpha[10],
float		*per_wt);

void IPC_quanlsp(
float		lspin[11],
float		lspout[11],
IdLsp		*idLsp);

void IPC_quanlsp_enh(
float		lspin[11],
float		qlspin[11],
float		lspout[11],
IdLsp		*idLsp,
float		wLsp[10]);

void IPC_set_const_lpcVM(void);

void IPC_set_const_lpcVM_dec(void);

void harm_quant(
float		(*am)[3],
float		*pch,
float		*per_wt,
float		*dumLSF,
int		normMode,
int		vuv,
int		*idxS,
int		*bitnum,
int		*idxG,
int		*id4k);

void IPC_make_f_coef(void);

void IPC_make_f_coef_dec(void);

void IPC_InterpolateParams(
float		rate,
int		*idVUV2,
float		(*lsp2)[10],
float		*pch2,
float		(*am2)[128][3],
float		(*uvExt2)[160],
int		*modVUV,
float		*modLsp,
float		*modPch,
float		(*modAm)[3],
float		*modUvExt);

void td_synt(
float		*qRes,
int		*VUVs,
float		(*qLsp)[10],
float		*synoutu);

void harm_srew_synt(
float		(*am)[3],
float		*PCHs,
int		*VUVs,
float		(*qLsp)[10],
int		lsUn,
float		*synoutv);

void IPC_make_w_celp(void);

void IPC_uvExt(
int		*vuv,
float		*suv,
float		*qRes);

void IPC_make_c_dis(void);

void IPC_vExt_fft(
float		*pch,
float		(*am)[3],
int		*vuv,
float		*sv);

void IPC_UvAdd(
float		*pch,
float		(*am)[3],
int		*vuv,
float		*add_uv);

void IPC_VUVDecisionRule(
float		fb,
float		lev,
short		int *pos,
float		vb,
int		numZeroXP,
float		tranRatio,
float		*r0r,
float		mfdpch);

void HvxcVUVDecisionRuleVR(
float           fb,
float           lev,
int             numZeroXP,
float           tranRatio,
float           *r0r,
float           mfdpch,
float           *gml,
int             *idVUV);

void td_encoder(
float		*arys,
float		rawLsp[10],
float		qLsp[10],
int		vuvFlg,
int		*idS,
int		*numBitS,
int		*idG,
int		*numBitG);

void td_decoder(
int		idVUV,
int		*idSL0,
int		*idGL0,
int		*idSL1,
int		*idGL1,
float		*qRes);

void IPC_ip_lsp(
float		(*lsp)[11],
float		(*lspip)[11]);

int AveragingCelpGain(
float           gain,
int             flag
);

/* Akira 980506 */
void
PAN_InitLpcAnalysisEncoder
(
long  order,                    /* In:  Order of LPC 			*/
PHI_PRIV_TYPE *PHI_Priv         /* In/Out: PHI private data (instance context) */
);

/*======================================================================*/
/*   Function Prototype: PAN_FreeLpcAnalysisEncoder                     */
/*======================================================================*/

/* Akira 980506 */
void
PAN_FreeLpcAnalysisEncoder
(
PHI_PRIV_TYPE *PHI_Priv         /* In/Out: PHI private data (instance context) */
);

/*======================================================================*/
/*   Function Prototype: celp_lpc_analysis                              */
/*======================================================================*/
void celp_lpc_analysis (float PP_InputSignal[],         /* In:  Input Signal                    */
                        float lpc_coefficients[],       /* Out: LPC Coefficients[0..lpc_order-1]*/
                        float *first_order_lpc_par,     /* Out: a_parameter for 1st-order fit   */
                        long  window_offsets[],         /* In:  offset for window w.r.t curr. fr*/
                        long  window_sizes[],           /* In:  LPC Analysis-Window Size        */
                        float *windows[],               /* In:  Array of LPC Analysis windows   */
                        long  SampleRateMode,
                        float *VAD_pcm_buffer,
                        long  vad_analysis_window_size,
                        int   *VAD_flag,
                        int   VAD_skip_flag,
                        float gamma_be[],               /* In:  Bandwidth expansion coefficients*/
                        long  lpc_order,                /* In:  Order of LPC                    */
                        long  n_lpc_analysis            /* In:  Number of LP analysis/frame     */
); 


long pc2lsf(		/* ret 1 on succ, 0 on failure */
float lsf[],		/* output: the np line spectral freq. */
const float pc[],	/* input : the np+1 predictor coeff. */
long np			/* input : order = # of freq to cal. */
);

void pan_lspqtz2_dd(float in[], float out_p[], float out[], 
    float weight[], float p_factor, float min_gap, 
    long lpc_order, long num_dc, long idx[], 
    float tbl[], float d_tbl[], float rd_tbl[], 
    long dim_1[], long ncd_1[], long dim_2[], long ncd_2[],
    long flagStab);

void pan_lspqtz2_ddVR(float in[], float out_p[], float out[],
    float weight[], float p_factor, float min_gap,
    long lpc_order, long num_dc, long idx[],
    float tbl[], float d_tbl[], float rd_tbl[],
    long dim_1[], long ncd_1[], long dim_2[], long ncd_2[], int level,
    int qMode);

void IPC_HVXCFree(void);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _hvxc_h_ */

