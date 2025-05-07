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
/**************************************************************************
  nok_bwp_enc.c  -  Nokia BWP for encoding
  Author(s): Mikko Suonio, Lin Yin, Kalervo Kontola
  	     Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/


#include        <stdio.h>
#include        <string.h>
#include        <math.h>
#include        <stdlib.h>

#include "allHandles.h"
#include "block.h"

#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "bitstream.h"
#include "nok_bwp_enc.h"
/*#include "tf_main.h"*/

static float NOK_codebook[NOK_CODESIZE] =
{
  -2.9763780e+00f, -2.9291339e+00f, -2.8818898e+00f, -2.8346457e+00f,
  -2.7874016e+00f, -2.7401575e+00f, -2.6929134e+00f, -2.6456693e+00f,
  -2.5984252e+00f, -2.5511811e+00f, -2.5039370e+00f, -2.4566929e+00f,
  -2.4094488e+00f, -2.3622047e+00f, -2.3149606e+00f, -2.2677165e+00f,
  -2.2204724e+00f, -2.1732283e+00f, -2.1259843e+00f, -2.0787402e+00f,
  -2.0314961e+00f, -1.9842520e+00f, -1.9370079e+00f, -1.8897638e+00f,
  -1.8425197e+00f, -1.7952756e+00f, -1.7480315e+00f, -1.7007874e+00f,
  -1.6535433e+00f, -1.6062992e+00f, -1.5590551e+00f, -1.5118110e+00f,
  -1.4645669e+00f, -1.4173228e+00f, -1.3700787e+00f, -1.3228346e+00f,
  -1.2755906e+00f, -1.2283465e+00f, -1.1811024e+00f, -1.1338583e+00f,
  -1.0866142e+00f, -1.0393701e+00f, -9.9212598e-01f, -9.4488189e-01f,
  -8.9763780e-01f, -8.5039370e-01f, -8.0314961e-01f, -7.5590551e-01f,
  -7.0866142e-01f, -6.6141732e-01f, -6.1417323e-01f, -5.6692913e-01f,
  -5.1968504e-01f, -4.7244094e-01f, -4.2519685e-01f, -3.7795276e-01f,
  -3.3070866e-01f, -2.8346457e-01f, -2.3622047e-01f, -1.8897638e-01f,
  -1.4173228e-01f, -9.4488189e-02f, -4.7244094e-02f,  0.0000000e+00f,
   4.7244094e-02f,  9.4488189e-02f,  1.4173228e-01f,  1.8897638e-01f,
   2.3622047e-01f,  2.8346457e-01f,  3.3070866e-01f,  3.7795276e-01f,
   4.2519685e-01f,  4.7244094e-01f,  5.1968504e-01f,  5.6692913e-01f,
   6.1417323e-01f,  6.6141732e-01f,  7.0866142e-01f,  7.5590551e-01f,
   8.0314961e-01f,  8.5039370e-01f,  8.9763780e-01f,  9.4488189e-01f,
   9.9212598e-01f,  1.0393701e+00f,  1.0866142e+00f,  1.1338583e+00f,
   1.1811024e+00f,  1.2283465e+00f,  1.2755906e+00f,  1.3228346e+00f,
   1.3700787e+00f,  1.4173228e+00f,  1.4645669e+00f,  1.5118110e+00f,
   1.5590551e+00f,  1.6062992e+00f,  1.6535433e+00f,  1.7007874e+00f,
   1.7480315e+00f,  1.7952756e+00f,  1.8425197e+00f,  1.8897638e+00f,
   1.9370079e+00f,  1.9842520e+00f,  2.0314961e+00f,  2.0787402e+00f,
   2.1259843e+00f,  2.1732283e+00f,  2.2204724e+00f,  2.2677165e+00f,
   2.3149606e+00f,  2.3622047e+00f,  2.4094488e+00f,  2.4566929e+00f,
   2.5039370e+00f,  2.5511811e+00f,  2.5984252e+00f,  2.6456693e+00f,
   2.6929134e+00f,  2.7401575e+00f,  2.7874016e+00f,  2.8346457e+00f,
   2.8818898e+00f,  2.9291339e+00f,  2.9763780e+00f,  2.9763780e+00f,
   1.000000e+00f,   9.921875e-01f,   9.843750e-01f,   9.765625e-01f,
   9.687500e-01f,   9.609375e-01f,   9.570312e-01f,   9.492188e-01f,
   9.414062e-01f,   9.335938e-01f,   9.257812e-01f,   9.218750e-01f,
   9.140625e-01f,   9.062500e-01f,   9.023438e-01f,   8.945312e-01f,
   8.906250e-01f,   8.828125e-01f,   8.750000e-01f,   8.710938e-01f,
   8.632812e-01f,   8.593750e-01f,   8.515625e-01f,   8.476562e-01f,
   8.437500e-01f,   8.359375e-01f,   8.320312e-01f,   8.242188e-01f,
   8.203125e-01f,   8.164062e-01f,   8.085938e-01f,   8.046875e-01f,
   8.007812e-01f,   7.968750e-01f,   7.890625e-01f,   7.851562e-01f,
   7.812500e-01f,   7.773438e-01f,   7.695312e-01f,   7.656250e-01f,
   7.617188e-01f,   7.578125e-01f,   7.539062e-01f,   7.500000e-01f,
   7.460938e-01f,   7.382812e-01f,   7.343750e-01f,   7.304688e-01f,
   7.265625e-01f,   7.226562e-01f,   7.187500e-01f,   7.148438e-01f,
   7.109375e-01f,   7.070312e-01f,   7.031250e-01f,   6.992188e-01f,
   6.953125e-01f,   6.914062e-01f,   6.875000e-01f,   6.835938e-01f,
   6.796875e-01f,   6.757812e-01f,   6.718750e-01f,   6.718750e-01f,
   6.679688e-01f,   6.640625e-01f,   6.601562e-01f,   6.562500e-01f,
   6.523438e-01f,   6.484375e-01f,   6.445312e-01f,   6.445312e-01f,
   6.406250e-01f,   6.367188e-01f,   6.328125e-01f,   6.289062e-01f,
   6.289062e-01f,   6.250000e-01f,   6.210938e-01f,   6.171875e-01f,
   6.171875e-01f,   6.132812e-01f,   6.093750e-01f,   6.054688e-01f,
   6.054688e-01f,   6.015625e-01f,   5.976562e-01f,   5.937500e-01f,
   5.937500e-01f,   5.898438e-01f,   5.859375e-01f,   5.859375e-01f,
   5.820312e-01f,   5.781250e-01f,   5.781250e-01f,   5.742188e-01f,
   5.703125e-01f,   5.703125e-01f,   5.664062e-01f,   5.625000e-01f,
   5.625000e-01f,   5.585938e-01f,   5.546875e-01f,   5.546875e-01f,
   5.507812e-01f,   5.507812e-01f,   5.468750e-01f,   5.429688e-01f,
   5.429688e-01f,   5.390625e-01f,   5.390625e-01f,   5.351562e-01f,
   5.351562e-01f,   5.312500e-01f,   5.273438e-01f,   5.273438e-01f,
   5.234375e-01f,   5.234375e-01f,   5.195312e-01f,   5.195312e-01f,
   5.156250e-01f,   5.156250e-01f,   5.117188e-01f,   5.117188e-01f,
   5.078125e-01f,   5.078125e-01f,   5.039062e-01f,   5.039062e-01f,
   9.99999e+37f,    8.507059e+37f,   4.253530e+37f,   2.126765e+37f,
   1.063382e+37f,   5.316912e+36f,   2.658456e+36f,   1.329228e+36f,
   6.646140e+35f,   3.323070e+35f,   1.661535e+35f,   8.307675e+34f,
   4.153837e+34f,   2.076919e+34f,   1.038459e+34f,   5.192297e+33f,
   2.596148e+33f,   1.298074e+33f,   6.490371e+32f,   3.245186e+32f,
   1.622593e+32f,   8.112964e+31f,   4.056482e+31f,   2.028241e+31f,
   1.014120e+31f,   5.070602e+30f,   2.535301e+30f,   1.267651e+30f,
   6.338253e+29f,   3.169127e+29f,   1.584563e+29f,   7.922816e+28f,
   3.961408e+28f,   1.980704e+28f,   9.903520e+27f,   4.951760e+27f,
   2.475880e+27f,   1.237940e+27f,   6.189700e+26f,   3.094850e+26f,
   1.547425e+26f,   7.737125e+25f,   3.868563e+25f,   1.934281e+25f,
   9.671407e+24f,   4.835703e+24f,   2.417852e+24f,   1.208926e+24f,
   6.044629e+23f,   3.022315e+23f,   1.511157e+23f,   7.555786e+22f,
   3.777893e+22f,   1.888947e+22f,   9.444733e+21f,   4.722366e+21f,
   2.361183e+21f,   1.180592e+21f,   5.902958e+20f,   2.951479e+20f,
   1.475740e+20f,   7.378698e+19f,   3.689349e+19f,   1.844674e+19f,
   9.223372e+18f,   4.611686e+18f,   2.305843e+18f,   1.152922e+18f,
   5.764608e+17f,   2.882304e+17f,   1.441152e+17f,   7.205759e+16f,
   3.602880e+16f,   1.801440e+16f,   9.007199e+15f,   4.503600e+15f,
   2.251800e+15f,   1.125900e+15f,   5.629500e+14f,   2.814750e+14f,
   1.407375e+14f,   7.036874e+13f,   3.518437e+13f,   1.759219e+13f,
   8.796093e+12f,   4.398047e+12f,   2.199023e+12f,   1.099512e+12f,
   5.497558e+11f,   2.748779e+11f,   1.374390e+11f,   6.871948e+10f,
   3.435974e+10f,   1.717987e+10f,   8.589935e+09f,   4.294967e+09f,
   2.147484e+09f,   1.073742e+09f,   5.368709e+08f,   2.684355e+08f,
   1.342177e+08f,   6.710886e+07f,   3.355443e+07f,   1.677722e+07f,
   8.388608e+06f,   4.194304e+06f,   2.097152e+06f,   1.048576e+06f,
   5.242880e+05f,   2.621440e+05f,   1.310720e+05f,   6.553600e+04f,
   3.276800e+04f,   1.638400e+04f,   8.192000e+03f,   4.096000e+03f,
   2.048000e+03f,   1.024000e+03f,   5.120000e+02f,   2.560000e+02f,
   1.280000e+02f,   6.400000e+01f,   3.200000e+01f,   1.600000e+01f,
   8.000000e+00f,   4.000000e+00f,   2.000000e+00f,   1.000000e+00f,
   5.000000e-01f,   2.500000e-01f,   1.250000e-01f,   6.250000e-02f,
   3.125000e-02f,   1.562500e-02f,   7.812500e-03f,   3.906250e-03f,
   1.953125e-03f,   9.765625e-04f,   4.882812e-04f,   2.441406e-04f,
   1.220703e-04f,   6.103516e-05f,   3.051758e-05f,   1.525879e-05f,
   7.629395e-06f,   3.814697e-06f,   1.907349e-06f,   9.536743e-07f,
   4.768372e-07f,   2.384186e-07f,   1.192093e-07f,   5.960464e-08f,
   2.980232e-08f,   1.490116e-08f,   7.450581e-09f,   3.725290e-09f,
   1.862645e-09f,   9.313226e-10f,   4.656613e-10f,   2.328306e-10f,
   1.164153e-10f,   5.820766e-11f,   2.910383e-11f,   1.455192e-11f,
   7.275958e-12f,   3.637979e-12f,   1.818989e-12f,   9.094947e-13f,
   4.547474e-13f,   2.273737e-13f,   1.136868e-13f,   5.684342e-14f,
   2.842171e-14f,   1.421085e-14f,   7.105427e-15f,   3.552714e-15f,
   1.776357e-15f,   8.881784e-16f,   4.440892e-16f,   2.220446e-16f,
   1.110223e-16f,   5.551115e-17f,   2.775558e-17f,   1.387779e-17f,
   6.938894e-18f,   3.469447e-18f,   1.734723e-18f,   8.673617e-19f,
   4.336809e-19f,   2.168404e-19f,   1.084202e-19f,   5.421011e-20f,
   2.710505e-20f,   1.355253e-20f,   6.776264e-21f,   3.388132e-21f,
   1.694066e-21f,   8.470329e-22f,   4.235165e-22f,   2.117582e-22f,
   1.058791e-22f,   5.293956e-23f,   2.646978e-23f,   1.323489e-23f,
   6.617445e-24f,   3.308722e-24f,   1.654361e-24f,   8.271806e-25f,
   4.135903e-25f,   2.067952e-25f,   1.033976e-25f,   5.169879e-26f,
   2.584939e-26f,   1.292470e-26f,   6.462349e-27f,   3.231174e-27f,
   1.615587e-27f,   8.077936e-28f,   4.038968e-28f,   2.019484e-28f,
   1.009742e-28f,   5.048710e-29f,   2.524355e-29f,   1.262177e-29f,
   6.310887e-30f,   3.155444e-30f,   1.577722e-30f,   7.888609e-31f,
   3.944305e-31f,   1.972152e-31f,   9.860761e-32f,   4.930381e-32f,
   2.465190e-32f,   1.232595e-32f,   6.162976e-33f,   3.081488e-33f,
   1.540744e-33f,   7.703720e-34f,   3.851860e-34f,   1.925930e-34f,
   9.629650e-35f,   4.814825e-35f,   2.407412e-35f,   1.203706e-35f,
   6.018531e-36f,   3.009266e-36f,   1.504633e-36f,   7.523164e-37f,
   3.761582e-37f,   1.880791e-37f,   9.403955e-38f,   4.701977e-38f,
   2.350989e-38f,   1.175494e-38f,   5.877472e-39f,   0.000000e+00f
};

