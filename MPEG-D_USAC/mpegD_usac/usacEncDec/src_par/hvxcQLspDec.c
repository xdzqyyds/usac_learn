
/*

This software module was originally developed by

    Kazuyuki Iijima (Sony Corporation)

    and edited by

    Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.)
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

#include <stdio.h>
#include <math.h>

#include "hvxc.h"
#include "hvxcDec.h"
#include "hvxcCommon.h"
#include "hvxcCbLsp.h"

#define THRSLD_L	0.020
#define THRSLD_M	0.020
#define THRSLD_H	0.020

static void StabilizeLsp(
float qLsp[LPCORDER])
{
    int		i;
    float	tmp;

    for(i = 0; i < 2; i++)
    {
	if(qLsp[i + 1] - qLsp[i] < 0)
	{
	    tmp = qLsp[i + 1];
	    qLsp[i + 1] = qLsp[i];
	    qLsp[i] = tmp;
	}

	if(qLsp[i + 1] - qLsp[i] < THRSLD_L)
	{
	    qLsp[i + 1] = qLsp[i] + THRSLD_L;
	}
    }


    for(i = 2; i < 6; i++)
    {
	if(qLsp[i + 1] - qLsp[i] < THRSLD_M)
	{
	    tmp = (qLsp[i + 1] + qLsp[i]) / 2.0;
	    qLsp[i + 1] = tmp + THRSLD_M / 2.0;
	    qLsp[i] = tmp - THRSLD_M / 2.0;
	}
    }


    for(i = 6; i < LPCORDER - 1; i++)
    {
	if(qLsp[i + 1] - qLsp[i] < 0)
	{
	    tmp = qLsp[i + 1];
	    qLsp[i + 1] = qLsp[i];
	    qLsp[i] = tmp;
	}

	if(qLsp[i + 1] - qLsp[i] < THRSLD_H)
	{
	    qLsp[i] = qLsp[i + 1] - THRSLD_H;
	}
    }
}

static void QuantizedLspEnh(
CbLsp	*cbLsp,
IdLsp	*idLsp,
float	qLsp[10])
{
    int		i;

    for(i = 0; i < DM; i++)
    {
	qLsp[i] = 0.0;
    }
    
    for(i = 0; i < DM; i++)
    {
	qLsp[i] = cbLsp->enh[idLsp->nVq[5]][i];
    }

    return;
}

void IPC_DecLspEnh(
IdLsp	*idLsp,
float	qLsp[LPCORDER])
{
    int		i;
    float	enhLsp[LPCORDER];

    QuantizedLspEnh(&cbLsp, idLsp, enhLsp);

    for(i = 0; i < DM; i++)
    {
	enhLsp[i] += qLsp[i];
    }
    
    StabilizeLsp(enhLsp);

    for(i = 0; i < LPCORDER; i++)
    {
	qLsp[i] = enhLsp[i];
    }
}
