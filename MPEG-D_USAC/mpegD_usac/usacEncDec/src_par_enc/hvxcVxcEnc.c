 
/*


This software module was originally developed by

    Kazuyuki Iijima  (Sony Corporation)

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
#include "hvxcEnc.h"
#include "hvxcCommon.h"

#include "hvxcVxcEnc.h"

#include "hvxcCbCelp.h"

#include "hvxcCbCelp4k.h"

#define RND_MAX	0x7fffffff

#ifdef __cplusplus
extern "C"
{
#endif
/* long random(void); */
#ifdef __cplusplus
}
#endif

#if defined(SUN)
#ifdef __cplusplus
extern "C"
{
#endif
/* void srandom(int seed); */
#ifdef __cplusplus
}
#endif
#endif

#define	BLND_RATIO_L0_4K	0.6
#define	BLND_RATIO_L1_4K	0.0

#define	BLND_RATIO_L0_2K	0.6
#define	BLND_RATIO_L1_2K	0.0

extern int	ipc_encMode;
extern int	ipc_decMode;

static void SpectralWeightingFilter(
float	*alpha,
float	*alphaFir)
{
    int		i, j;

    static float Lma[Np+1] =
    {
	0.0, 0.9, 0.81, 0.729, 0.6561, 0.59049,
	0.531441, 0.478297, 0.430467, 0.387420,
	0.348678
    };

    static float Lar[Np+1] =
    {
	0.0, 0.4, 0.16, 0.064, 0.0256, 0.01024,
	0.004096, 0.001638, 0.000655, 0.000262,
	0.000105
    };

    float	LAma[Nfir + 1], LAar[Np + 1];
    float	state[Np + 1], tmp;

    for(i = 1; i <= Np; i++)
    {
	state[i] = 0.0;
	LAma[i] = Lma[i] * alpha[i];
	LAar[i] = Lar[i] * alpha[i];
    }

    alphaFir[0] = state[1] = 1.0;
    LAma[Nfir] = 0.0;

    for(i = 1; i <= Nfir; i++)
    {
	tmp = LAma[i];
	for(j = Np; j > 0; j--)
	{
	    tmp -= LAar[j] * state[j];
	    state[j] = state[j - 1];
	}
	alphaFir[i] = tmp;
	state[1] = tmp;
    }
}


static void PerceptualWeightingFilter(
int	fsize,
float	*Ssub,
float	*alphaFir,
float	*spw,
float	*statePWF)
{
    int		i, j;
    float	tmp;

    for(i = 0; i < fsize; i++)
    {
	statePWF[i + FLTBUF - fsize] = Ssub[i];
    }

    for(i = 0; i < fsize; i++)
    {
	tmp = 0.0;
	for(j = 0; j <= Nfir; j++)
	{
	    tmp += statePWF[i + FLTBUF - fsize - j] * alphaFir[j];
	}
	
	spw[i] = tmp;
    }
    
    for(i = 0; i < FLTBUF - fsize; i++)
    {
	statePWF[i] = statePWF[fsize + i];
    }
}


static void ZeroInputResponseSubtraction(
float	*alphaFir,
float	*alphaP,
float	*stateI,
float	*stateAq,
int	fsize,
float	*spw,
float	*r)
{
    int			i, j;
    float		preS[FLTBUF], *s;
    float		*zero, tmp;

    s = (float *) calloc(FLTBUF + fsize, sizeof(float));
    zero = (float *) calloc(fsize, sizeof(float));
    
    for(i = 0; i < FLTBUF; i++)
    {
	preS[i] = 0.0;
    }
    for(i = 0; i < FLTBUF + fsize; i++)
    {
	s[i] = 0.0;
    }

    for(i = 0; i < Nfir; i++)
    {
	preS[i + FLTBUF - fsize - Nfir] = stateI[i];
    }
    
    for(i = 0; i < fsize; i++)
    {
	tmp = 0.0;
	for(j = 0; j <= Nfir; j++)
	{
	    tmp += preS[i + FLTBUF - fsize - j] * alphaFir[j];
	}
	
	s[FLTBUF + i] = tmp;
    }


    for(i = 1; i < ACR; i++) preS[i] = stateAq[i];

    for(i = 0; i < fsize; i++)
    {
	tmp = s[FLTBUF + i];
	for(j = ACR - 1; j > 0; j--)
	{
	    tmp -= alphaP[j] * preS[j];
	    preS[j] = preS[j - 1];
	}
	preS[1] = tmp;
	s[FLTBUF + i] = tmp;
	zero[i] = tmp;
	r[i] = spw[i] - zero[i];
    }

    free(s);
    free(zero);
}



