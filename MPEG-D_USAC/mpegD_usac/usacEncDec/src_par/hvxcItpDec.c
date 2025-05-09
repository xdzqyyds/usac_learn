
/*

This software module was originally developed by

    Kazuyuki Iijima and Masayuki Nishiguchi (Sony Corporation)

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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "hvxc.h"
#include "hvxcDec.h"
#include "hvxcCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

long random1();

#ifdef __cplusplus
}
#endif

#define	NORMALIZE       0.25

#define RND_MAX	0x7fffffff


static float get_gaussian()
{
    float   x,y,r;

    do
    {
	x = (float)(random1() % RND_MAX) / (float) RND_MAX;
	y = (float)(random1() % RND_MAX) / (float) RND_MAX;
	x = 2.0*x - 1.0;
	y = 2.0*y - 1.0;
    }
    while( x*x + y*y > 1.0 );

    r = x*x + y*y;

    return( NORMALIZE * x * sqrt( -2.0*log(r) / r ) );
}

void IPC_InterpolateParams(
float	rate,
int	*idVUV2,
float	(*lsp2)[LPCORDER],
float	*pch2,
float	(*am2)[128][3],
float	(*uvExt2)[FRM],
int	*modVUV,
float	*modLsp,
float	*modPch,
float	(*modAm)[3],
float	*modUvExt,
int 	testMode)
{
    int		i;
    float	nrmOrg, nrmRnd;
    float	tmp;

    if(idVUV2[0] != 0 && idVUV2[1] != 0)
    {
	tmp = idVUV2[0] * (1.0 - rate) + idVUV2[1] * rate;

	if(tmp > 2)
	{
	    *modVUV = 3;
	}
	else if(tmp > 1)
	{
	    *modVUV = 2;
	}
	else if(tmp > 0)
	{
	    *modVUV = 1;
	}

	if(pch2[1] == 0.0)
	{
	    *modPch = pch2[0];
	}
	else
	{
	    if((0.57 < pch2[0] / pch2[1]) && (pch2[0] / pch2[1] < 1.75))
	    {
		*modPch = pch2[0] * (1.0 - rate) + pch2[1] * rate;
	    }
	    else
	    {
		if(rate < 0.5)
		{
		    *modPch = pch2[0];
		}
		else
		{
		    *modPch = pch2[1];
		}
	    }
	}

	for(i = 0; i < SAMPLE / 2; i++)
	{
	    modAm[i][2] = am2[0][i][2] * (1.0 - rate) +
		am2[1][i][2] * rate; 
	}
	
	for(i = 0; i < P; i++)
	{
	    modLsp[i] = lsp2[0][i] * (1.0 - rate) + lsp2[1][i] * rate;
	}

	for(i = 0; i < FRM; i++)
	{
	    modUvExt[i] = 0.0;
	}
    }
    else if(idVUV2[0] == 0 && idVUV2[1] == 0)
    { 
	*modVUV = 0;

	*modPch = 148.0;

	for(i = 0; i < SAMPLE / 2; i++)
	{
	    modAm[i][2]=0.0;
	}
	    
	for(i = 0; i < P; i++)
	{
	    modLsp[i] = lsp2[0][i] * (1.0 - rate) + lsp2[1][i] * rate;
	}

	for(i = 0; i < FRM - (int) (FRM * rate); i++)
	{
	    modUvExt[i] = uvExt2[0][i + (int) (FRM * rate)];
	}
	for(i = FRM - (int) (FRM * rate); i < FRM; i++)
	{
	    modUvExt[i] = uvExt2[1][i - FRM + (int) (FRM * rate)];
	}

	if(rate != 0.0 && rate != 1.0)
	{
	    if (testMode & TM_VXC_DISABLE) {
	      /* disable random number generator for decoder conformance testing */
	      for(i = 0; i < FRM; i++) {
		modUvExt[i] = 0.0f;
	      }
	    }
	    else {
	      /* normal operation */
	      nrmOrg = 0.0;
	      for(i = 0; i < FRM; i++)
	      {
		nrmOrg += modUvExt[i] * modUvExt[i];
	      }
	    
	      nrmOrg = sqrt(nrmOrg);
	    
	      nrmRnd = 0.0;

	      for(i = 0; i < FRM; i++)
	      {
	        modUvExt[i] = get_gaussian();

		nrmRnd += modUvExt[i] * modUvExt[i];
	      }
	    
	      nrmRnd = sqrt(nrmRnd);
	    
	      for(i = 0; i < FRM; i++)
	      {
		modUvExt[i] /= nrmRnd / nrmOrg;
	      }
	    }
	}
    }
    else if(idVUV2[0] !=0 && idVUV2[1] ==0)
    { 
	if(rate < 0.5)
	{
	    *modVUV = idVUV2[0];
	    *modPch = pch2[0];
	    for(i = 0; i < SAMPLE / 2; i++)
	    {
		modAm[i][2] = am2[0][i][2];
	    }
	    
	    for(i=0;i<P;i++)
	    {
		modLsp[i] = lsp2[0][i];
	    }

	    for(i = 0; i < FRM; i++)
	    {
	        modUvExt[i] = 0.0;
	    }
	}
	else
	{
	    *modVUV = 0;

	    *modPch = 148.0;

	    for(i = 0; i < SAMPLE / 2; i++)
	    {
		modAm[i][2] = 0.0;
	    }

	    for(i = 0; i < P; i++)
	    {
		modLsp[i] = lsp2[1][i];
	    }

	    for(i = 0; i < FRM; i++)
	    {
		modUvExt[i] = uvExt2[1][i];
	    }

	    if(rate != 0.0 && rate != 1.0)
	    {
	      if (testMode & TM_VXC_DISABLE) {
		/* disable random number generator for decoder conformance testing */
		for(i = 0; i < FRM; i++) {
		  modUvExt[i] = 0.0f;
		}
	      }
	      else {
		/* normal operation */
		nrmOrg = 0.0;
		for(i = 0; i < FRM; i++)
		{
		    nrmOrg += modUvExt[i] * modUvExt[i];
		}
		
		nrmOrg = sqrt(nrmOrg);
		
		nrmRnd = 0.0;
		for(i = 0; i < FRM; i++)
		{
		    modUvExt[i] = get_gaussian();

		    nrmRnd += modUvExt[i] * modUvExt[i];
		}
		
		nrmRnd = sqrt(nrmRnd);
		
		for(i = 0; i < FRM; i++)
		{
		    modUvExt[i] /= nrmRnd / nrmOrg;
		}
	      }
	    }
	}
    }
    else if(idVUV2[0] == 0 && idVUV2[1] != 0)
    { 
	if(rate < 0.5)
	{
	    *modVUV = 0;

	    *modPch = 148.0;

	    for(i = 0; i < SAMPLE / 2; i++)
	    {
		modAm[i][2] = 0.0;
	    }

	    for(i = 0; i < P; i++)
	    {
		modLsp[i] = lsp2[0][i];
	    }

	    for(i = 0; i < FRM; i++)
	    {
		modUvExt[i] = uvExt2[0][i];
	    }
	    if(rate != 0.0 && rate != 1.0)
	    {
	      if (testMode & TM_VXC_DISABLE) {
		/* disable random number generator for decoder conformance testing */
		for(i = 0; i < FRM; i++) {
		  modUvExt[i] = 0.0f;
		}
	      }
	      else {
		/* normal operation */
		nrmOrg = 0.0;
		for(i = 0; i < FRM; i++)
		{
		    nrmOrg += modUvExt[i] * modUvExt[i];
		}
		
		nrmOrg = sqrt(nrmOrg);
		
		nrmRnd = 0.0;
		for(i = 0; i < FRM; i++)
		{
		    modUvExt[i] = get_gaussian();

		    nrmRnd += modUvExt[i] * modUvExt[i];
		}
		
		nrmRnd = sqrt(nrmRnd);
		
		for(i = 0; i < FRM; i++)
		{
		    modUvExt[i] /= nrmRnd / nrmOrg;
		}
	      }
	    }
	}
	else
	{
	    *modVUV = idVUV2[1];
	    *modPch = pch2[1];

	    for(i = 0; i < SAMPLE / 2; i++)
	    {
		modAm[i][2] = am2[1][i][2];
	    }
	    
	    for(i=0;i<P;i++)
	    {
		modLsp[i] = lsp2[1][i];
	    }

	    for(i = 0; i < FRM; i++)
	    {
	        modUvExt[i] = 0.0;
	    }
	}
    }
}

