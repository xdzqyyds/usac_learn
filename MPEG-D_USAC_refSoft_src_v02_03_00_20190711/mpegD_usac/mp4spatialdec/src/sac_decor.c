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


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "sac_decor.h"
#include "sac_sbrconst.h"
#include "sac_calcM1andM2.h"
#include "sac_hybfilter.h"
#include "sac_bitdec.h"

#include <assert.h>


#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#define PS_REVERB_SLOPE                 ( 0.05f )
#define PS_REVERB_DECAY_CUTOFF          ( 10 )
#define PS_REVERB_SERIAL_TIME_CONST     ( 7.0f )
#define PS_DUCK_PEAK_DECAY_FACTOR       ( 0.765928338364649f )
#define PS_DUCK_FILTER_COEFF            ( 0.25f )

static int REV_delay[NO_DECORR_BANDS] = {
        11,  10,   5,   2
};

static int REV_delay_PS[NO_DECORR_BANDS][MAX_NO_DECORR_CHANNELS] = {
        { 8,   8,   8,   8,   8,   8,   8,   8,   8,   8 },
        {14,  14,  14,  14,  14,  14,  14,  14,  14,  14 },
        { 1,   1,   1,   1,   1,   1,   1,   1,   1,   1 },
        { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0 }
};

static int REV_splitfreq0[NO_DECORR_BANDS] = { 3, 15, 24, 65 };
static int REV_splitfreq1[NO_DECORR_BANDS] = { 3, 50, 65, 65 };
static int REV_splitfreq2[NO_DECORR_BANDS] = { 0, 15, 65, 65 };
static int REV_splitfreqLP[NO_DECORR_BANDS] = { 0, 8, 24, 65 };

static int REV_splitfreq1_PS[NO_DECORR_BANDS] = {  8, 36, 65, 65 };

static float hybridCenterFreq[] = {
  -1.5f/4, -0.5f/4, 0.5f/4, 1.5f/4, 2.5f/4, 3.5f/4, 2.5f/2, 3.5f/2, 4.5f/2, 5.5f/2
};

static float lattice_coeff_0[DECORR_FILTER_ORDER_BAND_0] = {
  -0.6135f, -0.3819f, -0.2331f, -0.1467f, -0.0074f,  0.0281f,  0.1061f, -0.2914f,  0.1576f,  0.0898f
};

static float lattice_coeff_1[DECORR_FILTER_ORDER_BAND_1] = {
  -0.2874f, -0.0732f,  0.1000f, -0.1121f,  0.0822f,  0.0202f, -0.0521f, -0.1221f
};

static float lattice_coeff_2[DECORR_FILTER_ORDER_BAND_2] = {
  0.1358f, -0.0373f,  0.0357f
};

static float lattice_coeff_3[DECORR_FILTER_ORDER_BAND_3] = {
  0.0352f, -0.0130f
};

static float lattice_deltaPhi[DECORR_FILTER_ORDER_BAND_0] = {
  1.7910f,  0.4357f,  1.1439f,  0.9161f,  1.6801f,  1.4365f,  0.8604f,  0.0349f,  1.5483f,  0.8382f
};

struct DUCKER_INTERFACE
{
    void ( * Apply )
    (
        DUCKER_INTERFACE * const self,
        float              const inputReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
        float              const inputImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
        float                    outputReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
        float                    outputImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
        int                const timeSlots
    );

    void ( * Destroy )
    (
        DUCKER_INTERFACE ** const selfPtr
    );

};

typedef struct
{
    float          alpha;
    float          gamma;
    float          abs_thr;
    int            hybridBands;
    int            parameterBands;

    float          SmoothDirectNrg[MAX_PARAMETER_BANDS];
    float          SmoothReverbNrg[MAX_PARAMETER_BANDS];

} DUCK_INSTANCE;

typedef struct
{
    int            hybridBands;
    int            parameterBands;

    float          peakDecay[MAX_PARAMETER_BANDS];
    float          peakDiff[MAX_PARAMETER_BANDS];
    float          smoothDirectNrg[MAX_PARAMETER_BANDS];

} DUCK_INSTANCE_PS;


static void **callocMatrix(int rows, int cols, size_t size);
static void freeMatrix(void **aaMatrix, int rows);

static void ConvertLatticeCoefsReal
(
    float       const * const rfc,
    float             * const apar,
    int                 const order
);

static void ConvertLatticeCoefsComplex
(
    float       const * const rfcReal,
    float       const * const rfcImag,
    float             * const aparReal,
    float             * const aparImag,
    int                 const order
);

static int DecorrFilterCreate
(
    DECORR_FILTER_INSTANCE    ** const selfPtr,
    int                          const decorr_seed,
    int                          const qmf_band,
    int                          const reverb_band,
    int                          const decType
);

