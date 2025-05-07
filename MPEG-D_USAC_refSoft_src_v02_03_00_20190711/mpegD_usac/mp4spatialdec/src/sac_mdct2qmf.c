/*******************************************************************************
This software module was originally developed by

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips

and edited by

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
*******************************************************************************/









#include <math.h>

#include "sac_dec.h"
#include "sac_mdct2qmf.h"
#include "sac_mdct2qmf_wf_tables.h"
#include "sac_sbrconst.h"









static void                 LOCAL_zero         ( int const L, float * const b );
static void                 LOCAL_one          ( int const L, float * const b );
static void                 LOCAL_sin          ( int const T, int const P, int const L, float * const b );
static void                 LOCAL_imdet        ( float * const x, float * const w, int const L, float * const zReal, float * const zImag );
static void                 LOCAL_fold_out     ( float * const S, int const Lv, float * const w, int const Lw, float * const Vmain, float * const Vslave );
static void                 LOCAL_hybcmdct2qmf ( float * const Vmain, float * const Vslave, int const Lv, float * const w, int const Lw, float Z_real[ ][ 2 * MAX_TIME_SLOTS ], float Z_imag[ ][ 2 * MAX_TIME_SLOTS ] );
static void                 LOCAL_freq_window  ( int const L, float * const b );
static SPACE_MDCT2QMF_ERROR LOCAL_MDCT_windows ( int updQMF, BLOCKTYPE const windowType, float * const wf, float * const wt );

extern Twiddle *twiddleExtern;


SPACE_MDCT2QMF_ERROR SPACE_MDCT2QMF_Create(spatialDec* self)
{
    SPACE_MDCT2QMF_ERROR errorCode = SPACE_MDCT2QMF_ERROR_NO_ERROR;

    int i, j, k;

    for ( i = 0; i < MAX_RESIDUAL_CHANNELS; i++)
    {
        for ( j = 0; j < 2 * MAX_TIME_SLOTS; j++ )
        {
            for ( k = 0; k < MAX_NUM_QMF_BANDS; k++ )
            {
                self->qmfResidualReal[i][j][k] = 0.0f;
                self->qmfResidualImag[i][j][k] = 0.0f;
            }
        }
    }

    return errorCode;
}



