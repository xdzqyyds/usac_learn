/***********************************************************************
MPEG-4 Audio RM Module
Parametric based codec - SSC (SinuSoidal Coding) bit stream Encoder

This software was originally developed by:
* Arno Peters, Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
* Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
* Werner Oomen, Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

And edited by:
*

in the course of development of the MPEG-4 Audio standard ISO-14496-1, 2 and 3. 
This software module is an implementation of a part of one or more MPEG-4 Audio
tools as specified by the MPEG-4 Audio standard. ISO/IEC gives users of the 
MPEG-4 Audio standards free licence to this software module or modifications 
thereof for use in hardware or software products claiming conformance to the 
MPEG-4 Audio standards. Those intending to use this software module in hardware
or software products are advised that this use may infringe existing patents.
The original developers of this software of this module and their company, 
the subsequent editors and their companies, and ISO/EIC have no liability for 
use of this software module or modifications thereof in an implementation. 
Copyright is not released for non MPEG-4 Audio conforming products. The 
original developer retains full right to use this code for his/her own purpose,
assign or donate the code to a third party and to inhibit third party from
using the code for non MPEG-4 Audio conforming products. This copyright notice
must be included in all copies of derivative works.

Copyright  2001.

Source file: AdpcmTable.c

Required libraries: <none>

Authors:
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
AG: Andy Gerrits, Philips Research Eindhoven, <andy.gerrits@philips.com>

Changes:
03 Oct 2003	AG	Initial version
************************************************************************/
#include "PlatformTypes.h"
#include "SSC_System.h"


#define INF   (999999999.0)

const Double SSC_ADPCM_TableRepr[22*4] =
  {
    -4.24264068711928, -1.06066017177982,  1.06066017177982, 4.24264068711928,
    -3.56762134500816, -0.89190533625204,  0.89190533625204, 3.56762134500816,
    -3.00000000000000, -0.75000000000000,  0.75000000000000, 3.00000000000000,
    -2.52268924576114, -0.63067231144029,  0.63067231144029, 2.52268924576114,
    -2.12132034355964, -0.53033008588991,  0.53033008588991, 2.12132034355964,
    -1.78381067250408, -0.44595266812602,  0.44595266812602, 1.78381067250408,
    -1.50000000000000, -0.37500000000000,  0.37500000000000, 1.50000000000000,
    -1.26134462288057, -0.31533615572014,  0.31533615572014, 1.26134462288057,
    -1.06066017177982, -0.26516504294496,  0.26516504294496, 1.06066017177982,
    -0.89190533625204, -0.22297633406301,  0.22297633406301, 0.89190533625204,
    -0.75000000000000, -0.18750000000000,  0.18750000000000, 0.75000000000000,
    -0.63067231144029, -0.15766807786007,  0.15766807786007, 0.63067231144029,
    -0.53033008588991, -0.13258252147248,  0.13258252147248, 0.53033008588991,
    -0.44595266812602, -0.11148816703151,  0.11148816703151, 0.44595266812602,
    -0.37500000000000, -0.09375000000000,  0.09375000000000, 0.37500000000000,
    -0.31533615572014, -0.07883403893004,  0.07883403893004, 0.31533615572014,
    -0.26516504294496, -0.06629126073624,  0.06629126073624, 0.26516504294496,
    -0.22297633406301, -0.05574408351575,  0.05574408351575, 0.22297633406301,
    -0.18750000000000, -0.04687500000000,  0.04687500000000, 0.18750000000000,
    -0.15766807786007, -0.03941701946502,  0.03941701946502, 0.15766807786007,
    -0.13258252147248, -0.03314563036812,  0.03314563036812, 0.13258252147248,
    -0.11148816703151, -0.02787204175788,  0.02787204175788, 0.11148816703151
  };


const Double SSC_ADPCM_TableQuant[22*5] =
  {
    -INF,  -2.12132034355964,   0,   2.12132034355964,  INF,
    -INF,  -1.78381067250408,   0,   1.78381067250408,  INF,
    -INF,  -1.50000000000000,   0,   1.50000000000000,  INF,
    -INF,  -1.26134462288057,   0,   1.26134462288057,  INF,
    -INF,  -1.06066017177982,   0,   1.06066017177982,  INF,
    -INF,  -0.89190533625204,   0,   0.89190533625204,  INF,
    -INF,  -0.75000000000000,   0,   0.75000000000000,  INF,
    -INF,  -0.63067231144029,   0,   0.63067231144029,  INF,
    -INF,  -0.53033008588991,   0,   0.53033008588991,  INF,
    -INF,  -0.44595266812602,   0,   0.44595266812602,  INF,
    -INF,  -0.37500000000000,   0,   0.37500000000000,  INF,
    -INF,  -0.31533615572014,   0,   0.31533615572014,  INF,
    -INF,  -0.26516504294496,   0,   0.26516504294496,  INF,
    -INF,  -0.22297633406301,   0,   0.22297633406301,  INF,
    -INF,  -0.18750000000000,   0,   0.18750000000000,  INF,
    -INF,  -0.15766807786007,   0,   0.15766807786007,  INF,
    -INF,  -0.13258252147248,   0,   0.13258252147248,  INF,
    -INF,  -0.11148816703151,   0,   0.11148816703151,  INF,
    -INF,  -0.09375000000000,   0,   0.09375000000000,  INF,
    -INF,  -0.07883403893004,   0,   0.07883403893004,  INF,
    -INF,  -0.06629126073624,   0,   0.06629126073624,  INF,
    -INF,  -0.05574408351575,   0,   0.05574408351575,  INF     
  };

const Double SSC_ADPCM_FF[5] = {0.0, 0.07123792865283, 0.14247585730566, 0.56990342922264, 3.14159265358979};
                     /* In Hertz: 0.0, 500.0, 1000.0, 4000.0, 22050.0 Hz */
const Double SSC_ADPCM_FS[5] = {8.0, 4.0, 2.0, 1.0};
const Double SSC_ADPCM_Q[5]  = {-INF, -1.5, 0, 1.5, INF};                /* Boundaries */  
const Double SSC_ADPCM_R[4]  = {-3.0, -0.75, 0.75, 3.0};      /* Representation levels */

const Double SSC_ADPCM_delta_min  = SSC_PI/64.0;
const Double SSC_ADPCM_delta_max  = (3.0 * SSC_PI)/4.0;

const Double SSC_ADPCM_P_h[2] = {2.0,  -1.0};  /* 1st order and 2nd order coder prediction coeff */
const Double SSC_ADPCM_P_m[2] = {0.84089641525371, 1.41421356237310};    
                 /* 2^(-1/4), 2^(1/2) multipliers for adaptive quantisation */                                        


