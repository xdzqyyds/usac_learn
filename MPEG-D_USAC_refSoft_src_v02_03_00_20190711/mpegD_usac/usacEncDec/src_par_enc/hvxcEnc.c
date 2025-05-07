
/*


This software module was originally developed by

    Kazuyuki Iijima (Sony Corporation)
 
    and edited by

    Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.),
    Akira Inoue (Sony Corporation) and
    Yuji Maeda (Sony Corporation)

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


#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "hvxc.h"
#include "hvxcCommon.h"
#include "hvxcEnc.h"
#include "pan_par_const.h"

#include "hvxcProtect.h"

extern float	ipc_coef[SAMPLE];
extern float	ipc_coef160[FRM];

extern int	ipc_encMode;
extern int	ipc_decMode;

extern int	ipc_rateMode; /* YM 020110 */

extern int	ipc_encDelayMode;
extern int	ipc_decDelayMode;

extern int	ipc_bitstreamMode;
int judge = 0;

extern int	ipc_packMode;
extern int	ipc_extensionFlag;

extern int	ipc_vrScalFlag;

/* 971112 */
static float **hamWindow;
static float *gammaBE;

void IPC_HVXCInit(void)
{
    int	i;

    IPC_makeCoef(ipc_coef, SAMPLE);

    for(i = 48; i < 208; i++)
    {
	ipc_coef160[i - 48] = ipc_coef[i];
    }

    IPC_make_bss();
    IPC_set_const_lpcVM();
    IPC_make_f_coef();

/* 971112 */
    if(NULL==(hamWindow=(float **)calloc(1, sizeof(float *)))) {
	printf("\n Memory allocation error in HvxcInit\n");
	exit(1);
    }
    if(NULL==(*hamWindow=(float *)calloc((PAN_WIN_LEN_PAR), sizeof(float)))) {
	printf("\n Memory allocation error in HvxcInit\n");
	exit(1);
    }
    if(NULL==(gammaBE=(float *)calloc(P, sizeof(float)))) {
	printf("\n Memory allocation error in HvxcInit\n");
	exit(1);
    }
    for(i=0;i<PAN_WIN_LEN_PAR;i++) {
	*(*hamWindow+i) = (float)(.54-.46*cos((6.283185307*i)
		/(double)(PAN_WIN_LEN_PAR)));
    }
    *gammaBE = (PAN_GAMMA_BE_PAR);
    for(i=1;i<P;i++) {
	*(gammaBE+i) = (*(gammaBE+i-1))*(PAN_GAMMA_BE_PAR);
    }

}

/* 971112 */
void IPC_HVXCFree(void)
{
    free(*hamWindow);
    free(hamWindow);
    free(gammaBE);
}

#define LONG64	/* for 64bit platform like IRIX64@R10000(AI 990219) */

