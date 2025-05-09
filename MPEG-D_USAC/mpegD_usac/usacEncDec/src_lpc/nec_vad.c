/*
This software module was originally developed by
Souich Ohtsuka and Masahiro Serizawa (NEC Corporation)
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
*  Voice Activity Detection(VAD) Subroutines
*
*  Ver1.0  99.5.10 S. Ohtsuka and M. Serizawa (NEC)
*/

#include    <stdio.h>
#include    <stdlib.h>
#include    <math.h>

#include    "att_proto.h"
#include    "lpc_common.h"
#include    "nec_abs_const.h"
#include    "nec_vad.h"
#include    "nec_vad.tbl"

#define WB_DOWNSAMPLING

#define NEC_LPF_BUFLEN     (2*(NEC_LPF_DELAY))

/* LPC analysis window size */
#define WINSIZE_NB  200
#define WINSIZE_WB  320

#define const_p     10  /* = line spectral frequencies */
#define const_q     12  /* + 1 = autocorrelation coefficients */
#define const_N_max WINSIZE_WB
#define const_p_wb  20  /* for WB */
#define const_q_wb  22  /* for WB */
#define const_p_max const_p_wb
#define const_q_max const_q_wb

#define const_N_i       32
#define const_N_0       128

#define E15dB           ((float)12288/8192)

#define COUNTLOOP_ST        0x1000
#define COUNTLOOP_ED        0x4000

static unsigned long    vad_flags;  /* 1 bit = VAD/10ms */

static double   m_LSF[const_p_max];
static double   m_E_n, m_E_f, m_E_l, m_ZC;
static int  C_n;
static int  v_count;
static int  count;
static int  init_flag = 0;
static float    HammingWindow_DownSamplingWB[WINSIZE_NB];

/*
* initialize running agerage of LSF, Ef, El, ZC
*/
#define INITRUNAV_NOTYET    -1
#define INITRUNAV_TODO       0
#define INITRUNAV_DONE       1
static int  flag_initRunAv;

static int  initialVad( double, double, double, double );

/******************************************************************************/

void nec_vad_init (void)
{
    int i, k;
    float   *f;
    
    init_flag = 1;
    
    for( i = 0; i < const_p_max; i++ )
        m_LSF[i] = 0.0;
    
    m_E_f = 0.0;
    m_E_l = 0.0;
    m_E_n = 0.0;
    m_ZC = 0.0;
    C_n = 0;
    
    v_count = 0;
    count = 0;
    
    flag_initRunAv = INITRUNAV_NOTYET;
    
    vad_flags = 0;
    
    k = (WINSIZE_NB - NEC_PAN_NLB - NEC_PAN_NLA) / 2;
    f = HammingWindow_DownSamplingWB;
    
    for( i = 0; i < NEC_PAN_NLB + k; i++ )
    {
        *f++ = (float)( ((float)4423/8192) - ((float)3768/8192) * cos( NEC_PI * 2.0 * i 
            /((NEC_PAN_NLB + k) * 2.0 - 1.0) ) );
    }
    
    for( i = 0; i < NEC_PAN_NLA + k; i++ )
    {
        *f++ = (float)( cos( NEC_PI * 2.0 * i
            /((NEC_PAN_NLA + k) * 4.0 - 1.0) ) );
    }
}

/******************************************************************************/

int nec_vad_decision( void )
{
    int vd;
    
    if( vad_flags )
        vd = 1;
    else
        vd = 0;
    vad_flags = 0;
    
    return vd;
}

/******************************************************************************/

