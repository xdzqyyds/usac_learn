
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
#include	<math.h>
#include	<stdio.h>

#include	"hvxc.h"
#include	"hvxcEnc.h"

#define	V_POS	12

#define MIN_GML         10.0

static float sgm(float x, float a, float b)
{
    return(1.0 / (1.0 + exp(- (x - b) / a)));
}

static float isgm(float x, float a, float b)
{
    return(1.0 / (1.0 + exp((x - b) / a)));
}

static void MinLevelTracking(float lev, float *gml)
{
    static float	cdLev = 0.0, prevLev = 0.0;
    static int		gmlResetState = 0, gmlSetState = 0;

    if(*gml > lev)
    {
	*gml = lev;
	gmlResetState = 0;
	gmlSetState = 0;
    }
    else if(lev < 1000.0 && cdLev * 0.83 < lev && lev < cdLev * 1.18)
    {
	if(gmlSetState > 10)
	{
	    *gml = cdLev;
	    gmlResetState = 0;
	    gmlSetState = 0;
	}
	else
	{
	    gmlResetState = 0;
	    gmlSetState++;
	}
    }
    else if(lev < 1000.0 && prevLev * 0.83 < lev && lev < prevLev * 1.18)
    {
	cdLev = prevLev;
	gmlResetState = 0;
	gmlSetState++;
    }
    else if(gmlResetState > 40)
    {
	*gml = 0.0;
	gmlResetState = 0;
	gmlSetState = 0;
    }
    else
    {
	cdLev = 0.0;
	gmlSetState = 0;
	gmlResetState++;
    }
    
    prevLev = lev;
}

static void MinLevelTrackingVR(float lev, float *gml, int vCont)
{
    static float        cdLev = 0.0, prevLev = 0.0;
    static int          gmlResetState = 0, gmlSetState = 0;

    if(vCont > 4)
    {
        cdLev = 0.0;
        gmlSetState = 0;
        gmlResetState++;
    }
    else if(lev < MIN_GML)
    {
        *gml = MIN_GML;
        gmlResetState = 0;
        gmlSetState = 0;
    }
    else if(*gml > lev)
    {
        *gml = lev;
        gmlResetState = 0;
        gmlSetState = 0;
    }
    else if((lev < 500.0 && cdLev * 0.70 < lev && lev < cdLev * 1.30) ||
            lev < 100.0)
    {
        if(gmlSetState > 6)
        {
            *gml = lev;
            gmlResetState = 0;
            gmlSetState = 0;
        }
        else
        {
            cdLev = lev;
            gmlResetState = 0;
            gmlSetState++;
        }
    }
    else if((lev < 500.0 && prevLev * 0.70 < lev && lev < prevLev * 1.30) ||
            lev < 100.0)
    {
        cdLev = lev;
        gmlResetState = 0;
        gmlSetState++;
    }
    else if(gmlResetState > 40)
    {
        *gml = MIN_GML;
        gmlResetState = 0;
        gmlSetState = 0;
    }
    else
    {
        cdLev = 0.0;
        gmlSetState = 0;
        gmlResetState++;
    }

    prevLev = lev;
}