static void IPC_Normalization(
float	arysRaw[SAMPLE],
float	arysOut[FRM],
float	arysres[SAMPLE],
float	*frmPwr,
float	alpha[P],
float	rawLsp[P],
float	qLsp[P],
IdLsp	*idLsp,
int	*offset)
{
    float alphae[P + 1];
    float lsp[P + 1];
    float lspIn[P + 1];
    float lspOut[P + 1];

    static int		frm = 0;

    int		i;

    float	tmp;

    float	arysTmp[SAMPLE];

    float	alphaq[P + 1];

    static long window_sizes[1]={PAN_WIN_LEN_PAR};
    static long window_offsets[1]={PAN_WIN_OFFSET_PAR};

    float alphaTmp[P];
    float lspTmp[P];
    float first_order_lpc_par; /* dummy */

#include "inc_lsp_575.tbl"
    float panLsp[P];
    static float prevQLsp[P];

    static float p_factor=PAN_LSP_AR_R_PAR;
    static float min_gap=PAN_MINGAP_PAR;
    static long num_dc=PAN_N_DC_LSP_PAR;
    float lspWeight[P];
    float dLsp[P+1];
    float w_fact;

    float *dummybuf = NULL;
    int   *dummyflag = NULL;

#ifdef LONG64
    long idx[L_VQ];
#endif
    if(0==frm) {
        for(i=0;i<P;i++) prevQLsp[i] = (i+1.)/(float)(P+1);
    }

    for(i = 0; i < SAMPLE; i++)
    {
	arysTmp[i] = arysRaw[i];
    }

    IPC_hp_filter4(arysTmp, frm);

    for(i = 0; i < FRM; i++)
    {
	arysOut[i] = arysTmp[OVERLAP / 2 + i];
    }


    tmp = 0.0;
    for(i = 0; i < FRM; i++)
    {
	tmp += arysOut[i] * arysOut[i];
    }
    *frmPwr = tmp / (float) FRM;
    
    celp_lpc_analysis (arysTmp, 
                       alphaTmp, 
                       &first_order_lpc_par, 
                       window_offsets, 
                       window_sizes,  
                       hamWindow,
                       (long) 0, 
                       dummybuf,
                       (long) 0,
                       dummyflag, 
                       1,
                       gammaBE,
                       PAN_LPC_ORDER_PAR, 
                       PAN_NUM_ANA_PAR);


    alphae[0] = 1.;
    for(i=0;i<P;i++) alphae[i+1] = -alphaTmp[i];

/* LPC -> LSP */
    pc2lsf(lspTmp, alphae, P);

    for(i=0;i<P;i++) lspTmp[i] /= PAN_PI;

    lsp[0] = 0.;
    for(i=0;i<P;i++) lsp[i+1] = lspTmp[i]*.5;
	
    for(i=0;i<P;i++) panLsp[i] = lsp[i+1]*2.;

    dLsp[0] = panLsp[0];
    for(i=1;i<P;i++) dLsp[i] = panLsp[i]-panLsp[i-1];
    dLsp[P] = 1.-panLsp[P-1];
    for(i=0;i<=P;i++) {
        if(dLsp[i]<min_gap) dLsp[i] = min_gap;
    }
    for(i=0;i<=P;i++) dLsp[i] = 1./dLsp[i];
    for(i=0;i<P;i++) lspWeight[i] = dLsp[i]+dLsp[i+1];

    w_fact = 1.;
    for(i=0;i<4;i++) {
        lspWeight[i] *= w_fact;
    }
    for(i=4;i<8;i++) {
        w_fact *= .694;
        lspWeight[i] *= w_fact;
    }
    for(i=8;i<10;i++) {
        w_fact *= .510;
        lspWeight[i] *= w_fact;
    }

#ifdef LONG64
    pan_lspqtz2_dd(panLsp, prevQLsp, lspOut+1,
        lspWeight, p_factor, min_gap, P, num_dc, 
	idx, lsp_tbl, d_tbl, pd_tbl, 
        dim_1, ncd_1, dim_2, ncd_2, 1);
    for (i = 0; i < L_VQ; i++) idLsp->nVq[i] = (int)idx[i];
#else
    pan_lspqtz2_dd(panLsp, prevQLsp, lspOut+1,
        lspWeight, p_factor, min_gap, P, num_dc, 
        (long *)idLsp->nVq, lsp_tbl, d_tbl, pd_tbl, 
        dim_1, ncd_1, dim_2, ncd_2, 1);
#endif

/* 98.1.16 */
    for(i=0;i<P;i++) prevQLsp[i] = lspOut[i+1];

    if(ENC4K==ipc_encMode)
    {
	for(i = 0; i < P + 1; i++)
	{
	    lspIn[i] = 2.0 * lsp[i];
	}
	
	IPC_quanlsp_enh(lspIn, lspOut, lspOut, idLsp, lspWeight);
    }
    
    for(i=0;i<P;i++) lspOut[i+1] *= .5;

    for(i = 0; i < P; i++)
    {
	alpha[i] = alphae[i + 1];
    }
    
    for(i = 0; i < P; i++)
    {
	rawLsp[i] = lsp[i + 1];
    }
    
    for(i = 0; i < P; i++)
    {
	qLsp[i] = lspOut[i + 1];
    }

    alphaq[0] = 1.0;

    IPC_lsp_lpc(lspOut, alphaq);

    for(i = 1; i < 11; i++)
    {
	alphaq[i] *= -1.0;
    }

    IPC_calc_residue256(arysTmp, alphaq, arysres);
    
    *offset = - OVERLAP / 2;

    frm++;
    return;
}


