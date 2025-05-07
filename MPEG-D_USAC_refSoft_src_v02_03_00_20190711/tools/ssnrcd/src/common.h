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


#ifndef COMMON_H
#define COMMON_H


/*****************************************************************************/

/*     general constants    */             

#define         TRUE                            (1)
#define         FALSE                           (0)
                                                 
#define         MAX_STRING_LENGTH               (256)
                              
#define         MAX_PCM_CHANNELS                (64)
#define         DEFAULT_DELAY                   (0)     /* default delay (offset between input files) in samples    */

#define         GREP_STRING                     "==="
                                                
                                                              /*    instability for low rank matrices               */


/*  macros for min and max                      */

#ifndef MIN
#define         MIN(a,b)                        (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define         MAX(a,b)                        (((a) > (b)) ? (a) : (b))
#endif

/*  other macros                                */

#ifndef LD /* logarithmus dualis */
#define         LD(a)                           log10(a)/log10(2.0)
#endif


typedef enum {
  OKAY = 0,
  MEMORYALOCATION_ERROR,
  FILE_OPEN_ERROR,
  FILE_LENGTH_ERROR,
  WAVEFORM_HEADER_ERROR,
  CMDLINE_PARSE_ERROR,
  SAMPLING_RATE_ERROR,
  PNS_SPECTRALCONFORMANCE_ERROR,
  PNS_TEMPORAL_CONFORMANCE_ERROR,
  INIT_TEMPORAL_CONFORMANCE_ERROR,
  PNS_SETWINDOWSIZE_ERROR,
  PNS_TESTMODE_ERROR,
  PNS_WINDOWSIZE_ERROR,
  RMS_CALCULATION_ERROR,
  SSNRCD_FULL_ACCURACY_ERROR,
  SSNRCD_LIMITED_ACCURACY_ERROR
} ERRORCODE;

/*****************************************************************************/

typedef char   string [MAX_STRING_LENGTH];


/***************************************************************************/


#endif    /*    COMMON_H    */