SPACE_MDCT2QMF_ERROR SPACE_MDCT2QMF_Process
(
    int                      qmfBands,
    int                      updQMF,
    float        *   const   mdctIn,
    float                    qmfReal[ ][ MAX_NUM_QMF_BANDS ],
    float                    qmfImag[ ][ MAX_NUM_QMF_BANDS ],
    BLOCKTYPE        const   windowType,
    int                      qmfGlobalOffset
)
{
    SPACE_MDCT2QMF_ERROR errorCode = SPACE_MDCT2QMF_ERROR_NO_ERROR;

    int i;
    int j;
    int k;

    int L                 = 2 * updQMF;

    int N = 0;

    float wf[ 2 * MAX_TIME_SLOTS ];
    float wt[ 2 * MAX_TIME_SLOTS ];

    float V1[ MDCT_LENGTH_HI ];
    float V2[ MDCT_LENGTH_HI ];

    float Z1Real[ MAX_NUM_QMF_BANDS ][ 2 * MAX_TIME_SLOTS ];
    float Z1Imag[ MAX_NUM_QMF_BANDS ][ 2 * MAX_TIME_SLOTS ];

    float twipostReal[ MAX_NUM_QMF_BANDS ];
    float twipostImag[ MAX_NUM_QMF_BANDS ];

    float arg;

    int   windowOffset = 0;

    int   mdctOffset   = 0;
    int   mdctShift    = AAC_SHORT_FRAME_LENGTH;

    int   qmfOffset    = 0;
    int   qmfShift     = 0;

    int   nWindows     = 0;

    int   mdctLength = MDCT_LENGTH_LO;

    float mdctSF[ MDCT_LENGTH_SF ];             


    
    errorCode = LOCAL_MDCT_windows( updQMF, windowType, &wf[ 0 ], &wt[ 0 ] );

    switch( windowType )
    {
    case ONLYLONG_WINDOW:                                          
    case LONGSTART_WINDOW:                                        
    case LONGSTOP_WINDOW:                                         

        
        N = updQMF * qmfBands - MDCT_LENGTH_LO;

        if ( N > 0 )
        {
            LOCAL_zero( N, &mdctIn[ MDCT_LENGTH_LO ] );
        }

        mdctLength += N;

        
        LOCAL_fold_out( mdctIn, mdctLength, &wf[ 0 ], L, &V1[ 0 ], &V2[ 0 ] );
        LOCAL_hybcmdct2qmf( &V1[ 0 ], &V2[ 0 ], mdctLength, &wt[ 0 ], L, Z1Real, Z1Imag );

        
        for ( i = 0; i < qmfBands; i++ )
        {
#ifdef PARTIALLY_COMPLEX
            arg = ( 2.0f * i + 1.0f ) * (float) PI / 4.0f;
#else
            arg = ( 258.0f * i + 385.0f )* (float) PI / 256.0f;
#endif
            twipostReal[ i ]  =  (float) cos( arg );
            twipostImag[ i ]  = -(float) sin( arg );
        } 

        
        for ( i = 0; i < qmfBands; i++ )
        {
            for ( j = 0; j < L; j++ )
            {
                qmfReal[ j + qmfGlobalOffset ][ i ] += twipostReal[ i ] * Z1Real[ i ][ j ] - twipostImag[ i ] * Z1Imag[ i ][ j ];
                qmfImag[ j + qmfGlobalOffset ][ i ] += twipostReal[ i ] * Z1Imag[ i ][ j ] + twipostImag[ i ] * Z1Real[ i ][ j ];
            }
        }
        break;
    case EIGHTSHORT_WINDOW:                                             
        
        switch( updQMF )
        {
        case UPD_QMF_15:
            L = 4;
            mdctLength = AAC_SHORT_FRAME_LENGTH;
            qmfOffset = 6;
            qmfShift  = 2;
            nWindows  = 7;
            break;
        case UPD_QMF_16:
        case UPD_QMF_32:
            N = ( updQMF - UPD_QMF_16 ) * 8;
            mdctLength = AAC_SHORT_FRAME_LENGTH + N;
            L = 2 * updQMF / 8;
            qmfOffset = 7 * updQMF / 16;
            qmfShift  = updQMF / 8;         
            nWindows  = 8;
            break;
        case UPD_QMF_18:
            L = 4;
            mdctLength = AAC_SHORT_FRAME_LENGTH;
            qmfOffset = 8;
            qmfShift  = 2;              
            nWindows  = 9;
            break;
        case UPD_QMF_24:
            L = 4;
            mdctLength = AAC_SHORT_FRAME_LENGTH;
            qmfOffset = 11;
            qmfShift  = 2;
            nWindows  = 12;
            break;
        case UPD_QMF_30:
            L = 4;
            mdctLength = AAC_SHORT_FRAME_LENGTH;
            qmfOffset = 14;
            qmfShift  = 2;              
            nWindows  = 15;
            break;
        default:
            errorCode = SPACE_MDCT2QMF_ERROR_INVALID_QMF_UPDATE;
            break;
        }

        for ( k = 0; k < nWindows; k++ )
        {
            switch( updQMF )
            {
            case UPD_QMF_16:
            case UPD_QMF_32:
                for ( i = 0; i < AAC_SHORT_FRAME_LENGTH; i++ )
                {
                    mdctSF[ i ]  = mdctIn[ mdctOffset + i ];
                }
                
                LOCAL_zero( N, &mdctSF[ AAC_SHORT_FRAME_LENGTH ] );
                break;
            case UPD_QMF_15:
                if ( k < nWindows - 1 )
                {
                    for ( i = 0; i < AAC_SHORT_FRAME_LENGTH; i++ )
                    {
                        mdctSF[ i ]  = mdctIn[ mdctOffset + i ];
                    }
                }
                else
                {
                    windowOffset = L;
                    L = 6;
                    mdctLength = 192;

                    
                    for ( i = 0, j = 0; i < mdctLength / 2; i++ )
                    {
                        mdctSF[ j++ ] = mdctIn[                          mdctOffset + i ];
                        mdctSF[ j++ ] = mdctIn[ AAC_SHORT_FRAME_LENGTH + mdctOffset + i ];
                    }
                }
                break;
            case UPD_QMF_18:
            case UPD_QMF_24:
            case UPD_QMF_30:
                for ( i = 0; i < AAC_SHORT_FRAME_LENGTH; i++ )
                {
                    mdctSF[ i ]  = mdctIn[ mdctOffset + i ];
                }
                break;
            default:
                errorCode = SPACE_MDCT2QMF_ERROR_INVALID_QMF_UPDATE;
                break;
            }

            
            LOCAL_fold_out( mdctSF, mdctLength, &wf[ windowOffset ], L, &V1[ 0 ], &V2[ 0 ] );
            LOCAL_hybcmdct2qmf( &V1[ 0 ], &V2[ 0 ], mdctLength, &wt[ windowOffset ], L, Z1Real, Z1Imag );

            
            for ( i = 0; i < qmfBands; i++ )
            {
#ifdef PARTIALLY_COMPLEX
                arg = ( 2.0f * i + 1.0f ) * (float) PI / 4.0f;
#else
                arg = ( 258.0f * i + 385.0f )* (float) PI / 256.0f;
#endif
                twipostReal[ i ]  =  (float) cos( arg );
                twipostImag[ i ]  = -(float) sin( arg );
            } 

            
            for ( i = 0; i < qmfBands; i++ )
            {
                for ( j = 0; j < L; j++ )
                {
                    qmfReal[ qmfOffset + j + qmfGlobalOffset ][ i ] += twipostReal[ i ] * Z1Real[ i ][ j ] - twipostImag[ i ] * Z1Imag[ i ][ j ];
                    qmfImag[ qmfOffset + j + qmfGlobalOffset ][ i ] += twipostReal[ i ] * Z1Imag[ i ][ j ] + twipostImag[ i ] * Z1Real[ i ][ j ];
                }
            }

            mdctOffset += mdctShift;
            qmfOffset  += qmfShift;

        } 

        break;
    default:
        errorCode = SPACE_MDCT2QMF_ERROR_INVALID_WINDOW_TYPE;
        break;
    } 

    return errorCode;
}