static int AveragingLsp(
float 	stateLsp[][P],
int	aveIdx[]
)
{
    int         i, j;

    float       tmp;
    float lspTmp[P], lspOut[P+1];
    /* float first_order_lpc_par; : dummy */
#include "inc_lsp_575.tbl"
    float aveLsp[P];

    static float p_factor=PAN_LSP_AR_R_PAR;
    static float min_gap=PAN_MINGAP_PAR;
    static long num_dc=PAN_N_DC_LSP_PAR;
    float lspWeight[P];
    float dLsp[P+1];
    float w_fact;

    int qMode = 1;

#ifdef LONG64
    long idx[L_VQ];
#endif
    
    for(i=0;i<P;i++) lspTmp[i] = (i+1.)/(float)(P+1);
   
    for(i=0;i<P;i++) {
	tmp = 0;
	for (j = 0; j < 3; j++) tmp += stateLsp[j][i];
	aveLsp[i] = tmp / 3.0;
    }
    dLsp[0] = aveLsp[0];
    for(i=1;i<P;i++) dLsp[i] = aveLsp[i]-aveLsp[i-1];
    dLsp[P] = 1.- aveLsp[P-1];
    for(i=0;i<=P;i++) {
        if(dLsp[i]<min_gap) dLsp[i] = min_gap;
    }
    for(i=0;i<=P;i++) dLsp[i] = 1./dLsp[i];
    for(i=0;i<P;i++) lspWeight[i] = dLsp[i]+dLsp[i+1];

    w_fact = 1.;
    for(i=0;i<4;i++) {
        lspWeight[i] *= w_fact;
    }
    for(i=4;i<8;i++) {
        w_fact *= .694;
        lspWeight[i] *= w_fact;
    }
    for(i=8;i<10;i++) {
        w_fact *= .510;
        lspWeight[i] *= w_fact;
    }

#ifdef LONG64
    pan_lspqtz2_ddVR(aveLsp, lspTmp, lspOut+1,
        lspWeight, p_factor, min_gap, P, num_dc, 
	idx, lsp_tbl, d_tbl, pd_tbl, 
        dim_1, ncd_1, dim_2, ncd_2, 1, qMode);
    for (i = 0; i < L_VQ; i++) aveIdx[i] = (int)idx[i];

#else
    pan_lspqtz2_ddVR(aveLsp, lspTmp, lspOut+1,
        lspWeight, p_factor, min_gap, P, num_dc, 
        (long *)aveIdx, lsp_tbl, d_tbl, pd_tbl, 
        dim_1, ncd_1, dim_2, ncd_2, 1, qMode);

#endif

    return ( 0 );
}