static int initialVad( double d_E_l, double d_E_f, double d_S, double d_ZC )
{
/*
* d_S
    */
    if( d_S > ((float)7/8192) )
        return 1;
    
        /*
        * d_S and d_ZC
    */
    if( d_S > ((float)14/8192) * d_ZC + ((float)6/8192) )
        return 1;
    
    if( d_S > ((float)-37/8192) * d_ZC + ((float)9/8192) )
        return 1;
    
        /*
        * d_E_f
    */
    if( d_E_f < ((float)-3850/8192) )
        return 1;
    
        /*
        * d_E_f and d_ZC
    */
    if( d_E_f < ((float)-20480/8192) * d_ZC - ((float)4096/8192) )
        return 1;
    
    if( d_E_f < ((float)16384/8192) * d_ZC - ((float)4915/8192) )
        return 1;
    
    if( d_E_f < ((float)20480/8192) * d_ZC - ((float)5734/8192) )
        return 1;
    
    if( d_E_f < ((float)-23838/8192) * d_ZC - ((float)3948/8192) )
        return 1;
    
        /*
        * d_E_f and d_S
    */
    if( d_E_f < 880.0 * d_S - ((float)9994/8192) )
        return 1;
    
        /*
        * d_E_l and d_S
    */
    if( d_E_l < 1400.0 * d_S - ((float)12697/8192) )
        return 1;
    
        /*
        * d_E_l and d_E_f
    */
    if( d_E_l > ((float)7610/8192) * d_E_f + ((float)933/8192) )
        return 1;
    
    if( d_E_l < ((float)-12288/8192) * d_E_f - ((float)7372/8192) )
        return 1;
    
    if( d_E_l < ((float)5849/8192) * d_E_f - ((float)1753/8192) )
        return 1;
    
    return 0;
}

#define DEBUG_YM        /* flag for halting LPC analysis : '98/11/27 */

/******************************************************************************/

static void PHI_CalcAcf (double sp_in[],         /* In:  Input segment [0..N-1]                  */
                         double acf[],           /* Out: Autocorrelation coeffs [0..P]           */
                         int N,                  /* In:  Number of samples in the segment        */ 
                         int P                   /* In:  LPC Order                               */
                         )
{
    int     i, l;
    double  *pin1, *pin2;
    double  temp;
    
    for (i = 0; i <= P; i++)
    {
        pin1 = sp_in;
        pin2 = pin1 + i;
        temp = (double)0.0;
        for (l = 0; l < N - i; l++)
            temp += *pin1++ * *pin2++;
        *acf++ = temp;
    }
    return;
}

/******************************************************************************/

static int PHI_LevinsonDurbin (double acf[],           /* In:  Autocorrelation coeffs [0..P]           */
                               double apar[],          /* Out: Polynomial coeffciients  [0..P-1]       */
                               double rfc[],           /* Out: Reflection coefficients [0..P-1]        */
                               int P,                  /* In:  LPC Order                               */
                               double * E              /* Out: Residual energy at the last stage       */
                               )
{
    int     i, j;
    double  tmp1;
    double  *tmp;
    
    if ((tmp = (double *) malloc((unsigned int)P * sizeof(double))) == NULL)
    {
        printf("\nERROR in library procedure levinson_durbin: memory allocation failed!");
        exit(1);
    }
    
    *E = acf[0];
#ifdef DEBUG_YM
    if (*E > (double)1.0)
#else
        if (*E > (double)0.0)
#endif
        {
            for (i = 0; i < P; i++)
            {
                tmp1 = (double)0.0;
                for (j = 0; j < i; j++)
                    tmp1 += tmp[j] * acf[i-j];
                rfc[i] = (acf[i+1] - tmp1) / *E;
#ifdef DEBUG_YM
                if (fabs(rfc[i]) >= 1.0 || *E <= acf[0] * 0.000001)
#else
                    if (fabs(rfc[i]) >= 1.0)
#endif
                    {
                        for (j = i; j < P; j++)
                            rfc[j] = (double)0.0;
#ifdef DEBUG_YM
                        fprintf(stderr, "phi_lpc : stop lpc at %d\n", i);
                        for (j = i; j < P; j++)
                            apar[j] = (double)0.0;
#endif
                        free(tmp);
                        return(1);  /* error in calculation */
                    }
                    apar[i] = rfc[i];
                    for (j = 0; j < i; j++)
                        apar[j] = tmp[j] - rfc[i] * tmp[i-j-1];
                    *E *= (1.0 - rfc[i] * rfc[i]);
                    for (j = 0; j <= i; j++)
                        tmp[j] = apar[j];   
            }
            free(tmp);
            return(0);
        }
        else
        {
            for(i= 0; i < P; i++)
            {
                rfc[i] = 0.0;
                apar[i] = 0.0;
            }
            free(tmp);
            return(2);      /* zero energy frame */
        }
}

