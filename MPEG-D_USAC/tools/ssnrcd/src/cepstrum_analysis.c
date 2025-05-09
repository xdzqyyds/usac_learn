
/*

====================================================================
                        Copyright Header            
====================================================================

This software module was originally developed by Ralf Funken
(Philips, Eindhoven, The Netherlands) and edited by Takehiro Moriya 
(NTT Labs. Tokyo, Japan) in the course of development of the
MPEG-2/MPEG-4 Conformance ISO/IEC 13818-4, 14496-4. This software
module is an implementation of one or more of the Audio Conformance
tools as specified by MPEG-2/MPEG-4 Conformance. ISO/IEC gives users
of the MPEG-2 AAC/MPEG-4 Audio (ISO/IEC 13818-3,7, 14496-3) free
license to this software module or modifications thereof for use in
hardware or software products. Those intending to use this software
module in hardware or software products are advised that its use may
infringe existing patents. The original developer of this software
module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Philips retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party. This copyright notice must be included in 
all copies or derivative works. Copyright 2000.


Source file: CEPSTRUM_ANALYSIS.C

*/


#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "common.h"
#include "cepstrum_analysis.h"


/***************************************************************************/

#define         PI                              (3.141592653589793F)

/***************************************************************************/

void lpc2cepstrum (int      p,          /*    in:    LPC order                          */
                   double   lpc_coef[], /*    in:    LPC coefficients (a-parameters)    */
                   int      N,          /*    in:    LPC cepstrum order                 */
                   double   C[])        /*    out:   LPC cepstrum                       */
                   
{
    double ss;
    int   i, m;
    
    /* it is assumed that lpc_coef[0] is 1 ! */
    
    C[1] = -lpc_coef[1];
    
    for (m = 2; m <= p; m++)
    {
        ss= -lpc_coef[m] * m;
        for (i = 1; i < m; i++)
        {
            ss -= lpc_coef[i] * C[m-i];
        }
        C[m] = ss;
    }
    
    for (m = p+1; m <= N; m++)
    {
        ss = 0.0F;
        for (i = 1; i<= p; i++)
        {
            ss -= lpc_coef[i] * C[m-i];
        }
        C[m] = ss;
    }
    
    for (m = 2; m <= N; m++)
    {
        C[m] /= m;
    }
}

/***************************************************************************/

void hamwdw (float    wdw[],
             int      n)
{
    int        i;
    float      d;
    
    d = (float) (2.0 * PI /n);

    for (i = 0; i < n; i++)
    {
        wdw[i] = (float) (0.54 - 0.46 * cos (d * i));
    }
}

/***************************************************************************/

void lagwdw (float    wdw[],
             int      n,
             float    h)
{
    int      i;
    float    a, b, w;
    
    a = (float) (log (0.5) * 0.5 / log (cos (0.5 * PI * h)));
    a = (float) ((int) a);
    w = 1.0F;
    b = a;
    wdw[0] = 1.0F;
    
    for (i = 1; i <= n; i++)
    {
        b += 1.0F;
        w *= a / b;
        wdw[i] = w;
        a -= 1.0F;
    }
}

/***************************************************************************/

static void sigcor (double*  sig,
                    int      n,
                    double   cor[],
                    int      p) 
{ 
    int     k, ij; 
    double  c, dsqsum;
    double  sqsum = 1.0e-35F;
    
    if (n > 0)
    {
        for (ij = 0; ij < n; ij++)
        {
            sqsum += (double)(sig[ij] * sig[ij]);
        }
        
        dsqsum = (double)(COR_FACTOR / sqsum );
       
        for (k = 1; k <= p; k++)
        { 
            c = 0.0; 
            for(ij = k; ij < n; ij++)
            {
                c += (sig[ij - k] * sig[ij]); 
            }
            cor[k] = c * dsqsum; 
        }
        k = p;
    }

    cor[0] = 1.0F;
}

/***************************************************************************/

static void corref (int      p,            /*    in:    LPC analysis order              */
                    double   cor[],        /*    in:    correlation coefficients        */
                    double   alf[])        /*    out:   linear predictive coefficients  */
             
{
    int     i, j, k;
    double  resid, r, a;
    double  ref[N_PR + 1];
    
    ref[1] = cor[1];
    alf[1] = -ref[1];
    resid  = (double) ((1.0 - ref[1]) * (1.0 + ref[1]));
    
    for (i = 2; i <= p; i++)
    {
        r = cor[i];
        
        for (j = 1; j < i; j++)
        {
            r += alf[j] * cor[i-j];
        }
        
        alf[i] = -(ref[i] = (r /= resid));

        j = 0;
        k = i;
        
        while (++j <= --k)
        {
            a = alf[j];
            alf[j] -= r * alf[k];
            
            if (j < k)
            {
                alf[k] -= r * a;
            }
        }
        
        resid = (double) (resid * (1.0 - r) * (1.0 + r)); 
    }
}

/***************************************************************************/

void calculate_lpc (double      pcm[],              /*    in:    input PCM audio data                */
                    int         segment_length,     /*    in:    analysis frame length in samples    */
                    double*     lpc_coef,          /*    out:   LPC coefficients                    */
                    float       wdw [],             /*    in:    hamming window                      */
                    float       wlag [])            /*    in:    lag window                          */

{
    int      ip;
    double   cor[N_PR + 1];
    
    for (ip = 0; ip < segment_length; ip++)
    {
        pcm [ip] *= wdw[ip];
    }
    
    sigcor (pcm, segment_length, cor, N_PR);
    
    for (ip = 1; ip <= N_PR; ip++)
    {
        cor[ip] *= (double)wlag[ip];
    }
    
    corref (N_PR, cor, lpc_coef);
}

/***************************************************************************/


