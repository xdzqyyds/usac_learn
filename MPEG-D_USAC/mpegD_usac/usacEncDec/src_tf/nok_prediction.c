/**************************************************************************

This software module was originally developed by
Nokia in the course of development of the MPEG-2 AAC/MPEG-4 
Audio standard ISO/IEC13818-7, 14496-1, 2 and 3.
This software module is an implementation of a part
of one or more MPEG-2 AAC/MPEG-4 Audio tools as specified by the
MPEG-2 aac/MPEG-4 Audio standard. ISO/IEC  gives users of the
MPEG-2aac/MPEG-4 Audio standards free license to this software module
or modifications thereof for use in hardware or software products
claiming conformance to the MPEG-2 aac/MPEG-4 Audio  standards. Those
intending to use this software module in hardware or software products
are advised that this use may infringe existing patents. The original
developer of this software module, the subsequent
editors and their companies, and ISO/IEC have no liability for use of
this software module or modifications thereof in an
implementation. Copyright is not released for non MPEG-2 aac/MPEG-4
Audio conforming products. The original developer retains full right to
use the code for the developer's own purpose, assign or donate the code to a
third party and to inhibit third party from using the code for non
MPEG-2 aac/MPEG-4 Audio conforming products. This copyright notice
must be included in all copies or derivative works.
Copyright (c)1997.  

***************************************************************************/
#include <math.h>
#include <stdio.h>

#include "allHandles.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "allVariables.h"        /* variables */

#include "common_m4a.h"
#include "nok_prediction.h"
#include "port.h"

#define         SBMAX_L                49
#define         DATA_LEN               3
#define         NRCSTA                 4
#define		RESET_GROUPS	       30

#define CODESIZE 128+128+256