static void IPC_NormalizationVR(
float   arysRaw[SAMPLE],
float   arysOut[FRM],
float   arysres[SAMPLE],
float   *frmPwr,
float   alpha[P],
float   rawLsp[P],
float   qLsp[P],
IdLsp   *idLsp,
int     *offset,
int     idVUV)
{
    float alphae[P + 1];
    float lsp[P + 1];
    float lspIn[P + 1];
    float lspOut[P + 1];

    static int          frm = 0;

    int         i;

    float       tmp;

    float       arysTmp[SAMPLE];

    float       alphaq[P + 1];

    static long window_sizes[1]={PAN_WIN_LEN_PAR};
    static long window_offsets[1]={PAN_WIN_OFFSET_PAR};

    float alphaTmp[P];
    float lspTmp[P];
    float first_order_lpc_par; /* dummy */
#include "inc_lsp_575.tbl"
    float panLsp[P];
    static float prevQLsp[P];

    static float p_factor=PAN_LSP_AR_R_PAR;
    static float min_gap=PAN_MINGAP_PAR;
    static long num_dc=PAN_N_DC_LSP_PAR;
    float lspWeight[P];
    float dLsp[P+1];
    float w_fact;

    int qMode = 0;
    static int prevVUV = 0;

    int j;
    int aveIdx[L_VQ];
    static float stateLsp[3][P];
    float *dummybuf = NULL;
    int   *dummyflag = NULL;

#ifdef LONG64
    long idx[L_VQ];
#endif
    if(0==frm) {
        for(i=0;i<P;i++) prevQLsp[i] = (i+1.)/(float)(P+1);
    }

    for(i = 0; i < SAMPLE; i++)
    {
        arysTmp[i] = arysRaw[i];
    }

    IPC_hp_filter4(arysTmp, frm);

    for(i = 0; i < FRM; i++)
    {
        arysOut[i] = arysTmp[OVERLAP / 2 + i];
    }


    tmp = 0.0;
    for(i = 0; i < FRM; i++)
    {
        tmp += arysOut[i] * arysOut[i];
    }
    *frmPwr = tmp / (float) FRM;
    
    celp_lpc_analysis (arysTmp, 
                       alphaTmp, 
                       &first_order_lpc_par, 
                       window_offsets, 
                       window_sizes,  
                       hamWindow,
                       (long) 0, 
                       dummybuf,
                       (long) 0,
                       dummyflag, 
                       1,
                       gammaBE,
                       PAN_LPC_ORDER_PAR, 
                       PAN_NUM_ANA_PAR);
    
    alphae[0] = 1.;
    for(i=0;i<P;i++) alphae[i+1] = -alphaTmp[i];

/* LPC -> LSP */
    pc2lsf(lspTmp, alphae, P);

    for(i=0;i<P;i++) lspTmp[i] /= PAN_PI;

    lsp[0] = 0.;
    for(i=0;i<P;i++) lsp[i+1] = lspTmp[i]*.5;

    for(i=0;i<P;i++) panLsp[i] = lsp[i+1]*2.;

    for (i = 0; i < 2; i++)
        for (j = 0; j < P; j++)
            stateLsp[i][j] = stateLsp[i+1][j];
    for (j = 0; j < P; j++)
        stateLsp[2][j] = panLsp[j];

    dLsp[0] = panLsp[0];
    for(i=1;i<P;i++) dLsp[i] = panLsp[i]-panLsp[i-1];
    dLsp[P] = 1.-panLsp[P-1];
    for(i=0;i<=P;i++) {
        if(dLsp[i]<min_gap) dLsp[i] = min_gap;
    }
    for(i=0;i<=P;i++) dLsp[i] = 1./dLsp[i];
    for(i=0;i<P;i++) lspWeight[i] = dLsp[i]+dLsp[i+1];

    w_fact = 1.;
    for(i=0;i<4;i++) {
        lspWeight[i] *= w_fact;
    }
    for(i=4;i<8;i++) {
        w_fact *= .694;
        lspWeight[i] *= w_fact;
    }
    for(i=8;i<10;i++) {
        w_fact *= .510;
        lspWeight[i] *= w_fact;
    }

    if(idVUV == 1 || prevVUV == 1)
    {
        qMode = 1;
    }

#ifdef LONG64

    pan_lspqtz2_ddVR(panLsp, prevQLsp, lspOut+1,
        lspWeight, p_factor, min_gap, P, num_dc,
        idx, lsp_tbl, d_tbl, pd_tbl,
        dim_1, ncd_1, dim_2, ncd_2, 1, qMode);
    for (i = 0; i < L_VQ; i++) idLsp->nVq[i] = (int)idx[i];

#else

    pan_lspqtz2_ddVR(panLsp, prevQLsp, lspOut+1,
        lspWeight, p_factor, min_gap, P, num_dc,
        (long *)idLsp->nVq, lsp_tbl, d_tbl, pd_tbl,
        dim_1, ncd_1, dim_2, ncd_2, 1, qMode);

#endif

    if (idVUV == 1 && ENC4K==ipc_encMode) {
        AveragingLsp( stateLsp, aveIdx );
        for (i = 0; i < L_VQ; i++) idLsp->nVq[i] = aveIdx[i];
    }

/* 98.1.20 */
    for(i=0;i<P;i++) prevQLsp[i] = lspOut[i+1];

    if(ENC4K==ipc_encMode && idVUV != 0 && idVUV != 1) {
        for(i = 0; i < P + 1; i++)
        {
            lspIn[i] = 2.0 * lsp[i];
        }

        IPC_quanlsp_enh(lspIn, lspOut, lspOut, idLsp, lspWeight);
    }

    for(i=0;i<P;i++) lspOut[i+1] *= .5;

    for(i = 0; i < P; i++)
    {
        alpha[i] = alphae[i + 1];
    }

    for(i = 0; i < P; i++)
    {
        rawLsp[i] = lsp[i + 1];
    }

    for(i = 0; i < P; i++)
    {
        qLsp[i] = lspOut[i + 1];
    }




    alphaq[0] = 1.0;

    IPC_lsp_lpc(lspOut, alphaq);

    for(i = 1; i < 11; i++)
    {
        alphaq[i] *= -1.0;
    }

    IPC_calc_residue256(arysTmp, alphaq, arysres);

    *offset = - OVERLAP / 2;

    prevVUV = idVUV;

    frm++;
    return;
}

