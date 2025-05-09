/****************************************************************************
 
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Coding Technologies

and edited by
  -

in the course of development of the ISO/IEC 13818-7 and ISO/IEC 14496-3 
standards for reference purposes and its performance may not have been 
optimized. This software module is an implementation of one or more tools as 
specified by the ISO/IEC 13818-7 and ISO/IEC 14496-3 standards.
ISO/IEC gives users free license to this software module or modifications 
thereof for use in products claiming conformance to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International 
Standards. ISO/IEC gives users the same free license to this software module or 
modifications thereof for research purposes and further ISO/IEC standardisation.
Those intending to use this software module in products are advised that its 
use may infringe existing patents. ISO/IEC have no liability for use of this 
software module or modifications thereof. Copyright is not released for 
products that do not conform to audiovisual and image-coding related ITU 
Recommendations and/or ISO/IEC International Standards.
The original developer retains full right to modify and use the code for its 
own purpose, assign or donate the code to a third party and to inhibit third 
parties from using the code for products that do not conform to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International Standards.
This copyright notice must be included in all copies or derivative works.
Copyright (c) ISO/IEC 2002.

 $Id: ct_sbrcrc.c,v 1.1.1.1 2009-05-29 14:10:16 mul Exp $

*******************************************************************************/
/*!
  \file
  \brief  CRC check coutines $Revision: 1.1.1.1 $
*/
#include "ct_sbrcrc.h"
#include "ct_sbrbitb.h"
#include "ct_sbrconst.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef struct
{
  unsigned short crcState;
  unsigned short crcMask;
  unsigned short crcPoly;
}
CRC_BUFFER;

typedef CRC_BUFFER *HANDLE_CRC;

const unsigned short MAXCRCSTEP = 16;

/*******************************************************************************
 Functionname:calcCRC
 *******************************************************************************

 Description: crc calculation

 Arguments:   

 Return:      none

*******************************************************************************/

static unsigned long
calcCRC (HANDLE_CRC hCrcBuf, unsigned long bValue, int nBits)
{
  int i;
  unsigned long bMask = (1UL << (nBits - 1));

  for (i = 0; i < nBits; i++, bMask >>= 1) {
    unsigned short flag = (hCrcBuf->crcState & hCrcBuf->crcMask) ? 1 : 0;
    unsigned short flag1 = (bMask & bValue) ? 1 : 0;

    flag ^= flag1;
    hCrcBuf->crcState <<= 1;
    if (flag)
      hCrcBuf->crcState ^= hCrcBuf->crcPoly;
  }

  return (hCrcBuf->crcState);
}

/*******************************************************************************
 Functionname:getCrc
 *******************************************************************************

 Description: crc 

 Arguments:   

 Return:      none

*******************************************************************************/


static int
getCrc (HANDLE_BIT_BUFFER hBitBuf, unsigned long NrBits)
{

  int i;
  CRC_BUFFER CrcBuf;


  int CrcStep = NrBits / MAXCRCSTEP;
  int CrcNrBitsRest = (NrBits - CrcStep * MAXCRCSTEP);
  unsigned long bValue;

  CrcBuf.crcState = CRCSTART;
  CrcBuf.crcMask  = CRCMASK;
  CrcBuf.crcPoly  = CRCPOLY;

  for (i = 0; i < CrcStep; i++) {
    bValue = BufGetBits (hBitBuf, MAXCRCSTEP);
    calcCRC (&CrcBuf, bValue, MAXCRCSTEP);
  }

  bValue = BufGetBits (hBitBuf, CrcNrBitsRest);
  calcCRC (&CrcBuf, bValue, CrcNrBitsRest);

  return (CrcBuf.crcState & CRCRANGE);

}

/*******************************************************************************
 Functionname:SbrCrcCheck
 *******************************************************************************

 Description: crc interface

 Arguments:   

 Return:      none

*******************************************************************************/


int
SbrCrcCheck (HANDLE_BIT_BUFFER hBitBuf, unsigned long NrBits)
{
  int crcResult = 1;
  BIT_BUFFER BitBufferCRC;
  unsigned long NrCrcBits;
  unsigned long crcCheckResult;

  unsigned long crcCheckSum;

  crcCheckSum = BufGetBits (hBitBuf, SI_SBR_CRC_BITS);
  
  CopyBitbufferState (hBitBuf, &BitBufferCRC);

  NrCrcBits = min (NrBits, GetNrBitsAvailable (&BitBufferCRC));
  crcCheckResult = getCrc (&BitBufferCRC, NrCrcBits);
  
  if (crcCheckResult != crcCheckSum) {
    crcResult = 0;
  }

  return (crcResult);
}