static float codebook[CODESIZE] = { -2.9763780e+00F, -2.9291339e+00F,
-2.8818898e+00F, -2.8346457e+00F, -2.7874016e+00F, -2.7401575e+00F,
-2.6929134e+00F, -2.6456693e+00F, -2.5984252e+00F, -2.5511811e+00F,
-2.5039370e+00F, -2.4566929e+00F, -2.4094488e+00F, -2.3622047e+00F,
-2.3149606e+00F, -2.2677165e+00F, -2.2204724e+00F, -2.1732283e+00F,
-2.1259843e+00F, -2.0787402e+00F, -2.0314961e+00F, -1.9842520e+00F,
-1.9370079e+00F, -1.8897638e+00F, -1.8425197e+00F, -1.7952756e+00F,
-1.7480315e+00F, -1.7007874e+00F, -1.6535433e+00F, -1.6062992e+00F,
-1.5590551e+00F, -1.5118110e+00F, -1.4645669e+00F, -1.4173228e+00F,
-1.3700787e+00F, -1.3228346e+00F, -1.2755906e+00F, -1.2283465e+00F,
-1.1811024e+00F, -1.1338583e+00F, -1.0866142e+00F, -1.0393701e+00F,
-9.9212598e-01F, -9.4488189e-01F, -8.9763780e-01F, -8.5039370e-01F,
-8.0314961e-01F, -7.5590551e-01F, -7.0866142e-01F, -6.6141732e-01F,
-6.1417323e-01F, -5.6692913e-01F, -5.1968504e-01F, -4.7244094e-01F,
-4.2519685e-01F, -3.7795276e-01F, -3.3070866e-01F, -2.8346457e-01F,
-2.3622047e-01F, -1.8897638e-01F, -1.4173228e-01F, -9.4488189e-02F,
-4.7244094e-02F, 0.0000000e+00F, 4.7244094e-02F, 9.4488189e-02F,
1.4173228e-01F, 1.8897638e-01F, 2.3622047e-01F, 2.8346457e-01F,
3.3070866e-01F, 3.7795276e-01F, 4.2519685e-01F, 4.7244094e-01F,
5.1968504e-01F, 5.6692913e-01F, 6.1417323e-01F, 6.6141732e-01F,
7.0866142e-01F, 7.5590551e-01F, 8.0314961e-01F, 8.5039370e-01F,
8.9763780e-01F, 9.4488189e-01F, 9.9212598e-01F, 1.0393701e+00F,
1.0866142e+00F, 1.1338583e+00F, 1.1811024e+00F, 1.2283465e+00F,
1.2755906e+00F, 1.3228346e+00F, 1.3700787e+00F, 1.4173228e+00F,
1.4645669e+00F, 1.5118110e+00F, 1.5590551e+00F, 1.6062992e+00F,
1.6535433e+00F, 1.7007874e+00F, 1.7480315e+00F, 1.7952756e+00F,
1.8425197e+00F, 1.8897638e+00F, 1.9370079e+00F, 1.9842520e+00F,
2.0314961e+00F, 2.0787402e+00F, 2.1259843e+00F, 2.1732283e+00F,
2.2204724e+00F, 2.2677165e+00F, 2.3149606e+00F, 2.3622047e+00F,
2.4094488e+00F, 2.4566929e+00F, 2.5039370e+00F, 2.5511811e+00F,
2.5984252e+00F, 2.6456693e+00F, 2.6929134e+00F, 2.7401575e+00F,
2.7874016e+00F, 2.8346457e+00F, 2.8818898e+00F, 2.9291339e+00F,
2.9763780e+00F, 2.9763780e+00F,

1.000000e+00F, 9.921875e-01F, 9.843750e-01F, 9.765625e-01F, 9.687500e-01F,
9.609375e-01F, 9.570312e-01F, 9.492188e-01F, 9.414062e-01F, 9.335938e-01F,
9.257812e-01F, 9.218750e-01F, 9.140625e-01F, 9.062500e-01F, 9.023438e-01F,
8.945312e-01F, 8.906250e-01F, 8.828125e-01F, 8.750000e-01F, 8.710938e-01F,
8.632812e-01F, 8.593750e-01F, 8.515625e-01F, 8.476562e-01F, 8.437500e-01F,
8.359375e-01F, 8.320312e-01F, 8.242188e-01F, 8.203125e-01F, 8.164062e-01F,
8.085938e-01F, 8.046875e-01F, 8.007812e-01F, 7.968750e-01F, 7.890625e-01F,
7.851562e-01F, 7.812500e-01F, 7.773438e-01F, 7.695312e-01F, 7.656250e-01F,
7.617188e-01F, 7.578125e-01F, 7.539062e-01F, 7.500000e-01F, 7.460938e-01F,
7.382812e-01F, 7.343750e-01F, 7.304688e-01F, 7.265625e-01F, 7.226562e-01F,
7.187500e-01F, 7.148438e-01F, 7.109375e-01F, 7.070312e-01F, 7.031250e-01F,
6.992188e-01F, 6.953125e-01F, 6.914062e-01F, 6.875000e-01F, 6.835938e-01F,
6.796875e-01F, 6.757812e-01F, 6.718750e-01F, 6.718750e-01F, 6.679688e-01F,
6.640625e-01F, 6.601562e-01F, 6.562500e-01F, 6.523438e-01F, 6.484375e-01F,
6.445312e-01F, 6.445312e-01F, 6.406250e-01F, 6.367188e-01F, 6.328125e-01F,
6.289062e-01F, 6.289062e-01F, 6.250000e-01F, 6.210938e-01F, 6.171875e-01F,
6.171875e-01F, 6.132812e-01F, 6.093750e-01F, 6.054688e-01F, 6.054688e-01F,
6.015625e-01F, 5.976562e-01F, 5.937500e-01F, 5.937500e-01F, 5.898438e-01F,
5.859375e-01F, 5.859375e-01F, 5.820312e-01F, 5.781250e-01F, 5.781250e-01F,
5.742188e-01F, 5.703125e-01F, 5.703125e-01F, 5.664062e-01F, 5.625000e-01F,
5.625000e-01F, 5.585938e-01F, 5.546875e-01F, 5.546875e-01F, 5.507812e-01F,
5.507812e-01F, 5.468750e-01F, 5.429688e-01F, 5.429688e-01F, 5.390625e-01F,
5.390625e-01F, 5.351562e-01F, 5.351562e-01F, 5.312500e-01F, 5.273438e-01F,
5.273438e-01F, 5.234375e-01F, 5.234375e-01F, 5.195312e-01F, 5.195312e-01F,
5.156250e-01F, 5.156250e-01F, 5.117188e-01F, 5.117188e-01F, 5.078125e-01F,
5.078125e-01F, 5.039062e-01F, 5.039062e-01F,

9.99999e+37F,  8.507059e+37F, 4.253530e+37F, 2.126765e+37F, 1.063382e+37F,
5.316912e+36F, 2.658456e+36F, 1.329228e+36F, 6.646140e+35F, 3.323070e+35F,
1.661535e+35F, 8.307675e+34F, 4.153837e+34F, 2.076919e+34F, 1.038459e+34F,
5.192297e+33F, 2.596148e+33F, 1.298074e+33F, 6.490371e+32F, 3.245186e+32F,
1.622593e+32F, 8.112964e+31F, 4.056482e+31F, 2.028241e+31F, 1.014120e+31F,
5.070602e+30F, 2.535301e+30F, 1.267651e+30F, 6.338253e+29F, 3.169127e+29F,
1.584563e+29F, 7.922816e+28F, 3.961408e+28F, 1.980704e+28F, 9.903520e+27F,
4.951760e+27F, 2.475880e+27F, 1.237940e+27F, 6.189700e+26F, 3.094850e+26F,
1.547425e+26F, 7.737125e+25F, 3.868563e+25F, 1.934281e+25F, 9.671407e+24F,
4.835703e+24F, 2.417852e+24F, 1.208926e+24F, 6.044629e+23F, 3.022315e+23F,
1.511157e+23F, 7.555786e+22F, 3.777893e+22F, 1.888947e+22F, 9.444733e+21F,
4.722366e+21F, 2.361183e+21F, 1.180592e+21F, 5.902958e+20F, 2.951479e+20F,
1.475740e+20F, 7.378698e+19F, 3.689349e+19F, 1.844674e+19F, 9.223372e+18F,
4.611686e+18F, 2.305843e+18F, 1.152922e+18F, 5.764608e+17F, 2.882304e+17F,
1.441152e+17F, 7.205759e+16F, 3.602880e+16F, 1.801440e+16F, 9.007199e+15F,
4.503600e+15F, 2.251800e+15F, 1.125900e+15F, 5.629500e+14F, 2.814750e+14F,
1.407375e+14F, 7.036874e+13F, 3.518437e+13F, 1.759219e+13F, 8.796093e+12F,
4.398047e+12F, 2.199023e+12F, 1.099512e+12F, 5.497558e+11F, 2.748779e+11F,
1.374390e+11F, 6.871948e+10F, 3.435974e+10F, 1.717987e+10F, 8.589935e+09F,
4.294967e+09F, 2.147484e+09F, 1.073742e+09F, 5.368709e+08F, 2.684355e+08F,
1.342177e+08F, 6.710886e+07F, 3.355443e+07F, 1.677722e+07F, 8.388608e+06F,
4.194304e+06F, 2.097152e+06F, 1.048576e+06F, 5.242880e+05F, 2.621440e+05F,
1.310720e+05F, 6.553600e+04F, 3.276800e+04F, 1.638400e+04F, 8.192000e+03F,
4.096000e+03F, 2.048000e+03F, 1.024000e+03F, 5.120000e+02F, 2.560000e+02F,
1.280000e+02F, 6.400000e+01F, 3.200000e+01F, 1.600000e+01F, 8.000000e+00F,
4.000000e+00F, 2.000000e+00F, 1.000000e+00F, 5.000000e-01F, 2.500000e-01F,
1.250000e-01F, 6.250000e-02F, 3.125000e-02F, 1.562500e-02F, 7.812500e-03F,
3.906250e-03F, 1.953125e-03F, 9.765625e-04F, 4.882812e-04F, 2.441406e-04F,
1.220703e-04F, 6.103516e-05F, 3.051758e-05F, 1.525879e-05F, 7.629395e-06F,
3.814697e-06F, 1.907349e-06F, 9.536743e-07F, 4.768372e-07F, 2.384186e-07F,
1.192093e-07F, 5.960464e-08F, 2.980232e-08F, 1.490116e-08F, 7.450581e-09F,
3.725290e-09F, 1.862645e-09F, 9.313226e-10F, 4.656613e-10F, 2.328306e-10F,
1.164153e-10F, 5.820766e-11F, 2.910383e-11F, 1.455192e-11F, 7.275958e-12F,
3.637979e-12F, 1.818989e-12F, 9.094947e-13F, 4.547474e-13F, 2.273737e-13F,
1.136868e-13F, 5.684342e-14F, 2.842171e-14F, 1.421085e-14F, 7.105427e-15F,
3.552714e-15F, 1.776357e-15F, 8.881784e-16F, 4.440892e-16F, 2.220446e-16F,
1.110223e-16F, 5.551115e-17F, 2.775558e-17F, 1.387779e-17F, 6.938894e-18F,
3.469447e-18F, 1.734723e-18F, 8.673617e-19F, 4.336809e-19F, 2.168404e-19F,
1.084202e-19F, 5.421011e-20F, 2.710505e-20F, 1.355253e-20F, 6.776264e-21F,
3.388132e-21F, 1.694066e-21F, 8.470329e-22F, 4.235165e-22F, 2.117582e-22F,
1.058791e-22F, 5.293956e-23F, 2.646978e-23F, 1.323489e-23F, 6.617445e-24F,
3.308722e-24F, 1.654361e-24F, 8.271806e-25F, 4.135903e-25F, 2.067952e-25F,
1.033976e-25F, 5.169879e-26F, 2.584939e-26F, 1.292470e-26F, 6.462349e-27F,
3.231174e-27F, 1.615587e-27F, 8.077936e-28F, 4.038968e-28F, 2.019484e-28F,
1.009742e-28F, 5.048710e-29F, 2.524355e-29F, 1.262177e-29F, 6.310887e-30F,
3.155444e-30F, 1.577722e-30F, 7.888609e-31F, 3.944305e-31F, 1.972152e-31F,
9.860761e-32F, 4.930381e-32F, 2.465190e-32F, 1.232595e-32F, 6.162976e-33F,
3.081488e-33F, 1.540744e-33F, 7.703720e-34F, 3.851860e-34F, 1.925930e-34F,
9.629650e-35F, 4.814825e-35F, 2.407412e-35F, 1.203706e-35F, 6.018531e-36F,
3.009266e-36F, 1.504633e-36F, 7.523164e-37F, 3.761582e-37F, 1.880791e-37F,
9.403955e-38F, 4.701977e-38F, 2.350989e-38F, 1.175494e-38F, 5.877472e-39F,
0.000000e+00F
};


