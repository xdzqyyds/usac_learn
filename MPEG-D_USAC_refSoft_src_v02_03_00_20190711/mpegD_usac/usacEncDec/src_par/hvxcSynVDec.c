
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
#include <stdlib.h>

#include "hvxc.h"
#include "hvxcDec.h"
#include "hvxcCommon.h"

static void calc_syn_cont2v(
float *res,
float (*alphaip)[11],
float *syn,
HvxcDecStatus *HDS	/* in: pointer to decoder status(AI 990129) */
)
{
  int i, ii, j;

  float out2;

  /* static float mem2[P+1]={0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
			  0.0, 0.0}; */

  for (ii = 0; ii < IP; ii++) {

    for (i = 0; i < 20; i++) {
      out2 = 0.0;
      for (j = P; j > 0; j--) {
	out2 += HDS->lpv_mem2[j]*alphaip[ii][j];
	HDS->lpv_mem2[j] = HDS->lpv_mem2[j-1];
      }
      out2 = res[ii*20+i] - out2;
      HDS->lpv_mem2[1] = out2;
      syn[ii*20+i] = out2;
    }

  }
}


static void ip_lsp2(
float (*lsp)[P+1], 
float (*lspip)[P+1]
)
{
  int i, j;
  float fip;
    
  fip =(float)IP*2;
    
  for (j = 0; j < IP; j++) {
    for (i = 0; i < P+1; i++)
      lspip[j][i] = ((j*2.0+1.0)/fip)*lsp[1][i] + ((fip-(j*2.0+1.0))/fip)*lsp[0][i];
  }
}

void IPC_ip_lsp_LD(
float (*lsp)[P+1], 
float (*lspip)[P+1]
)
{
  int i, j;
  float fj;
	    
  for (j = 0; j < 7; j++) {
    fj = (float)j;
    for (i = 0; i < P+1; i++)
      lspip[j][i] = ((fj*2.0+1.0)/14.0)*lsp[1][i] + ((14.0-(fj*2.0+1.0))/14.0)*lsp[0][i];
  }
  for (j = 7; j < 8; j++) {
    for (i = 0; i < P+1; i++)
      lspip[j][i] = lsp[1][i] ;
  }
}

static void lp_synV(
int *vuv,
float *synoutv,
float *resinv,
float (*qLsp)[10],
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
  int i, j;
  float lsp[2][P+1];
  float lspip[IP][P+1];
  float alphaip[IP][P+1];
  float res[FRM];
  float syn[FRM];
    
  if (vuv[0] != 0 && vuv[1] != 0) { 
    for (i = 0; i < P; i++) {
      lsp[0][i+1] = qLsp[0][i];
      lsp[1][i+1] = qLsp[1][i];
    }
  }
  if (vuv[0] != 0 && vuv[1] == 0) { 
    for (i = 0; i < P; i++) {
      lsp[0][i+1] = qLsp[0][i];
      lsp[1][i+1] = qLsp[1][i];
    }
  }
  if (vuv[0] == 0 && vuv[1] != 0) { 
    for (i = 0; i < P; i++) {
      lsp[0][i+1] = qLsp[0][i];
      lsp[1][i+1] = qLsp[1][i];
    }
  }
  if (vuv[0] == 0 && vuv[1] == 0) { 
    for (i = 0; i < P; i++) {
      lsp[0][i+1] = (i+1)*0.5/11.0;
      lsp[1][i+1] = (i+1)*0.5/11.0;
    }
  }
    
  lsp[0][0] = lsp[1][0] = 0.0;

  if (HDS->decDelayMode == DM_SHORT) {
    IPC_ip_lsp_LD(lsp, lspip);
  }
  else {
    ip_lsp2(lsp, lspip);
  }
  
  for (j = 0; j < IP; j++) {
    IPC_lsp_lpc(lspip[j], alphaip[j]);
	
    alphaip[j][0] = 1.0;
    for (i = 1; i <= P; i++)
      alphaip[j][i] = -alphaip[j][i];
  }

  for (i = 0; i < FRM; i++)
    res[i] = resinv[i];

  calc_syn_cont2v(res, alphaip, syn, HDS);
  
  if (!(HDS->testMode & TM_POSFIL_DISABLE)) IPC_posfil_v(syn, alphaip, HDS);
    
  for (i = 0; i < FRM; i++)
    synoutv[i] = syn[i];
}



void harm_srew_synt(
float (*am)[3],
float *PCHs,
int *VUVs,
float (*qLsp)[P],
int lsUn,
float *synoutv,
HvxcDecStatus *HDS)	/* in: pointer to decoder status(AI 990129) */
{
  int i;
  float sv[FRM];
  float add_uv[FRM];

  if (lsUn != 8) {
    printf("Invalid LSF update number !!");
    exit(10);
  }

  IPC_vExt_fft(PCHs, am, VUVs, sv, HDS);

  if (!(HDS->testMode & TM_NOISE_ADD_DISABLE)) {	/* 99/01/12 */
    IPC_UvAdd(PCHs, am, VUVs, add_uv, HDS);

    for (i = 0; i < FRM; i++) {
      sv[i] = sv[i] + add_uv[i];
    }
  }
  
  lp_synV(VUVs, synoutv, sv, qLsp, HDS);
}
