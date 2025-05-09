/*
  ====================================================================
  Copyright Header            
  ====================================================================
  
  This software module was originally developed by 
  
  Manuela Schinn (Fraunhofer IIS, Germany)
  
  and edited by 
  
  Ralph Sperschneider (Fraunhofer IIS, Germany)
  
  in the course of  development of the MPEG-2/MPEG-4 Conformance ISO/IEC
  13818, 14496. This software module is an implementation of one or more
  of  the   Audio   Conformance tools    as  specified  by MPEG-2/MPEG-4
  Conformance. ISO/IEC gives  users  of   the MPEG-2 AAC/MPEG-4    Audio
  (ISO/IEC 13818,  14496)  free  license  to  this  software  module  or
  modifications thereof for use in hardware  or software products. Those
  intending to use this software module in hardware or software products
  are advised that its use may infringe  existing patents.  The original
  developer of this software  module and his/her company, the subsequent
  editors and their companies, and ISO/IEC have  no liability for use of
  this software module or modifications   thereof in an  implementation.
  Fraunhofer  IIS  retains full  right to use   the code for his/her own
  purpose, assign or  donate the code to  a third party.  This copyright
  notice must be  included in all copies  or derivative works. Copyright
  2008, 2009.

*/



/*  Standard includes                           */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*  Includes for AFsp Library                   */

#include "libtsp.h"
#include "libtsp/AFpar.h"

/*  Private includes                            */

#include "common.h"
#include "audiofile_io.h"
#include "ssnrcd.h"
#include "pns.h"

/*****************************************************************************/

/*     general constants    */             

#define         MODULE_NAME_FRAMEWORK           "FRAMEWORK"
#define         VERSION_FRAMEWORK               "4.0"

#define         MODULE_NAME_SSNRCD              "SSNRCD (Segmental SNR / Cepstral Distortion Analysis)"
#define         VERSION_SSNRCD                  "3.0"

#define         MODULE_NAME_RMS                 "RMS (Root Mean Square / Maximum Absolute Difference Analysis)"
#define         VERSION_RMS                     "3.0"

#define         MODULE_NAME_PNS                 "PNS (Perceptual Noise Substitution Analysis)"
#define         VERSION_PNS                     "2.0"

#define         EXECUTABLE_NAME_1               "ssnrcd"
#define         EXECUTABLE_NAME_2               "mpegaudioconf"
                                                 

/***************************************************************************/

typedef struct conf_obj * conf_ptr;

struct conf_obj
{
  ssnrcd_ptr              m_ssnrcdData;
  pns_ptr                 m_pnsData;
    
  /* chosen tests */
  int                     m_pns;
  int                     m_ssnrcd;
    
  int                     m_verbose;
};

/***************************************************************************/


static conf_ptr confDataInit ( void )
{  
  conf_ptr self;
    
  /* allocate memory */
  if(! ( self = (conf_ptr) calloc(1,sizeof(struct conf_obj))) )
    return NULL;
          
        
  /* chosen test(s) */
  self->m_ssnrcd      = TRUE;
  self->m_pns         = FALSE;

  self->m_ssnrcdData  = Ssnrcd_Init();
  self->m_pnsData     = Pns_Init();

  return self;
}


/***************************************************************************/

static void confDataFree (WAVEFILES* wavefiles,
                          conf_ptr   self)
{
  Waveform_CloseInput(&(wavefiles->m_waveformReference));
  Waveform_CloseInput(&(wavefiles->m_waveformUnderTest));
    
  Pns_Free   (self->m_pnsData);
  Ssnrcd_Free(self->m_ssnrcdData);
    
  free(self);
}


/***************************************************************************/

static void AudioFileInit(AUDIOFILE *self)
{
  self->m_channels        = 0;
  self->m_samplefreq      = 0;
  self->m_nrofsamples     = 0;
  self->m_bits_per_sample = 0;
  self->m_stream          = NULL;

  strcpy (self->m_filename, "");
}

/***************************************************************************/

static void WavefilesInit(WAVEFILES *self)
{
  AudioFileInit(&(self->m_waveformReference));
  AudioFileInit(&(self->m_waveformUnderTest));

  self->m_numberOfChannels  = 0;
  self->m_samplingFrequency = 0;
  self->m_samplesToCompare  = 0;
  self->m_delay             = DEFAULT_DELAY;
}

/***************************************************************************/