#define         BWP_LPC                2
#define         BWP_BLN5              10
#define         BWP_BLEN             168

#define         SBMAX_L               49
#define         BLN5                  10
#define         CLEN                 672
#define         DATA_LEN               3
#define         NRCSTA                 4

#define         ONE_BIT                1
#define         BWP_RESET_BIT          1
#define         BWP_RESET_INDEX_BITS   5
#define         BWP_USED               1
#define         NOT_USED              -1
#define         MAX_SFB_SW             4
#define         MAX_SFB_LW             6
#define         NOK_BWP_PRED_SFB_MAX          40
#define SPECLEN 1024
#define LONG_BLOCK  0
#define START_BLOCK 1
#define SHORT_BLOCK 2
#define STOP_BLOCK 3
#define PRED_RESET_GROUPS 5
#define	RESET_PERIOD 8
#define	RESET_GROUPS 30
#define LEN_PREDICTOR_DATA_PRESENT 1
#define LEN_PREDICTION_USED 1
#define LEN_PREDICTOR_RESET 1
#define LEN_PREDICTOR_RESET_INDEX 5
#define DEBUG_THRESHOLD 4

static int indicate = 0;
static unsigned short x_buffer[BWP_BLN5][BWP_BLEN];
static unsigned char pred[2][CLEN];
static float sb_samples_pred[CLEN];
static int debuglevel;