SPACE_MDCT2QMF_ERROR SPACE_MDCT2QMF_Destroy
(
    void
)
{
    SPACE_MDCT2QMF_ERROR errorCode = SPACE_MDCT2QMF_ERROR_NO_ERROR;

    return errorCode;
}



static void LOCAL_fold_out
(
    float        *   const   S,
    int              const   Lv,
    float        *   const   w,
    int              const   Lw,
    float        *   const   Vmain,
    float        *   const   Vslave
)
{
    int n;
    int i;
    int j;
    int k;

    float * w1;
    float * w2;
    float * w3;
    float * w4;

    int M = Lw / 2;
    int L = Lv / M;
    int M2w = M / 2;
    int M2a = M - M2w;

    
    
    for ( i = M; i < Lv; i += Lw )
    {
        for ( n = i + Lw; i < n; i++ )
        {
            S[ i ] = -S[ i ];
        }
    }

    for ( i = 0; i < Lv; i++ )
    {
        Vmain [ i ] = S[ i ];
        Vslave[ i ] = 0.0;
    }

    w1 = &w[ 0 - M2a ];
    w2 = &w[ 0 + M2w ];
    w3 = &w[ M - M2a ];
    w4 = &w[ M + M2w ];

    
    for ( n = 0, j = 0, k = M; n < L - 1; n++ )
    {
        for ( i = 0; i < M2a; i++, j++, k++)
        {
            Vmain [ k ] = w2[ i ] * S[ k ];
            Vslave[ j ] = w4[ i ] * S[ k ];
        }

        for ( ; i < M; i++, j++, k++ )
        {
            Vmain [ j ] = w3[ i ] * S[ j ];
            Vslave[ k ] = w1[ i ] * S[ j ];
        }
    }

    
    for ( i = 0, j = M - 1; i < M2a; i++, j-- )
    {
        Vmain [ i ] = w2[ i ] * S[ i ];
        Vslave[ j ] = w4[ i ] * S[ i ];
    }

    
    j = L * M - M2w;
    k = L * M - M2a - 1;
    for ( ; i < M; i++, j++, k-- )
    {
        Vmain [ j ] = w3[ i ] * S[ j ];
        Vslave[ k ] = w1[ i ] * S[ j ];
    }
}



