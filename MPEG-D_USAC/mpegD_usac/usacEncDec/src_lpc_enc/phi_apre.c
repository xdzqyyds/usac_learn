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
/*      SOURCE_FILE:    PHI_APRE.C                                      */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Adaptive Codebook Preselection Module           */
/*                                                                      */
/*======================================================================*/

/*======================================================================*/
/*      I N C L U D E S                                                 */
/*======================================================================*/
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <assert.h>

#include "lpc_common.h"
#include "phi_cons.h"
#include "phi_apre.h"

/*======================================================================*/
/* L O C A L     D A T A     D E C L A R A T I O N                      */
/*======================================================================*/
static float **cba_e_tbl;

/*======================================================================*/
/* Function Definition: PHI_allocate_energy_table                       */
/*======================================================================*/
void PHI_allocate_energy_table
(
    const long sbfrm_size,          /* In: Subframe size in samples     */
    const long acbk_size,           /* In: Length of Adaptive Codebook  */
    const long sac_search_step      /* In: Try every $1-th sample       */
    
)
{
    int i,j;
    int num_acbk_sbfrms = (int)(acbk_size/sbfrm_size);
    int n_energy_vals   = (1 + (int)((sbfrm_size -1)/sac_search_step));
            
    /*==============================================================*/
    /* Memory Allocation for the  energy table                      */
    /*==============================================================*/
    if ((cba_e_tbl   = (float **) malloc((unsigned int)num_acbk_sbfrms * sizeof(float *))) == NULL)
    { 
        printf("MALLOC FAILURE in init_abs_excitation_analysis \n");
        exit(1);
    }    
    for (i=0; i<(int)num_acbk_sbfrms; i++)
    {
        if ((cba_e_tbl[i]= (float *) malloc((unsigned int)n_energy_vals * sizeof(float))) == NULL)
        { 
            printf("MALLOC FAILURE in init_abs_excitation_analysis \n");
            exit(1);
        }
    }

    /*==============================================================*/
    /* Initialise the energy table                                  */
    /*==============================================================*/
    for (i = 0; i < (int)num_acbk_sbfrms;  i ++)
    {
        for (j = 0; j < (int)n_energy_vals; j ++)
        {
            cba_e_tbl[i][j] = FLT_MAX;
        }
    }
}
    