static void autocorr (unsigned short x[BWP_BLN5][BWP_BLEN], double last_spec[CLEN], unsigned char pred[2][CLEN], float sb_samples_pred[CLEN], int m, int indicate);
static void flt_round_8 (float *pf);
static unsigned short fs (float x);
static float sf (unsigned short ps);
static void nok_quantize (float coef, unsigned char *findex);


void
nok_InitPrediction (int debug)
{
  int i, j;

  for (i = 0; i < BWP_BLN5; i++)
    for (j = 0; j < BWP_BLEN; j++)
      x_buffer[i][j] = 0;

  for (i = 0; i < BWP_LPC; i++)
    for (j = 0; j < CLEN; j++)
      pred[i][j] = 63;

  for (j = 0; j < CLEN; j++)
    sb_samples_pred[j] = 0;

  indicate = 0;

  debuglevel = debug;
}

/**************************************************************************
  Title:	PredCalcPrediction

  Purpose:	BWP for encoding

  Usage:	PredCalcPrediction (act_spec, last_spec, btype, nsfb,
  				    isfb_width, pred_global_flag,
				    pred_sfb_flag, reset_flag,
				    reset_group);

  Input:	act_spec	- output of MDCT
                last_spec       - dequantized values of the last predicted 
                                  spectrum; should be identically zero
				  on the first call
                btype           - window type (short/long)
                nsfb            - # of SFBs
                isfb_width      - table of SFB-widths
  Output:	act_spec	- prediction error if prediction is utilized
                pred_global_flag- TRUE if prediction used
                pred_sfb_flag   - a flag for each SFB indicating prediction
                reset_flag      - TRUE if reset used
                reset_group     - indicates which group (of 30 possibles) is 
                                  reset  (5 bit index)
  References:	

  Explanation:

  Author(s):	Mikko Suonio, Lin Yin, Kalervo Kontola
  *************************************************************************/

