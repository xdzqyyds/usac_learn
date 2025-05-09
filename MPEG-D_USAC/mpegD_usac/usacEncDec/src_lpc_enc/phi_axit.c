/*====================================================================*/
/*         MPEG-4 Audio (ISO/IEC 14496-3) Copyright Header            */
/*====================================================================*/
/*
This software module was originally developed by Rakesh Taori and Andy
Gerrits (Philips Research Laboratories, Eindhoven, The Netherlands) in
the course of development of the MPEG-4 Audio (ISO/IEC 14496-3). This
software module is an implementation of a part of one or more MPEG-4
Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
(ISO/IEC 14496-3). ISO/IEC gives users of the MPEG-4 Audio (ISO/IEC
14496-3) free license to this software module or modifications thereof
for use in hardware or software products claiming conformance to the
MPEG-4 Audio (ISO/IEC 14496-3). Those intending to use this software
module in hardware or software products are advised that its use may
infringe existing patents. The original developer of this software
module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not
released for non MPEG-4 Audio (ISO/IEC 14496-3) conforming products.
CN1 retains full right to use the code for his/her own purpose, assign
or donate the code to a third party and to inhibit third parties from
using the code for non MPEG-4 Audio (ISO/IEC 14496-3) conforming
products.  This copyright notice must be included in all copies or
derivative works. Copyright 1996.
*/
/*====================================================================*/
/*======================================================================*/
/*                                                                      */
/*      SOURCE_FILE:    PHI_AXIT.C                                      */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Excitation Analysis Modules                     */
/*                                                                      */
/*======================================================================*/

/*======================================================================*/
/*      I N C L U D E S                                                 */
/*======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <assert.h>

#include "lpc_common.h"               /* Prototype definitions          */      
#include "phi_cons.h"                 /* Prototype definitions          */      
#include "phi_apre.h"                 /* Prototype definitions          */      
#include "phi_xits.h"                 /* Prototype definitions          */      
#include "phi_axit.h"                 /* Prototype definitions          */      

/*======================================================================*/
/*       L O C A L    D A T A   D E C L A R A T I O N                   */
/*======================================================================*/
static float *PHI_s_state;     /* Synthesis filter states during search */
static float *PHI_Wden_states; /* Synthesis filter states for decoder   */
static float *PHI_cba_codebook;/* Adaptive Codebook                     */
static float PHI_afp = (float)0.0;/* First-order LPC parameter(previous)*/
static long  PHI_sbfrm_ctr = 0;/* Local counter: current subframe       */ 
static long  PHI_D;
static long  PHI_Np;
static long  PHI_Nz;
static long  PHI_Nf;

