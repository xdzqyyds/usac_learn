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




#include "sac_mdct2qmf_wf_tables.h"



const float wf_02[02] =
{
     2.25159422791899e-001f,
     9.75300216421083e-001f
};

const float wf_03[03] =
{
    3.776593725453505e-001f,
    9.260029738839094e-001f,
    1.000000000000000e+000f 
};

const float wf_04[04] =
{
     6.96503775627941e-002f,
     4.61853083731484e-001f,
     8.86894602377496e-001f,
     9.98790120205249e-001f
};

const float wf_15[15] =
{
    3.108607658174681e-002f,
    7.684574106763581e-002f,
    1.508354429213179e-001f,
    2.531644925199998e-001f,
    3.776593725453505e-001f,
    5.130597832310549e-001f,
    6.458917069517359e-001f,
    7.638311369132910e-001f,
    8.583559874082372e-001f,
    9.260029738839094e-001f,
    9.681920012517699e-001f,
    9.899965908597906e-001f,
    9.983403183137536e-001f,
    1.000033158322195e+000f,
    1.000000000000000e+000f
};

const float wf_16[16] =
{
     1.65475125062983e-002f,
     4.65240767348341e-002f,
     9.89602832916821e-002f,
     1.76902905856576e-001f,
     2.78831786952801e-001f,
     3.98430480799321e-001f,
     5.25831178519259e-001f,
     6.49858917237363e-001f,
     7.60468046976490e-001f,
     8.50620915105811e-001f,
     9.17198211446560e-001f,
     9.60923129495122e-001f,
     9.85537849989136e-001f,
     9.96542721419397e-001f,
     9.99771769707441e-001f,
     1.00003374095677e+000f
};

const float wf_18[18] =
{
     1.54272357075635e-002f,
     4.01056583972472e-002f,
     8.18912638533254e-002f,
     1.43547011618227e-001f,
     2.25159422791899e-001f,
     3.23727710751240e-001f,
     4.33511567516425e-001f,
     5.47034164787458e-001f,
     6.56436993346885e-001f,
     7.54811649929518e-001f,
     8.37198606574045e-001f,
     9.01092348426438e-001f,
     9.46449282032434e-001f,
     9.75300216421083e-001f,
     9.91106019492545e-001f,
     9.97984869463190e-001f,
     9.99917752053543e-001f,
     1.00002790373369e+000f
};

const float wf_24[24] =
{
     1.33344898119880e-002f,
     2.90607489815381e-002f,
     5.35733084899849e-002f,
     8.84809749081922e-002f,
     1.34720976611448e-001f,
     1.92339751279503e-001f,
     2.60391996391985e-001f,
     3.36979154288995e-001f,
     4.19421306618108e-001f,
     5.04531772744345e-001f,
     5.88947180873229e-001f,
     6.69460465973682e-001f,
     7.43309991463507e-001f,
     8.08391818789828e-001f,
     8.63379473642823e-001f,
     9.07751738914918e-001f,
     9.41740717799601e-001f,
     9.66218394063355e-001f,
     9.82540921463216e-001f,
     9.92367937267032e-001f,
     9.97471588213559e-001f,
     9.99548248354220e-001f,
     1.00004547262230e+000f,
     1.00001684978847e+000f
};

const float wf_30[30] =
{
     1.21701874987223e-002f,
     2.35062123092812e-002f,
     4.01056583972472e-002f,
     6.28961453327895e-002f,
     9.25867277500073e-002f,
     1.29578041287072e-001f,
     1.73896248044280e-001f,
     2.25159422791899e-001f,
     2.82580915231516e-001f,
     3.45009538562499e-001f,
     4.11001924411292e-001f,
     4.78918737883468e-001f,
     5.47034164787458e-001f,
     6.13647398093030e-001f,
     6.77185703487687e-001f,
     7.36290721083768e-001f,
     7.89882478441777e-001f,
     8.37198606574045e-001f,
     8.77808975766847e-001f,
     9.11608053596863e-001f,
     9.38788574477624e-001f,
     9.59800629511943e-001f,
     9.75300216421083e-001f,
     9.86090888708778e-001f,
     9.93061666985504e-001f,
     9.97124011928831e-001f,
     9.99150489219049e-001f,
     9.99917752053543e-001f,
     1.00005651297348e+000f,
     1.00001112602121e+000f
};

const float wf_32[32] =
{
     1.18894978514784e-002f,
     2.22355013318738e-002f,
     3.71254015544489e-002f,
     5.73415208375520e-002f,
     8.35091312497429e-002f,
     1.16028814280589e-001f,
     1.55023452985960e-001f,
     2.00305931728145e-001f,
     2.51371327015577e-001f,
     3.07414627534250e-001f,
     3.67372216087139e-001f,
     4.29982868367448e-001f,
     4.93862191842022e-001f,
     5.57583444043476e-001f,
     6.19757592535477e-001f,
     6.79106226817397e-001f,
     7.34522308134036e-001f,
     7.85115477922992e-001f,
     8.30240452578559e-001f,
     8.69508657445507e-001f,
     9.02784515399660e-001f,
     9.30168618388376e-001f,
     9.51970382536667e-001f,
     9.68672804668346e-001f,
     9.80891731819072e-001f,
     9.89331765860614e-001f,
     9.94740668245914e-001f,
     9.97863972503918e-001f,
     9.99401464920565e-001f,
     9.99967216813160e-001f,
     1.00005487312988e+000f,
     1.00000984349249e+000f
};

float const * wf[MAX_UPD_QMF] =
{
    NULL,
    wf_02,
    wf_03,
    wf_04,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    wf_15,
    wf_16,
    NULL,
    wf_18,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    wf_24,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    wf_30,
    NULL,
    wf_32
};



