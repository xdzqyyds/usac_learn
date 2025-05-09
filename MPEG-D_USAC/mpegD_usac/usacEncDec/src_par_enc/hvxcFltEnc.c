
/*


This software module was originally developed by

    Kazuyuki Iijima, Masayuki Nishiguchi, Shiro Omori (Sony Corporation)

    in the course of development of the MPEG-4 Audio standard (ISO/IEC 14496-3).
    This software module is an implementation of a part of one or more
    MPEG-4 Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
    standard (ISO/IEC 14496-3).
    ISO/IEC gives users of the MPEG-4 Audio standards (ISO/IEC 14496-3)
    free license to this software module or modifications thereof for use
    in hardware or software products claiming conformance to the MPEG-4
    Audio standards (ISO/IEC 14496-3).
    Those intending to use this software module in hardware or software
    products are advised that this use may infringe existing patents.
    The original developer of this software module and his/her company,
    the subsequent editors and their companies, and ISO/IEC have no
    liability for use of this software module or modifications thereof in
    an implementation.
    Copyright is not released for non MPEG-4 Audio (ISO/IEC 14496-3)
    conforming products. The original developer retains full right to use
    the code for his/her own purpose, assign or donate the code to a third
    party and to inhibit third party from using the code for non MPEG-4
    Audio (ISO/IEC 14496-3) conforming products.
    This copyright notice must be included in all copies or derivative works.

    Copyright (c)1996.

                                                                  
*/

#include <math.h>
#include <stdio.h>

#include "hvxc.h"
#include "hvxcEnc.h"

#define INVGAIN_4ji     1.1

#define A1_4ji  -1.998066423746901
#define B1_4ji  1.000000000000000
#define C1_4ji  -1.962822436245804
#define D1_4ji  0.9684991816600951

#define A2_4ji  -1.999633313803449
#define B2_4ji  0.9999999999999999
#define C2_4ji  -1.858097918647416
#define D2_4ji  0.8654599838007603


void IPC_hp_filterp(float *arys, int h)
{
    int i;
    float x3;
    static float x1old1=0.,x2old1=0.,x3old1=0.,x4old1=0.;
    static float x1old2=0.,x2old2=0.,x3old2=0.,x4old2=0.;

    static float prevout[OVERLAP];
    
    if(h ==0)
    {
	for(i=0; i<OVERLAP; i++)
	{
	    x3 = arys[i] + x1old1 * A1_4ji + x2old1 * B1_4ji
		- ( x3old1 * C1_4ji + x4old1 * D1_4ji );
	    x2old1=x1old1;
	    x1old1=arys[i];
	    x4old1=x3old1;
	    x3old1=x3;
	    arys[i]=x3;
	}
	for(i=0; i<OVERLAP; i++)
	{
	    x3 = arys[i] + x1old2 * A2_4ji + x2old2 * B2_4ji
		- (x3old2 * C2_4ji + x4old2 * D2_4ji);
	    x2old2=x1old2;
	    x1old2=arys[i];
	    x4old2=x3old2;
	    x3old2=x3;
	    arys[i]=x3 / INVGAIN_4ji;
	}
    }
    else
    {
	for(i=0;i<OVERLAP;i++)
	    arys[i]=prevout[i];
    }
    
    for(i=OVERLAP; i<SAMPLE; i++)
    {
	x3 = arys[i] + x1old1 * A1_4ji + x2old1 * B1_4ji
	    - ( x3old1 * C1_4ji + x4old1 * D1_4ji );
	x2old1=x1old1;
	x1old1=arys[i];
	x4old1=x3old1;
	x3old1=x3;
	arys[i]=x3;
    }
    for(i=OVERLAP; i<SAMPLE; i++)
    {
	x3 = arys[i] + x1old2 * A2_4ji + x2old2 * B2_4ji
	    - (x3old2 * C2_4ji + x4old2 * D2_4ji);
	x2old2=x1old2;
	x1old2=arys[i];
	x4old2=x3old2;
	x3old2=x3;
	arys[i]=x3 / INVGAIN_4ji;
    }
    
    for(i=0; i<OVERLAP ; i++)
	prevout[i] = arys[SAMPLE-OVERLAP+i];

}