static void Convolute
(
    float                const * const xReal,
    float                const * const xImag,
    int                          const xLength,
    float                const * const yReal,
    float                const * const yImag,
    int                          const yLength,
    float                      * const zReal,
    float                      * const zImag,
    int                        * const zLength
);

static int DecorrFilterCreatePS
(
    DECORR_FILTER_INSTANCE    ** const facePtr,
    int                          const hybridBand,
    int                          const reverbBand
);

static void DecorrFilterDestroy
(
    DECORR_FILTER_INSTANCE    ** const facePtr
);

static void DecorrFilterApply
(
    DECORR_FILTER_INSTANCE     * const face,
    float                const * const inputReal,
    float                const * const inputImag,
    int                          const length,
    float                      * const outputReal,
    float                      * const outputImag
);

static int DuckerCreate
(
    DUCKER_INTERFACE ** const selfPtr,
    int                 const hybridBands
);

static void DuckerDestroy
(
    DUCKER_INTERFACE ** const selfPtr
);

static void DuckerApply
(
    DUCKER_INTERFACE * const self,
    float              const inputReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
    float              const inputImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
    float                    outputReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
    float                    outputImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
    int                const timeSlots
);

static int DuckerCreatePS
(
    DUCKER_INTERFACE ** const selfPtr,
    int                 const hybridBands
);

static void DuckerApplyPS
(
    DUCKER_INTERFACE * const self,
    float              const inputReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
    float              const inputImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
    float                    outputReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
    float                    outputImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
    int                const timeSlots
);








static void **
callocMatrix(int rows,    
             int cols,    
             size_t size) 
{
  int i;
  void **pp;

  pp =(void **) calloc(rows, sizeof(void*));
  if (pp == NULL)
    return NULL;
  for (i=0 ; i<rows ; i++) {
    pp[i] = calloc(cols, size);
    if (pp[i] == NULL) {
      for (i-- ; i>=0 ; i--)
        free(pp[i]);
      free(pp);
      return NULL;
    }
  }
  return (void**)pp;
}



static void
freeMatrix(void **aaMatrix, 
           int rows)        
{
  int i;

  for (i=0 ; i<rows ; i++) {
    free (aaMatrix[i]);
  }
  free (aaMatrix);
}


static void ConvertLatticeCoefsReal
(
    float       const * const rfc,
    float             * const apar,
    int                 const order
)
{
    int     i, j;
    float   tmp[MAX_DECORR_FILTER_ORDER+1];

    apar[0] = 1.0;
    for (i = 0; i < order; i++)
    {
        apar[i+1] = rfc[i];
        for (j = 0; j < i; j++)
        {
            apar[j+1] = tmp[j] + rfc[i] * tmp[i-j-1];
        }
        for (j = 0; j <= i; j++)
        {
            tmp[j] = apar[j+1];
        }
    }
}

static void ConvertLatticeCoefsComplex
(
    float       const * const rfcReal,
    float       const * const rfcImag,
    float             * const aparReal,
    float             * const aparImag,
    int                 const order
)
{
    int     i, j;
    float   tmpReal[MAX_DECORR_FILTER_ORDER+1];
    float   tmpImag[MAX_DECORR_FILTER_ORDER+1];

    aparReal[0] = 1.0f;
    aparImag[0] = 0.0f;
    for (i = 0; i < order; i++)
    {
        aparReal[i+1] = rfcReal[i];
        aparImag[i+1] = rfcImag[i];
        for (j = 0; j < i; j++)
        {
            aparReal[j+1] = tmpReal[j] + rfcReal[i] * tmpReal[i-j-1] + rfcImag[i] * tmpImag[i-j-1];
            aparImag[j+1] = tmpImag[j] - rfcReal[i] * tmpImag[i-j-1] + rfcImag[i] * tmpReal[i-j-1];
        }
        for (j = 0; j <= i; j++)
        {
            tmpReal[j] = aparReal[j+1];
            tmpImag[j] = aparImag[j+1];
        }
    }
}



