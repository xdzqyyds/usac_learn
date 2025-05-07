/*

====================================================================
                        Copyright Header            
====================================================================

This  software module was originally  developed by 

Ralf Funken (Philips,Eindhoven, The  Netherlands)   

and  edited by

Takehiro  Moriya   (NTT Labs. Tokyo,  Japan) 
Ralph Sperschneider (Fraunhofer IIS, Germany)

in the course  of development of  the MPEG-2/MPEG-4  Conformance ISO/IEC
13818-4, 14496-4.  This software module  is an implementation of one  or
more of the  Audio   Conformance  tools as specified   by  MPEG-2/MPEG-4
Conformance. ISO/IEC gives users of the MPEG-2 AAC/MPEG-4 Audio (ISO/IEC
13818-3,7,  14496-3)  free  license     to   this software  module    or
modifications thereof  for use in  hardware or software products.  Those
intending  to use this software  module in hardware or software products
are advised that its use   may infringe existing patents.  The  original
developer of this  software module and  his/her company,  the subsequent
editors and  their companies, and  ISO/IEC have no  liability for use of
this     software   module    or     modifications   thereof      in  an
implementation. Philips  retains full right to  use the code for his/her
own purpose, assign or donate the code to  a third party. This copyright
notice must   be included in all copies   or derivative works. Copyright
2000, 2009.

*/


#ifndef FILE_IO_H
#define FILE_IO_H


#include <stdio.h>

#include "libtsp.h"
#include "libtsp/AFpar.h"

#include "common.h"

/***************************************************************************/

typedef struct {
  long int       m_channels;
  float          m_samplefreq; /* Hz */
  long int       m_nrofsamples;
  int            m_bits_per_sample;
  string         m_filename;
  AFILE*         m_stream;
  long           m_offset;
} AUDIOFILE;

/***************************************************************************/

ERRORCODE Waveform_OpenInput (AUDIOFILE* self);

void Waveform_CloseInput (AUDIOFILE *self);

ERRORCODE Waveform_CheckHeader (AUDIOFILE *self);

int Waveform_GetSample (double     pcm[],
                        AFILE*     streampointer, 
                        long int   Nchannels, 
                        long int   offset,
                        int        segment_length);

void Waveform_CopySample (double    target[], 
                          double    source[],
                          long int  ch, 
                          long int  n_ch, 
                          int       segment_length);

void Waveform_ApplyZeropadding (double    pcm[], 
                                long int  Nchannels,
                                int       start, 
                                int       end);

/***************************************************************************/


#endif   /*   FILE_IO_H   */
