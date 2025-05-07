/*
This software module was originally developed by
Hironori Ito (NEC Corporation)
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
 *  MPEG-4 Audio Verification Model (LPC-ABS Core)
 *  
 *  Confort Noise Generator(CNG) Modules
 *
 *  Ver1.0  99.5.10 H.Ito(NEC)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "lpc_common.h"
#include "nec_abs_const.h"
#include "nec_exc_proto.h"
#include "nec_vad.h"

#define MIN_SBFRM_SIZE  40
#define CNG_NUM_PULSE   10
#define ALPHA       ((float)4915/8192)

/******************************************************************************/

short Random(short *seed)
{
    *seed = (short)( (int)(*seed) * 31821 + 13849 );
    return( *seed );
}

/******************************************************************************/

float Gauss(short *seed)
{
    int i;
    float temp;

    temp = 0.0;
    for(i=0;i<12;i++) temp+=(float)Random(seed);
    temp /= (2 * 32768);
    return(temp);
}

/******************************************************************************/

float Randacbg(short *seed)
{
    return((float)((float)Random(seed)/(float)32768));
}

/******************************************************************************/

void setRandomBits(unsigned long *l, int n, short *seed)
{

    *l = 0xffff & Random(seed);     /* unsigned short range */

    /* put bits into upper part of long int in case of n > 16 */
    if( n > 16 ) *l |= (0xffff & Random(seed)) << 16;

    /* get n bit random index */
    if( n < 32 ) *l &= ((unsigned long)1 << n) - 1;
}

/******************************************************************************/

void rms_smooth_decision(float qxnorm, float SID_rms, long SampleRateMode,
             long frame_size, short prev_tx, short *sm_off_flag){

    short n10msec = 0;
    short sm_off_len;
    static short sm_off_count_nb = 0;
    static short sm_off_count_wb = 0;
    short *sm_off_count = 0;

    if(SampleRateMode == 0){ /* 8kHz sampling */
    n10msec = (short)((float)frame_size / 80.0);
    sm_off_count = &sm_off_count_nb;
    }else if(SampleRateMode == 1){ /* 16kHz sampling */
    n10msec = (short)((float)frame_size / 160.0);
    sm_off_count = &sm_off_count_wb;
    }   
    sm_off_len = (short)((float)RMS_SMOOTH_OFF_LEN/(float)n10msec);

    /* Smoothing OFF in the first sm_off_len of unvoice period */
    if(prev_tx == 1 && sm_off_len != 0){
    *sm_off_flag = 1;
    *sm_off_count = 1;
    return;
    }else if(*sm_off_count < sm_off_len){
    *sm_off_flag = 1;
    (*sm_off_count)++;
    }else if(*sm_off_count == sm_off_len){
    *sm_off_flag = 0;
    *sm_off_count = sm_off_len+1;
    }

    /* Smoothing OFF if RMS changes by more than RMS_CHANGE dB */
    if( fabs(20*log10(qxnorm/SID_rms)) < RMS_CHANGE) *sm_off_flag = 0;
}

/******************************************************************************/

static void mk_rpexc (long  shape_index, 
                      int   Np,
                      int   D,
                      float gf,
                      long  sbfrm_size,
                      float *rpexc)
{
    int rpe_index;
    int rpe_phase;
    int rpe_amps [6];
    int n;

    /*  Decode amplitudes and phases   */

    rpe_index = shape_index;
    rpe_phase = rpe_index % D;
    rpe_index = rpe_index / D;

    for (n = Np - 1; n >= 0; n--)
    {
        rpe_amps [n] = (rpe_index % 3) - 1;
        rpe_index = rpe_index / 3;
    }

    /*   Set subframe sample values   */

    for (n = 0; n < sbfrm_size; n++)
    {
        rpexc [n] = 0.0F;
    }

    for (n = 0; n < Np; n++)
    {
        rpexc [rpe_phase + D*n] = gf * (float) (rpe_amps [n]);
    }
}

/******************************************************************************/