static int DecorrFilterCreate
(
    DECORR_FILTER_INSTANCE    ** const selfPtr,
    int                          const decorr_seed,
    int                          const qmf_band,
    int                          const reverb_band,
    int                          const decType
)
{
    int           errorCode = ERR_NONE;
    DECORR_FILTER_INSTANCE *self      = NULL;
    int           i;
    float         *lattice_coeff = NULL;
    float         lattice_coeffReal[MAX_DECORR_FILTER_ORDER];
    float         lattice_coeffImag[MAX_DECORR_FILTER_ORDER];

    self = ( DECORR_FILTER_INSTANCE* )calloc( 1, sizeof( *self ) );

    if ( self == NULL )
    {
        errorCode = ERR_UGLY_ERROR;
    }

    if ( errorCode == ERR_NONE )
    {
        switch (reverb_band)
        {
            case 0: self->numLength = self->denLength = DECORR_FILTER_ORDER_BAND_0+1;
					lattice_coeff = &(lattice_coeff_0[0]);
					
			break;
            case 1: self->numLength = self->denLength = DECORR_FILTER_ORDER_BAND_1+1;
					lattice_coeff = &(lattice_coeff_1[0]);
					
			break;
            case 2: self->numLength = self->denLength = DECORR_FILTER_ORDER_BAND_2+1;
                    lattice_coeff = &(lattice_coeff_2[0]);
            break;
            case 3: self->numLength = self->denLength = DECORR_FILTER_ORDER_BAND_3+1;
                    lattice_coeff = &(lattice_coeff_3[0]);
            break;
        }
        self->stateLength = ( self->numLength > self->denLength )? self->numLength: self->denLength;
    }

    if ( errorCode == ERR_NONE )
    {
        self->numeratorReal   = NULL;
        self->numeratorImag   = NULL;
        self->denominatorReal = NULL;
        self->denominatorImag = NULL;

        if ( decType == 1 ) 
        {
            self->numeratorReal = ( float* )calloc( self->numLength, sizeof( *self->numeratorReal ) );
            self->numeratorImag = ( float* )calloc( self->numLength, sizeof( *self->numeratorImag ) );
            self->denominatorReal = ( float* )calloc( self->denLength, sizeof( *self->denominatorReal ) );
            self->denominatorImag = ( float* )calloc( self->denLength, sizeof( *self->denominatorImag ) );

            if ( ( self->numeratorReal != NULL ) && ( self->numeratorImag != NULL ) &&
                 ( self->denominatorReal != NULL ) && ( self->denominatorImag != NULL ) )
            {
                for ( i = 0; i < self->numLength-1; i++ )
                {
                    lattice_coeffReal[i] =  (float)cos ( qmf_band * 0.5 * lattice_deltaPhi[i] ) * lattice_coeff[i];
                    lattice_coeffImag[i] =  (float)sin ( qmf_band * 0.5 * lattice_deltaPhi[i] ) * lattice_coeff[i];
                }

                
                ConvertLatticeCoefsComplex (lattice_coeffReal, lattice_coeffImag, self->denominatorReal, self->denominatorImag, self->numLength-1);
                for ( i = 0; i < self->numLength; i++ )
                {
                    self->numeratorReal[i] = self->denominatorReal[self->numLength-1 - i];
                    self->numeratorImag[i] = -self->denominatorImag[self->numLength-1 - i];
                }

                self->complex = 1;
            }
            else
            {
                errorCode = ERR_UGLY_ERROR;
            }
        }
        else
        {
            self->numeratorReal = ( float* )calloc( self->numLength, sizeof( *self->numeratorReal ) );
            self->denominatorReal = ( float* )calloc( self->denLength, sizeof( *self->denominatorReal ) );

            if ( ( self->numeratorReal != NULL ) && ( self->denominatorReal != NULL ) )
            {
                
                ConvertLatticeCoefsReal (lattice_coeff, self->denominatorReal, self->numLength-1);

                for ( i = 0; i < self->numLength; i++ )
                {
                    self->numeratorReal[i] = self->denominatorReal[self->numLength-1 - i];
                }

                self->complex = 0;
            }
            else
            {
                errorCode = ERR_UGLY_ERROR;
            }
        }
    }

    if ( errorCode == ERR_NONE )
    {
        self->stateReal = ( float* )calloc( self->stateLength, sizeof( *self->stateReal ) );
        self->stateImag = ( float* )calloc( self->stateLength, sizeof( *self->stateImag ) );

        if ( ( self->stateReal == NULL ) || ( self->stateImag == NULL ) )
        {
            errorCode = ERR_UGLY_ERROR;
        }
    }

    if ( errorCode != ERR_NONE )
    {
        DecorrFilterDestroy( &self );
    }

    *selfPtr = self;

    return errorCode;
}


static void Convolute
(
    float const * const xReal,
    float const * const xImag,
    int           const xLength,
    float const * const yReal,
    float const * const yImag,
    int           const yLength,
    float       * const zReal,
    float       * const zImag,
    int         * const zLength
)
{
    int   i;
    int   j;
    int   k;
    int   count;

    *zLength = xLength + yLength - 1;

    for ( i = 0; i < *zLength; i++ )
    {
        k     = ( i < yLength ) ? i : yLength - 1;
        j     = i - k;
        count = min( k + 1, xLength );

        zReal[i] = 0.0f;
        zImag[i] = 0.0f;

        for ( ; j < count; j++, k-- )
        {
            zReal[i] += xReal[j] * yReal[k] - xImag[j] * yImag[k];
            zImag[i] += xReal[j] * yImag[k] + xImag[j] * yReal[k];
        }
    }
}


