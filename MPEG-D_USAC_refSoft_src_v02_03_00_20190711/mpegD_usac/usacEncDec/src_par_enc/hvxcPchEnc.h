
/*

This software module was originally developed by

    Kazuyuki Iijima, Jun Matsumoto (Sony Corporation)

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

static float lpfCoefPI3per4[FILTER_ORDER + 1] =
{
    0.001996,
    0.000000,
    -0.006017,
    0.016404,
    -0.020660,
    0.000000,
    0.057623,
    -0.141664,
    0.218276,
    0.748085,
    0.218276,
    -0.141664,
    0.057623,
    0.000000,
    -0.020660,
    0.016404,
    -0.006017,
    0.000000,
    0.001996,
};

static float lpfCoefPIper2[FILTER_ORDER + 1] =
{
    0.002822,
    0.000000,
    -0.008508,
    0.000000,
    0.029212,
    0.000000,
    -0.081476,
    0.000000,
    0.308634,
    0.498634,
    0.308634,
    0.000000,
    -0.081476,
    0.000000,
    0.029212,
    0.000000,
    -0.008508,
    0.000000,
    0.002822,
};


static float lpfCoefPI3per8[FILTER_ORDER + 1] =
{
    -0.002620,
    0.000000,
    0.007899,
    0.011654,
    -0.011233,
    -0.049433,
    -0.031331,
    0.100641,
    0.286527,
    0.375794,
    0.286527,
    0.100641,
    -0.031331,
    -0.049433,
    -0.011233,
    0.011654,
    0.007899,
    0.000000,
    -0.002620,
};

static float lpfCoefPIper4[FILTER_ORDER + 1] =
{
    0.001991,
    0.000000,
    -0.006003,
    -0.016366,
    -0.020612,
    0.000000,
    0.057490,
    0.141337,
    0.217772,
    0.248785,
    0.217772,
    0.141337,
    0.057490,
    0.000000,
    -0.020612,
    -0.016366,
    -0.006003,
    0.000000,
    0.001991,
};


static float lpfCoefPIper8[FILTER_ORDER + 1] =
{
    -0.001211,
    0.000000,
    0.003652,
    0.013007,
    0.030268,
    0.055172,
    0.084422,
    0.112325,
    0.132462,
    0.139807,
    0.132462,
    0.112325,
    0.084422,
    0.055172,
    0.030268,
    0.013007,
    0.003652,
    0.000000,
    -0.001211,
};

static float hpfCoefPI3per4[FILTER_ORDER + 1] =
{
    -0.001193,
    0.000000,
    0.003596,
    0.009803,
    0.012346,
    0.000000,
    -0.034435,
    -0.084658,
    -0.130442,
    0.447054,
    -0.130442,
    -0.084658,
    -0.034435,
    0.000000,
    0.012346,
    0.009803,
    0.003596,
    0.000000,
    -0.001193,
};

static float hpfCoefPI4per5[FILTER_ORDER + 1] =
{
    0.000982,
    0.002406,
    0.004789,
    0.005705,
    0.000000,
    -0.017112,
    -0.045858,
    -0.079719,
    -0.107358,
    0.472144,
    -0.107358,
    -0.079719,
    -0.045858,
    -0.017112,
    0.000000,
    0.005705,
    0.004789,
    0.002406,
    0.000982,
};

static float hpfCoefPI7per8[FILTER_ORDER + 1] =
{
    0.000657,
    0.000000,
    -0.001981,
    -0.007055,
    -0.016417,
    -0.029925,
    -0.045790,
    -0.060924,
    -0.071846,
    0.530812,
    -0.071846,
    -0.060924,
    -0.045790,
    -0.029925,
    -0.016417,
    -0.007055,
    -0.001981,
    0.000000,
    0.000657,
};


static float hpfCoefPI1per8[FILTER_ORDER + 1] =
{
    0.001205,
    0.000000,
    -0.003634,
    0.012944,
    -0.030122,
    0.054906,
    -0.084015,
    0.111783,
    -0.131824,
    0.139133,
    -0.131824,
    0.111783,
    -0.084015,
    0.054906,
    -0.030122,
    0.012944,
    -0.003634,
    0.000000,
    0.001205,
};

static float hpfCoefPI1per4[FILTER_ORDER + 1] =
{
-0.001699,
0.000000,
0.005123,
-0.013965,
0.017588,
0.000000,
-0.049056,
0.120602,
-0.185824,
0.212287,
-0.185824,
0.120602,
-0.049056,
0.000000,
0.017588,
-0.013965,
0.005123,
0.000000,
-0.001699,
};