void
nok_reset_pred_state (NOK_PRED_STATUS *psp)
{
    int i,j;
    for (i = 0; i < NOK_BLN5; i++)
	for (j = 0; j < NOK_BLEN; j++)
	    psp->x_buffer[i][j] = 0;

    for (i = 0; i < NOK_LPC; i++)
	for (j = 0; j < NOK_CLEN; j++)
	    psp->pred[i][j] = 63;

    psp->indicate = 0;
}


void
nok_init_pred_stat (NOK_PRED_STATUS *psp)
{
    nok_reset_pred_state (psp);
}


static unsigned short
fs(float x)
{
    unsigned short ps;
    unsigned long tmp;
    float *pf = (float *) &tmp;
  
    *pf = x;
    ps = tmp>>16;
    return ps;
}


static float
sf(unsigned short ps)
{
    float x;
    unsigned long tmp;
    float *pf = (float *) &tmp;
  
    tmp = ps<<16;
    x = *pf;
    return x;
}


static void
quantize(float coef, unsigned char *findex)
{
    int sig, index;
    float freq;

    freq = coef *0.992187500/3.0-0.00781250 ;
    if (freq >= 0)
	sig = 1;
    else {
	sig = 0;
	freq += 1.0;
    }
    index = (unsigned int) (freq * (float) (1L<<6));
    if (sig)
	index = index|1<<6;
    *findex = index;
}