static int DecorrFilterCreatePS
(
    DECORR_FILTER_INSTANCE    ** const selfPtr,
    int                          const hybridBand,
    int                          const reverbBand
)
{
    int                     errorCode = ERR_NONE;
    DECORR_FILTER_INSTANCE *self      = NULL;
    int                     i;
    int                     qmfBand;
    float                   centerFreq;
    float                   decay;
    float                   temp0Real[MAX_DECORR_FILTER_ORDER];
    float                   temp0Imag[MAX_DECORR_FILTER_ORDER];
    float                   temp1Real[MAX_DECORR_FILTER_ORDER];
    float                   temp1Imag[MAX_DECORR_FILTER_ORDER];
    float                   numReal[MAX_DECORR_FILTER_ORDER];
    float                   numImag[MAX_DECORR_FILTER_ORDER];
    float                   denReal[MAX_DECORR_FILTER_ORDER];
    float                   denImag[MAX_DECORR_FILTER_ORDER];
    int                     length;
    float                   fract0Real;
    float                   fract0Imag;
    float                   temp;

    self = ( DECORR_FILTER_INSTANCE* )calloc( 1, sizeof( *self ) );

    if ( self == NULL )
    {
        errorCode = ERR_UGLY_ERROR;
    }

    if ( errorCode == ERR_NONE )
    {
        if ( reverbBand == 0 )
        {
            self->numLength       = MAX_DECORR_FILTER_ORDER;
            self->denLength       = MAX_DECORR_FILTER_ORDER;
            self->numeratorReal   = ( float* ) calloc( self->numLength, sizeof( *self->numeratorReal ) );
            self->numeratorImag   = ( float* ) calloc( self->numLength, sizeof( *self->numeratorImag ) );
            self->denominatorReal = ( float* ) calloc( self->denLength, sizeof( *self->denominatorReal ) );
            self->denominatorImag = ( float* ) calloc( self->denLength, sizeof( *self->denominatorImag ) );

            if ( ( self->numeratorReal   == NULL ) || ( self->numeratorImag   == NULL ) ||
                 ( self->denominatorReal == NULL ) || ( self->denominatorImag == NULL ) )
            {
                errorCode = ERR_UGLY_ERROR;
            }

            if ( errorCode == ERR_NONE )
            {
                qmfBand = SacGetQmfSubband( hybridBand );

                if ( qmfBand < QMF_BANDS_TO_HYBRID )
                {
                    centerFreq = hybridCenterFreq[hybridBand];
                }
                else
                {
                    centerFreq = qmfBand + 0.5f;
                }

                decay = 1.0f + PS_REVERB_SLOPE * ( PS_REVERB_DECAY_CUTOFF - hybridBand );

                if ( decay > 1.0f )
                {
                    decay = 1.0f;
                }

                memset( temp0Real, 0, sizeof( temp0Real ) );
                memset( temp0Imag, 0, sizeof( temp0Imag ) );

                temp0Real[0] = ( float ) -exp( -3.0f / PS_REVERB_SERIAL_TIME_CONST ) * decay;
                temp0Real[3] = ( float ) cos( -PI * centerFreq * 0.43f );
                temp0Imag[3] = ( float ) sin( -PI * centerFreq * 0.43f );

                memset( temp1Real, 0, sizeof( temp1Real ) );
                memset( temp1Imag, 0, sizeof( temp1Imag ) );

                temp1Real[0] = ( float ) -exp( -4.0f / PS_REVERB_SERIAL_TIME_CONST ) * decay;
                temp1Real[4] = ( float ) cos( -PI * centerFreq * 0.75f );
                temp1Imag[4] = ( float ) sin( -PI * centerFreq * 0.75f );

                Convolute( temp0Real, temp0Imag, 4, temp1Real, temp1Imag, 5, numReal, numImag, &length );

                temp0Real[3] *= temp0Real[0];
                temp0Imag[3] *= temp0Real[0];
                temp0Real[0]  = 1.0f;

                temp1Real[4] *= temp1Real[0];
                temp1Imag[4] *= temp1Real[0];
                temp1Real[0]  = 1.0f;

                Convolute( temp0Real, temp0Imag, 4, temp1Real, temp1Imag, 5, denReal, denImag, &length );

                memset( temp0Real, 0, sizeof( temp0Real ) );
                memset( temp0Imag, 0, sizeof( temp0Imag ) );

                temp0Real[0] = ( float ) -exp( -5.0f / PS_REVERB_SERIAL_TIME_CONST ) * decay;
                temp0Real[5] = ( float ) cos( -PI * centerFreq * 0.347f );
                temp0Imag[5] = ( float ) sin( -PI * centerFreq * 0.347f );

                Convolute( temp0Real, temp0Imag, 6, numReal, numImag, 8, self->numeratorReal, self->numeratorImag, &self->numLength );

                temp0Real[5] *= temp0Real[0];
                temp0Imag[5] *= temp0Real[0];
                temp0Real[0]  = 1.0f;

                Convolute( temp0Real, temp0Imag, 6, denReal, denImag, 8, self->denominatorReal, self->denominatorImag, &self->denLength );

                fract0Real = ( float ) cos( -PI * centerFreq * 0.39f );
                fract0Imag = ( float ) sin( -PI * centerFreq * 0.39f );


                for ( i = 0; i < self->numLength; i++ )
                {
                    temp                   = self->numeratorReal[i] * fract0Real - self->numeratorImag[i] * fract0Imag;
                    self->numeratorImag[i] = self->numeratorImag[i] * fract0Real + self->numeratorReal[i] * fract0Imag;
                    self->numeratorReal[i] = temp;
                }

                self->complex = 1;
            }
        }
    }

    if ( errorCode == ERR_NONE )
    {
        self->stateLength = ( self->numLength > self->denLength )? self->numLength: self->denLength;

        if ( self->stateLength > 0 )
        {
            self->stateReal = ( float* ) calloc( self->stateLength, sizeof( *self->stateReal ) );
            self->stateImag = ( float* ) calloc( self->stateLength, sizeof( *self->stateImag ) );

            if ( ( self->stateReal == NULL ) || ( self->stateImag == NULL ) )
            {
                errorCode = ERR_UGLY_ERROR;
            }
        }
    }

    if ( errorCode != ERR_NONE )
    {
        DecorrFilterDestroy( &self );
    }

    *selfPtr = self;

    return errorCode;
}


