/*

  ====================================================================
  Copyright Header            
  ====================================================================
  
  This software module was originally developed by 

  Ralf Funken (Philips, Eindhoven, The Netherlands) 

  and edited by 

  Takehiro Moriya (NTT Labs. Tokyo, Japan) 
  Ralph Sperschneider (Fraunhofer IIS, Germany)

  in the course of  development of the MPEG-2/MPEG-4 Conformance ISO/IEC
  13818-4, 14496-4. This software module is  an implementation of one or
  more of  the Audio Conformance  tools  as specified  by  MPEG-2/MPEG-4
  Conformance. ISO/IEC gives users     of the MPEG-2    AAC/MPEG-4 Audio
  (ISO/IEC  13818-3,7, 14496-3) free license to  this software module or
  modifications thereof for use in hardware  or software products. Those
  intending to use this software module in hardware or software products
  are advised that  its use may  infringe existing patents. The original
  developer of this software  module and his/her company, the subsequent
  editors and their companies, and ISO/IEC  have no liability for use of
  this      software   module    or   modifications      thereof   in an
  implementation. Philips retains full right to use the code for his/her
  own  purpose,  assign or   donate  the  code  to  a third  party. This
  copyright  notice  must  be included   in   all copies  or  derivative
  works. Copyright 2000, 2009.

*/


#ifndef __COMMON_STRUCTS_H__
#define __COMMON_STRUCTS_H__


#include "audiofile_io.h"

/*****************************************************************************/

typedef struct {
  AUDIOFILE      m_waveformReference;
  AUDIOFILE      m_waveformUnderTest;
  long int       m_numberOfChannels;
  float          m_samplingFrequency;
  unsigned long  m_samplesToCompare;
  signed long    m_delay;
} WAVEFILES;

/***************************************************************************/


#endif    /*    __COMMON_STRUCTS_H__    */