void
nok_PredCalcPrediction (double *act_spec, double *last_spec,
			int btype, int nsfb,
			int *isfb_width, short *pred_global_flag,
			int *pred_sfb_flag, short *reset_flag,
			int *reset_group, int *side_info)
{
  int i, k, j, cb_long, rg, last_band;
  float ftemp;
  double num_bit, snr[SBMAX_L];
  double energy[CLEN], snr_p[CLEN], temp1, temp2;

  static int do_reset = 0, resetting_group = 0, short_set = 0;
  static short pred_global_prev = 0;

  *reset_flag = 0;
  last_band = ( nsfb < NOK_BWP_PRED_SFB_MAX ) ? nsfb : NOK_BWP_PRED_SFB_MAX;
  if (debuglevel >= DEBUG_THRESHOLD)
    fprintf (stderr, "nok_PredCalcPrediction:  last_band %d\n", last_band);

  if (btype == EIGHT_SHORT_SEQUENCE)	/* was: ==2 */
    {
      for (i = 0; i < BWP_BLN5; i++)
	for (j = 0; j < BWP_BLEN; j++)
	  x_buffer[i][j] = 0;

      for (i = 0; i < BWP_LPC; i++)
	for (j = 0; j < CLEN; j++)
	  pred[i][j] = 63;

      for (j = 0; j < CLEN; j++)
	{
	  last_spec[j] = 0.0;
	  sb_samples_pred[j] = 0.0;
	}

      pred_global_flag[0] = 0;

      pred_global_prev = 0;

      indicate = 0;

      do_reset = 0;

      short_set = 0;

      if (debuglevel >= DEBUG_THRESHOLD)
	fprintf (stderr, "nok_PredCalcPrediction: number of bit save -1\n");

    }
  else
    {

      short_set++;

      for (i = 0; i < CLEN; i++)
	{
	  ftemp = (float)last_spec[i];
	  flt_round_8 (&ftemp);
	  last_spec[i] = ftemp;
	  last_spec[i] += sb_samples_pred[i];
	}

      if (short_set > 81 && pred_global_prev == 1 && do_reset == 1)
	{
	  rg = (resetting_group > 0) ? resetting_group - 1 : RESET_GROUPS - 1;
	  for (j = rg; j < CLEN; j += RESET_GROUPS)
	    last_spec[j] = 0.0;
	  if (debuglevel >= DEBUG_THRESHOLD)
	    fprintf (stderr, "nok_PredCalcPrediction: reset_group2 %d %d %d %d\n", short_set, pred_global_prev, do_reset, rg);
	}

      autocorr (x_buffer, last_spec, pred, sb_samples_pred, BWP_LPC, indicate);

      indicate += 1;
      indicate = indicate % NRCSTA;

/*******************************************************************/

      for (k = 0; k < CLEN; k++)
	{
	  energy[k] = act_spec[k] * act_spec[k];
	  snr_p[k] = (act_spec[k] - sb_samples_pred[k]) * (act_spec[k] - sb_samples_pred[k]);
	}

      cb_long = 0;
      for (i = 0; i < last_band; i++)
	{
	  pred_sfb_flag[i] = 1;
	  temp1 = 0.0;
	  temp2 = 0.0;
	  for (j = cb_long; j < cb_long + isfb_width[i]; j++)
	    {
	      temp1 += energy[j];
	      temp2 += snr_p[j];
	    }
	  if (temp2 < 1.e-20)
	    {
	      temp2 = 1.e-20;
	    }
	  if (temp1 != 0.0)
	    snr[i] = -10. * log10 (temp2 / temp1);
	  else
	    snr[i] = 0.0;
	  if (snr[i] <= 0.0)
	    {
	      pred_sfb_flag[i] = 0;
	      for (j = cb_long; j < cb_long + isfb_width[i]; j++)
		sb_samples_pred[j] = 0.0;
	    }
	  cb_long += isfb_width[i];
	}
      for ( ; i < NOK_BWP_PRED_SFB_MAX; i++)
	{
	  pred_sfb_flag[i] = 0;
	  for (j = cb_long; j < cb_long + isfb_width[i]; j++)
	    sb_samples_pred[j] = 0.0;
	  cb_long += isfb_width[i];
	}

      num_bit = 0.0;
      for (i = 0; i < last_band; i++)
	if (snr[i] > 0.0)
	  num_bit += snr[i] / 6. * isfb_width[i];

      pred_global_flag[0] = 1;
      if (num_bit < 50.0)
	{
	  if (debuglevel >= DEBUG_THRESHOLD)
	    fprintf (stderr, "nok_PredCalcPrediction: num_bit=%5.3f<50\n", num_bit);
	  pred_global_flag[0] = 0;
	  num_bit = 0.0;
	  for (j = 0; j < CLEN; j++)
	    sb_samples_pred[j] = 0.0;
	  for (j = 0; j < NOK_BWP_PRED_SFB_MAX; j++)
	    pred_sfb_flag[j] = 0;
	}

      if (debuglevel >= DEBUG_THRESHOLD)
	fprintf (stderr, "nok_PredCalcPrediction: bitsave %f\n", num_bit);
      if (debuglevel >= DEBUG_THRESHOLD)
	{
	  fprintf (stderr, "nok_PredCalcPrediction: pred_global_flag %d\n",
		   *pred_global_flag);
	  fprintf (stderr, "nok_PredCalcPrediction: pred_sfb_flag ");
	  for (j = 0; j < NOK_BWP_PRED_SFB_MAX; j++)
	    fprintf (stderr, "%d ", pred_sfb_flag[j]);
	  fprintf (stderr, "\n");
	}

      for (j = 0; j < CLEN; j++)
	act_spec[j] -= sb_samples_pred[j];
    }

  *side_info = LEN_PREDICTOR_DATA_PRESENT;
  if (*pred_global_flag)
    *side_info += last_band * LEN_PREDICTION_USED;

  if (*pred_global_flag)
    {	
      if (short_set > 80 && do_reset == 0)
	{
	  pred_global_prev = 1;
	  *reset_flag = 1;

	  if (debuglevel >= DEBUG_THRESHOLD)
	    fprintf (stderr, "nok_PredCalcPrediction: reset_group1 %d\n", resetting_group);

	  for (j = resetting_group; j < CLEN; j += RESET_GROUPS)
	    {
	      pred[0][j] = 63;
	      pred[1][j] = 63;
	      rg = j / 4;
	      if (indicate == 0)
		{
		  x_buffer[0][rg] = 0;
		  x_buffer[1][rg] = 0;
		  x_buffer[2][rg] = 0;
		  x_buffer[3][rg] = 0;
		}
	      if (indicate == 1)
		{
		  x_buffer[4][rg] = 0;
		  x_buffer[5][rg] = 0;
		  x_buffer[6][rg] = 0;
		}
	      if (indicate == 2)
		{
		  x_buffer[7][rg] = 0;
		  x_buffer[8][rg] = 0;
		}
	      if (indicate == 3)
		{
		  x_buffer[9][rg] = 0;
		}
	    }


	  *reset_group = resetting_group + 1;

	  resetting_group++;
	  resetting_group %= RESET_GROUPS;

	  *side_info += LEN_PREDICTOR_RESET + LEN_PREDICTOR_RESET_INDEX;
	}

      do_reset++;
      do_reset %= RESET_PERIOD;
    }
}