void IPC_HVXCEncParFrm(
short int	*frmBuf,
IdLsp		*idLsp,
int		*idVUV,
IdCelp		*idCelp,
float		*mfdpch,
IdAm		*idAm)
{
    int		i, m;
    float	arysres[SAMPLE];
    static float	arys[FRM];

    static float	arysRaw[2][SAMPLE];

    static float	alpha[LPCORDER];
    static float	rawLsp[LPCORDER];
    static float	qLsp[LPCORDER];

    static float	ringBuffer[SAMPLE];

    float	pitchOL;

    float       r0h;

    float	rms[SAMPLE], am[SAMPLE/2][3];

    float       *dumLSF = NULL;
    float	*dummyHarm = NULL;

    int		idAmS[2], idAmG, bitNum;
    int		idAm4k[4];
    int		idCelpSL0[N_SFRM_L0], idCelpGL0[N_SFRM_L0];
    int		idCelpSL1[N_SFRM_L1], idCelpGL1[N_SFRM_L1];

    float	per_wt[129];

    float	frmPwr;

    static float	r0r[3];

    static int	frm = 0;

    static int	normMode = 0;
    static int	offset = -48;

    int		tmpIdVUV;

    static int prevVUV = 0;	/* AI 990322 */

    if(frm == 0)
    {
	for(i = 0; i < FRM; i++)
	{
	    arys[i] = 0.0;
	}

	if(ipc_encDelayMode == DM_LONG)
	{
	    for(i = 0; i < SAMPLE; i++)
	    {
		arysRaw[0][i] = 0.0;
		arysRaw[1][i] = 0.0;
	    }
	}
	else
	{
	    for(i = 0; i < SAMPLE; i++)
	    {
		arysRaw[0][i] = 0.0;
	    }
	}

	for(i = 0; i < SAMPLE; i++)
	{
	    ringBuffer[i] = 0.0;
	}
    }


    if(ipc_encDelayMode == DM_LONG)
    {
	for(m = 0; m < SAMPLE; m++)
	{
	    arysRaw[0][m] = arysRaw[1][m];
	}
    }


    for(i = 0; i < FRM; i++)
    {
	ringBuffer[(frm * FRM + i) % SAMPLE] = (float) frmBuf[i];
    }
    
    if(ipc_encDelayMode == DM_LONG)
    {
	for(m = 0; m < SAMPLE; m++)
	{
	    arysRaw[1][m] =
		ringBuffer[(m + frm * FRM - OVERLAP + SAMPLE) % SAMPLE];
	}
    }
    else
    {
	for(m = 0; m < SAMPLE; m++)
	{
	    arysRaw[0][m] =
		ringBuffer[(m + frm * FRM - OVERLAP + SAMPLE) % SAMPLE];
	}
    }	

    if(ipc_bitstreamMode == BM_VARIABLE)
    {
        if(ipc_encDelayMode == DM_LONG)
        {
            pitch_estimation(arysRaw[1], r0r, prevVUV, &pitchOL, &r0h);	/* AI 990324 */
        }
        else
        {
            pitch_estimation(arysRaw[0], r0r, prevVUV, &pitchOL, &r0h);	/* AI 990324 */
        }
        idCelp->upFlag =
            HvxcVUVDecisionVR(arysRaw[0], pitchOL, r0r, r0h, idVUV);
        IPC_NormalizationVR(arysRaw[0], arys, arysres, &frmPwr, alpha, rawLsp,
                          qLsp, idLsp, &offset, *idVUV);

        harm_srew(frmPwr, arysres, 256, offset, pitchOL, mfdpch, am, rms,
	          dummyHarm, &normMode);
    }
    else
    {
        IPC_Normalization(arysRaw[0], arys, arysres, &frmPwr, alpha, rawLsp,
		          qLsp, idLsp, &offset);

        if(ipc_encDelayMode == DM_LONG)
        {
            pitch_estimation(arysRaw[1], r0r, prevVUV, &pitchOL, &r0h);	/* AI 990324 */
        }
        else
        {
            pitch_estimation(arysRaw[0], r0r, prevVUV, &pitchOL, &r0h);	/* AI 990324 */
        }

        harm_srew(frmPwr, arysres, 256, offset, pitchOL, mfdpch, am, rms,
	          dummyHarm, &normMode);

        vuv_decision(arysRaw[0], mfdpch, am, rms, r0r, idVUV, r0h);
	
    }
    
    prevVUV = *idVUV;	/* AI 990324 */

    if(ipc_bitstreamMode == BM_VARIABLE)
    {
        if(*idVUV == 1)
        {
            tmpIdVUV = 0;
        }
	else
	{
	    tmpIdVUV = *idVUV;
	}
        td_encoder(arys, rawLsp, qLsp, tmpIdVUV, idCelpSL0, idCelpGL0,
               idCelpSL1, idCelpGL1);
    }
    else
        td_encoder(arys, rawLsp, qLsp, *idVUV, idCelpSL0, idCelpGL0,
	       idCelpSL1, idCelpGL1);

    for(i = 0; i < 2; i++)
    {
	idCelp->idSL0[i] = idCelpSL0[i];
	idCelp->idGL0[i] = idCelpGL0[i];
    }
    
    if(ipc_encMode == ENC4K)
    {
	for(i = 0; i < N_SFRM_L1; i++)
	{
	    idCelp->idSL1[i] = idCelpSL1[i];
	    idCelp->idGL1[i] = idCelpGL1[i];
	}
    }
    
    percep_weight_alpha(alpha, per_wt);

    if(ipc_bitstreamMode == BM_VARIABLE)
    {
        if(*idVUV == 1)
        {
            tmpIdVUV = 0;
        }
        else
        {
            tmpIdVUV = *idVUV;
        }
        harm_quant(am, mfdpch, per_wt, dumLSF, normMode, tmpIdVUV, idAmS,
	           &bitNum, &idAmG, idAm4k);
    }
    else
    {
        harm_quant(am, mfdpch, per_wt, dumLSF, normMode, *idVUV, idAmS,
	           &bitNum, &idAmG, idAm4k);
    }

    idAm->idS0 = idAmS[0];  
    idAm->idS1 = idAmS[1]; 
    idAm->idG = idAmG;   

    if(ipc_encMode == ENC4K)
    {
	idAm->id4kS0 = idAm4k[0];
	idAm->id4kS1 = idAm4k[1];
	idAm->id4kS2 = idAm4k[2];
	idAm->id4kS3 = idAm4k[3];
    }
    
    frm++;
}