static void print_banner (void)
{
  fprintf(stderr, "\n");
  fprintf(stderr, "*************************** MPEG-4/D/H Audio Coder ****************************\n");
  fprintf(stderr, "*                                                                             *\n");
  fprintf(stderr, "*                      Audio Conformance Verification Tool                    *\n");
  fprintf(stderr, "*                                                                             *\n");
  fprintf(stderr, "*                                  %s                                *\n", __DATE__);
  fprintf(stderr, "*                                                                             *\n");
  fprintf(stderr, "*******************************************************************************\n");

  fprintf (stderr, "\n");
  fprintf (stderr, "  version %s of: %s\n", VERSION_FRAMEWORK, MODULE_NAME_FRAMEWORK);
  fprintf (stderr, "  version %s of: %s\n", VERSION_SSNRCD, MODULE_NAME_SSNRCD);
  fprintf (stderr, "  version %s of: %s\n", VERSION_RMS, MODULE_NAME_RMS);
  fprintf (stderr, "  version %s of: %s\n", VERSION_PNS, MODULE_NAME_PNS);
  fprintf (stderr, "\n");
}

/***************************************************************************/

static void show_usage (void)
{
  print_banner ();
  fprintf (stderr, "Usage:\n %s|%s [options] <reference wav> <wav under test>\n", EXECUTABLE_NAME_1, EXECUTABLE_NAME_2);
  fprintf (stderr, "\n");
  fprintf (stderr, "general options:\n");
  
  fprintf (stderr, "                -d <num>     Time-offset in samples between reference inputfile\n");
  fprintf (stderr, "                             and inputfile under test. Positive value means\n");
  fprintf (stderr, "                             inputfile under test is delayed over <num> samples\n");
  fprintf (stderr, "                             and the first <num>  samples of that input file\n");
  fprintf (stderr, "                             under test are skipped. Negative values are.\n");
  fprintf (stderr, "                             permitted.\n");
  fprintf (stderr, "                             Default: %d samples.\n", DEFAULT_DELAY);
  
  fprintf (stderr, "                -v [<num>]   Verbose level.\n");
  fprintf (stderr, "                               1 : Progress report (default)\n");
  fprintf (stderr, "                               2 : Show segments with accuracy below K bit\n");
  fprintf (stderr, "                                   resolution (see -k option)\n");
  fprintf (stderr, "                               3 : Samples with accuracy below K bit\n");
  fprintf (stderr, "                                   resolution (see -k option)\n");
  fprintf (stderr, "                               4 : Show accuracy of all samples\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Note: Only samples being available in both files are evaluated - samples being\n");
  fprintf (stderr, "      available only in one waveform will be ignored\n");
  fprintf (stderr, "\n");

  Ssnrcd_Usage();

  fprintf (stderr, "PNS related options:\n");

  fprintf (stderr, "                -p <num> <frameSize> PNS conformance test\n");
  fprintf (stderr, "                        <num>:\n");
  fprintf (stderr, "                               1 : PNS-1 (spectral PNS test for long  blocks)\n");
  fprintf (stderr, "                               2 : PNS-2 (spectral PNS test for short blocks)\n");
  fprintf (stderr, "                               3 : PNS-3 (temporal PNS test for short blocks)\n");
  fprintf (stderr, "                        <frameSize>:\n");
  fprintf (stderr, "                               480 : for testCase 1\n");
  fprintf (stderr, "                               512 : for testCase 1\n");
  fprintf (stderr, "                               960 : for testCase 1|2|3\n");
  fprintf (stderr, "                              1024 : for testCase 1|2|3\n");
    
}


/***************************************************************************/

static ERRORCODE parse_commandline (WAVEFILES* wavefiles,
                                    conf_ptr   self,
                                    int        argc,
                                    char**     argv)