static void DecorrFilterDestroy
(
    DECORR_FILTER_INSTANCE ** const selfPtr
)
{
    DECORR_FILTER_INSTANCE *self = *selfPtr;

    if ( self != NULL )
    {
        if ( self->numeratorReal != NULL )
        {
            free( self->numeratorReal );
        }

        if ( self->numeratorImag != NULL )
        {
            free( self->numeratorImag );
        }

        if ( self->denominatorReal != NULL )
        {
            free( self->denominatorReal );
        }

        if ( self->denominatorImag != NULL )
        {
            free( self->denominatorImag );
        }

        if ( self->stateReal != NULL )
        {
            free( self->stateReal );
        }

        if ( self->stateImag != NULL )
        {
            free( self->stateImag );
        }

        free( self );
    }

    *selfPtr = NULL;
}


static void DecorrFilterApply
(
    DECORR_FILTER_INSTANCE       * const self,
    float                  const * const inputReal,
    float                  const * const inputImag,
    int                            const length,
    float                        * const outputReal,
    float                        * const outputImag
)
{
    float  xReal;
    float  xImag;
    float  yReal;
    float  yImag;
    int    commonPart;
    int    i;
    int    j;

    commonPart = ( self->denLength > self->numLength )? self->numLength: self->denLength;

    for ( i = 0; i < length; i++ )
    {
        if ( self->stateLength > 0 )
        {
            if ( self->complex == 1 )
            {
                assert( self->numLength == self->denLength );

                xReal         = inputReal[i];
                xImag         = inputImag[i];
                yReal         = self->stateReal[0] + xReal * self->numeratorReal[0] - xImag * self->numeratorImag[0];
                yImag         = self->stateImag[0] + xReal * self->numeratorImag[0] + xImag * self->numeratorReal[0];
                outputReal[i] = yReal;
                outputImag[i] = yImag;

                for ( j = 1; j < commonPart; j++ )
                {
                    self->stateReal[j - 1]  = self->stateReal[j];
                    self->stateImag[j - 1]  = self->stateImag[j];

                    self->stateReal[j - 1] += xReal * self->numeratorReal[j] - xImag * self->numeratorImag[j];
                    self->stateImag[j - 1] += xReal * self->numeratorImag[j] + xImag * self->numeratorReal[j];

                    self->stateReal[j - 1] -= yReal * self->denominatorReal[j] - yImag * self->denominatorImag[j];
                    self->stateImag[j - 1] -= yReal * self->denominatorImag[j] + yImag * self->denominatorReal[j];
                }
            }
            else
            {
                xReal         = inputReal[i];
                xImag         = inputImag[i];
                yReal         = self->stateReal[0] + xReal * self->numeratorReal[0];
                yImag         = self->stateImag[0] + xImag * self->numeratorReal[0];
                outputReal[i] = yReal;
                outputImag[i] = yImag;

                for ( j = 1; j < commonPart; j++ )
                {
                    self->stateReal[j - 1] = self->stateReal[j] + self->numeratorReal[j] * xReal - self->denominatorReal[j] * yReal;
                    self->stateImag[j - 1] = self->stateImag[j] + self->numeratorReal[j] * xImag - self->denominatorReal[j] * yImag;
                }

                for ( ; j < self->denLength; j++ )
                {
                    self->stateReal[j - 1] = self->stateReal[j] - self->denominatorReal[j] * yReal;
                    self->stateImag[j - 1] = self->stateImag[j] - self->denominatorReal[j] * yImag;
                }

                for ( ; j < self->numLength; j++ )
                {
                    self->stateReal[j - 1] = self->stateReal[j] + self->numeratorReal[j] * xReal;
                    self->stateImag[j - 1] = self->stateImag[j] + self->numeratorReal[j] * xImag;
                }
            }
        }
        else
        {
            outputReal[i] = inputReal[i];
            outputImag[i] = inputImag[i];
        }
    }
}


