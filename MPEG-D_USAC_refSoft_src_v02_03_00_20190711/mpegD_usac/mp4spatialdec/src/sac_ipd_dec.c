#include <math.h>
#include <stdio.h>

#include "sac_ipd_dec.h"

const float QuantTableIPDOPD[NO_IPD_BINS] =  
{
   0,                0.78539816339745f, 1.57079632679490f, 2.35619449019234f, 
   3.14159265358979f, 3.92699081698724f, 4.71238898038469f, 5.49778714378214f 
};


#define FLOAT_EPSILON                   0.00000001f


void SplitAngle( float fAmpL, float fAmpR, float fAngle, float* pAngleL, float* pAngleR)
{
  
  *pAngleL = (float)(atan2( fAmpR * sin( fAngle ), ( fAmpL + fAmpR * cos( fAngle ))));
  *pAngleR = *pAngleL-fAngle;
  
  if(fabs(fabs(fAngle)-QuantTableIPDOPD[4])<0.1 && fAmpL==fAmpR){
    *pAngleL = 0.;
    *pAngleR = fAngle;
  }
}


static float 
GetAngle( float fReal, float fImag)
{
  return (float)atan2( fImag, fReal );
}

void RotateFreq( float* pReal, float* pImag, float fAddAngle)
{
  float fAmp   = (float)sqrt( (*pReal)*(*pReal) + (*pImag)*(*pImag) );
  float fAngle = GetAngle( *pReal, *pImag ) + fAddAngle;
  
  *pReal = (float)(fAmp*cos( fAngle));
  *pImag = (float)(fAmp*sin( fAngle));
}

