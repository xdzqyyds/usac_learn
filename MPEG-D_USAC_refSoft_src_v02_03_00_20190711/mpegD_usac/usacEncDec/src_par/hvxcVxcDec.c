
/*

This software module was originally developed by

    Kazuyuki Iijima (Sony Corporation)

    and edited by

    Yuji Maeda (Sony Corporation)
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "hvxc.h"
#include "hvxcDec.h"
#include "hvxcCommon.h"
#include "hvxcVxcDec.h"

#include "hvxcCbCelp.h"
#include "hvxcCbCelp4k.h"

#define RND_MAX 0x7fffffff

#ifdef __cplusplus
extern "C" {
#endif

long random1();

#ifdef __cplusplus
}
#endif


static void gengs(float *x, int size)
{
    int         n;
    long        ra1, ra2;
    float       fra1,fra2,uni1,uni2,s,z,x1;

    /*  srandom(time(NULL));  */
    n=0;
    while(n < size)
    {
        ra1 = random1();
        ra2 = random1();
        fra1 = (float) ra1 / (float) RND_MAX;
        fra2 = (float) ra2 / (float) RND_MAX;

        uni1 = fra1 * 2.0 - 1.0;
        uni2 = fra2 * 2.0 - 1.0;
        s = uni1 * uni1 + uni2 * uni2;
        if(s <= 1.0)
        {
            z = sqrt(( -2.0 * log(s)) / s);
            x1 = uni1 * z;
            x[n] = x1;
            n++;
        }
    }
}

static void ClippingVec(float *vec, int size, float thr)
{
    int         i;

    for(i = 0; i < size; i++)
    {
        if(fabs(vec[i]) - thr < 0.0)
        {
            vec[i] = 0.0;
        }
        else if(vec[i] > 0.0)
        {
            vec[i] -= thr;
        }
        else
        {
            vec[i] += thr;
        }
    }
}

static float RegularizeVector(float *vec, int size)
{
    int         i;
    float       nrm;

    nrm = 0.0;
    for(i = 0; i < size; i++)
    {
        nrm += vec[i] * vec[i];
    }
    nrm = sqrt(nrm);

    if(nrm == 0.0)
    {
        for(i = 0; i < size; i++)
        {
            vec[i] = 0.0;
        }
    }
    else
    {
        for(i = 0; i < size; i++)
        {
            vec[i] /= nrm;
        }
    }

    return(nrm);
}

static void DecRes(
IdCelp	*idCelp,
float	*res,
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    int i, j, k;
    
    for(i = 0; i < N_SFRM_L0; i++)
    {
	for(j = 0; j < FRM / 2; j++)
	{
	    res[j + FRM / 2 * i] = cb.g[idCelp->idGL0[i]] *
		cb.s[idCelp->idSL0[i]][j];
	}
    }

    if(HDS->decMode == DEC4K || HDS->decMode == DEC3K)
    {
	if (HDS->decMode == DEC4K) k = N_SFRM_L1;
	else k = N_SFRM_L1-1;

	for(i = 0; i < k; i++)
	{
	    for(j = 0; j < FRM / 4; j++)
	    {
		res[j + FRM / 4 * i] += cbL1.g[idCelp->idGL1[i]] *
		    cbL1.s[idCelp->idSL1[i]][j];
	    }
	}
    }
}

static void DecResVR(
IdCelp  *idCelp,
float   *res,
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    int i, j;

    float       vec[FRM];

    /* if(HDS->decMode == DEC3K || HDS->decMode == DEC4K || HDS->vrScalFlag == SF_ON) { */
    if(HDS->rateMode == DEC3K || HDS->rateMode == DEC4K || HDS->vrScalFlag == SF_ON) {    /* VR scalability (YM 990802) */
        for(i = 0; i < N_SFRM_L0; i++)
        {
            for(j = 0; j < FRM / 2; j++)
            {
                res[j + FRM / 2 * i] = cb.g[idCelp->idGL0[i]] *
                    cb.s[idCelp->idSL0[i]][j];
            }
        }
    }
    else
    {
        for(i = 0; i < N_SFRM_L0; i++)
        {
            /* normal operation */
            gengs(vec, FRM / N_SFRM_L0);
            ClippingVec(vec, FRM / N_SFRM_L0, 0.3);
            RegularizeVector(vec, FRM / N_SFRM_L0);

            for(j = 0; j < FRM / 2; j++)
            {
                res[j + FRM / 2 * i] = cb.g[idCelp->idGL0[i]] *
                    vec[j];
            }
        }
    }

}

static void DecUVReasionFBF(
int	idVUV,
IdCelp	*idCelp,
float	*qRes,
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    int i;
    
    if(idVUV == 0)
    {
	DecRes(idCelp, qRes, HDS);
    }
    else
    {
	for(i = 0; i < FRM; i++)
	{
	    qRes[i] = 0.0;
	}
    }
    
    return;
}

static void DecUVReasionFBFVR(
                              int     idVUV,
                              IdCelp  *idCelp,
                              float   *qRes,
                              HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
  int i;

  if(idVUV == 0)
    {
      DecResVR(idCelp, qRes, HDS);
    }
  else
    {
      for(i = 0; i < FRM; i++)
        {
          qRes[i] = 0.0;
        }
    }

  return;
}

void td_decoder(
int	idVUV,
int	*idSL0,
int	*idGL0,
int	*idSL1,
int	*idGL1,
float	*qRes,
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
    IdCelp	idCelp;
    int		i;

    for(i = 0; i < N_SFRM_L0; i++)
    {
	idCelp.idSL0[i] = idSL0[i];
	idCelp.idGL0[i] = idGL0[i];
    }

    if(HDS->decMode == DEC4K || HDS->decMode == DEC3K)
    {
	for(i = 0; i < N_SFRM_L1; i++)
	{
	    idCelp.idSL1[i] = idSL1[i];
	    idCelp.idGL1[i] = idGL1[i];
	}
    }

    DecUVReasionFBF(idVUV, &idCelp, qRes, HDS);
}

void td_decoderVR(
                  int     idVUV,
                  int     *idSL0,
                  int     *idGL0,
                  int     *idSL1,
                  int     *idGL1,
                  float   *qRes,
                  HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
  IdCelp      idCelp;
  int         i;

  if (!(HDS->testMode & TM_VXC_DISABLE)) {
    for(i = 0; i < N_SFRM_L0; i++) {
      idCelp.idSL0[i] = idSL0[i];
      idCelp.idGL0[i] = idGL0[i];
    }

    if(HDS->decMode == DEC3K || HDS->decMode == DEC4K)
        {
          for(i = 0; i < N_SFRM_L1; i++) {
            idCelp.idSL1[i] = idSL1[i];
            idCelp.idGL1[i] = idGL1[i];
          }
        }
    DecUVReasionFBFVR(idVUV, &idCelp, qRes, HDS);
  }
  else {
    /* the output of Time Domain Decoder is diabled */
    for (i = 0; i < FRM; i++) {
      qRes[i] = 0.0f;
    }
  }
}

