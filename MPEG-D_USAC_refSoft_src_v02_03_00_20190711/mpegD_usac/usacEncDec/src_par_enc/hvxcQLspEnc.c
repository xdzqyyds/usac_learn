
/*


This software module was originally developed by

    Kazuyuki Iijima (Sony Corporation)

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
#include <stdlib.h>
#include <math.h>
/* #include <values.h> */

#include "hvxc.h"
#include "hvxcEnc.h"

#include "hvxcCbLsp.h"

extern int	ipc_encMode;
extern int	ipc_decMode;

typedef struct
{
    int		nVq;
    float	err;
    float	res[DM];
}
CndtLsp;

static void VectorQuantizeLspW(
float	*in,
int	cbBits,
float	(*vqLsp)[10],
float	*w,
int	*optNVq,
float	*res)
{
    int		i, nVq, cbSize;

    float	minErr;
    float	err, tmp;

    cbSize = 1;
    cbSize = cbSize << cbBits;
    
    err = 0.0;
    for(i = 0; i < DM; i++)
    {
	tmp = in[i] - vqLsp[0][i];
	tmp *= tmp;
	err += w[i] * tmp;
    }

    minErr = err;
    *optNVq = 0;

    for(nVq = 1; nVq < cbSize; nVq++)
    {
	err = 0.0;
	for(i = 0; i < DM; i++)
	{
	    tmp = in[i] - vqLsp[nVq][i];
	    tmp *= tmp;
	    err += w[i] * tmp;
	}
	
	if(minErr > err)
	{
	    *optNVq = nVq;
	    minErr = err;
	}
    }
	
    for(i = 0; i < DM; i++)
    {
	res[i] = in[i] - vqLsp[*optNVq][i];
    }

    return;
}




#define THRSLD_L	0.020
#define THRSLD_M	0.020
#define THRSLD_H	0.020

static void StabilizeLsp(
float qLsp[11])
{
    int		i;
    float	tmp;

    for(i = 0; i < 2; i++)
    {
	if(qLsp[i + 2] - qLsp[i + 1] < 0)
	{
	    tmp = qLsp[i + 2];
	    qLsp[i + 2] = qLsp[i + 1];
	    qLsp[i + 1] = tmp;
	}

	if(qLsp[i + 2] - qLsp[i + 1] < THRSLD_L)
	{
	    qLsp[i + 2] = qLsp[i + 1] + THRSLD_L;
	}
    }


    for(i = 2; i < 6; i++)
    {
	if(qLsp[i + 2] - qLsp[i + 1] < THRSLD_M)
	{
	    tmp = (qLsp[i + 2] + qLsp[i + 1]) / 2.0;
	    qLsp[i + 2] = tmp + THRSLD_M / 2.0;
	    qLsp[i + 1] = tmp - THRSLD_M / 2.0;
	}
    }


    for(i = 6; i < DM - 1; i++)
    {
	if(qLsp[i + 2] - qLsp[i + 1] < 0)
	{
	    tmp = qLsp[i + 2];
	    qLsp[i + 2] = qLsp[i + 1];
	    qLsp[i + 1] = tmp;
	}

	if(qLsp[i + 2] - qLsp[i + 1] < THRSLD_H)
	{
	    qLsp[i + 1] = qLsp[i + 2] - THRSLD_H;
	}
    }
}

static void QuantizedLspEnh(
CbLsp	*cbLsp,
IdLsp	*idLsp,
float	qLsp[11])
{
    int		i;

    for(i = 0; i < DM + 1; i++)
    {
	qLsp[i] = 0.0;
    }
    
    for(i = 0; i < DM; i++)
    {
	qLsp[i + 1] = cbLsp->enh[idLsp->nVq[5]][i];
    }

    return;
}

void IPC_quanlsp_enh(
float	lspin[11],
float	qlspin[11],
float	lspout[11],
IdLsp	*idLsp,
float	wLsp[10])
{
    int		i;

    float	qLsp[ACR];

    int		optNVq;
    float	resVq[2][DM];

    for(i = 0; i < DM; i++)
    {
	resVq[0][i] = lspin[i + 1] - qlspin[i + 1];
    }
    
    VectorQuantizeLspW(resVq[0], 8, cbLsp.enh, wLsp, &optNVq,
		       resVq[1]);
    
    idLsp->nVq[5] = optNVq;
    
    QuantizedLspEnh(&cbLsp, idLsp, qLsp);
    
    for(i=0;i<ACR;i++) {
	qLsp[i] += qlspin[i];
    }
    
    StabilizeLsp(qLsp);
    
    for(i = 0; i < ACR; i++)
    {
	lspout[i] = qLsp[i];
    }
}