/*======================================================================*/
/* Function Definition: PHI_init_excitation_analysis                    */
/*======================================================================*/
void 
PHI_init_excitation_analysis
(
const long max_lag,   /* In:Maximum permitted lag in the adaptive cbk */ 
const long lpc_order, /* In:The LPC order                             */
const long sbfrm_size,/* In:Size of subframe in samples               */
const long RPE_configuration /* In:Confguration                        */
)
{
    int i;
 
    /* -----------------------------------------------------------------*/
    /* Allocate memory for the local-decoder synthesis filter-states    */
    /* -----------------------------------------------------------------*/
    if(( PHI_s_state = (float *)malloc((unsigned int)lpc_order * sizeof(float))) == NULL )
    {
        printf("MALLOC FAILURE in init_abs_excitation_analysis \n");
        exit(1);
    }
    
    /* -----------------------------------------------------------------*/
    /* Allocate memory for the encoder perceptual wighting filter-states*/
    /* -----------------------------------------------------------------*/
    if(( PHI_Wden_states = (float *)malloc((unsigned int)lpc_order * sizeof(float))) == NULL )
    {
        printf("MALLOC FAILURE in init_abs_excitation_analysis \n");
        exit(1);
    }
    
    /* -----------------------------------------------------------------*/
    /* Allocate memory for the adaptive Codebook                        */
    /* -----------------------------------------------------------------*/
    if(( PHI_cba_codebook = (float *)malloc((unsigned int)max_lag * sizeof(float))) == NULL )
    {
        printf("MALLOC FAILURE in init_abs_excitation_analysis \n");
        exit(1);
    }
    
    /* -----------------------------------------------------------------*/
    /* Initialise the adaptive codebook                                 */
    /* -----------------------------------------------------------------*/
    for (i = 0; i < (int)max_lag; i ++)
    {
        PHI_cba_codebook[i] = (float)0.0;
    }

    /* -----------------------------------------------------------------*/
    /* Initialise the filter states                                     */
    /* -----------------------------------------------------------------*/
    for (i = 0; i < (int)lpc_order; i ++)
    {
        PHI_s_state[i]  = PHI_Wden_states[i] = (float)0.0;
    }
    
    /* -----------------------------------------------------------------*/
    /* Set the excitation Parameters                                    */
    /* -----------------------------------------------------------------*/
    
    PHI_D = 1;
    if ((RPE_configuration == 0) || (RPE_configuration == 1))
    {
         PHI_D = 8;
    }

    if (RPE_configuration == 2)
    {
         PHI_D = 5;
    }

    if (RPE_configuration == 3)
    {
         PHI_D = 4;
    }
    
    PHI_Np = sbfrm_size/PHI_D;
    PHI_Nz = PHI_Np - 4;
    PHI_Nf = (long)1 << (PHI_Np - PHI_Nz);
    if (PHI_Nf != 16)
    {
         fprintf(stderr, "ERROR:Incorrect parameters in excitation analysis \n");
         exit(1);
    }
}

/*======================================================================*/
/* Function Definition: reset_cba_codebook                              */
/*======================================================================*/
void reset_cba_codebook (long max_lag)
{
    int i;

    for (i = 0; i < (int)max_lag; i++)
    {
        PHI_cba_codebook [i] = 0.0F;
    }
}