static void LOCAL_hybcmdct2qmf
(
    float        *   const   Vmain,
    float        *   const   Vslave,
    int              const   Lv,
    float        *   const   w,
    int              const   Lw,
    float                    Z_real[ ][ 2 * MAX_TIME_SLOTS ],
    float                    Z_imag[ ][ 2 * MAX_TIME_SLOTS ]
)
{
    int n;
    int i;
    int addr;

    float x[ 2 * MAX_TIME_SLOTS ];
    float * em;
    float * ep;

    int M = Lw / 2;
    int L = Lv / M;

    int oddf = 0;                       

    em = &x[ 0 ];
    ep = &x[ M ];

    for ( n = 0; n < L; n++ )
    {
        
        addr = n * M;
        for ( i = 0; i < M; i++ )
        {
            if ( oddf % 2 )
            {
               em[ i ] = Vmain [ addr + i ];
               ep[ i ] = Vslave[ addr + i ];
            }
            else
            {
               ep[ i ] = Vmain [ addr + i ];
               em[ i ] = Vslave[ addr + i ];
            }
        }

        LOCAL_imdet( x, w, M, Z_real[ n ], Z_imag[ n ] );

        oddf++;              
    }
}



static void LOCAL_imdet
(
    float        *   const   x,
    float        *   const   w,
    int              const   L,
    float        *   const   zReal,
    float        *   const   zImag
)
{
    int Lw = 2 * L;
    int kMin = -(int) floor( L / 2.0f );
    float gain = (float) sqrt( 1.0f / Lw );
    float argument;
    int k;
    int n;

    
    for ( k = 0; k < Lw; k++ )
    {
        zReal[ k ] = 0.0f;
        zImag[ k ] = 0.0f;

        for ( n = 0; n < Lw; n++ )
        {
            argument    = ( k + kMin + 0.5f ) * ( n - L + 0.5f ) * (float) PI / L;
            zReal[ k ] += x[ n ] * gain * (float) cos( argument );
            zImag[ k ] += x[ n ] * gain * (float) sin( argument );
        }

        zReal[ k ] *= w[ k ];
        zImag[ k ] *= w[ k ];
    }
}



static void LOCAL_sin
(
    int              const   T,
    int              const   P,
    int              const   L,
    float        *   const   b
)
{
    int i;
    int oddLength = T % 2;

    for ( i = 0; i < L; i++ )
    {
        b[ i ] = (float) sin( PI / ( 2 * T ) * ( i + P + .5 * ( oddLength + 1 ) ) );
    }
}



static void LOCAL_one
(
    int              const   L,
    float        *   const   b
)
{
    int i;

    for ( i = 0; i < L; i++ )
    {
        b[ i ] = 1.0;
    }
}



static void LOCAL_zero
(
    int              const   L,
    float        *   const   b
)
{
    int i;

    for ( i = 0; i < L; i++ )
    {
        b[ i ] = 0.0;
    }
}



static void LOCAL_freq_window
(
    int              const  L,
    float         *  const  b
)
{
    int i;
    int oddLength = L % 2;

    for ( i = 0; i < L - oddLength; i++ )
    {
        b[                         i ] = wf[ L - 1 ][ i ];
        b[ 2 * L - 1 - oddLength - i ] = wf[ L - 1 ][ i ];
    }

    if ( oddLength == 1 )
    {
        b[     L - 1 ] = wf[ L - 1 ][ L - 1 ];
        b[ 2 * L - 1 ] = 0.0f;
    }
}



