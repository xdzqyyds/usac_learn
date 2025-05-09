
/*


This software module was originally developed by

    Kazuyuki Iijima, Masayuki Nishiguchi  (Sony Corporation)

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
/* #include <values.h> */
#include <stdio.h>

#include "hvxc.h"
#include "hvxcEnc.h"

extern float	ipc_coef[SAMPLE];
extern float	ipc_coef160[FRM];

void IPC_calc_residue256(
float	arys[SAMPLE],
float	alphaq[11],
float	resi[SAMPLE])
{
    int i,j;
    float out;
    float mem[P+1]={0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.};
    
    float arysTmp[SAMPLE + P];
    float resiTmp[SAMPLE + P];

    static float arysOld[SAMPLE];

    for(i = 0; i < P; i++)
    {
	arysTmp[i] = arysOld[SAMPLE - OVERLAP - P + i];
    }

    for(i = 0; i < SAMPLE; i++)
    {
	arysTmp[P + i] = arys[i];
    }

    for(i = 0; i < SAMPLE + P; i++)
    {
	mem[0]=arysTmp[i];
	out = 0.0;
	for(j = P; j > 0; j--)
	{
	    out += mem[j]*alphaq[j];
	    mem[j]=mem[j-1];
	}
	out += mem[0];
	resiTmp[i]= out;
    }

    for(i = 0; i < SAMPLE; i++)
    {
	resi[i] = resiTmp[i + P];
    }

    for(i = 0; i < SAMPLE; i++)
    {
	arysOld[i] = arys[i];
    }
}