static void ZeroStateSynthesis(
int	fsize,
float	*in,
float	*out,
float	*alphaFir,
float	*alphaP)
{
    int i, j;
    float pre_s[FLTBUF];
    float temp;
    float *s;
    
    s = (float *) calloc(FLTBUF + fsize, sizeof(float));

    for(i = 0; i <= Nfir; i++) pre_s[i+FLTBUF-fsize-Nfir] = 0.0;
    for(i = 0; i < fsize; i++)
    {
	pre_s[i + FLTBUF - fsize] = in[i];
	temp = 0.0;
	
	for(j=0; j <= Nfir; j++)
	{
	    temp += pre_s[i + FLTBUF - fsize - j] * alphaFir[j];
	}
	
	s[FLTBUF + i] = temp;
    }
    
    for (i = 1; i < ACR; i++) pre_s[i] = 0.0;
    
    for (i = 0; i < fsize; i++)
    {
	temp = s[FLTBUF + i];
	for(j = ACR - 1; j > 0; j--)
	{
	    temp -= alphaP[j] * pre_s[j];
	    pre_s[j] = pre_s[j-1];
	}
	pre_s[1] = temp;
	out[i] = temp;
    }

    free(s);
}

static void UpdateFilterStates(
float	g,
float	*SC,
float	*stateAq,
float	*stateI,
float	*alphaFir,
float	*alphaQ,
int	fsize,
float	*ex)
{
    int		i, j;
    float	pre_s[FLTBUF], *s;
    float	temp;

    s = (float *) calloc(FLTBUF + fsize, sizeof(float));

    for(i = 0; i < FLTBUF; i++)
    {
	pre_s[i] = 0.0;
    }
    for(i = 0; i < FLTBUF + fsize; i++)
    {
	s[i] = 0.0;
    }

    for(i = 0; i < fsize; i++)
    {
	ex[i] = g * SC[i];
    }

    for (i = 0; i < Nfir; i++)
    {
	pre_s[i + FLTBUF - fsize - Nfir] = stateI[i];
	stateI[i] = ex[fsize - Nfir + i];
    }

    for (i=0;i<fsize;i++)
    {
	pre_s[i + FLTBUF - fsize] = ex[i];
	temp = 0.0;
	for(j = 0;j <= Nfir; j++)
	{
	    temp += pre_s[i + FLTBUF - fsize - j]
		* alphaFir[j];
	}
	s[FLTBUF + i] = temp;
    }

    for (i = 0; i < fsize; i++)
    {
	temp = s[FLTBUF+i];
	
	for(j = ACR - 1; j > 0; j--)
	{
	    temp -= alphaQ[j] * stateAq[j];
	    stateAq[j] = stateAq[j-1];
	}

	stateAq[1] = temp;
	s[FLTBUF + i] = temp;
    }

    free(s);

}

static float Power(float *vec, int size)
{
    int		i;
    float	nrm;
    
    nrm = 0.0;
    for(i = 0; i < size; i++)
    {
	nrm += vec[i] * vec[i];
    }
    return(nrm);
}

static float Norm(float *vec, int size)
{
    return((float) sqrt((double) Power(vec, size)));
}