/**************************************************************************
  Title:	nok_bwp_encode

  Purpose:      Writes BWP parameters to the bit stream.

  Usage:	nok_bwp_encode (bs, num_of_sfb, status)

  Input:        bs         - bit stream
                num_of_sfb - number of scalefactor bands
                status	   - side_info:
			     1, if prediction not used in this frame
			     >1 otherwise
			   - pred_sfb_flag:
                             1 bit for each scalefactor band (sfb) where BWP 
                             can be used indicating whether BWP is switched 
                             on (1) /off (0) in that sfb.
			   - global_pred_flag:
			     1, if prediction used on some sfb's
			     0 otherwise
			   - reset_flag:
			     1, if reset has been performed
			     0 otherwise
			   - reset_group:
			     number of the group where reset was done

  Output:	-

  References:	1.) BsPutBit

  Explanation:  -

  Author(s):	Mikko Suonio, Juha Ojanperä
  *************************************************************************/

void
nok_bwp_encode (HANDLE_BSBITSTREAM bs, int num_of_sfb,
		NOK_BW_PRED_STATUS *status)
{
  int i, last_band;

  BsPutBit (bs, status->global_pred_flag, LEN_PREDICTOR_DATA_PRESENT); 
  if (status->global_pred_flag)
    {
      BsPutBit (bs, status->reset_flag, LEN_PREDICTOR_RESET);
      if (status->reset_flag)
	BsPutBit (bs, status->reset_group, LEN_PREDICTOR_RESET_INDEX);
      
      last_band = (num_of_sfb < NOK_BWP_PRED_SFB_MAX) ? num_of_sfb : NOK_BWP_PRED_SFB_MAX;

      for (i = 0; i < last_band; i++)
	BsPutBit (bs, status->pred_sfb_flag[i], LEN_PREDICTION_USED);   
    }
    
}


