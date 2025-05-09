#ifndef __PS_IPD_DEC_H
#define __PS_IPD_DEC_H

#define	NO_IPD_BINS		8

#ifndef	P_PI
#define P_PI            3.1415926535897932
#endif

void  SplitAngle( float fAmpL, float fAmpR, float fAngle, float* pAngleL, float* pAngleR);
void  RotateFreq( float* pReal, float* pImag, float fAddAngle);

#endif