void IPC_VUVDecisionRule(float fb, float lev, short int *pos, float vh, int numZeroXP, float tranRatio, float *r0r, float mfdpch)
{
    float pPos, pNumZeroXP, pR0r, pGlobLev, pMfdPch;
    float prob;
    static float	gml = 0.0;

    float pVh;

    static int frm = 0;
    static int prevVUV = 0;
    static float prevProb = 0.0;
    static float prevLev = 0.0;

    MinLevelTracking(lev, &gml);

    pR0r = sgm(r0r[1], 0.06, 0.3);

    if(gml > 500.0)
    {
	pGlobLev = sgm((float) lev, 100.0, 380.0);
    }
    else
    {
	pGlobLev = sgm((float) lev, 100.0, 380.0 + gml);
    }

    pMfdPch = sgm(mfdpch, 2.5, 12.0) * isgm(mfdpch, 6.0, 140.0);

    pPos = sgm((float) *pos, 0.4, -0.55);
    
    pVh = sgm(vh, 0.10, -0.08);

    if(numZeroXP == 0)
    {
	pNumZeroXP = 0.0;
    }
    else
    {
	pNumZeroXP = isgm((float) numZeroXP, 17.0, 68.0);
    }
    
    prob = pPos * pNumZeroXP * (1.20 * pR0r + 0.80 * pGlobLev) / 2.0;

    if(fb == 0.0)
    {
	*pos = 0;
    }
    else if(lev < 35.0)
    {
	*pos = 0;
    }
    else if((numZeroXP < 18 && lev > 700) ||
	    (r0r[1] > 0.65 && numZeroXP < 90 && lev > 100.0))
    {
	*pos = V_POS;
    }
    else if((numZeroXP < 14 && lev < 350) ||
	    (numZeroXP < 12 && lev < 620 && r0r[1] < 0.4))
    {
	*pos = 0;
    }
    else
    {
	if(0.45 < prob && prob < 0.55)
	{
	    if(lev - prevLev > 1500.0)
	    {
		*pos = V_POS;
	    }
	    else if(lev - prevLev < -1500.0)
	    {
		*pos = 0;
	    }
	    else
	    {
		if(0.45 >= prevProb || prevProb >= 0.55)
		{
		    *pos = prevVUV;
		}
		else if(vh < 0.1)
		{
		    *pos = 0;
		}
		else
		{
		    *pos = V_POS;
		}
	    }
	}
	else if(prob >= 0.55)
	{
	    *pos = V_POS;
	}
	else
	{
	    *pos = 0;
	}
    }

    prevVUV = *pos;
    prevProb = prob;
    prevLev = lev;

    frm++;

    return;
}
void HvxcVUVDecisionRuleVR(
float   fb,
float   lev,
int     numZeroXP,
float   tranRatio,
float   *r0r,
float   mfdpch,
float   *gml,
int     *idVUV)
{
    float pNumZeroXP, pR0r, pGlobLev, pMfdPch;
    float prob;

    float pVh;

    static int frm = 0;
    static int prevVUV = 0;
    static float prevProb = 0.0;
    static float prevLev = 0.0;

    static int vCont = 0;


    MinLevelTrackingVR(lev, gml, vCont);


    pR0r = sgm(r0r[1], 0.06, 0.3);

    if(*gml > 500.0)
    {
        pGlobLev = sgm((float) lev, 100.0, 380.0);
    }
    else
    {
        pGlobLev = sgm((float) lev, 100.0, 380.0 + *gml);
    }

    pMfdPch = sgm(mfdpch, 2.5, 12.0) * isgm(mfdpch, 6.0, 140.0);


    if(numZeroXP == 0)
    {
        pNumZeroXP = 0.0;
    }
    else
    {
        pNumZeroXP = isgm((float) numZeroXP, 17.0, 68.0);
    }

    prob = pNumZeroXP * (1.20 * pR0r + 0.80 * pGlobLev) / 2.0;

    if(fb == 0.0)
    {
        *idVUV = 0;
    }
    else if(lev < 35.0)
    {
        *idVUV = 0;
    }
    else if((numZeroXP < 18 && lev > 700) ||
            (r0r[1] > 0.65 && numZeroXP < 90 && lev > 100.0))
    {
        *idVUV = 1;
    }
    else if((numZeroXP < 14 && lev < 350) ||
            (numZeroXP < 12 && lev < 620 && r0r[1] < 0.4))
    {
        *idVUV = 0;
    }
    else
    {
        if(0.45 < prob && prob < 0.55)
        {
            if(lev - prevLev > 1500.0)
            {
                *idVUV = 1;
            }
            else if(lev - prevLev < -1500.0)
            {
                *idVUV = 0;
            }
            else
            {
                if(0.45 >= prevProb || prevProb >= 0.55)
                {
                    *idVUV = prevVUV;
                }
                else
                {
                    *idVUV = 1;
                }
            }
        }
        else if(prob >= 0.55)
        {
            *idVUV = 1;
        }
        else
        {
            *idVUV = 0;
        }
    }

    if(prevVUV != 0)
    {
        if(*idVUV != 0)
        {
            vCont++;
        }
        else
        {
            vCont = 0;
        }
    }


    prevVUV = *idVUV;
    prevProb = prob;
    prevLev = lev;

    frm++;

    return;
}