static int DuckerCreate
(
    DUCKER_INTERFACE ** const facePtr,
    int                 const hybridBands
)
{
    DUCKER_INTERFACE *face      = NULL;
    DUCK_INSTANCE    *self      = NULL;
    int               errorCode = ERR_NONE;

    face = calloc( 1, sizeof( *face ) + sizeof( *self ) );

    if ( face == NULL )
    {
        errorCode = ERR_UGLY_ERROR;
    }

    if ( errorCode == ERR_NONE )
    {
        self = ( DUCK_INSTANCE * ) &face[1];

        self->alpha = (float)DUCK_ALPHA;
        self->gamma = (float)DUCK_GAMMA;
        self->abs_thr = (float)ABS_THR;
        self->hybridBands = hybridBands;
        self->parameterBands = MAX_PARAMETER_BANDS;

        face->Apply = DuckerApply;
        face->Destroy = DuckerDestroy;
    }

    if ( errorCode != ERR_NONE )
    {
        DuckerDestroy( &face );
    }

    *facePtr = face;

    return errorCode;
}


static void DuckerDestroy
(
    DUCKER_INTERFACE ** const facePtr
)
{
    DUCKER_INTERFACE *face = *facePtr;

    if ( face != NULL )
    {
        free( face );
    }

    *facePtr = NULL;
}


static void DuckerApply
(
    DUCKER_INTERFACE * const face,
    float              const inputReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
    float              const inputImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
    float                    outputReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
    float                    outputImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
    int                const timeSlots
)
{
    DUCK_INSTANCE *self = ( DUCK_INSTANCE * ) &face[1];
    float          directNrg[MAX_PARAMETER_BANDS];
    float          reverbNrg[MAX_PARAMETER_BANDS];
    float          duckGain[MAX_PARAMETER_BANDS];
    int            ts;
    int            qs;
    int            pb;

    for ( ts = 0; ts < timeSlots; ts++ )
    {
        
        memset( directNrg, 0, sizeof( directNrg ) );
        memset( reverbNrg, 0, sizeof( reverbNrg ) );

        
        for ( qs = 0; qs < self->hybridBands; qs++ )
        {
            pb = SpatialDecGetParameterBand( self->parameterBands, qs );

            directNrg[pb] += inputReal [ts][qs] * inputReal [ts][qs] + inputImag [ts][qs] * inputImag [ts][qs];
            reverbNrg[pb] += outputReal[ts][qs] * outputReal[ts][qs] + outputImag[ts][qs] * outputImag[ts][qs];
        }

        for ( pb = 0; pb < self->parameterBands; pb++)
        {
            
            self->SmoothDirectNrg[pb] = self->SmoothDirectNrg[pb] * self->alpha + directNrg[pb] * ( 1.0f - self->alpha );
            self->SmoothReverbNrg[pb] = self->SmoothReverbNrg[pb] * self->alpha + reverbNrg[pb] * ( 1.0f - self->alpha );

            duckGain[pb] = 1.0f;
            
            if ( self->SmoothReverbNrg[pb] > self->SmoothDirectNrg[pb] * self->gamma )
            {
                duckGain[pb] = ( float ) sqrt( self->SmoothDirectNrg[pb] * self->gamma / ( self->SmoothReverbNrg[pb] + self->abs_thr ) );
            }

            
            if ( self->SmoothDirectNrg[pb] > self->SmoothReverbNrg[pb] * self->gamma )
            {
                duckGain[pb] = min( 2.0f, ( float ) sqrt( self->SmoothDirectNrg[pb] / ( self->gamma * self->SmoothReverbNrg[pb] + self->abs_thr ) ) );
            }
        }

        for ( qs = 0; qs < self->hybridBands; qs++)
        {
            
            pb = SpatialDecGetParameterBand( self->parameterBands, qs );

            outputReal[ts][qs] *= duckGain[pb];
            outputImag[ts][qs] *= duckGain[pb];
        }
    }
}


