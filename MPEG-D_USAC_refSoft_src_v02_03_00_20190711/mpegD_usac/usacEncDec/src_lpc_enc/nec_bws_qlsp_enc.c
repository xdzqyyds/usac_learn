/*
This software module was originally developed by
Toshiyuki Nomura (NEC Corporation)
and edited by

in the course of development of the
MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.
This software module is an implementation of a part of one or more
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio
standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards
free license to this software module or modifications thereof for use in
hardware or software products claiming conformance to the MPEG-2 AAC/
MPEG-4 Audio  standards. Those intending to use this software module in
hardware or software products are advised that this use may infringe
existing patents. The original developer of this software module and
his/her company, the subsequent editors and their companies, and ISO/IEC
have no liability for use of this software module or modifications
thereof in an implementation. Copyright is not released for non
MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer
retains full right to use the code for his/her  own purpose, assign or
donate the code to a third party and to inhibit third party from using
the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.
This copyright notice must be included in all copies or derivative works.
Copyright (c)1996.
*/
/*
 *	MPEG-4 Audio Verification Model (LPC-ABS Core)
 *	
 *	LSP Parameters Encoding Subroutines
 *
 *	Ver1.0	97.09.08	T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "nec_abs_proto.h"
#include "nec_abs_const.h"
#include "nec_lspnw20.tbl"

#define	NEC_LSPPRDCT_ORDER	4
#define NEC_NUM_LSPSPLIT1	2
#define NEC_NUM_LSPSPLIT2	4
#define NEC_QLSP_CAND		2
#define NEC_LSP_MINWIDTH_FRQ16	0.028

#define NEC_MAX_LSPVQ_ORDER	20

static void nec_psvq( float *, float *, float *,
		     long, long, float *, long *, long );
static void nec_lsp_sort( float *, long );

static float	nec_lsp_minwidth;

void nec_bws_lsp_quantizer(
		       float lsp[],		/* input  */
		       float qlsp8[],		/* input  */
		       float qlsp[],		/* output  */
		       float blsp[][NEC_MAX_LSPVQ_ORDER], /* input/output */
		       long indices[],		/* output  */
		       long frame_bit_allocation[], /* configuration input */
		       long lpc_order,		/* configuration input */
		       long lpc_order_8)	/* configuration input */
{
   long		i, j, k;
   long		cb_size;
   long		cidx = 0;
   long         sp_order;
   long		cand1[NEC_NUM_LSPSPLIT1][NEC_QLSP_CAND];
   long		cand2[NEC_NUM_LSPSPLIT2][NEC_QLSP_CAND];
   float	*qqlsp, *tlsp;
   float	*error, *error2;
   float	*vec_hat, *weight;
   float	mindist, dist, sub;
   float	*cb[1+NEC_NUM_LSPSPLIT1+NEC_NUM_LSPSPLIT2];

   /* Memory allocation */
   if ((qqlsp = (float *)calloc(lpc_order, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_lsp_quantizer \n");
      exit(1);
   }
   if ((tlsp = (float *)calloc(lpc_order, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_lsp_quantizer \n");
      exit(1);
   }
   if ((error = (float *)calloc(lpc_order, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_lsp_quantizer \n");
      exit(1);
   }
   if ((error2 = (float *)calloc(lpc_order*NEC_QLSP_CAND, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_lsp_quantizer \n");
      exit(1);
   }
   if ((vec_hat = (float *)calloc(lpc_order, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_lsp_quantizer \n");
      exit(1);
   }
   if ((weight = (float *)calloc(lpc_order+2, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_lsp_quantizer \n");
      exit(1);
   }

   if ( lpc_order == 20 && lpc_order_8 == 10 ) {
     cb[0] = nec_lspnw_p;
     cb[1] = nec_lspnw_1a;
     cb[2] = nec_lspnw_1b;
     cb[3] = nec_lspnw_2a;
     cb[4] = nec_lspnw_2b;
     cb[5] = nec_lspnw_2c;
     cb[6] = nec_lspnw_2d;
     nec_lsp_minwidth = (float)NEC_LSP_MINWIDTH_FRQ16;
   } else {
      printf("Error in nec_bws_lsp_quantizer\n");
      exit(1);
   }

   /*--- calc. weight ----*/
   weight[0] = 0.0;
   weight[lpc_order+1] = (float)NEC_PAI;
   for ( i = 0; i < lpc_order; i++ )
      weight[i+1] = lsp[i];
   for ( i = 0; i <= lpc_order; i++ )
      weight[i] = (float)(1.0/(weight[i+1]-weight[i]));
   for ( i = 0; i < lpc_order; i++ )
      weight[i] = (weight[i]+weight[i+1]);

   /*--- vector linear prediction ----*/
   for ( i = 0; i < lpc_order; i++)
     blsp[NEC_LSPPRDCT_ORDER-1][i] = 0.0;
   for ( i = 0; i < lpc_order_8; i++)
     blsp[NEC_LSPPRDCT_ORDER-1][i] = qlsp8[i];

   for ( i = 0; i < lpc_order; i++ ) {
      vec_hat[i] = 0.0;
      for ( k = 1; k < NEC_LSPPRDCT_ORDER; k++ ) {
	 vec_hat[i] += (cb[0][k*lpc_order+i] * blsp[k][i]);
      }
   }
   for ( i = 0; i < lpc_order; i++) error[i] = lsp[i] - vec_hat[i];

   /*--- 1st VQ -----*/
   sp_order = lpc_order/NEC_NUM_LSPSPLIT1;
   for ( i = 0; i < NEC_NUM_LSPSPLIT1; i++ ) {
      cb_size = 1<<frame_bit_allocation[i];
      nec_psvq(error+i*sp_order,&cb[0][i*sp_order],cb[i+1],cb_size,sp_order,
	       weight+i*sp_order,cand1[i],NEC_QLSP_CAND);
   }
   for ( k = 0; k < NEC_QLSP_CAND; k++ ) {
      for ( i = 0; i < NEC_NUM_LSPSPLIT1; i++ ) {
	 for ( j = 0; j < sp_order; j++)
	    error2[k*lpc_order+i*sp_order+j] = 
	       error[i*sp_order+j] - cb[0][i*sp_order+j]
		                   * cb[i+1][sp_order*cand1[i][k]+j];
      }
   }

   /*--- 2nd VQ -----*/
   sp_order = lpc_order/NEC_NUM_LSPSPLIT2;
   for ( k = 0; k < NEC_QLSP_CAND; k++ ) {
      for ( i = 0; i < NEC_NUM_LSPSPLIT2; i++ ) {
	 cb_size = 1<<frame_bit_allocation[i+NEC_NUM_LSPSPLIT1];
	 nec_psvq(error2+k*lpc_order+i*sp_order,&cb[0][i*sp_order],
		  cb[i+1+NEC_NUM_LSPSPLIT1], cb_size,sp_order,
		  weight+i*sp_order,&cand2[i][k],1);
      }
   }

   mindist = (float)1.0e30;
   for ( k = 0; k < NEC_QLSP_CAND*NEC_QLSP_CAND; k++ ) {
      switch ( k ) {
      case 0:
	 sp_order = 10;
	 for ( j = 0; j < sp_order; j++)
	    tlsp[0+j] = cb[0+1][sp_order*cand1[0][0]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[10+j] = cb[1+1][sp_order*cand1[1][0]+j];
	 sp_order = 5;
	 for ( j = 0; j < sp_order; j++)
	    tlsp[0+j] += cb[0+1+2][sp_order*cand2[0][0]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[5+j] += cb[1+1+2][sp_order*cand2[1][0]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[10+j] += cb[2+1+2][sp_order*cand2[2][0]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[15+j] += cb[3+1+2][sp_order*cand2[3][0]+j];
	 break;
      case 1:
	 sp_order = 10;
	 for ( j = 0; j < sp_order; j++)
	    tlsp[0+j] = cb[0+1][sp_order*cand1[0][0]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[10+j] = cb[1+1][sp_order*cand1[1][1]+j];
	 sp_order = 5;
	 for ( j = 0; j < sp_order; j++)
	    tlsp[0+j] += cb[0+1+2][sp_order*cand2[0][0]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[5+j] += cb[1+1+2][sp_order*cand2[1][0]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[10+j] += cb[2+1+2][sp_order*cand2[2][1]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[15+j] += cb[3+1+2][sp_order*cand2[3][1]+j];
	 break;
      case 2:
	 sp_order = 10;
	 for ( j = 0; j < sp_order; j++)
	    tlsp[0+j] = cb[0+1][sp_order*cand1[0][1]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[10+j] = cb[1+1][sp_order*cand1[1][0]+j];
	 sp_order = 5;
	 for ( j = 0; j < sp_order; j++)
	    tlsp[0+j] += cb[0+1+2][sp_order*cand2[0][1]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[5+j] += cb[1+1+2][sp_order*cand2[1][1]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[10+j] += cb[2+1+2][sp_order*cand2[2][0]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[15+j] += cb[3+1+2][sp_order*cand2[3][0]+j];
	 break;
      case 3:
	 sp_order = 10;
	 for ( j = 0; j < sp_order; j++)
	    tlsp[0+j] = cb[0+1][sp_order*cand1[0][1]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[10+j] = cb[1+1][sp_order*cand1[1][1]+j];
	 sp_order = 5;
	 for ( j = 0; j < sp_order; j++)
	    tlsp[0+j] += cb[0+1+2][sp_order*cand2[0][1]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[5+j] += cb[1+1+2][sp_order*cand2[1][1]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[10+j] += cb[2+1+2][sp_order*cand2[2][1]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[15+j] += cb[3+1+2][sp_order*cand2[3][1]+j];
	 break;
      }

      for ( i = 0; i < lpc_order; i++ ) qqlsp[i] = vec_hat[i]+cb[0][i]*tlsp[i];
      nec_lsp_sort( qqlsp, lpc_order );
      dist = 0.0;
      for ( i = 0; i < lpc_order; i++ ) {
	 sub = lsp[i] - qqlsp[i];
	 dist += weight[i] * sub * sub;
      }
      if ( dist < mindist || k == 0 ) {
	 mindist = dist;
	 cidx = k;
	 for ( i = 0; i < lpc_order; i++ ) qlsp[i] = qqlsp[i];
	 for ( i = 0; i < lpc_order; i++ ) blsp[0][i] = tlsp[i];
      }
   }
   
   /*--- store previous vector ----*/
   for ( k = NEC_LSPPRDCT_ORDER-2; k > 0; k-- ) {
      for ( i = 0; i < lpc_order; i++ ) blsp[k][i] = blsp[k-1][i];
   }

   /*---- set INDEX -----*/
   indices[0] = cand1[0][cidx/2];
   indices[1] = cand1[1][cidx%2];
   indices[2] = cand2[0][cidx/2];
   indices[3] = cand2[1][cidx/2];
   indices[4] = cand2[2][cidx%2];
   indices[5] = cand2[3][cidx%2];

   free( qqlsp );
   free( tlsp );
   free( error );
   free( error2 );
   free( vec_hat );
   free( weight );

}

void nec_psvq( float vector[], float p[], float cb[],
	      long size, long order,
	      float weight[], long code[], long num )
{
   long		i, j, k;
   float	mindist, sub, *dist;

   if ((dist = (float *)calloc(size, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_svq \n");
      exit(1);
   }

   for ( i = 0; i < size; i++ ) {
      dist[i] = 0.0;
      for ( j = 0; j < order; j++ ) {
	 sub = vector[j] - p[j] * cb[i*order+j];
	 dist[i] += weight[j] * sub * sub;
      }
   }

   for ( k = 0; k < num; k++ ) {
      code[k] = 0;
      mindist = (float)1.0e30;
      for ( i = 0; i < size; i++ ) {
	 if ( dist[i] < mindist ) {
	    mindist = dist[i];
	    code[k] = i;
	 }
      }
      dist[code[k]] = (float)1.0e30;
   }
   free( dist );

}

void nec_lsp_sort( float x[], long order )
{
   long		i, j;
   float	tmp;

   for ( i = 0; i < order; i++ ) {
      if ( x[i] < 0.0 || x[i] > (float)NEC_PAI ) {
	 x[i] = (float)(0.05 + (float)NEC_PAI * (float)i / (float)order);
      }
   }

   for ( i = (order-1); i > 0; i-- ) {
      for ( j = 0; j < i; j++ ) {
         if ( x[j] + nec_lsp_minwidth > x[j+1] ) {
            tmp = (float)(0.5 * (x[j] + x[j+1]));
            x[j] = (float)(tmp - 0.51 * nec_lsp_minwidth);
            x[j+1] = (float)(tmp + 0.51 * nec_lsp_minwidth);
         }
      }
   }
}