static void PackBitsChar(char *encBitChar, int *ptr, int param, int nbit)
{
    int     i;
    
    for(i = 0; i < nbit; i++)
    {
	if(param & (0x01 << (nbit - i - 1)))
	{
	    encBitChar[*ptr] = '1';
	}
	else
	{
	    encBitChar[*ptr] = '0';
	}
	(*ptr)++;
    }
}


static void Prm2Bit(EncPrm *encPrm, unsigned char *encBit)
{
    char	encBitChar[80]; /* for 4kbps one frame */
    int		i, j;
    int		ptr = 0;

    PackBitsChar(encBitChar, &ptr, encPrm->idLsp.nVq[0], 
        PAN_BIT_LSP18_0);
    PackBitsChar(encBitChar, &ptr, encPrm->idLsp.nVq[1], 
        PAN_BIT_LSP18_1);
    PackBitsChar(encBitChar, &ptr, encPrm->idLsp.nVq[2], 
        PAN_BIT_LSP18_2);
    PackBitsChar(encBitChar, &ptr, encPrm->idLsp.nVq[3], 
        PAN_BIT_LSP18_3);
    PackBitsChar(encBitChar, &ptr, encPrm->idLsp.nVq[4], 
        PAN_BIT_LSP18_4);

    PackBitsChar(encBitChar, &ptr, encPrm->vuvFlag, 2);

    if(encPrm->vuvFlag == 0)
    {
	for(i = 0; i < N_SFRM_L0; i++)
	{
	    PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idUV.idSL0[i], 6);
	    PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idUV.idGL0[i], 4);
	}
    }
    else
    {
	PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idV.pchcode, 7);
	PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idV.idS0, 4);
	PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idV.idS1, 4);
	PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idV.idG, 5);
    }

    for(i = 0; i < 5; i++)
    {
	encBit[i] = 0;
	for(j = 0; j < 8; j++)
	{
	    if(encBitChar[i * 8 + j] == '1')
	    {
		encBit[i] |= 0x01 << (8 - j - 1);
	    }
	}
    }


    if(ipc_encMode == ENC4K)
    {
	PackBitsChar(encBitChar, &ptr, encPrm->idLsp.nVq[5], 8);
	
	if(encPrm->vuvFlag == 0)
	{
	    for(i = 0; i < N_SFRM_L1; i++)
	    {
		PackBitsChar(encBitChar, &ptr,
			     encPrm->vuvPrm.idUV.idSL1[i], 5);
		PackBitsChar(encBitChar, &ptr,
			     encPrm->vuvPrm.idUV.idGL1[i], 3);
	    }
	}
	else
	{
	    PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idV.id4kS0, 7);
	    PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idV.id4kS1, 10);
	    PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idV.id4kS2, 9);
	    PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idV.id4kS3, 6);
	}
	
	for(i = 5; i < 10; i++)
	{
	    encBit[i] = 0;
	    for(j = 0; j < 8; j++)
	    {
		if(encBitChar[i * 8 + j] == '1')
		{
		    encBit[i] |= 0x01 << (8 - j - 1);
		}
	    }
	}
    }
}