/******************************************************************************/

/* for VAD */
static void nec_lpf_down2 (float xin[], float xout[], int len)
{
    int      i, j, k;
    float    *x;
    static float buf[NEC_LPF_BUFLEN];
    static int   flag = 0;
    
    
    if ( flag == 0 )
    {
        for ( i = 0; i < NEC_LPF_BUFLEN; i++ ) buf[i] = 0.0;
        flag = 1;
    }
    
    if ((x = (float *)calloc(NEC_LPF_BUFLEN+len,sizeof(float))) == NULL) 
    {
        printf("\n Memory allocation error in nec_lpf \n");
        exit(1);
    }
    
    for ( i = 0; i < NEC_LPF_BUFLEN; i++ ) 
    {
        x[i] = buf[i];
    }
    
    for ( i = 0; i < len; i++ ) 
    {
        x[NEC_LPF_BUFLEN+i] = xin[i];
    }
    
    for ( i = 0, k = 0; i < len; i+=2, k++ ) 
    {
        xout[k] = nec_lpf_coef[0] * x[NEC_LPF_DELAY+i];
        
        for ( j = 1; j <= NEC_LPF_DELAY; j++ ) 
        {
            xout[k] += ( nec_lpf_coef[j] * (x[NEC_LPF_DELAY+i-j]+x[NEC_LPF_DELAY+i+j]));
        }
    }
    
    for ( i = 0; i < NEC_LPF_BUFLEN; i++ ) buf[i] = x[len+i];
    
    free( x );
    
}

/******************************************************************************/

int nec_vad (float     PP_InputSignal[], 
             float     HamWin[],
             long      window_offset, 
             long      window_size, 
             long      SampleRateMode,
             long      ExcitationMode)