{
  int       i, l, j;
  int       option;
  string    c;
  int       d;
  int       pns_test;
  int       frame_size;
  string*   inputfilename1 = &(wavefiles->m_waveformReference.m_filename);
  string*   inputfilename2 = &(wavefiles->m_waveformUnderTest.m_filename);

    
  /*  n = number of command line arguments  */
  if (argc == 1) { 
    goto bail;
  }
  else { 
    for (i = 1; i < argc; i++) {
      strcpy (c, argv[i]);

      if (c[0] == '-') {
        l = strlen (c);
        for (j = 1; j < l; j++) {
          option = (char) (c[j]);
          switch (option) {
          case 'h': 
            goto bail;
            break;

          case 'a': 
            Ssnrcd_SetShowAccuracyDistribution(self->m_ssnrcdData, TRUE);
            break;
                         
          case 'd': if (i < (argc - 1)) {
            (wavefiles->m_delay) = atol (argv[++i]);
          }
          break;    

          case 'e': 
            Ssnrcd_SetExponentialOutput(self->m_ssnrcdData, TRUE);
            if (i < (argc - 1)) {
              d = atol (argv[++i]);
              if (d > 0) {
                Ssnrcd_SetBitsResolution(self->m_ssnrcdData, d);
              }
            }
            break;
                         
          case 'k': if (i < (argc - 1)) {
            d = atol (argv[++i]);
            Ssnrcd_SetBitsResolution(self->m_ssnrcdData, d);
          }
          break;

          case 's': if (i < (argc - 1)) {
            d = atol (argv[++i]);
            Ssnrcd_SetSegSnrThreshold(self->m_ssnrcdData, d);
          }
          break;

          case 'l': if (i < (argc - 1)) {
            d = atol (argv[++i]);
            if (d > 0) {
              Ssnrcd_SetSegmentLength(self->m_ssnrcdData, d);
            }
          }
          break;    

          case 't':
            if (i < (argc - 1)) {
              d = atol (argv[++i]);
              if (d > 0) {
                Ssnrcd_SetApplyCriteria(self->m_ssnrcdData, (unsigned int) d);
              }
            }
            break;

          case 'v': self->m_verbose = 1;
            if (i < (argc - 1)) {
              d = atol (argv[++i]);
              if (d > 0) {
                self->m_verbose = (unsigned int) d;
              }
            }
            break;
                                
          case 'p': self->m_pns = TRUE;
            self->m_ssnrcd = FALSE;
            if (i < (argc - 1)) {
              /* PNS-testmode */
              pns_test = atoi (argv[++i]);
              if ( ( pns_test > 0 ) && ( pns_test < 4 ) ) {
                Pns_SetTest(self->m_pnsData, pns_test);
              } else {
                fprintf(stderr,"ERROR: PNS test mode or out of range: %s\n",argv[i]);
                goto bail;
              }
            }
            else {
              fprintf(stderr,"ERROR: PNS test mode missing\n");
              goto bail;
            }
            if (i < (argc - 1)) {
              /* AAC frameSize */
              frame_size = atoi (argv[++i]);
              if (Pns_EvaluateFrameSize(self->m_pnsData, frame_size) != 0) {
                goto bail;
              }
            }
            else {
              fprintf(stderr,"ERROR: PNS frame size missing\n");
              goto bail;
            }                          
            break;

          case '?': 
            goto bail;
            break;
          } /* switch */
        } /* for */
      }  /*  if - or / found   */
      else {
        if (strcmp (*inputfilename1, "") == 0) {
          strcpy (*inputfilename1, c);
        }
        else {
          strcpy (*inputfilename2, c);
        }
      }
    }
         
    if ((strcmp (*inputfilename1, "")) == 0) {
      fprintf(stderr, "ERROR: reference waveform not specified\n");
      goto bail;
    }
    if ((strcmp (*inputfilename2, "")) == 0) {
      fprintf(stderr, "ERROR: waveform under test not specified\n");
      goto bail;
    }
  }
  return OKAY;

 bail:
  show_usage();
  return CMDLINE_PARSE_ERROR;
}
        
/***************************************************************************/


static ERRORCODE compare_audioheaderdata (WAVEFILES* self)
{
  ERRORCODE status = OKAY;

  if (self->m_waveformReference.m_channels != 
      self->m_waveformUnderTest.m_channels) {
    status = WAVEFORM_HEADER_ERROR;
  }
  else {
    self->m_numberOfChannels = self->m_waveformReference.m_channels;
  }

  if (self->m_waveformReference.m_samplefreq != 
      self->m_waveformUnderTest.m_samplefreq) {
    status = WAVEFORM_HEADER_ERROR;
  }
  else {
    self->m_samplingFrequency = self->m_waveformReference.m_samplefreq;
  }

  return status;
}

/***************************************************************************/

static ERRORCODE compare_filelength (WAVEFILES* wavefiles)
{
  unsigned long   start_compare_pos, end_compare_pos;
  unsigned long   samples_lost[2];

  long int       nrofsamples1       = wavefiles->m_waveformReference.m_nrofsamples;
  long int       nrofsamples2       = wavefiles->m_waveformUnderTest.m_nrofsamples;
  unsigned long* samples_to_compare = &(wavefiles->m_samplesToCompare);
    
  start_compare_pos  = (unsigned long) MIN (nrofsamples1, MAX (-wavefiles->m_delay, 0));
  end_compare_pos    = (unsigned long) MAX (MIN (nrofsamples1, (nrofsamples2 - wavefiles->m_delay)), (long) start_compare_pos);

  *samples_to_compare = end_compare_pos - start_compare_pos;

  samples_lost[0] = (unsigned long) MAX ((nrofsamples1 - end_compare_pos), 0);
  samples_lost[1] = (unsigned long) MAX ((nrofsamples2 - wavefiles->m_delay - (signed long) end_compare_pos), 0);

  wavefiles->m_waveformReference.m_offset = -MIN (0, wavefiles->m_delay);
  wavefiles->m_waveformUnderTest.m_offset =  MAX (0, wavefiles->m_delay);

  if (samples_to_compare == 0) {  
    printf ("ERROR: Nothing to compare. Either one of the input files has zero length or delay is out of range.\n");
    return FILE_LENGTH_ERROR;
  }

  if (nrofsamples1 != nrofsamples2) {
    printf ("WARNING: Input files have unequal length.\n");
  }  

  /*  print delay and number of skipped samples    */
    
  if (wavefiles->m_delay > 0) {   
    printf ("    delay of waveform under test                      : %ld samples\n", 
            wavefiles->m_delay);
  }
  if (wavefiles->m_delay < 0) {   
    printf ("    delay of reference waveform                       : %ld samples ( %2.1f %% )\n", 
            -wavefiles->m_delay, 
            (double)-wavefiles->m_delay / ((double)*samples_to_compare + (double)MAX(samples_lost[0], samples_lost[1])) * 100);
  }
    
  if (samples_lost[0] > 0) {
    printf ("    samples lost at the end of reference waveform     : %ld samples ( %2.1f %% )\n", 
            samples_lost[0], 
            (double)samples_lost[0] / ((double)*samples_to_compare + (double)samples_lost[0]) * 100);
  }
  if (samples_lost[1] > 0) {
    printf ("    samples lost at the end of waveform under test    : %ld samples\n", 
            samples_lost[1]);
  }
  return OKAY;   
}