static void IpLsp2Lpc2(
float	qLsp[10],
float	(*alphaip)[11])
{
    int		i, j;
    float	ipLsp[2][P + 1];
    
    ipLsp[0][0] = ipLsp[1][0] = 0.0;
    for(i = 0; i < P; i++)
    {
	ipLsp[0][i + 1] = qLsp[i];
	ipLsp[1][i + 1] = qLsp[i];
    }
    
    IPC_lsp_lpc(ipLsp[0], alphaip[0]);
    IPC_lsp_lpc(ipLsp[1], alphaip[1]);
  
  
    for(j = 0; j < 2; j++)
    {
	alphaip[j][0] = 1.0;
	for(i = 1; i < P + 1; i++)
	{
	    alphaip[j][i] = -alphaip[j][i];
	}
    }
    return;
}    

static void IpLsp2Lpc4(
float	qLsp[10],
float	(*alphaip)[11])
{
    int		i, j;
    float	ipLsp[4][P + 1];
    
    ipLsp[0][0] = ipLsp[1][0] = 0.0;
    for(i = 0; i < P; i++)
    {
	ipLsp[0][i + 1] = qLsp[i];
	ipLsp[1][i + 1] = qLsp[i];
	ipLsp[2][i + 1] = qLsp[i];
	ipLsp[3][i + 1] = qLsp[i];
    }
    
    IPC_lsp_lpc(ipLsp[0], alphaip[0]);
    IPC_lsp_lpc(ipLsp[1], alphaip[1]);
    IPC_lsp_lpc(ipLsp[2], alphaip[2]);
    IPC_lsp_lpc(ipLsp[3], alphaip[3]);

    for(j = 0; j < 4; j++)
    {
	alphaip[j][0] = 1.0;
	for(i = 1; i < P + 1; i++)
	{
	    alphaip[j][i] = -alphaip[j][i];
	}
    }
    return;
}    

static void ClearStates(float *stateAq, float *stateI, float *statePWF)
{
    int		i;
    
    for(i = 0; i < P + 1; i++)
    {
	stateAq[i] = 0.0;
    }
    for(i = 0; i < Nfir + 1; i++)
    {
	stateI[i] = 0.0;
    }
    for(i = 0; i < FLTBUF; i++)
    {
	statePWF[i] = 0.0;
    }
}






#define		Nfir		11
#define		FLTBUF		100

#define		REF


typedef struct
{
    int		num;
    float	val;
}
SelCb;

static int CmpIp(const void *a, const void *b)
{
    if((* (SelCb *) a).val < (* (SelCb *) b).val)
    {
	return(-1);
    }
    else if((* (SelCb *) a).val == (* (SelCb *) b).val)
    {
	return(0);
    }
    else
    {
	return(1);
    }
}

static void VxcSynthesis(	/* appended by Y.Maeda on 98/08/06 */ 
float *qRes, 
float *qVec, 
float *qAlpha, 
int csflag )
{
    int i, j;
    float temp;
    static float w[ACR];

    if (csflag != 0) 
	for (i = 0; i < ACR; i++) w[i] = 0;

    for (i = 0; i < FRM/2; i++)
    {
	temp = qRes[i];
	for(j = ACR - 1; j > 0; j--)
	{
	    temp -= qAlpha[j] * w[j];
	    w[j] = w[j-1];
	}
	w[1] = temp;
	qVec[i] = temp;
    }
}				/* appended by Y.Maeda on 98/08/06 */

#define N_PRE_SEL	16