static void Prm2BitVR(EncPrm *encPrm, unsigned char *encBit)
{
    char        encBitChar[80]; /* for 4kbps one frame */
    int         i, j;
    int         ptr = 0;

    for(i = 0; i < 80; i++)
    {
        encBitChar[i] = 0;
    }

    PackBitsChar(encBitChar, &ptr, encPrm->vuvFlag, 2);

    if(encPrm->vuvFlag != 1)
    {
        PackBitsChar(encBitChar, &ptr, encPrm->idLsp.nVq[0],
                     PAN_BIT_LSP18_0);
        PackBitsChar(encBitChar, &ptr, encPrm->idLsp.nVq[1],
                     PAN_BIT_LSP18_1);
        PackBitsChar(encBitChar, &ptr, encPrm->idLsp.nVq[2],
                     PAN_BIT_LSP18_2);
        PackBitsChar(encBitChar, &ptr, encPrm->idLsp.nVq[3],
                     PAN_BIT_LSP18_3);
        PackBitsChar(encBitChar, &ptr, encPrm->idLsp.nVq[4],
                     PAN_BIT_LSP18_4);

        if(encPrm->vuvFlag == 0)
        {
            for(i = 0; i < N_SFRM_L0; i++)
            {
                if(ipc_encMode == ENC4K)
                    PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idUV.idSL0[i], 6);
                PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idUV.idGL0[i], 4);
            }
        }
        else
        {
            PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idV.pchcode, 7);
            PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idV.idS0, 4);
            PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idV.idS1, 4);
            PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idV.idG, 5);
        }
    }
    else if(ipc_encMode == ENC4K)
    {
	PackBitsChar(encBitChar, &ptr, encPrm->upFlag, 1);
	if(encPrm->upFlag == 1) {
            PackBitsChar(encBitChar, &ptr, encPrm->idLsp.nVq[0],
                     PAN_BIT_LSP18_0);
            PackBitsChar(encBitChar, &ptr, encPrm->idLsp.nVq[1],
                     PAN_BIT_LSP18_1);
            PackBitsChar(encBitChar, &ptr, encPrm->idLsp.nVq[2],
                     PAN_BIT_LSP18_2);
            PackBitsChar(encBitChar, &ptr, encPrm->idLsp.nVq[3],
                     PAN_BIT_LSP18_3);
            PackBitsChar(encBitChar, &ptr, encPrm->idLsp.nVq[4],
                     PAN_BIT_LSP18_4);
            PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idUV.idGL0[0],
                     4);
	}
    }

    for(i = 0; i < 5; i++)
    {
        encBit[i] = 0;
        for(j = 0; j < 8; j++)
        {
            if(encBitChar[i * 8 + j] == '1')
            {
                encBit[i] |= 0x01 << (8 - j - 1);
            }
        }
    }


    if(ipc_encMode == ENC4K)
    {
        if(encPrm->vuvFlag != 1)
        {

            if(encPrm->vuvFlag == 0)
            {
            }
            else
            {
                PackBitsChar(encBitChar, &ptr, encPrm->idLsp.nVq[5], 8);
                PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idV.id4kS0, 7);
                PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idV.id4kS1, 10);
                PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idV.id4kS2, 9);
                PackBitsChar(encBitChar, &ptr, encPrm->vuvPrm.idV.id4kS3, 6);
            }
        }

        for(i = 5; i < 10; i++)
        {
            encBit[i] = 0;
            for(j = 0; j < 8; j++)
            {
                if(encBitChar[i * 8 + j] == '1')
                {
                    encBit[i] |= 0x01 << (8 - j - 1);
                }
            }
        }
    }
}