static void
autocorr (unsigned short x[BWP_BLN5][BWP_BLEN], double last_spec[CLEN], unsigned char pred[2][CLEN], float sb_samples_pred[CLEN], int m, int indicate)
{
  unsigned short last_spec_short[CLEN];
  float BWP_LPC_coef[BWP_LPC], tmp1, tmp2, tmp3, err;
  float y[DATA_LEN + 2];
  float r[2][2], r1[2];
  int i, j, i5, ij;
  unsigned short xtemp, temp1, temp2;

  for (i = 0; i < CLEN; i++)
    last_spec_short[i] = fs ((float)last_spec[i]);

  for (i = 0; i < BWP_BLEN; i++)
    {
      i5 = i * 4;
      ij = i5 + indicate;
      xtemp = last_spec_short[ij];

      for (j = 0; j < DATA_LEN + 1; j++)
	y[j] = sf (x[j][i]);
      y[DATA_LEN + 1] = sf (last_spec_short[ij]);

      tmp1 = y[1] * y[1];
      tmp2 = y[2] * y[2];
      tmp3 = y[3] * y[3];
      r[0][0] = (tmp1 + tmp2 + tmp3) * 2.0f * 1.005f;
      r[1][1] = (y[0] * y[0] + tmp1 + tmp2 * 2.0f + tmp3 + y[4] * y[4]) * 1.005f;
      r[0][1] = y[0] * y[1] + ((y[1] + y[3]) * y[2]) * 2.0f + y[3] * y[4];
      r1[1] = ((y[0] + y[4]) * y[2] + y[1] * y[3]) * 2.0f;

      err = r[0][0] * r[1][1] - r[0][1] * r[0][1];
      if (fabs (err) < 1.0e+1)
	{
	  BWP_LPC_coef[0] = 0.0f;
	  BWP_LPC_coef[1] = 0.0f;
	}
      else
	{
	  temp1 = fs (err);
	  temp2 = (temp1 >> 7);
	  temp1 &= 0x007F;
	  tmp1 = NOK_codebook[temp1 + 128] * NOK_codebook[temp2 + 256];
	  BWP_LPC_coef[0] = (r[1][1] - r1[1]) * r[0][1] * tmp1;
	  BWP_LPC_coef[1] = (-r[0][1] * r[0][1] + r[0][0] * r1[1]) * tmp1;
	}


      nok_quantize (BWP_LPC_coef[0], &pred[0][ij]);
      nok_quantize (BWP_LPC_coef[1], &pred[1][ij]);

      sb_samples_pred[ij] = NOK_codebook[pred[0][ij]] * y[DATA_LEN + 1] + NOK_codebook[pred[1][ij]] * y[DATA_LEN];

      ij = i5 + (indicate + 1) % NRCSTA;
      for (j = 0; j < DATA_LEN; j++)
	x[j][i] = x[j + 4][i];
      x[3][i] = last_spec_short[ij];
      sb_samples_pred[ij] = NOK_codebook[pred[0][ij]] * sf (x[3][i]) + NOK_codebook[pred[1][ij]] * sf (x[2][i]);

      ij = i5 + (indicate + 2) % NRCSTA;
      for (j = 0; j < DATA_LEN - 1; j++)
	x[4 + j][i] = x[j + 7][i];
      x[6][i] = last_spec_short[ij];
      sb_samples_pred[ij] = NOK_codebook[pred[0][ij]] * sf (x[6][i]) + NOK_codebook[pred[1][ij]] * sf (x[5][i]);

      ij = i5 + (indicate + 3) % NRCSTA;
      for (j = 0; j < DATA_LEN - 2; j++)
	x[7 + j][i] = x[j + 9][i];
      x[8][i] = last_spec_short[ij];
      sb_samples_pred[ij] = NOK_codebook[pred[0][ij]] * sf (x[8][i]) + NOK_codebook[pred[1][ij]] * sf (x[7][i]);

      x[9][i] = xtemp;
    }

}