/**************************************************************************
  Title:	flt_round_8

  Purpose:	Rounds-to-infinity with 8 significant mantissa bits.

  Usage:	flt_round_8(pf)

  Input:	pf	- a pointer to float

  Output:		-    

  References:		-

  Explanation:	Function stores the float into a variable of type
  		'unsigned long int', and performs the rounding
		with bit manipulation. The implementation assumes
		that the machine uses IEEE floating point numbers
		and that the size of the unsigned integers is 32
		bits. 
		This is based on code written for MPEG development.
		See document ISO/IEC JTC1/SC29/WG11 MPEG97/M1893.

  Author(s):		-
  Modified by:	Mikko Suonio
  *************************************************************************/

static void
flt_round_8(float *pf)
{
    int flg;
    unsigned long tmp, tmp1;
    float *pt = (float *)&tmp;

    *pt = *pf;			/* Write float to memory */
    tmp1 = tmp;			/* save in tmp1 */
    /* 'ul' denotes 'unsigned long' */
    flg = tmp & 0x00008000ul;	/* rounding position */
    tmp &= 0xffff0000ul;		/* truncated float */
    *pf = *pt;
  
    /* round 1/2 lsb towards infinity */
    if (flg) {
	tmp = tmp1 & 0xff810000ul;	/* 2^e + 1/2 lsb */
	*pf += *pt;		/* add 2^e + 1/2 lsb */
	tmp &= 0xff800000ul;	/* extract 2^e */
	*pf -= *pt;		/* substract 2^e */
    }
}