/***************************************************************************/
/* public function main()                                                  */
/***************************************************************************/

int main (int argc, char *argv[])
{
  WAVEFILES  wavefiles;  
  ERRORCODE  status      =  OKAY;
  conf_ptr   self        =  NULL;
  
    
  /****************************************************************************/
  /*  Init confData                                                           */
  /****************************************************************************/
      
  self = confDataInit();

  WavefilesInit(&wavefiles);
     
  /****************************************************************************/
  /*   Get command line parameters                                            */
  /****************************************************************************/
    
  status = parse_commandline (&wavefiles,
                              self,
                              argc,
                              argv);
  if ( OKAY != status ) goto bail;
    
  /****************************************************************************/
  /*   Program banner on screen                                               */
  /****************************************************************************/

  print_banner ();
    
  fprintf (stderr,"%s  %s and %s:  \n", 
           GREP_STRING, 
           wavefiles.m_waveformReference.m_filename, 
           wavefiles.m_waveformUnderTest.m_filename);


  /****************************************************************************/
  /*   Open both input files                                                  */
  /****************************************************************************/

  status = Waveform_OpenInput (&wavefiles.m_waveformReference);
  if (OKAY != status) {
    fprintf (stderr,"ERROR: Cannot open reference waveform.\n");
    goto bail;
  }
 
  status = Waveform_OpenInput (&wavefiles.m_waveformUnderTest);
  if (OKAY != status) {
    fprintf (stderr,"ERROR: Cannot open waveform under test.\n");
    goto bail;
  }
        
  /****************************************************************************/
  /*   Check audio header data                                                */
  /****************************************************************************/

  status = Waveform_CheckHeader (&(wavefiles.m_waveformReference));
  if (OKAY != status) {
    printf ("ERROR: Header of reference waveform is invalid.\n");
    goto bail;
  }

  status = Waveform_CheckHeader (&(wavefiles.m_waveformUnderTest));
  if (OKAY != status) {
    printf ("ERROR: Header of waveform under test is invalid.\n");
    goto bail;
  }
    
  status = compare_audioheaderdata (&wavefiles);
  if (OKAY != status) {
    printf ("ERROR: Waveform headers differ.\n");
    goto bail;
  }

  if (wavefiles.m_waveformReference.m_bits_per_sample != 
      wavefiles.m_waveformUnderTest.m_bits_per_sample) {    
    printf ("WARNING: Input files have different resolutions.\n");
  }

  status = Ssnrcd_CheckSettings(self->m_ssnrcdData, 
                                &wavefiles);
  if (OKAY != status) goto bail;

  /****************************************************************************/
  /*   Compare file length                                                    */
  /****************************************************************************/

  status = compare_filelength (&wavefiles);
  if ( OKAY != status ) goto bail;

    
  /****************************************************************************/
  /*   SSNRCD                                                                 */
  /****************************************************************************/

  if (self->m_ssnrcd == TRUE) {     
    Ssnrcd_CompareAudiodata (&wavefiles,  
                             self->m_verbose,
                             self->m_ssnrcdData);
      
    status = Ssnrcd_GenerateReport (&wavefiles,
                                    self->m_ssnrcdData);
  }

  /****************************************************************************/
  /*   PNS                                                                    */
  /****************************************************************************/
    
  if (self->m_pns == TRUE) {
    status = Pns_ApplyConfTest(&wavefiles,
                               self->m_pnsData,
                               self->m_verbose);
  }
    
  /****************************************************************************/
  /*   Free Data                                                              */
  /****************************************************************************/

 bail:

  confDataFree (&wavefiles, 
                self);

  return status;
}

/***************************************************************************/