static int DuckerCreatePS
(
    DUCKER_INTERFACE ** const facePtr,
    int                 const hybridBands
)
{
    DUCKER_INTERFACE *face      = NULL;
    DUCK_INSTANCE_PS *self      = NULL;
    int               errorCode = ERR_NONE;

    face = calloc( 1, sizeof( *face ) + sizeof( *self ) );

    if ( face == NULL )
    {
        errorCode = ERR_UGLY_ERROR;
    }

    if ( errorCode == ERR_NONE )
    {
        self = ( DUCK_INSTANCE_PS * ) &face[1];

        self->hybridBands = hybridBands;
        self->parameterBands = MAX_PARAMETER_BANDS;

        face->Apply = DuckerApplyPS;
        face->Destroy = DuckerDestroy;
    }

    if ( errorCode != ERR_NONE )
    {
        DuckerDestroy( &face );
    }

    *facePtr = face;

    return errorCode;
}


static void DuckerApplyPS
(
    DUCKER_INTERFACE * const face,
    float              const inputReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
    float              const inputImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
    float                    outputReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
    float                    outputImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
    int                const timeSlots
)
{
    DUCK_INSTANCE_PS *self = ( DUCK_INSTANCE_PS * ) &face[1];
    float             directNrg[MAX_PARAMETER_BANDS];
    float             duckGain[MAX_PARAMETER_BANDS];
    int               ts;
    int               qs;
    int               pb;

    for ( ts = 0; ts < timeSlots; ts++ )
    {
        memset( directNrg, 0, sizeof( directNrg ) );

        for ( qs = 0; qs < self->hybridBands; qs++ )
        {
            pb = SpatialDecGetParameterBand( self->parameterBands, qs );

            directNrg[pb] += inputReal[ts][qs] * inputReal[ts][qs] + inputImag[ts][qs] * inputImag[ts][qs];
        }

        for ( pb = 0; pb < self->parameterBands; pb++)
        {
            self->peakDecay[pb] = max( directNrg[pb], self->peakDecay[pb] * PS_DUCK_PEAK_DECAY_FACTOR );

            self->peakDiff[pb] += PS_DUCK_FILTER_COEFF * ( self->peakDecay[pb] - directNrg[pb] - self->peakDiff[pb] );

            self->smoothDirectNrg[pb] += PS_DUCK_FILTER_COEFF * ( directNrg[pb] - self->smoothDirectNrg[pb] );

            if ( ( 1.5f * self->peakDiff[pb] ) > self->smoothDirectNrg[pb] )
            {
                duckGain[pb] = self->smoothDirectNrg[pb] / ( 1.5f * self->peakDiff[pb] + ABS_THR );
            }
            else
            {
                duckGain[pb] = 1.0f;
            }
        }

        for ( qs = 0; qs < self->hybridBands; qs++)
        {
            pb = SpatialDecGetParameterBand( self->parameterBands, qs );

            outputReal[ts][qs] *= duckGain[pb];
            outputImag[ts][qs] *= duckGain[pb];
        }
    }
}