static void EncSGVQResBF(
float	*inVec,
float	(*cb0)[80],
float	*g0,
float	*qVec,
float	*qRes,
float	*alpha,
float	*qAlpha,
float	*alphaFir,
int	*idS,
int	*idG,
float	*outVec,
int	csflag,		/* appended by Y.Maeda on 98/08/06 */
float	*stateAq,
float	*stateI,
float	*statePWF)
{
    int		i, j;
    float	ip, maxIp;
    float	sq, minSq;
    
    float	inVecW[FRM / 2], synCbS[FRM / 2], ref[FRM / 2];
    float	ex[FRM / 2], rvsRef[FRM / 2], synRvsRef[FRM / 2];
    float	synRef[FRM / 2];
    float	tmp, pwr, nrmRef, pwrVec, gain;
    
    SelCb	preSelCb[N_PRE_SEL];
    SelCb	selCb;

    float	blnd;

    static int	frm = 0;

    if(ipc_encMode == ENC2K)
    {
	blnd = BLND_RATIO_L0_2K;
    }
    else
    {
	blnd = BLND_RATIO_L0_4K;
    }

    SpectralWeightingFilter(alpha, alphaFir);
    PerceptualWeightingFilter(FRM / 2, inVec, alphaFir, inVecW, statePWF);
    ZeroInputResponseSubtraction(alphaFir, qAlpha, stateI, stateAq, 
				 FRM / 2, inVecW, ref);
    
    for(i = 0; i < FRM / 2; i++)
    {
	rvsRef[i] = ref[FRM / 2 - i - 1];
    }

    ZeroStateSynthesis(FRM / 2, rvsRef, synRvsRef, alphaFir, qAlpha);

    for(i = 0; i < FRM / 2; i++)
    {
	synRef[i] = synRvsRef[FRM / 2 - i - 1];
    }

    for(i = 0; i < N_PRE_SEL; i++)
    {
	preSelCb[i].num = 0;
	preSelCb[i].val = 0.0;
    }




    for(i = 0; i < N_SHAPE_L0; i++)
    {
	ip = 0.0;
	for(j = 0; j < FRM / 2; j++)
	{
	    ip += synRef[j] * cb0[i][j];
	}

	if(preSelCb[0].val < ip)
	{
	    preSelCb[0].val = ip;
	    preSelCb[0].num = i;
	    qsort(preSelCb, N_PRE_SEL, sizeof(SelCb), CmpIp);
	}
    }

    ZeroStateSynthesis(FRM / 2, cb0[preSelCb[0].num], synCbS, alphaFir,
		       qAlpha);

    selCb.num = 0;
    if(preSelCb[0].val < 0)
    {
	selCb.val = -1.0 * preSelCb[0].val * preSelCb[0].val;
    }
    else
    {
	selCb.val = preSelCb[0].val * preSelCb[0].val;
    }
    
    
    pwrVec = Power(synCbS, FRM / 2);

    maxIp = preSelCb[0].val;

    for(i = 1; i < N_PRE_SEL; i++)
    {
	ZeroStateSynthesis(FRM / 2, cb0[preSelCb[i].num], synCbS, alphaFir,
			   qAlpha);
	pwr = Power(synCbS, FRM / 2);

	if(preSelCb[i].val < 0)
	{
	    tmp = -1.0 * preSelCb[i].val * preSelCb[i].val;
	}
	else
	{
	    tmp = preSelCb[i].val * preSelCb[i].val;
	}

	if(selCb.val * pwr  < tmp * pwrVec)
	{
	    selCb.num = preSelCb[i].num;
	    selCb.val = tmp;

	    maxIp = preSelCb[i].val;

	    pwrVec = pwr;
	}
    }

    *idS = selCb.num;

    nrmRef = Norm(ref, FRM / 2);

    maxIp = maxIp * (1.0 - blnd) + nrmRef * sqrt(pwrVec) * blnd;
    
    gain = maxIp / pwrVec;
   
    AveragingCelpGain( gain, 0 );

    tmp = g0[0] - gain;
    sq = tmp * tmp;
    minSq = sq;
    *idG = 0;
    
    for(i = 1; i < N_GAIN_L0; i++)
    {
	tmp = g0[i] - gain;
	sq = tmp * tmp;
	
	if(sq < minSq)
	{
	    minSq = sq;
	    *idG = i;
	}
    }
   
    for(i = 0; i < FRM / 2; i++) /* appended by Y.Maeda on 98/08/06 */
	qRes[i] = g0[*idG] * cb0[*idS][i];

    VxcSynthesis( qRes, qVec, qAlpha, csflag ); /* appended : end */

    for(i = 0; i < FRM / 2; i++)
    {
	/* qVec[i] = g0[*idG] * synCbS[i]; deleted by Y.Maeda */
	/* qRes[i] = g0[*idG] * cb0[*idS][i]; on 98/08/06     */
	outVec[i] = inVec[i] - qVec[i];
    }
    
    UpdateFilterStates(g0[*idG], cb0[*idS], stateAq, stateI, alphaFir,
		       qAlpha, FRM / 2, ex);

    frm++;
}