void IPC_hp_filterp2(float *arys, int h)
{
    int i;
    float x3;
    static float x1old1=0.,x2old1=0.,x3old1=0.,x4old1=0.;
    static float x1old2=0.,x2old2=0.,x3old2=0.,x4old2=0.;

    static float prevout[OVERLAP];
    
    if(h ==0)
    {
	for(i=0; i<OVERLAP; i++)
	{
	    x3 = arys[i] + x1old1 * A1_4ji + x2old1 * B1_4ji
		- ( x3old1 * C1_4ji + x4old1 * D1_4ji );
	    x2old1=x1old1;
	    x1old1=arys[i];
	    x4old1=x3old1;
	    x3old1=x3;
	    arys[i]=x3;
	}
	for(i=0; i<OVERLAP; i++)
	{
	    x3 = arys[i] + x1old2 * A2_4ji + x2old2 * B2_4ji
		- (x3old2 * C2_4ji + x4old2 * D2_4ji);
	    x2old2=x1old2;
	    x1old2=arys[i];
	    x4old2=x3old2;
	    x3old2=x3;
	    arys[i]=x3 / INVGAIN_4ji;
	}
    }
    else
    {
	for(i=0;i<OVERLAP;i++)
	    arys[i]=prevout[i];
    }
    
    for(i=OVERLAP; i<SAMPLE; i++)
    {
	x3 = arys[i] + x1old1 * A1_4ji + x2old1 * B1_4ji
	    - ( x3old1 * C1_4ji + x4old1 * D1_4ji );
	x2old1=x1old1;
	x1old1=arys[i];
	x4old1=x3old1;
	x3old1=x3;
	arys[i]=x3;
    }
    for(i=OVERLAP; i<SAMPLE; i++)
    {
	x3 = arys[i] + x1old2 * A2_4ji + x2old2 * B2_4ji
	    - (x3old2 * C2_4ji + x4old2 * D2_4ji);
	x2old2=x1old2;
	x1old2=arys[i];
	x4old2=x3old2;
	x3old2=x3;
	arys[i]=x3 / INVGAIN_4ji;
    }
    
    for(i=0; i<OVERLAP ; i++)
	prevout[i] = arys[SAMPLE-OVERLAP+i];

}




typedef struct
{
    double	gainAdj;
    double	a[4], b[4];
}
CoefFltr4;

static CoefFltr4 cf4 = 
{
     1.1,

    {
        -1.998066423746901,
         1.000000000000000,
        -1.962822436245804,
         0.9684991816600951,
    },
     
   {
        -1.999633313803449,
         0.9999999999999999,
        -1.858097918647416,
        0.8654599838007603,
    }
};

typedef struct
{
    double	a[4], b[4];
}
StateFltr4;



static void hpf4ji(
                   float       *data,
                   int         size,
                   StateFltr4	*state,
                   CoefFltr4   *coef)
{
    int i;
    float x3;

    for(i = 0; i < size; i++)
    {
        x3 = data[i] + state->a[0] * coef->a[0] + state->a[1] * coef->a[1]
            - (state->a[2] * coef->a[2] + state->a[3] * coef->a[3]);
        state->a[1] = state->a[0];
        state->a[0] = data[i];
        state->a[3] = state->a[2];
        state->a[2] = x3;
        data[i] = x3;
    }
    for(i = 0; i < size; i++)
    {
        x3 = data[i] + state->b[0] * coef->b[0] + state->b[1] * coef->b[1]
            - (state->b[2] * coef->b[2] + state->b[3] * coef->b[3]);
        state->b[1] = state->b[0];
        state->b[0] = data[i];
        state->b[3] = state->b[2];
        state->b[2] = x3;
        data[i] = x3 / coef->gainAdj;
    }
}

static void ClearStateFltr4(StateFltr4 *state)
{
    int		i;

    for(i = 0; i < 4; i++)
    {
	state->a[i] = 0.0;
	state->b[i] = 0.0;
    }
}


void IPC_hp_filter4(
float	arys[SAMPLE],
int	frm)
{
    int		i;

    static StateFltr4	state;
    static float prevout[OVERLAP];

    if(frm == 0)
    {
	ClearStateFltr4(&state);
	hpf4ji(arys, OVERLAP, &state, &cf4);
    }
    else
    {
	for(i = 0; i < OVERLAP; i++)
	{
	    arys[i] = prevout[i];
	}
    }

    hpf4ji(&arys[OVERLAP], FRM, &state, &cf4);

    for(i = 0; i < OVERLAP; i++)
    {
	prevout[i] = arys[SAMPLE - OVERLAP + i];
    }
}