/********************************************************************************
 *** FUNCTION: nok_predict()							*
 ***										*
 ***    carry out prediction for all allowed spectral lines			*
 ***										*
 ********************************************************************************/

/* This function is based on the AT&T's implementation of the AAC decoder
   (compliant to the draft international standard). 
   1997-03-11  Mikko Suonio 
   1997-06-26  Lin Yin      Final version */


void
nok_predict (Info *info, int profile, int *lpflag, NOK_PRED_STATUS *psp, Float *prev_quant,
	 Float *coef)
{
    int i, j, ki, b, to, flag0, i0,i1,i2,i3,i4;
    const short *top;
    Float pred_temp;
    Float coef_temp[1024];
    float lpc_coef[NOK_LPC], tmp1, tmp2, tmp3, err;
    float y[DATA_LEN+2];
    float r[2][2], r1[2];
    unsigned short xtemp, temp1, temp2;

    if (profile != Main_Profile) {
	if (*lpflag != 0) {
	   	CommonExit(1, "Prediction not allowed in this profile!\n");
	}
	else {
	    /* prediction calculations not required */
	    return;
	}
    }
    
    if (info->islong) {
	for (i = 0; i < NOK_CLEN; i++) {
	    coef_temp[i] = coef[i];
	    flt_round_8 (&coef_temp[i]);
	}

	b = 0;
	i = 0;
	top = info->sbk_sfb_top[b];
	flag0 = *lpflag++;
	for (j = 0; j < MAX_PRED_SFB; j++) {
	    to = *top++;
	    for ( ; i < to/NRCSTA; i++) {
		i4 = i*4;

		i0 = i4 + psp->indicate;
		i1 = i4 + (psp->indicate + 1) % NRCSTA;
		i2 = i4 + (psp->indicate + 2) % NRCSTA;
		i3 = i4 + (psp->indicate + 3) % NRCSTA; 

		xtemp=fs(prev_quant[i0]);
		
		for (ki = 0; ki<DATA_LEN+1; ki++)
		  y[ki] = sf(psp->x_buffer[ki][i]);
		y[DATA_LEN+1] = sf(xtemp);

                tmp1 = y[1]*y[1];
                tmp2 = y[2]*y[2];
                tmp3 = y[3]*y[3];
                r[0][0] = (tmp1+tmp2+tmp3)*2.0*1.005;
                r[1][1] = (y[0]*y[0]+tmp1+tmp2*2.0+tmp3+y[4]*y[4])*1.005;
                r[0][1] = y[0]*y[1]+((y[1]+y[3])*y[2])*2+y[3]*y[4];
                r1[1] = ((y[0]+y[4])*y[2]+y[1]*y[3])*2.0;
                
                err = r[0][0]*r[1][1]-r[0][1]*r[0][1];
                if (fabs(err) < 1.0e+1) {
		    lpc_coef[0]=0.0;
		    lpc_coef[1]=0.0;
		} 
                else {
		    temp1 = fs(err);
		    temp2 = (temp1>>7);
		    temp1 &= 0x007F;
		    tmp1 = codebook[temp1+128]*codebook[temp2+256];
		    lpc_coef[0] = (r[1][1]-r1[1])*r[0][1]*tmp1;
		    lpc_coef[1] = (-r[0][1]*r[0][1]+r[0][0]*r1[1])*tmp1;
		}
                
                quantize(lpc_coef[0], &psp->pred[0][i0]);
                quantize(lpc_coef[1], &psp->pred[1][i0]);

		for (ki=0; ki<DATA_LEN;ki++)
		  psp->x_buffer[ki][i] = psp->x_buffer[ki+4][i];
		psp->x_buffer[3][i] = fs(prev_quant[i1]);

		for (ki=0; ki<DATA_LEN-1;ki++)
		  psp->x_buffer[4+ki][i] = psp->x_buffer[ki+7][i];
		psp->x_buffer[6][i] = fs(prev_quant[i2]);

		for (ki=0; ki<DATA_LEN-2;ki++)
		  psp->x_buffer[7+ki][i] = psp->x_buffer[ki+9][i];
		psp->x_buffer[8][i] = fs(prev_quant[i3]);

		psp->x_buffer[9][i] = xtemp;

		if (flag0 && *lpflag) {
		    pred_temp = codebook[psp->pred[0][i0]]*y[DATA_LEN+1]
			+ codebook[psp->pred[1][i0]]*y[DATA_LEN];
		    coef[i0] += pred_temp;
		    coef_temp[i0] += pred_temp;
		
		    pred_temp = codebook[psp->pred[0][i1]]*sf(psp->x_buffer[3][i])
			+ codebook[psp->pred[1][i1]]*sf(psp->x_buffer[2][i]);
		    coef[i1] += pred_temp;
		    coef_temp[i1] += pred_temp;

		    pred_temp = codebook[psp->pred[0][i2]]*sf(psp->x_buffer[6][i])
			+ codebook[psp->pred[1][i2]]*sf(psp->x_buffer[5][i]);
		    coef[i2] += pred_temp;
		    coef_temp[i2] += pred_temp;

		    pred_temp = codebook[psp->pred[0][i3]]*sf(psp->x_buffer[8][i])
			+ codebook[psp->pred[1][i3]]*sf(psp->x_buffer[7][i]);
		    coef[i3] += pred_temp;
		    coef_temp[i3] += pred_temp;
		}
	    }
	    lpflag++;
	}
	psp->indicate++;
	psp->indicate = psp->indicate % NRCSTA;
      
        fltcpy(prev_quant, coef_temp, LN2);
    }
}