/*======================================================================*/
/* Function Definition: celp_excitation_analysis                        */
/*======================================================================*/
void celp_excitation_analysis
(
                                  /* -----------------------------------*/
                                  /* INPUT PARAMETERS                   */
                                  /* -----------------------------------*/
float lpc_residual[],             /* Inverse Filtered Signal            */
float int_Qlpc_coefficients[],    /* Interpolated LPC Coeffs            */
long  lpc_order,                  /* Order of LPC                       */        
float first_order_lpc_par,        /* apar corresponding to 1st-order fit*/
long  lag_candidates[],           /* Array of Lag candidates            */
long  n_lag_candidates,           /* Number of lag candidates           */
long  sbfrm_size,                 /* Number of samples in the subframe  */
long  n_subframes,                /* Number of subframes                */
                                  /* -----------------------------------*/
                                  /* OUTPUT PARAMETERS                  */
                                  /* -----------------------------------*/
long  shape_indices[],            /* Adaptive and Fixed codebook lags   */
long  gain_indices[],             /* Adaptive and Fixed codebook gains  */
long  num_shape_cbks,             /* Number of shape codebooks          */
long  num_gain_cbks,              /* Number of gain codebooks           */
float decoded_excitation[]        /* Synthesised Signal                 */
)
{
    /*==================================================================*/
    /*  Volatile Variables for this subroutine                          */
    /*==================================================================*/
    long  *amp = 0;     /* Amplitude Array for the RPE sequence         */
    long  *pos = 0;     /* Position of fixed amplitude in RPE codebook  */
    float *h;           /* Array for the impulse response  of 1/A(z)    */
    float *t = 0;       /* Array for the cbk-search target signal       */
    float *e = 0;
    float *vtmp = 0;    /* Temporary Array (Used duing Backward Filt    */
    float *scratch = 0; /* Temporary Array (Used duing Backward Filt    */
    long  **fixed_cbk;  /* 2-d Array for generating the fixed codebook  */
    float cba_gain;     /* Adaptive Codebook Gain                       */
    float cbf_gain;     /* Fixed Codebook  Gain                         */
    long  cbf_pi[FCBK_PRESELECT_VECS];   /* Array for pre-selected fixed cbk candidates  */
    long  phase;        /* Phase of the RPE codebook                    */
    int   i;            /* Local indices                                */
    
    /*==================================================================*/
    /* Memory allocation for temporary arrays                           */
    /*==================================================================*/
    if
    (  
    (( h = (float *)malloc((unsigned int)sbfrm_size * sizeof(float)))    == NULL ) ||
    (( t = (float *)malloc((unsigned int)sbfrm_size * sizeof(float)))    == NULL ) || 
    (( e = (float *)malloc((unsigned int)sbfrm_size * sizeof(float)))    == NULL ) ||
    (( amp  = (long *)malloc((unsigned int)PHI_Np * sizeof(long)))       == NULL ) ||
    (( pos  = (long *)malloc((unsigned int)PHI_Np * sizeof(long)))       == NULL ) ||
    (( scratch = (float *)malloc((unsigned int)sbfrm_size * sizeof(float)))== NULL ) ||
    (( vtmp = (float *)malloc((unsigned int)sbfrm_size * sizeof(float))) == NULL ) 
    ) 
    {
       printf("ERROR: Malloc Failure in Block: Excitation Anlaysis \n");
       exit(1);
    }

    if ((fixed_cbk = (long **) malloc((unsigned int)PHI_Nf * sizeof(long *))) == NULL)
    { 
        printf("Malloc failure in Block:Excitation Anlysis\n");
        exit(1);
    }    
    for (i = 0; i < (int)PHI_Nf; i++)
    {
        fixed_cbk[i] = (long *) malloc ((unsigned int)sbfrm_size * sizeof (long));
        if (fixed_cbk[i] == NULL)
        {
            printf("ERROR: Malloc Failure in Block: Excitation Anlaysis \n");
            exit (1);
        }
    }
    
    /*==================================================================*/
    /* Make sure the number of shape and gain codebooks is correct      */
    /*==================================================================*/
    if (num_shape_cbks != 2)
    { 
        fprintf(stderr, "Wrong number of shape codebooks in Block: Excitation Anlysis\n");
        exit(1);
    }    
    if (num_gain_cbks != 2)
    { 
        fprintf(stderr, "Wrong number of gain codebooks in Block: Excitation Anlysis\n");
        exit(1);
    }    

    /*==================================================================*/
    /* We need to know which subframe we are in                         */
    /*==================================================================*/
    if (PHI_sbfrm_ctr % n_subframes == 0)
    {
        PHI_sbfrm_ctr = 0;
    }

    /*==================================================================*/
    /*  2.7 perceptual weighting filter                                 */
    /*==================================================================*/
    PHI_perceptual_weighting(sbfrm_size, lpc_residual, scratch, lpc_order, int_Qlpc_coefficients, PHI_Wden_states);
    
    
    /*==================================================================*/
    /*   2.9 zero-input response, weighted target signal and impulse    */
    /*       response calculator                                        */
    /*==================================================================*/
    PHI_calc_impulse_response(sbfrm_size, h, lpc_order, int_Qlpc_coefficients);
    PHI_calc_zero_input_response(sbfrm_size, vtmp, lpc_order, int_Qlpc_coefficients, PHI_s_state);
    PHI_calc_weighted_target(sbfrm_size, scratch, vtmp, t);
    
    
    if (PHI_sbfrm_ctr == n_subframes/2)
    {
       PHI_afp  = first_order_lpc_par;
    }
    PHI_afp  = first_order_lpc_par;

    /*==============================================================*/
    /*  2.10 adaptive codebook search - preselection                */
    /*==============================================================*/

    PHI_backward_filtering(sbfrm_size, t, vtmp, h);

    PHI_cba_preselection(sbfrm_size, Lmax, Lmin, n_subframes,
    n_lag_candidates, PHI_cba_codebook, vtmp, PHI_afp, lag_candidates, PHI_sbfrm_ctr);
    
    /*==================================================================*/
    /* 2.11 adaptive codebook search                                    */
    /*==================================================================*/
    PHI_cba_search(sbfrm_size, (long)Lmax, (long)Lmin, PHI_cba_codebook, lag_candidates, n_lag_candidates, h, t, &cba_gain,
    &shape_indices[0], &gain_indices[0]);
              
    PHI_calc_cba_excitation(sbfrm_size, Lmax, Lmin, PHI_cba_codebook, *shape_indices, scratch);
    
    PHI_calc_cba_residual(sbfrm_size, scratch, cba_gain, h, t, e);
    
    /*==================================================================*/
    /*          2.12 fixed codebook search - preselection               */
    /*      REDUCED COMPLEXITY FIXED CODEBOOK PRESELECTION              */ 
    /*==================================================================*/
    
    PHI_backward_filtering(sbfrm_size, e, vtmp, h);
    
    PHI_calc_cbf_phase(PHI_D,  sbfrm_size, vtmp, &phase);
    
    PHI_CompAmpArray(PHI_Np, PHI_D, vtmp, phase, amp);
    
    PHI_CompPosArray(PHI_Np, PHI_D, PHI_Nz, vtmp, phase, pos);
   
    PHI_generate_cbf(PHI_Np, PHI_D, PHI_Nf, sbfrm_size, fixed_cbk, phase, amp, pos);
    
    PHI_cbf_preselection((long)PHI_D, (long)FCBK_PRESELECT_VECS, PHI_Nf, sbfrm_size, fixed_cbk, phase, vtmp,
    PHI_afp, cbf_pi);

    /*==================================================================*/
    /* 2.13 fixed codebook search                                       */
    /*==================================================================*/

    PHI_cbf_search(PHI_Np, (long)PHI_D, (long)FCBK_PRESELECT_VECS, sbfrm_size, fixed_cbk, phase, cbf_pi, h, e, &cbf_gain,
    &gain_indices[1], amp, PHI_sbfrm_ctr);

    /*==================================================================*/
    /* 2.13 Code RPE index and phase into fixed_cbk_index               */
    /*==================================================================*/
    PHI_code_cbf_amplitude_phase(PHI_Np, (long)PHI_D, amp, phase, &shape_indices[1]);

    /*==================================================================*/
    /*   2.14 simulation of decoder                                     */
    /*==================================================================*/

    PHI_calc_cbf_excitation(sbfrm_size, PHI_Np, (long)PHI_D, amp, phase, vtmp);
    
    PHI_sum_excitations(sbfrm_size, cba_gain, scratch, cbf_gain, vtmp, decoded_excitation);
    
    PHI_update_cba_memory(sbfrm_size, Lmax, PHI_cba_codebook, decoded_excitation);
    
    PHI_update_filter_states(sbfrm_size, lpc_order, decoded_excitation, PHI_s_state, int_Qlpc_coefficients);
    
    /*==================================================================*/
    /*   Update the subframe counter                                    */
    /*==================================================================*/
    
    PHI_sbfrm_ctr++;

    /*==================================================================*/
    /* Free temp states                                                 */
    /*==================================================================*/
    free(h);
    free(t);
    free(e);
    free(amp);
    free(pos);
    free(vtmp);
    free(scratch);
    for (i = 0; i < (int)PHI_Nf; i++)
    {
        free(fixed_cbk[i]);
    }
    free(fixed_cbk);

}

/*======================================================================*/
/* Function Definition: PHI_close_excitation_analysis                   */
/*======================================================================*/
void PHI_close_excitation_analysis(void)
{
    free (PHI_Wden_states);
    free (PHI_s_state);
    free (PHI_cba_codebook);
}
   
/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 17-04-96 R. Taori  Initial Version                                   */
/* 30-06-96 R. Taori  Modified interface  to meet the MPEG-4 requirement*/
/* 20-08-96 R. Taori  Modified interface to accomodate Tampere results  */