static void VqResL0(
float	*ary,
IdCelp	*idCelp,
float	*rawLsp,
float	*qLsp,
int	csflag,
float	*res)
{
    int i, dim;
    
    float inVec[2][FRM/2];
    float qAry[2][FRM/2];
    float qRes[2][FRM/2];
    float outVec[2][FRM/2];


    float	qAlphaIp[2][P + 1], alphaIp[2][P + 1], alphaFir[2][Nfir + 1];
    
    static float	stateI[Nfir + 1], stateAq[P + 1], statePWF[FLTBUF];
    
    static int	frm = 0;

    dim = FRM/2;
    
    if(frm == 0)
    {
	ClearStates(stateAq, stateI, statePWF);
    }
    
    for(i = 0; i < dim; i++)
    {
	inVec[0][i] = ary[i];
	inVec[1][i] = ary[dim + i];
    }
    
    IpLsp2Lpc2(qLsp, qAlphaIp);
    IpLsp2Lpc2(rawLsp, alphaIp);
    
    if(csflag)
    {
	ClearStates(stateAq, stateI, statePWF);
    }
    
    EncSGVQResBF(inVec[0], cb.s, cb.g, qAry[0],
	       qRes[0], alphaIp[0], qAlphaIp[0], alphaFir[0],
	       &(idCelp->idSL0[0]), &(idCelp->idGL0[0]),
	       outVec[0], csflag, stateAq, stateI, statePWF);
	       /* modified by Y.Maeda on 98/08/06 */
    
    EncSGVQResBF(inVec[1], cb.s, cb.g, qAry[1],
	       qRes[1], alphaIp[1], qAlphaIp[1], alphaFir[1],
	       &(idCelp->idSL0[1]), &(idCelp->idGL0[1]),
	       outVec[1], csflag, stateAq, stateI, statePWF);
	       /* modified by Y.Maeda on 98/08/06 */

    for(i=0;i<dim;i++)
    {
	ary[i] = qAry[0][i];
	res[i] = outVec[0][i];
    }
    
    for(i=0;i<dim;i++)
    {
	ary[dim + i] = qAry[1][i];
	res[dim + i] = outVec[1][i];
    }
    frm++;
}

#define N_PRE_SEL_L1	16

