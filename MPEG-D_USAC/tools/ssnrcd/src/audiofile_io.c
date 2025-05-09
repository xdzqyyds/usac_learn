/*

This software module was originally developed by 

Ralf Funken (Philips, Eindhoven, The Netherlands) 

and edited by 

Takehiro Moriya (NTT Labs. Tokyo, Japan) 
Ralph Sperschneider (Fraunhofer IIS, Germany)

in  the course of development of  the  MPEG-2/MPEG-4 Conformance ISO/IEC
13818-4, 14496-4.  This software module is  an implementation  of one or
more  of  the  Audio Conformance tools   as   specified by MPEG-2/MPEG-4
Conformance. ISO/IEC gives users of the MPEG-2 AAC/MPEG-4 Audio (ISO/IEC
13818-3,7,  14496-3)  free   license    to this   software    module  or
modifications  thereof for use in   hardware or software products. Those
intending to use this  software module in  hardware or software products
are  advised that its  use may  infringe  existing patents. The original
developer of this software  module  and his/her company, the  subsequent
editors  and their companies,  and ISO/IEC have  no liability for use of
this    software     module   or     modifications    thereof   in    an
implementation.  Philips retains full right  to use the code for his/her
own purpose, assign or donate the code to  a third party. This copyright
notice must be  included in all   copies or derivative works.  Copyright
2000, 2009.

*/

#include <stdio.h>
#include <string.h>

#include "libtsp.h"
#include "libtsp/AFpar.h"

#include "common.h"
#include "audiofile_io.h"


/***************************************************************************/

ERRORCODE Waveform_OpenInput (AUDIOFILE* self)
{
  ERRORCODE status = OKAY;

  self->m_stream = AFopenRead (self->m_filename, 
                               &(self->m_nrofsamples), 
                               &(self->m_channels), 
                               &(self->m_samplefreq),
                               NULL);
  
  if (self->m_stream == NULL) {
    status = FILE_OPEN_ERROR;
  }
  else {
    self->m_nrofsamples = self->m_nrofsamples / self->m_channels;
        
    switch ((self->m_stream)->Format) {
    case    FD_MULAW8   :   self->m_bits_per_sample =  8;
      break;
    case    FD_ALAW8    :   self->m_bits_per_sample =  8;
      break;
    case    FD_UINT8    :   self->m_bits_per_sample =  8;
      break;
    case    FD_INT16    :   self->m_bits_per_sample = 16;
      break;
    case    FD_INT24    :   self->m_bits_per_sample = 24;
      break;
    case    FD_INT32    :   self->m_bits_per_sample = 32;
      break;
    case    FD_FLOAT32  :   self->m_bits_per_sample = 32;
      break;
    case    FD_FLOAT64  :   self->m_bits_per_sample = 64;
      break;
    default             :   self->m_bits_per_sample =  0;
      break;
    }
  }
  return status;
}

/***************************************************************************/

void Waveform_CloseInput (AUDIOFILE *self)
{
  if (self->m_stream != NULL) {
    AFclose (self->m_stream);
  }
}

/***************************************************************************/

ERRORCODE Waveform_CheckHeader (AUDIOFILE *self)
{

  ERRORCODE status = OKAY;

  if (self->m_channels > MAX_PCM_CHANNELS) {
    status = WAVEFORM_HEADER_ERROR;
  }

  if (self->m_nrofsamples < 1) {
    status = WAVEFORM_HEADER_ERROR;
  }

  if (self->m_bits_per_sample == 0) {
    status = WAVEFORM_HEADER_ERROR;
  }

  return status;
}

/***************************************************************************/

int Waveform_GetSample (double     pcm [],
                        AFILE*     streampointer, 
                        long int   Nchannels, 
                        long int   offset,
                        int        segment_length)
{
  int     samples_read;

  samples_read = AFdReadData (streampointer, 
                              Nchannels * offset, 
                              pcm, 
                              Nchannels * segment_length);
    
  samples_read /= Nchannels;
    
  return samples_read;
}

/***************************************************************************/

void Waveform_CopySample (double    target[], 
                          double    source[],
                          long int  ch, 
                          long int  n_ch, 
                          int       segment_length)
{
  int n;

  for (n = 0; n < segment_length; n++)
    {
      target [n] = source [(n * n_ch) + ch];
    }
}

/***************************************************************************/

void Waveform_ApplyZeropadding (double    pcm [], 
                                long int  Nchannels, 
                                int       start, 
                                int       end)
{
  int         n;
  long int    ch;

  for (ch = 0; ch < Nchannels; ch++)
    {
      for (n = start; n < end; n++)
        {
          pcm [(n * Nchannels) + ch] = 0.0F;
        }
    }
}

/***************************************************************************/