/********************************************************************************
 *** FUNCTION: predict_reset()							*
 ***										*
 ***    carry out cyclic predictor reset mechanism (long blocks)		*
 ***    resp. full reset (short blocks)						*
 ***										*
 ********************************************************************************/
void
nok_predict_reset(Info* info, int *prstflag, NOK_PRED_STATUS **psp, 
	      int firstCh, int lastCh)
{
    int j, i, prstflag0, prstgrp, ch;

    prstgrp = 0;
    if (info->islong) {
	prstflag0 = *prstflag++;
	if (prstflag0) {
	    for (j=0; j<LEN_PRED_RSTGRP-1; j++) {
		prstgrp |= prstflag[j];
		prstgrp <<= 1;
	    }
	    prstgrp |= prstflag[LEN_PRED_RSTGRP-1];

	    if (debug['r']) {
		fprintf(stderr,"PRST: prstgrp: %d  prstbits: ", prstgrp);
		for (j=0; j<LEN_PRED_RSTGRP; j++)
		   fprintf(stderr,"%d ", prstflag[j]);
		fprintf(stderr,"FIRST: %d LAST %d\n", firstCh, lastCh);
	    }

	    if ( (prstgrp<1) || (prstgrp>30) ) {
		fprintf(stderr, "ERROR in prediction reset pattern\n");
		return;
	    }

	    for (ch=firstCh; ch<=lastCh; ch++) {
		for (j=prstgrp-1; j<NOK_CLEN; j+=30) {
		    psp[ch]->pred[0][j] = 63;
		    psp[ch]->pred[1][j] = 63;
		    i=j/4;
		    if(psp[ch]->indicate==0) {
		        psp[ch]->x_buffer[0][i] = 0;
			psp[ch]->x_buffer[1][i] = 0;
			psp[ch]->x_buffer[2][i] = 0;
			psp[ch]->x_buffer[3][i] = 0;
		    }
		    if(psp[ch]->indicate==1) {
		        psp[ch]->x_buffer[4][i] = 0;
		        psp[ch]->x_buffer[5][i] = 0;
			psp[ch]->x_buffer[6][i] = 0;
		    }
		    if(psp[ch]->indicate==2) {
		        psp[ch]->x_buffer[7][i] = 0;
		        psp[ch]->x_buffer[8][i] = 0;
		    }
		    if(psp[ch]->indicate==3)
		        psp[ch]->x_buffer[9][i] = 0;
		}

		for (j=prstgrp-1; j<LN2; j+=30) 
		    psp[ch]->prev_quant[j] = 0.0F;
	    }
	} /* end predictor reset */
    } /* end islong */
    else { /* short blocks */
	/* complete prediction reset in all bins */
	for (ch=firstCh; ch<=lastCh; ch++) {
	    nok_reset_pred_state (&psp[ch][0]);
	    fltclr (psp[ch]->prev_quant, LN2);
	}
    }
}