static void mk_mpexc(long pos_idx, long sgn_idx, long num_pulse, float *mpexc,
                     long sbfrm_size)
{
    long i, j, k;
    long *pul_loc;
    long *bit_pos, *num_pos, *chn_pos;
    float *tmp_exc, *sgn;

    /* Memory Allocation */
    if((bit_pos = (long *)calloc (num_pulse, sizeof(long)))==NULL) {
    printf("\n Memory allocation error in nec_cng_module \n");
    exit(1);
    }
    if((num_pos = (long *)calloc (num_pulse, sizeof(long)))==NULL) {
    printf("\n Memory allocation error in nec_cng_module \n");
    exit(1);
    }
    if((chn_pos = (long *)calloc (num_pulse*sbfrm_size, sizeof(long)))==NULL) {
    printf("\n Memory allocation error in nec_cng_module \n");
    exit(1);
    }
    if((tmp_exc = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
    printf("\n Memory allocation error in nec_cng_module \n");
    exit(1);
    }
    if((sgn = (float *)calloc (num_pulse, sizeof(float)))==NULL) {
    printf("\n Memory allocation error in nec_cng_module \n");
    exit(1);
    }
    if((pul_loc = (long *)calloc (num_pulse, sizeof(long)))==NULL) {
    printf("\n Memory allocation error in nec_cng_module \n");
    exit(1);
    }
    
    nec_mp_position(sbfrm_size, num_pulse, bit_pos, chn_pos);
    for ( i = 0; i < num_pulse; i++ ) num_pos[i] = 1 << bit_pos[i];
    
    for ( i = num_pulse-1, k = 0; i >= 0; i-- ) {
    pul_loc[i] = 0;
    for ( j = 0; j < bit_pos[i]; j++ ) {
        pul_loc[i] |= ((pos_idx>>k)&0x1)<<j;
        k++;
    }
    sgn[i] = 1.0;
    if ( (sgn_idx&0x1) == 1 ) sgn[i] = -1.0;
    sgn_idx = sgn_idx >> 1;
    pul_loc[i] = chn_pos[i*sbfrm_size+pul_loc[i]];
    }
    
    for (i = 0; i < sbfrm_size; i++)
    tmp_exc[i] = 0.0;
    
    for (i = 0; i < num_pulse; i++) {
    tmp_exc[pul_loc[i]] = sgn[i];
    }

    for (i = 0; i < sbfrm_size; i++) mpexc[i] = tmp_exc[i];

    free( bit_pos );
    free( num_pos );
    free( chn_pos );
    free( pul_loc );
    free( tmp_exc );
    free( sgn );
}

/******************************************************************************/

void mk_cng (float *fcb_cng, 
             float *g_ec, 
             float *excg, 
             float *g_gc,
             float *alpha, 
             short lpc_order,
             float rms,
             short sbfrm_size, 
             short *seed, 
             short sm_off_flag,
             float *prev_G, 
             long  ExcitationMode,
             long  Configuration)
{
    long  i;
    long  mp_pos_idx, mp_sgn_idx, posbit, num_pulse;
    float *par, parpar0;
    float G, K, Ef, E1, E2, E3, tmp;
    long  shape_index;
    int   index_size;
    int   Np [4] = {5, 5, 6, 6};
    int   D [4]  = {8, 8, 5, 4};
    float gf [4] = {(float)14188/8192,
                    (float)14188/8192,
                    (float)11217/8192,
                    (float)10033/8192};

    
    /* Memory Allocation */
    if((par = (float *)calloc (lpc_order, sizeof(float)))==NULL) 
    {
        printf("\n Memory allocation error in nec_cng_module \n");
        exit(1);
    }
    
    /* Make Gaussian Excitation */
    for(i=0;i<sbfrm_size;i++) excg[i] = Gauss(seed);
    
    num_pulse = CNG_NUM_PULSE;
    posbit = 20; /* 10pulse/40sbfrm */
    
    if (ExcitationMode == RegularPulseExc)
    {   

        if ((Configuration == 0) || (Configuration == 1))
        {
            index_size = 11;
        }
        else
        {
            index_size = 12;
        }
          
        /*  Set random RPE shape index   */
        setRandomBits ((unsigned long *)&shape_index, index_size, seed);

        /* Make Regular-Pulse Excitation */
        mk_rpexc (shape_index, 
                  Np[Configuration], 
                  D[Configuration], 
                  gf[Configuration],
                  sbfrm_size, 
                  fcb_cng); 
        
    }
    else
    {
        for(i=0; i<sbfrm_size/MIN_SBFRM_SIZE; i++)
        {
            
            /* Set Pulse Position Index */
            setRandomBits((unsigned long *)&mp_pos_idx, posbit, seed);
            
            /* Set Pulse Sign Index */
            setRandomBits((unsigned long *)&mp_sgn_idx, num_pulse, seed);
            
            /* Make Multi-Pulse Excitation */
            mk_mpexc(mp_pos_idx, mp_sgn_idx, num_pulse, &fcb_cng[i*MIN_SBFRM_SIZE], MIN_SBFRM_SIZE);
        }
    }
    
    nec_lpc2par(alpha, par, lpc_order);
    parpar0 = 1.0;
    
    for (i = 0; i < lpc_order; i++)
    {
        parpar0 *= (1 - par[i] * par[i]);
    }
    
    parpar0 = (float)((parpar0 > 0.0) ? sqrt(parpar0) : 0.0);
    
    G = (float)(rms*parpar0*0.8);
    
    /* Gain Smoothing */
    if(sm_off_flag != 1)
    {
        G = RMS_SM_COEFF*(*prev_G) + (1-RMS_SM_COEFF)*G;
    }
    *prev_G = G;
    
    /* Calculate Multi-pulse and Gaussian Excitation Gains */
    K = sbfrm_size*G*G;
    Ef = (float)(num_pulse*sbfrm_size/MIN_SBFRM_SIZE);
    *g_ec = (float)sqrt(K/Ef);
    
    E1 = 0.0; E2 = 0.0; E3 = 0.0;
    
    for (i = 0; i < sbfrm_size; i++)
    {
        tmp = (*g_ec)*fcb_cng[i];
        E1 += tmp*tmp;
        E2 += excg[i]*excg[i];
        E3 += tmp*excg[i];
    }
    
    if(E2 == 0.0)
    {
        fprintf(stderr, "ERROR in nec_cng_module.c\n");
        exit(0);
    }
    else
    {
        *g_gc = (float)((-ALPHA*E3 + sqrt(ALPHA*ALPHA*E3*E3+(1-ALPHA*ALPHA)*E1*E2))/E2);
    }
    
    if( *g_gc < 0.0 )
    {
        *g_gc = 0.0;
    }
    else
    {
        *g_ec *= ALPHA;
    }
    free(par);
}

/******************************************************************************/