static void EncSGVQResBFL1(
float	*inVec,
float	(*cb0)[FRM / 4],
float	*g0,
float	*qVec,
float	*qRes,
float	*alpha,
float	*qAlpha,
float	*alphaFir,
int	*idS,
int	*idG,
float	*outVec,
float	*stateAq,
float	*stateI,
float	*statePWF)
{
    int		i, j;
    float	ip, maxIp;
    float	sq, minSq;
    
    float	inVecW[FRM / 4], synCbS[FRM / 4], ref[FRM / 4];
    float	ex[FRM / 4], rvsRef[FRM / 2], synRvsRef[FRM / 2];
    float	synRef[FRM / 2];
    float	tmp, pwr, nrmRef, nrmInVecW, pwrVec, gain;
    
    SelCb	preSelCb[N_PRE_SEL_L1];
    SelCb	selCb;

    float	blnd;

    static int	frm = 0;

    if(ipc_encMode == ENC2K)
    {
	blnd = BLND_RATIO_L1_2K;
    }
    else
    {
	blnd = BLND_RATIO_L1_4K;
    }

    SpectralWeightingFilter(alpha, alphaFir);
    PerceptualWeightingFilter(FRM / 4, inVec, alphaFir, inVecW, statePWF);
    ZeroInputResponseSubtraction(alphaFir, qAlpha, stateI, stateAq,
				 FRM / 4, inVecW, ref);
    
    nrmInVecW = Norm(inVecW, FRM / 4);

    for(i = 0; i < FRM / 4; i++)
    {
	rvsRef[i] = ref[FRM / 4 - i - 1];
    }

    ZeroStateSynthesis(FRM / 4, rvsRef, synRvsRef, alphaFir, qAlpha);
    
    for(i = 0; i < FRM / 4; i++)
    {
	synRef[i] = synRvsRef[FRM / 4 - i - 1];
    }

    for(i = 0; i < N_PRE_SEL_L1; i++)
    {
	preSelCb[i].num = 0;
	preSelCb[i].val = 0.0;
    }


    for(i = 0; i < N_SHAPE_L1; i++)
    {
	ip = 0.0;
	for(j = 0; j < FRM / 4; j++)
	{
	    ip += synRef[j] * cb0[i][j];
	}

	if(preSelCb[0].val < ip)
	{
	    preSelCb[0].val = ip;
	    preSelCb[0].num = i;
	    qsort(preSelCb, N_PRE_SEL_L1, sizeof(SelCb), CmpIp);
	}
    }

    ZeroStateSynthesis(FRM / 4, cb0[preSelCb[0].num], synCbS, alphaFir,
		       qAlpha);

    selCb.num = 0;
    if(preSelCb[0].val < 0)
    {
	selCb.val = -1.0 * preSelCb[0].val * preSelCb[0].val;
    }
    else
    {
	selCb.val = preSelCb[0].val * preSelCb[0].val;
    }

    pwrVec = Power(synCbS, FRM / 4);

    maxIp = preSelCb[0].val;

    for(i = 1; i < N_PRE_SEL_L1; i++)
    {
	ZeroStateSynthesis(FRM / 4, cb0[preSelCb[i].num], synCbS, alphaFir,
			   qAlpha);
	pwr = Power(synCbS, FRM / 4);

	if(preSelCb[i].val < 0)
	{
	    tmp = -1.0 * preSelCb[i].val * preSelCb[i].val;
	}
	else
	{
	    tmp = preSelCb[i].val * preSelCb[i].val;
	}

	if(selCb.val * pwr < tmp * pwrVec)
	{
	    selCb.num = preSelCb[i].num;
	    selCb.val = tmp;

	    maxIp = preSelCb[i].val;

	    pwrVec = pwr;
	}
    }

    *idS = selCb.num;

    nrmRef = Norm(ref, FRM / 4);

    maxIp = maxIp * (1.0 - blnd) + nrmRef * sqrt(pwrVec) * blnd;
    
    gain = maxIp / pwrVec;

    tmp = g0[0] - gain;
    sq = tmp * tmp;
    minSq = sq;
    *idG = 0;
    
    for(i = 1; i < N_GAIN_L1; i++)
    {
	tmp = g0[i] - gain;
	sq = tmp * tmp;
	
	if(sq < minSq)
	{
	    minSq = sq;
	    *idG = i;
	}
    }
    
    for(i = 0; i < FRM / 4; i++)
    {
	qVec[i] = g0[*idG] * synCbS[i];
	qRes[i] = g0[*idG] * cb0[*idS][i];
	outVec[i] = inVec[i] - qVec[i];
    }
    
    UpdateFilterStates(g0[*idG], cb0[*idS], stateAq, stateI, alphaFir,
		       qAlpha, FRM / 4, ex);

    frm++;
}


static void VqResL1(
float	*ary,
IdCelp	*idCelp,
float	rawLsp[10],
float	qLsp[10],
int	csflag,
float	*res)
{
    int	i, j;
    
    float inVec[4][FRM/4];
    float qAry[4][FRM/4];
    float qRes[4][FRM/4];
    float outVec[4][FRM/4];


    float	qAlphaIp[4][P + 1], alphaIp[4][P + 1], alphaFir[4][Nfir + 1];
    
    static float	stateI[Nfir + 1], stateAq[P + 1], statePWF[FLTBUF];
    
    static int	frm = 0;

    if(frm == 0)
    {
	ClearStates(stateAq, stateI, statePWF);
    }
    
    for(j = 0; j < 4; j++)
    {
	for(i = 0; i < FRM / 4; i++)
	{
	    inVec[j][i] = ary[i + FRM / 4 * j];
	}
    }
    
    IpLsp2Lpc4(qLsp, qAlphaIp);
    IpLsp2Lpc4(rawLsp, alphaIp);
    
    if(csflag)
    {
	ClearStates(stateAq, stateI, statePWF);
    }
    
    for(i = 0; i < 4; i++)
    {
	EncSGVQResBFL1(inVec[i], cbL1.s, cbL1.g, qAry[i],
			qRes[i], alphaIp[i], qAlphaIp[i], alphaFir[i],
			&(idCelp->idSL1[i]), &(idCelp->idGL1[i]),
			outVec[i], stateAq, stateI, statePWF);
    }

    for(j = 0; j < 4; j++)
    {
	for(i = 0; i < FRM / 4; i++)
	{
	    ary[i + j * FRM / 4] = qAry[j][i];
	    res[i + j * FRM / 4] = outVec[j][i];
	}
    }

    frm++;
}