static void
flt_round_8 (float *pf)
{
  int flg;
  unsigned long tmp, tmp1;
  float *pt = (float *) &tmp;

  *pt = *pf;			/* Write float to memory */
  tmp1 = tmp;			/* save in tmp1 */
  /* 'ul' denotes 'unsigned long' */
  flg = tmp & 0x00008000ul;	/* rounding position */
  tmp &= 0xffff0000ul;		/* truncated float */
  *pf = *pt;

  /* round 1/2 lsb towards infinity */
  if (flg)
    {
      tmp = tmp1 & 0xff810000ul;	/* 2^e + 1/2 lsb */
      *pf += *pt;		/* add 2^e + 1/2 lsb */
      tmp &= 0xff800000ul;	/* extract 2^e */
      *pf -= *pt;		/* substract 2^e */
    }
}

static unsigned short
fs (float x)
{
  unsigned short ps;
  unsigned long tmp;
  float *pf = (float *) &tmp;

  *pf = x;
  ps = tmp >> 16;
  return (ps);
}

static float
sf (unsigned short ps)
{
  float x;
  unsigned long tmp;
  float *pf = (float *) &tmp;

  tmp = ps << 16;
  x = *pf;
  return (x);
}

static void
nok_quantize (float coef, unsigned char *findex)
{
  int sig, index;
  float freq;

/*  if(coef>=3.0||coef<= -3.0) coef=0.0; */
  freq = coef * 0.992187500 / 3.0 - 0.00781250;
  if (freq >= 0)
    sig = 1;
  else
    {
      sig = 0;
      freq += 1.0;
    }
  index = (unsigned int) (freq * (float) (1L << 6));
  if (sig)
    index = index | 1 << 6;
  *findex = index;
}