int     SpatialDecDecorrelateCreate(    HANDLE_DECORR_DEC *hDecorrDec,
                                                      int  subbands,
                                                      int  seed,
                                                      int  decType,
                                                      int  decorrType,
                                                      int  decorrConfig,
													  int  treeConfig
                                   )
{
    HANDLE_DECORR_DEC self      = NULL;
    int               errorCode = ERR_NONE;
    int               i, reverb_band;

    int              *REV_splitfreq;

    switch(decorrConfig)
    {
        case 0:
            REV_splitfreq = REV_splitfreq0;
            break;
        case 1:
            REV_splitfreq = REV_splitfreq1;
            break;
        case 2:
            REV_splitfreq = REV_splitfreq2;
            break;
        default:
            return(ERR_UGLY_ERROR);
            break;
    }

#ifdef PARTIALLY_COMPLEX
    REV_splitfreq = REV_splitfreqLP;
#endif

    if ( decorrType == 1 )
    {
#ifdef PARTIALLY_COMPLEX
        REV_splitfreq = REV_splitfreq1_PS;
#endif

    }

    self = ( HANDLE_DECORR_DEC )calloc( 1, sizeof( *self ) );

    if ( self == NULL )
    {
        errorCode = ERR_UGLY_ERROR;
    }

    if ( errorCode == ERR_NONE )
    {
        self->decorr_seed = seed;
        self->numbins     = subbands;

        for (i = 0; i < self->numbins; i++)
        {
            reverb_band = 0;
            while ( (reverb_band < 3) && (SacGetQmfSubband(i) >= (REV_splitfreq[reverb_band]-1)) )
                reverb_band++;

            if ( decorrType == 1 )
            {
                self->noSampleDelay[i] = REV_delay_PS[reverb_band][self->decorr_seed];  

                errorCode = DecorrFilterCreatePS( &self->Filter[i], i, reverb_band );
            }
            else
            {
				self->noSampleDelay[i] = REV_delay[reverb_band];  
				errorCode = DecorrFilterCreate( &self->Filter[i], self->decorr_seed, SacGetQmfSubband(i), reverb_band, decType);
			}
        }

        if ( errorCode == ERR_NONE )
        {
            self->DelayBufferReal = (float**)callocMatrix(self->numbins,MAX_TIME_SLOTS+MAX_NO_TIME_SLOTS_DELAY,sizeof(float));
            if ( self->DelayBufferReal == NULL )
            {
                errorCode = ERR_UGLY_ERROR;
            }
        }

        if ( errorCode == ERR_NONE )
        {
            self->DelayBufferImag = (float**)callocMatrix(self->numbins,MAX_TIME_SLOTS+MAX_NO_TIME_SLOTS_DELAY,sizeof(float));
            if ( self->DelayBufferImag == NULL )
            {
                errorCode = ERR_UGLY_ERROR;
            }
        }

        if ( errorCode == ERR_NONE )
        {
            if ( decorrType == 1 )
            {
                errorCode = DuckerCreatePS( &self->ducker, self->numbins );
            }
            else
            {
#ifdef PARTIALLY_COMPLEX
              errorCode = DuckerCreatePS( &self->ducker, self->numbins );
#else
              errorCode = DuckerCreate( &self->ducker, self->numbins );
#endif
            }
        }
    }

    if ( errorCode != ERR_NONE )
    {
        SpatialDecDecorrelateDestroy( &self );
    }

    *hDecorrDec = self;

    return (errorCode);
}


void    SpatialDecDecorrelateDestroy(   HANDLE_DECORR_DEC *hDecorrDec )
{
    HANDLE_DECORR_DEC self = *hDecorrDec;
    int           i;

    if ( self != NULL )
    {
        for (i = 0; i < self->numbins; i++)
        {
            if ( self->Filter[i] != NULL )
            {
                DecorrFilterDestroy( &self->Filter[i] );
            }
        }
        if ( self->DelayBufferReal != NULL )
        {
            freeMatrix( (void**)self->DelayBufferReal, self->numbins );
        }
        if ( self->DelayBufferImag != NULL )
        {
            freeMatrix( (void**)self->DelayBufferImag, self->numbins );
        }
        if ( self->ducker != NULL )
        {
            self->ducker->Destroy( &self->ducker );
        }

        free (self);

        *hDecorrDec = NULL;
    }
}


void    SpatialDecDecorrelateApply(     HANDLE_DECORR_DEC hDecorrDec,
                                        float InputReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                                        float InputImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                                        float OutputReal[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                                        float OutputImag[MAX_TIME_SLOTS][MAX_HYBRID_BANDS],
                                        int length
                                  )
{
    HANDLE_DECORR_DEC self = hDecorrDec;
    int idx, sb_sample;

    float TempReal[MAX_TIME_SLOTS];
    float TempImag[MAX_TIME_SLOTS];

    if ( self != NULL )
    {
        for (idx = 0; idx < self->numbins; idx++)
        {
            
            for (sb_sample = 0; sb_sample < length; sb_sample++)
            {
                self->DelayBufferReal[idx][self->noSampleDelay[idx]+sb_sample] = InputReal[sb_sample][idx];
                self->DelayBufferImag[idx][self->noSampleDelay[idx]+sb_sample] = InputImag[sb_sample][idx];
            }
            DecorrFilterApply( self->Filter[idx],
                               self->DelayBufferReal[idx],
                               self->DelayBufferImag[idx],
                               length,
                               TempReal,
                               TempImag
                            );

            
            for (sb_sample = 0; sb_sample < length; sb_sample++)
            {
                OutputReal[sb_sample][idx] = TempReal[sb_sample];
                OutputImag[sb_sample][idx] = TempImag[sb_sample];
            }

            
            for (sb_sample = 0; sb_sample < self->noSampleDelay[idx]; sb_sample++)
            {
                self->DelayBufferReal[idx][sb_sample] = self->DelayBufferReal[idx][length+sb_sample];
                self->DelayBufferImag[idx][sb_sample] = self->DelayBufferImag[idx][length+sb_sample];
            }
        }

        self->ducker->Apply( self->ducker, InputReal, InputImag, OutputReal, OutputImag, length );
    }
}