static SPACE_MDCT2QMF_ERROR LOCAL_MDCT_windows
(
    int updQMF,
    BLOCKTYPE        const   windowType,
    float        *   const   wf,
    float        *   const   wt
)
{
    int length       = 0;
    int lengthRight  = 0;
    int lengthLeft   = 0;
    int lengthConst1 = 0;
    int lengthConst2 = 0;


    SPACE_MDCT2QMF_ERROR errorCode = SPACE_MDCT2QMF_ERROR_NO_ERROR;

    switch( windowType )
    {
    case ONLYLONG_WINDOW:                                              
        
        length = updQMF;
        LOCAL_sin( updQMF, 0, 2 * length, &wt[ 0 ] );

        
        length = updQMF;
        break;
    case LONGSTART_WINDOW:                                        
        
        
        switch( updQMF )
        {
        case UPD_QMF_15:
            lengthConst1 = 6;
            lengthConst2 = 7;
            lengthRight  = 2;
            break;
        case UPD_QMF_16:
        case UPD_QMF_32:
            lengthConst1 = 7 * updQMF / 16;
            lengthConst2 = lengthConst1;
            lengthRight  = updQMF / 8;
            break;
        case UPD_QMF_18:
            lengthConst1 = 8;
            lengthConst2 = lengthConst1;
            lengthRight = 2;
            break;
        case UPD_QMF_24:
            lengthConst1 = 11;
            lengthConst2 = lengthConst1;
            lengthRight  = 2;
            break;
        case UPD_QMF_30:
            lengthConst1 = 14;
            lengthConst2 = lengthConst1;
            lengthRight  = 2;
            break;
        default:
            errorCode = SPACE_MDCT2QMF_ERROR_INVALID_QMF_UPDATE;
            break;
        }
        LOCAL_sin( updQMF, 0, updQMF, &wt[ 0 ] );                                          
        LOCAL_one( lengthConst1, &wt[ updQMF ] );                                          
        LOCAL_sin( lengthRight, lengthRight, lengthRight, &wt[ updQMF + lengthConst1 ] );  
        LOCAL_zero( lengthConst2, &wt[ updQMF + lengthConst1 + lengthRight ] );            

        
        length = updQMF;
        break;
    case EIGHTSHORT_WINDOW:                                             
        
        
        switch( updQMF )
        {
        case UPD_QMF_16:
        case UPD_QMF_32:
            length = updQMF / 8;         
            break;
        case UPD_QMF_15:
        case UPD_QMF_18:
        case UPD_QMF_30:
        case UPD_QMF_24:
            length = 2;              
            break;
        default:
            errorCode = SPACE_MDCT2QMF_ERROR_INVALID_QMF_UPDATE;
            break;
        }

        
        LOCAL_sin( length, 0, 2 * length, &wt[ 0 ] );
        break;
    case LONGSTOP_WINDOW:                                         
        
        
        switch( updQMF )
        {
        case UPD_QMF_15:
            lengthConst1 = 6;
            lengthConst2 = 7;
            lengthLeft   = 2;
            break;
        case UPD_QMF_16:
        case UPD_QMF_32:
            lengthConst1 = 7 * updQMF / 16;
            lengthConst2 = lengthConst1;
            lengthLeft   = updQMF / 8;
            break;
        case UPD_QMF_18:
            lengthConst1 = 8;
            lengthConst2 = lengthConst1;
            lengthLeft   = 2;
            break;
        case UPD_QMF_24:
            lengthConst1 = 11;
            lengthConst2 = lengthConst1;
            lengthLeft   = 2;
            break;
        case UPD_QMF_30:
            lengthConst1 = 14;
            lengthConst2 = lengthConst1;
            lengthLeft   = 2;
            break;
        default:
            errorCode = SPACE_MDCT2QMF_ERROR_INVALID_QMF_UPDATE;
            break;
        }

        LOCAL_zero( lengthConst1, &wt[ 0 ] );                                                 
        LOCAL_sin( lengthLeft, 0, lengthLeft, &wt[ lengthConst1 ] );                          
        LOCAL_one( lengthConst2, &wt[ lengthConst1 + lengthLeft ] );                          
        LOCAL_sin( updQMF, updQMF, updQMF, &wt[ lengthConst1 + lengthLeft + lengthConst2 ] ); 

        
        length = updQMF;
        break;
    default:
        errorCode = SPACE_MDCT2QMF_ERROR_INVALID_WINDOW_TYPE;
        break;
    }

    
    LOCAL_freq_window( length, &wf[ 0 ] );

    
    if ( ( updQMF == UPD_QMF_15 ) && ( windowType == EIGHTSHORT_WINDOW ) )
    {
        int length2 = 3;

        LOCAL_sin( length, 0, length, &wt[ 2 * length ] );
        LOCAL_one( 1, &wt[ 3 * length ] );                          
        LOCAL_sin( length, length, length, &wt[ 3 * length + 1 ] );
        LOCAL_zero( 1, &wt[ 4 * length + 1 ] );                     

        LOCAL_freq_window( length2, &wf[ 2 * length ] );
    }

    return errorCode;
}