static void QuanUVReasionFBF(
float	arys[FRM],
int	*vuvFlgVXC,
IdCelp	*idCelp,
float	rawLsp[10],
float	qLsp[10])
{
    int i, csflag;
    float ary[3][FRM];

    for(i = 0; i < FRM; i++)
    {
	ary[0][i] = arys[i];
    }


    if(vuvFlgVXC[1] == 0)
    {
	if(vuvFlgVXC[0] != 0)
	{
	    csflag = 1;
	}
	else
	{
	    csflag = 0;
	}

	VqResL0(ary[0], idCelp, rawLsp, qLsp, csflag, ary[1]);

	if(ipc_encMode == ENC4K)
	{
	    VqResL1(ary[1], idCelp, rawLsp, qLsp, csflag, ary[2]);
	}
    }
    else 
    {
        for (i = 0; i < 2; i++)
            AveragingCelpGain( 0.0, 0 );
    }

    return;
}

void td_encoder(
float	arys[FRM],
float	rawLsp[LPCORDER],
float	qLsp[LPCORDER],
int	vuvFlg,
int	*idSL0,
int	*idGL0,
int	*idSL1,
int	*idGL1)
{
    static int	frm = 0;
    int		i;
    static int		vuvFlgVXC[2];

    IdCelp	idCelp;

    vuvFlgVXC[0] = vuvFlgVXC[1];
    vuvFlgVXC[1] = vuvFlg;
    
    QuanUVReasionFBF(arys, vuvFlgVXC, &idCelp, rawLsp, qLsp);

    frm++;

    for(i = 0; i < N_SFRM_L0; i++)
    {
	idSL0[i] = idCelp.idSL0[i];
	idGL0[i] = idCelp.idGL0[i];
    }

    if(ipc_encMode == ENC4K)
    {
	for(i = 0; i < N_SFRM_L1; i++)
	{
	    idSL1[i] = idCelp.idSL1[i];
	    idGL1[i] = idCelp.idGL1[i];
	}
    }
}

/*********************************************/
/* Function : AveragingCelpGain              */
/*  calculate average of celp gain into      */
/* previous 4 frames                         */
/*  flag = 0 --- update state                */
/*         1 --- calculate average           */
/*********************************************/
int AveragingCelpGain( float gain, int flag )
{
    int i, idG;
    float av_gain, tmp, sq, minSq;
    static float st_gain[8];

    if (flag == 0) {
	for (i = 0; i < 7; i++) st_gain[i] = st_gain[i+1];
	st_gain[7] = gain;

	/* for (i = 0; i < 8; i++)
	    printf(" %7.1f", st_gain[i]);
        printf("\n"); */
	return ( 0 ); 
    } else {
	av_gain = 0;
	for (i = 0; i < 8; i++) av_gain += st_gain[i];
	av_gain /= 8.0;

        tmp = cb.g[0] - av_gain;
        sq = tmp * tmp;
        minSq = sq;
        idG = 0;
    
        for(i = 1; i < N_GAIN_L0; i++)
        {
	    tmp = cb.g[i] - av_gain;
	    sq = tmp * tmp;
	
	    if(sq < minSq)
	    {
	        minSq = sq;
	        idG = i;
	    }
        }
	/* printf("av_gain = %7.1f %2d %7.1f\n", 
	    av_gain, idG, cb.g[idG]); */

	return ( idG );
    }
}