void IPC_PackPrm2Bit(
IdLsp		*idLsp,
int		idVUV,
IdCelp		*idCelp,
float		pitch,
IdAm		*idAm,
unsigned char	*encBit)
{
    int		i;

    static int	frm = 0;
    static EncPrm	encPrm;

    int j, n;
    static unsigned char chout[20];
    static unsigned char encBitDouble[2][10];

    encPrm.upFlag = idCelp->upFlag;

    for(i = 0; i < L_VQ - 1; i++)
    {
	encPrm.idLsp.nVq[i] = idLsp->nVq[i];
    }
    
    encPrm.vuvFlag = idVUV;

    if(ipc_bitstreamMode == BM_VARIABLE)
    {
        if(idVUV == 1 && ipc_encMode == ENC4K) {
            idCelp->idGL0[0]
                = AveragingCelpGain( 0.0, 1 );
            idVUV = 0;
        }
    }

    if(idVUV == 0)
    {
	for(i = 0; i < N_SFRM_L0; i++)
	{
	    encPrm.vuvPrm.idUV.idSL0[i] = idCelp->idSL0[i];
	    encPrm.vuvPrm.idUV.idGL0[i] = idCelp->idGL0[i];
	}
    }
    else
    {
	encPrm.vuvPrm.idV.pchcode = (int) (pitch - 20.0);
	encPrm.vuvPrm.idV.idS0 = idAm->idS0;
	encPrm.vuvPrm.idV.idS1 = idAm->idS1;
	encPrm.vuvPrm.idV.idG = idAm->idG;
    }
    
    if(ipc_encMode == ENC4K)
    {
	encPrm.idLsp.nVq[L_VQ - 1] = idLsp->nVq[L_VQ - 1];
	    
	if(idVUV == 0)
	{
	    for(i = 0; i < N_SFRM_L1; i++)
	    {
		encPrm.vuvPrm.idUV.idSL1[i] = idCelp->idSL1[i];
		encPrm.vuvPrm.idUV.idGL1[i] = idCelp->idGL1[i];
	    }
	}
	else
	{
	    encPrm.vuvPrm.idV.id4kS0 = idAm->id4kS0;
	    encPrm.vuvPrm.idV.id4kS1 = idAm->id4kS1;
	    encPrm.vuvPrm.idV.id4kS2 = idAm->id4kS2;
	    encPrm.vuvPrm.idV.id4kS3 = idAm->id4kS3;
	}
    }

    if(ipc_extensionFlag)
    {
        if(ipc_bitstreamMode == BM_VARIABLE)
	    Prm2BitVR(&encPrm, encBit);
        else
	    Prm2Bit(&encPrm, encBit);
        IPC_BitOrdering( encBit, chout, ipc_rateMode, ipc_bitstreamMode, ipc_vrScalFlag );
	for (i = 0; i < 10; i++)
            encBit[i] = chout[i];
    } 
    else
    {
        if(ipc_bitstreamMode == BM_VARIABLE)
        {
	    Prm2BitVR(&encPrm, encBit);
        }
        else
        {
            Prm2Bit(&encPrm, encBit);
        }
    }
    
    frm++;
}