/*======================================================================*/
/* Function Definition: PHI_cba_preselection                            */
/*======================================================================*/
void PHI_cba_preselection
(
const long  nos,          /* In:   Number of samples to be processed    */
const long  max_lag,      /* In:   Maximum Permitted Adapt cbk Lag      */
const long  min_lag,      /* In:   Minimum Permitted Adapt cbk Lag      */
const long  n_sbfrm_frm,  /* In:   Number of subframes in a frame       */
const long  n_lags,       /* In:   Number of lag candidates             */
const float ca[],         /* In:   Segment from adaptive codebook       */
const float ta[],         /* In:   Backward filtered target signal      */
const float a,            /* In:   Lpc Coefficients (weighted)          */
long  pi[],               /* Out:  Result preselection: Lag candidates  */
const long  n             /* In:   Subframe Counter (we need this)      */
                          /*       For explanation :see philips proposal*/
)
{
    /*==================================================================*/
    /* Volatile variables for this subroutine                           */
    /*==================================================================*/
    float v, e_tmp, **rap;        
    int *is;                           
    int  i_tmp, j_tmp;
    int  i, j, k, l;
    int  n_sbfrm_acbk = (int)((max_lag - min_lag + 1)/nos);
    int  Lm = ((int)(1 + (int)(nos -1)/Sa));
    
    if ((int)Pa * (int)Sa != (int)n_lags)
    {
        printf("--------------------------------------------------------\n");
        printf("            NON-FATAL WARNING MESSAGE: \n");
        printf("The number of lag candidates is not a multiple of %d. \n", (int)Sa);
        printf("The consequence is that only %d candidates will be tried. \n",
        (int)(Pa*Sa));
        printf("--------------------------------------------------------\n");
    }
    /*==================================================================*/
    /* Memory Allocation for the "rap" array                            */
    /*==================================================================*/

    if ((rap   = (float **) malloc((unsigned int)n_sbfrm_acbk * sizeof(float *))) == NULL)
    { 
        printf("Malloc failure in Block:Excitation Anlysis\n");
        exit(1);
    }    
    for (i = 0; i < (int)n_sbfrm_acbk; i++)
    {
        rap[i] = (float *) malloc ((unsigned int)Lm * sizeof (float));
        if (rap[i] == NULL)
        {
            printf("\n Malloc Failure in Block: Excitation Anlaysis \n");
            exit (1);
        }
    }

    /*==================================================================*/
    /* Allocate Memory for is                                           */
    /*==================================================================*/
    if ((is = (int *)malloc((unsigned int)Pa * sizeof(int)))== NULL )
    {
       printf("\n Malloc Failure in Block:Excitation Anlaysis \n");
       exit(1);
    }
    
    /*==================================================================*/
    /* Check if energy table needs updating or not                      */
    /*==================================================================*/
    i_tmp = (n == n_sbfrm_frm / 2) ? (int)n_sbfrm_acbk : 1;

    if (i_tmp == 1)
        for (i = (int)n_sbfrm_acbk - 1; i > 0; i --)
            for (j = 0; j < Lm; j ++)
                cba_e_tbl[i][j] = cba_e_tbl[i - 1][j];

    for (i = 0; i < i_tmp; i ++)
    {
        l = (int)(max_lag - min_lag) - (i* (int)nos);
        for (j = 0; j < (int)Lm; j ++)
        {
            e_tmp = FLT_MIN;
            v = (float)0.0;

            for (k = 0; k < (int)nos; k ++)
            {
                v = v * a + ca[l + k];
                if (fabs((double)v) > 1e-17)
                    e_tmp += v * v;
                else
                    v = (float)0.0;
            }
            cba_e_tbl[i][j] = (float)1.0/e_tmp;
            l -= (int)Sa;
        }
    }

    /*==================================================================*/
    /* calculates ratios                                                */
    /*==================================================================*/

    for (i = 0; i < (int)n_sbfrm_acbk;  i ++)
    {
        l = (int)(max_lag - min_lag) - (i* (int)nos);
        for (j = 0; j < (int)Lm; j ++)
        {
            v = (float)0.0;

            for (k = 0; k < (int)nos; k ++)
                v += ca[l + k] * ta[k];
            
            if (fabs((double)v) > 1e-17)
                rap[i][j] = v * v * cba_e_tbl[i][j];
            else 
                rap[i][j] = (float)0.0;
                
            l -= (int)Sa;
        }
    }

    /*==================================================================*/
    /* selects Pa indices with largest rap's                            */
    /*==================================================================*/
    i_tmp = j_tmp = 0; 
    for (k = 0; k < (int)Pa; k ++)
    {
        v = -FLT_MAX; 
        for (i = 0; i < (int)n_sbfrm_acbk;  i ++)
        {
            for (j = 0; j < (int)Lm; j ++)
            {
                if (rap[i][j] > v)
                {
                    v = rap[i][j];
                    i_tmp = i;
                    j_tmp = j;
                }
            }
        }

        is[k] = (i_tmp * (int)nos) + (j_tmp * (int)Sa); /* lag - Lmin */
        rap[i_tmp][j_tmp] = -FLT_MAX;
    }
    
    /*==================================================================*/
    /* stores Pa*(2Sam+1) preselected neighboring candidates            */
    /*==================================================================*/
 
    for (k = 0; k < (int)Pa; k ++)
    {
        register int temp = 2*(int)Sam + 1;
        register int index = temp * k;
        
        if ((i = is[k] - (int)Sam) < 0) 
            i = 0; 
 
        for (j = 0; j < temp; j ++, index++) 
            pi[index] = (long)(i ++); 
    } 
 
    /*==================================================================*/
    /* Free the 2-d array "rap" and the array "is"                      */
    /*==================================================================*/
    for(i = 0; i <  (int)n_sbfrm_acbk; i++)
    {
        free (rap[i]);
    }
    free (rap);
    free (is);
    
    return; 
} /* cba_preselection() */


/*======================================================================*/
/* Function Definition: PHI_free_energy_table                           */
/*======================================================================*/
void 
PHI_free_energy_table
(
    const long sbfrm_size,          /* In: Subframe size in samples     */
    const long acbk_size            /* In: Length of Adaptive Codebook  */
)
{
    int i;
    int num_acbk_sbfrms = (int)(acbk_size/sbfrm_size);

    for(i = 0; i <  (int)num_acbk_sbfrms; i++)
    {
        free (cba_e_tbl[i]);
    }
    free (cba_e_tbl);
}

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 17-04-96 R. Taori  Initial Version                                   */
