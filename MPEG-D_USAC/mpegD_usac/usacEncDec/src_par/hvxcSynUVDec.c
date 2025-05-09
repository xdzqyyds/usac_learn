
/*

This software module was originally developed by

    Masayuki Nishiguchi and Kazuyuki Iijima (Sony Corporation)
    
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

#include "hvxc.h"
#include "hvxcDec.h"
#include "hvxcCommon.h"

#define PU_IP  2 

static void calc_syn_cont2u_LD(
float *res,
float (*alphaip)[11],
float *syn,
HvxcDecStatus *HDS	/* in: pointer to decoder status(AI 990129) */
)
{
  int i, j;

  float out2;
  /*
  static float mem2[P+1] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 
			    0.0, 0.0};
			    */
  for (i = 0; i < FRM/2-LD_LEN; i++) {
    out2 = 0.0;
    for (j = P; j > 0; j--) {
      out2 += HDS->lpu_mem2[j] * alphaip[0][j];
      HDS->lpu_mem2[j] = HDS->lpu_mem2[j-1];
    }
    out2 = res[i] - out2;
    HDS->lpu_mem2[1] = out2;
    syn[i] = out2;
  }

  for (i = FRM/2-LD_LEN; i < FRM; i++) {
    out2 = 0.0;
    for (j = P; j > 0; j--) {
      out2 += HDS->lpu_mem2[j] * alphaip[1][j];
      HDS->lpu_mem2[j] = HDS->lpu_mem2[j-1];
    }
    out2 = res[i] - out2;
    HDS->lpu_mem2[1]= out2;
    syn[i]= out2;
  }
}

static void calc_syn_cont2u(
float *res,
float (*alphaip)[11],
float *syn,
HvxcDecStatus *HDS	/* in: pointer to decoder status(AI 990129) */
)
{
  int i, j, ii;
    
  float out2;
  /*
  static float mem2[P+1]={0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
			  0.0, 0.0};
			  */
  for (ii = 0; ii < 2; ii++) {
    for (i = 0; i < FRM/2; i++) {
      out2 = 0.0;
      for (j = P; j > 0; j--) {
	out2 += HDS->lpu_mem2[j] * alphaip[ii][j];
	HDS->lpu_mem2[j] = HDS->lpu_mem2[j-1];
      }
	    
      out2 = res[ii*FRM/2+i] - out2;
      HDS->lpu_mem2[1] = out2;
      syn[ii*FRM/2+i]= out2;
    }
  }
}

static void IpLsp2Lpc(
float	(*qLsp)[10],
float	(*alphaip)[11])     	                  
{
    int		i, j;
    float	ipLsp0[P + 1], ipLsp1[P + 1];
    
    ipLsp0[0] = ipLsp1[0] = 0.0;
    for(i = 0; i < P; i++)
    {
	ipLsp0[i + 1] = qLsp[0][i];
	ipLsp1[i + 1] = qLsp[1][i];
    }
    
    IPC_lsp_lpc(ipLsp0, alphaip[0]);
    IPC_lsp_lpc(ipLsp1, alphaip[1]);
    
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


static void lp_synU(
                    float *synoutu, 
                    float *resinu,
                    float (*qLsp)[10],
                    HvxcDecStatus *HDS	/* in: pointer to decoder status(AI 990129) */
                    )
{
  int i;
  float lsp[2][P];
  float alphaip[2][P+1];
  float res[FRM];
  float syn[FRM];

  for(i=0;i<P;i++){
    lsp[0][i]=qLsp[0][i];
    lsp[1][i]=qLsp[1][i];
  }
    
  IpLsp2Lpc(lsp, alphaip);

  for(i = 0; i < FRM; i++)
    res[i] = resinu[i];
    
  if(HDS->decDelayMode == DM_SHORT)
    {
      calc_syn_cont2u_LD(res, alphaip, syn, HDS);
      if (!(HDS->testMode & TM_POSFIL_DISABLE)) 
        IPC_posfil_u_LD(syn, alphaip, HDS);
    }
  else
    {
      calc_syn_cont2u(res, alphaip, syn, HDS);
      if (!(HDS->testMode & TM_POSFIL_DISABLE)) 
        IPC_posfil_u(syn, alphaip, HDS);
    }	
    
  for (i = 0; i < FRM; i++)
    synoutu[i] = syn[i];
}

void td_synt(
float	*qRes,
int	*VUVs,
float	(*qLsp)[10],
float	*synoutu,
HvxcDecStatus *HDS	/* in: pointer to decoder status(AI 990129) */
)
{
  float suv[FRM];

  IPC_uvExt(VUVs, suv, qRes, HDS);

  lp_synU(synoutu, suv, qLsp, HDS);
}