{
    static double   E_min_buffer[16];
    static double   next_E_min, E_min;
    double  sigWin[const_N_max], *ppSig;
    double  R[const_q_max+1];
    double  lpc[const_p_max];
    double  refcs[const_p_max], refc;
    double  Energy;
    int M, p, q;
    int sampleRate;
    float   *inputSignal;
    int windowSize, windowOffset;
    float   *hamWin;
    double  E_f, E_l;
    double  ZC;
    double  d_E_l, d_E_f, d_S, d_ZC;
    double  hRh;
    double  LSF[const_p_max];
    double  *LBF_corr;
    int vd = -1;
    int i, k;

  #ifdef  WB_DOWNSAMPLING
    float   buff[WINSIZE_NB];    
#endif
      
    if (!init_flag)
    {
        init_flag = 1;
        nec_vad_init();
    }
    
    sampleRate = SampleRateMode;
    inputSignal = PP_InputSignal;
    windowSize = window_size;
    windowOffset = window_offset;
    hamWin = HamWin;

    
    if (sampleRate == fs8kHz)
    {   
        /* == fs8kHz */
        p = const_p;
        q = const_q;
        LBF_corr = LBF_corr_nb;
        M = windowSize - NEC_PAN_NLB - NEC_PAN_NLA;
    }
    else
    { 
        /* == fs16kHz */
        p = const_p_wb;
        q = const_q_wb;
        LBF_corr = LBF_corr_wb;
        M = windowSize - NEC_FRQ16_NLB - NEC_FRQ16_NLA;
    }
    
    
#ifdef  WB_DOWNSAMPLING
    if( sampleRate == fs16kHz )
    {
        static float    inbuff[WINSIZE_NB*2];
           
        if (ExcitationMode == RegularPulseExc)
        { 
            /*   RPE   */   
            
            if (windowSize != (WINSIZE_NB*2))
            {
                fprintf( stderr, "VAD Error: unexpected window size\n" );
                exit( 1 );
            }  
            
            for( k = 0; k < windowSize; k++ )
            {
                inbuff[k] = inputSignal[k + windowOffset];
            }
 
        }
        else
        {
            /*   MPE   */
            
            if( windowSize != (WINSIZE_NB*2-M) || (windowOffset % M) != 0 )
            {
                fprintf( stderr, "VAD Error: unexpected window size\n" );
                exit( 1 );
            }
            
            if( windowOffset % (M * 2) == 0 )
            {
                for( k = 0; k < windowSize; k++ )
                    inbuff[k] = inputSignal[k + windowOffset];
                goto skip_vad;
            }  
            
            for( k = 0; k < M; k++ )
            {
                inbuff[k + windowSize] = inputSignal[k + windowSize - M + windowOffset];
            }

        }
        
        nec_lpf_down2( inbuff, buff, WINSIZE_NB*2 );
        
        sampleRate = fs8kHz;
        inputSignal = buff;
        windowSize = WINSIZE_NB;
        windowOffset = 0;
        p = const_p;
        q = const_q;
        LBF_corr = LBF_corr_nb;
        hamWin = HammingWindow_DownSamplingWB;
        M = windowSize - NEC_PAN_NLB - NEC_PAN_NLA;
       
    }
#endif  /* WB_DOWNSAMPLING */
    
    /* set sigWin[] */
    if( const_N_max < windowSize )
    {
        fprintf( stderr, "VAD Error: LPC analysis window is too large\n" );
        exit( 1 );
    }
    
    for( k = 0; k < windowSize; k++ )
    {
        sigWin[k] = (double)inputSignal[k + windowOffset] * hamWin[k] / 2;
    }
    
    /* set R[] */
    PHI_CalcAcf( sigWin, R, windowSize, q );
    {
        double  *lagWin;
        
        if( sampleRate == fs8kHz )
        {
            lagWin = lagWin_NB;
        }
        else
        {
            lagWin = lagWin_WB;
        }

        for( k = 0; k <= q; k++ )
        {
            R[k] *= lagWin[k];
        }
    }
    
    /* set lpc[] and refcs[] */
    PHI_LevinsonDurbin( R, lpc, refcs, p, &Energy);
    
    /* for ZC */
    for( k = 0; k < windowSize; k++ )
    {
        int tmp;
        
        tmp = (int)inputSignal[k + windowOffset];
        sigWin[k] = (double)tmp;
    }
    
    if( sampleRate == fs8kHz )
    {
        ppSig = &sigWin[windowSize - NEC_PAN_NLA - M];
    }
    else
    {
        ppSig = &sigWin[windowSize - NEC_FRQ16_NLA - M];
    }
    
    refc = -refcs[1];
    
    ++count;
    if( count == COUNTLOOP_ED )
        count = COUNTLOOP_ST;   /* avoid count overflow */
    
    {
        float   tmp_lpc[const_p_max+1];
        float   tmp_lsf[const_p_max];
        
        tmp_lpc[0] = 1.0;
        for( i = 0; i < p; i++ )
            tmp_lpc[i+1] = -(float)lpc[i];
        pc2lsf( tmp_lsf, tmp_lpc, (long)p );
        for( i = 0; i < p; i++ )
            LSF[i] = (double)tmp_lsf[i] / (2.0 * NEC_PI);
    }
    
    if( R[0] <= 0.0 )
        E_f = 0.0;
    else
        E_f = log10( R[0] / windowSize );
    
    hRh = 0.0;
    for( i = 1; i <= q; i++ )
        hRh += R[i] * LBF_corr[i];
    hRh *= 2.0;
    hRh += R[0] * LBF_corr[0];
    if( hRh <= 0.0 )
        E_l = 0.0;
    else
        E_l = log10( hRh / windowSize );
    
    ZC = 0.0;
    for( i = 0; i < M; i++ ){
        if( ppSig[i] * ppSig[i+1] < 0.0 )
            ZC += 1.0;
    }
    ZC /= M;
    
    /*
    * long-term minimum energy
    */
    if( (count & 0x0007) == 1 ){
    /*
    *  1  2  3  4  5  6  7  8
    * ^^^ 1st frame
    *
    * E_min <-- min of last const_N_0 frames
    * next_E_min <-- initial value of this set(8frames)
        */
        if( count <= const_N_0 )
            E_min = E_f;
        else{
            E_min = E_min_buffer[0];
            for( i = 1; i < 16; i++ ){
                if( E_min > E_min_buffer[i] )
                    E_min = E_min_buffer[i];
            }
        }
        next_E_min = E_f;
    }
    
    if( E_min > E_f )
        E_min = E_f;
    if( next_E_min > E_f )
        next_E_min = E_f;
    
    if( (count & 0x0007) == 0 ){
    /*
    *  1  2  3  4  5  6  7  8
    *                      ^^^ last frame
    *
    * E_min_buffer[] <-- add min of this set(8frames)
        */
        if( count <= const_N_0 )
            E_min_buffer[count / 8 - 1] = next_E_min;
        else{
            for( i = 0; i < 15; i++ )
                E_min_buffer[i] = E_min_buffer[i+1]; 
            E_min_buffer[i] = next_E_min;
        }
    }
    
    if( flag_initRunAv == INITRUNAV_NOTYET ){
        if( E_f < E15dB )
            vd = 0;
        else{
            ++v_count;
            vd = 1;
            m_E_n += E_f;
            m_ZC += ZC;
            for( i = 0; i < p; i++ )
                m_LSF[i] += LSF[i];
        }
        if( count == const_N_i )
            flag_initRunAv = INITRUNAV_TODO;
    }
    
    if( flag_initRunAv == INITRUNAV_TODO ){
        if( v_count > 0 ){
            m_E_n /= v_count;
            m_ZC /= v_count;
            for( i = 0; i < p; i++ )
                m_LSF[i] /= v_count;
        }
        
        m_E_f = m_E_n - 1.0;
        m_E_l = m_E_n - ((float)9830/8192);
        flag_initRunAv = INITRUNAV_DONE;
    }
    
    if( flag_initRunAv == INITRUNAV_DONE ){
        d_S = 0.0;
        for( i = 0; i < p; i++ ){
            double  d;
            
            d = LSF[i] - m_LSF[i];
            d_S += d * d;
        }
        d_E_f = m_E_f - E_f;
        d_E_l = m_E_l - E_l;
        d_ZC = m_ZC - ZC;
        
        if( E_f < E15dB )
            vd = 0;
        else
            vd = initialVad( d_E_l, d_E_f, d_S, d_ZC );
        
        if( d_E_f > ((float)-2457/8192) && refc < ((float)6144/8192) && d_S < ((float)20/8192) ){
            double  Be_Ef, Be_El, Be_ZC, Be_LSF;
            
            ++C_n;
            i = C_n - 20;
            if( i < 0 )
                i = 0;
            else{
                i = i / 10 + 1;
                if( i > 5 )
                    i = 5;
            }
            Be_Ef = Be_Ef_tbl[i];
            Be_El = Be_El_tbl[i];
            Be_ZC = Be_ZC_tbl[i];
            Be_LSF = Be_LSF_tbl[i];
            
            m_E_f = Be_Ef * m_E_f + ( 1.0 - Be_Ef ) * E_f;
            m_E_l = Be_El * m_E_l + ( 1.0 - Be_El ) * E_l;
            m_ZC = Be_ZC * m_ZC + ( 1.0 - Be_ZC ) * ZC;
            for( i = 0; i < p; i++ )
                m_LSF[i] = Be_LSF * m_LSF[i]
                + ( 1.0 - Be_LSF ) * LSF[i];
        }
        if( count > const_N_0 ){
          if((m_E_f < E_min && d_S < ((float)20/8192))
                || m_E_f > E_min + 1.0 ){
                m_E_f = E_min;
                C_n = 0;
            }
        }
    }
    
    vad_flags <<= 1;
    if( vd )
        vad_flags |=1;
    
skip_vad:
    
    return vd;
}


/******************************************************************************/
