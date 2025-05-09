/**********************************************************************
MPEG-4 Audio VM
Encoder frame work



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

and edited by

Olaf Kaehler (Fraunhofer IIS-A)
Guillaume Fuchs (Fraunhofer IIS)

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1997.



Source file: mp4enc.c


Required modules:
common.o                common module
cmdline.o               command line module
bitstream.o             bit stream module
audio.o                 audio i/o module
enc_par.o               encoder core (parametric)
enc_lpc.o               encoder core (CELP)
enc_tf.o                encoder core (T/F)

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BG    Bernhard Grill, Uni Erlangen <grl@titan.lte.e-technik.uni-erlangen.de>
NI    Naoki Iwakami, NTT <iwakami@splab.hil.ntt.jp>
SE    Sebastien ETienne, Jean Bernard Rault CCETT <jbrault@ccett.fr>
OK    Olaf Kaehler, Fhg/IIS <kaehleof@iis.fhg.de>

Changes:
13-jun-96   HP    first version
14-jun-96   HP    added debug code
17-jun-96   HP    added bit reservoir control / switches -r -b
18-jun-96   HP    added delay compensation
19-jun-96   HP    using new ComposeFileName()
25-jun-96   HP    changed frameNumBit
26-jun-96   HP    improved handling of switch -o
04-jul-96   HP    joined with t/f code by BG (check "DISABLE_TF")
14-aug-96   HP    adapted to new cmdline module, added mp4.h
16-aug-96   HP    adapted to new enc.h
                  added multichannel signal handling
21-aug-96   HP    added AAC_RM4 switches by BG
26-aug-96   HP    CVS
28-aug-96   NI    moved "static CmdLineSwitch switchList[]" into main routine.
29-aug-96   HP    moved "switchlist[]" back again
                  NI stated, that it just was a locally required bug-fix:
                  This table is moved into the main routine,
                  because there was a run-time problem on a
                  SGI (IRIX-5.3) system (using g++).
30-oct-96   HP    additional frame work options
15-nov-96   HP    adapted to new bitstream module
18-nov-96   HP    changed int to long where required
                  added bit stream header options
                  removed AAC_RM4 switches (new -nh, -ms, -bi options)
10-dec-96   HP    new -ni, -vr options, added variable bit rate
23-jan-97   HP    added audio i/o module
07-apr-97   HP    i/o filename handling improved / "-" supported
08-apr-97   SE    added G729-based coder
15-may-97   HP    clean up
30-jun-97   HP    fixed totNumSample bug / calc encNumFrame
05-nov-97   HP    update by FhG/UER
12-nov-97   HP    added options -n & -s
02-dec-98   HP    merged most FDIS contributions ...
20-jan-99   HP    due to the nature of some of the modifications merged
                  into this code, I disclaim any responsibility for this
                  code and/or its readability -- sorry ...
21-jan-99   HP    trying to clean up a bit ...
22-jan-99   HP    stdin/stdout "-" support, -d 1 uses stderr
22-apr-99   HP    merging all contributions
19-jun-01   OK    removed G723/G729 dummy calls
22-jul-03   AR/RJ add extension 2 parametric coding
19-oct-03   OK    clean up for rewrite
**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "allHandles.h"
#include "encoder.h"

#include "streamfile.h"
#include "common_m4a.h" /* common module */
#include "bitstream.h"  /* bit stream module */
#include "audio.h"      /* audio i/o module */
#include "mp4au.h"      /* frame work declarations */
#include "enc_tf.h"
#include "enc_usac.h"
#include "fir_filt.h"   /* fir lowpass filter */
#include "plotmtv.h"
#ifdef EXT2PAR
#include "SscEnc.h"
#endif

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif


#include "flex_mux.h"

#ifdef I2R_LOSSLESS
#include "int_mdct_defs.h"
#endif
#include "streamfile.h"
#include "streamfile_helper.h"
#include "sac_dec_interface.h"
#include "sac_enc.h"
#include "sac_stream_enc.h"
#include "defines.h"

#include "signal_classifier.h"

#ifdef __linux__
#include <unistd.h>
#endif

/* ---------- declarations ---------- */



#define PROGVER "MPEG-4 Natural Audio Encoder (Edition 2001)   2003-08-27"
#define CVSID "$Id: mp4auenc.c,v 1.16 2011-09-30 09:13:13 mul Exp $"
#define EDITLIST_SUPPORT

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

/* ---------- variables ---------- */
#ifdef DEBUGPLOT
extern int framePlot; /* global for debugging purposes */
extern int firstFrame;
#endif

#ifdef WIN32
#define USAC_EXE USAC_WIN_EXE
#else
#define USAC_EXE USAC_UNIX_EXE
#endif

/* USAC executable naming */
typedef enum _Binary_Naming {
  USAC_DESCRIPTION,
  USAC_WIN_EXE,
  USAC_UNIX_EXE
} USAC_Binary_Naming;

int	    debug[256];
spatialDec* hSpatialDec = NULL;

typedef enum _VERBOSE_LEVEL
{
  VERBOSE_NONE,
  VERBOSE_LVL1,
  VERBOSE_LVL2
} VERBOSE_LEVEL;

  typedef enum _RECYCLE_LEVEL {
    RECYCLE_INACTIVE,
    RECYCLE_ACTIVE
  } RECYCLE_LEVEL;

static struct EncoderMode encoderCodecsList[] = {
  {MODE_TF,
   MODENAME_TF,
   EncTfInfo,
   EncTfInit,
   EncTfFrame,
   EncTfFree
  },
  {MODE_USAC,
   MODENAME_USAC,
   EncUsacInfo,
   EncUsacInit,
   EncUsacFrame,
   EncUsacFree
  },
  {-1,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL
  }
};

typedef struct _DRC_PARAMS {
  char binary_DrcEncoder[FILENAME_MAX];
  char drcGainFileName[FILENAME_MAX];
  char drcConfigXmlFileName[FILENAME_MAX];
  char tmpFile_Drc_payload[FILENAME_MAX];
  char tmpFile_Drc_config[FILENAME_MAX];
  char tmpFile_Loudness_config[FILENAME_MAX];
  char logfile[FILENAME_MAX];
} DRC_PARAMS;

const char usac_binaries[][3][FILENAME_MAX] =
{
/*
  module description                      Windows executable                      Unix executable
  USAC_DESCRIPTION                        USAC_WIN_EXE                            USAC_UNIX_EXE
  */
  {{"DRC encoder"},                       {"drcToolEncoderCmdl.exe"},             {"drcToolEncoder"}}
};


int PARusedNumBit;

/* SSC data */
#ifdef EXT2PAR
PROCESSINFO*  pPI;
ENCODERINFO*  pEI;
SSCBUNCH*     pSSCBunch;
CHUNK_LENGTHS ChunkLengths;
#endif

/* ###################################################################### */
/* ##                 MPEG USAC encoder static functions               ## */
/* ###################################################################### */
static int PrintCmdlineHelp(
            int                               argc,
            char                           ** argv
) {
  int bPrintCmdlHelp = 0;
  int i;

  fprintf(stdout, "\n");
  fprintf(stdout, "******************* MPEG-D USAC Audio Coder - Edition 2.3 *********************\n");
  fprintf(stdout, "*                                                                             *\n");
  fprintf(stdout, "*                              USAC Encoder Module                            *\n");
  fprintf(stdout, "*                                                                             *\n");
  fprintf(stdout, "*                                  %s                                *\n", __DATE__);
  fprintf(stdout, "*                                                                             *\n");
  fprintf(stdout, "*    This software may only be used in the development of the MPEG USAC Audio *\n");
  fprintf(stdout, "*    standard, ISO/IEC 23003-3 or in conforming implementations or products.  *\n");
  fprintf(stdout, "*                                                                             *\n");
  fprintf(stdout, "*******************************************************************************\n");

  for (i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "-h")) {
      bPrintCmdlHelp = 1;
      break;
    }
    if (!strcmp(argv[i], "-xh")) {
      bPrintCmdlHelp = 2;
      break;
    }
  }

  if (argc < 7 || bPrintCmdlHelp >= 1) {
    fprintf(stdout, "\n" );
    fprintf(stdout, "Usage: %s -if infile.wav -of outfile.mp4 -br <i> [options]\n\n", argv[0]);
    fprintf(stdout, "    -h                         Print help\n");
    fprintf(stdout, "    -xh                        Print extended help\n");
    fprintf(stdout, "    -if <s>                    Path to input wav\n");
    fprintf(stdout, "    -of <s>                    Path to output mp4\n");
    fprintf(stdout, "    -br <i>                    Bitrate in kbit/s\n");
    fprintf(stdout, "\n[options]:\n\n");
    fprintf(stdout, "    -numChannelOut <i>         Set number of audio channels (0 = as input file) (default: 0)\n");
    fprintf(stdout, "    -fSampleOut <i>            Set sampling frequency [Hz] for bit stream header (0 = as input file) (default: 0)\n");
    fprintf(stdout, "    -drcConfigFile <s>         xml file containing the DRC configuration\n");
    fprintf(stdout, "    -drcGainFile <s>           file containing the DRC Gain data\n");
    fprintf(stdout, "    -bw <i>                    Bandwidth in Hz\n");
    fprintf(stdout, "    -streamID <ui>             Stream identifier\n");
    fprintf(stdout, "    -drc                       Enable DRC encoding\n");
    fprintf(stdout, "    -cfg                       Path to binary configuration file\n");
    fprintf(stdout, "    -usac_switched             USAC Switched Coding (default)\n");
    fprintf(stdout, "    -usac_fd                   USAC Frequency Domain Coding\n");
    fprintf(stdout, "    -usac_td                   USAC Temporal Domain Coding\n");
    fprintf(stdout, "    -noSbr                     Disable the usage of the SBR tool\n");
    fprintf(stdout, "    -sbrRatioIndex <i>         Set SBR Ratio Index: 0, 1, 2, 3 (default)\n");
    fprintf(stdout, "    -mps_res                   Allow MPEG Surround residual (requires enabled SBR)\n");
    fprintf(stdout, "    -hSBR                      Activate the usage of the harmonic patching for the SBR\n");
    fprintf(stdout, "    -ms <0,1,2>                Set msMask instead of autodetect: 0, 1(default), 2\n");
    fprintf(stdout, "    -wshape                    Use window shape WS_DOLBY instead of WS_FHG (default)\n");
    fprintf(stdout, "    -deblvl <0,1>              Activate debug level: 0 (default), 1\n");
    fprintf(stdout, "    -v                         Activate verbose level 1\n");
    fprintf(stdout, "    -vv                        Activate verbose level 2\n");
    fprintf(stdout, "    -nogc                      Disable the garbage collector\n");
    if( bPrintCmdlHelp == 2 ){
      fprintf(stdout, "\n[experimental options]:\n\n");
      fprintf(stdout, "    -noIPF                     Disable the embedding of IPF data\n");
      fprintf(stdout, "    -ep                        Create bitstreams with epConfig 1 syntax\n");
      fprintf(stdout, "    -tns                       Activate Temporal Noise Shaping (active by default)\n");
      fprintf(stdout, "    -tns_lc                    Activate Temporal Noise Shaping with low complexity (deactivates TNS -> not working)\n");
      fprintf(stdout, "    -usac_tw                   Activate the usage of the time-warped MDCT\n");
      fprintf(stdout, "    -noiseFilling              Activate Noise Filling\n");
      fprintf(stdout, "    -ipd                       Apply IPD coding in Mps212\n");
      fprintf(stdout, "    -cplx_pred                 Complex prediction\n");
      fprintf(stdout, "    -aac_nosfacs               All AAC-scalefactors will be equal\n");
      fprintf(stdout, "    -tsd <s>                   Activate applause coding\n");
    } else {
      fprintf(stdout, "\nUse -xh for extended help!\n");
    }
    fprintf(stdout, "\n[formats]:\n\n");
    fprintf(stdout, "    <s>                        string\n");
    fprintf(stdout, "    <i>                        integer number\n");
    fprintf(stdout, "    <ui>                       unsigned integer number\n");
    fprintf(stdout, "    <f>                        floating point number\n");
    fprintf(stdout, "\n");
    return -1;
  }
  return 0;
}

static int ParseEncoderOptions(
            int                               argc,
            char                           ** argv,
            HANDLE_ENCPARA                    encPara,
            int                             * mainDebugLevel
) {
  int error_usac              = 0;
  int i                       = 0;
  int required                = 0;
  int codecModeSet            = 0;
  int tnsLcSet                = 0;

  /* ========================== */
  /* Initialize Encoder Options */
  /* ========================== */

  encPara->debugLevel        = *mainDebugLevel;
  encPara->bw_limit          = 0;
  encPara->streamID_present  = 0;
  encPara->streamID          = 0;
  encPara->embedIPF          = 1;
  encPara->codecMode         = USAC_SWITCHED;
  encPara->tns_select        = NO_TNS;
  encPara->ms_select         = 1;
  encPara->ep_config         = -1;
  encPara->wshape_flag       = 0;
  encPara->bUseFillElement   = 1;
  encPara->sbrenable         = 1;
  encPara->flag_960          = 0;
  encPara->flag_768          = 0;
  encPara->flag_twMdct       = 0;
  encPara->flag_harmonicBE   = 0;
  encPara->flag_noiseFilling = 0;
  encPara->flag_ipd          = 0;
  encPara->flag_mpsres       = 0;
  encPara->flag_cplxPred     = 0;

  /* get selected encoder mode */
  for ( i = 0 ; i < 2 ; i++ ) {
    encPara->prev_coreMode[i] = CORE_MODE_FD;
    encPara->coreMode[i]      = CORE_MODE_FD;
  }

  /* ===================== */
  /* Parse Encoder Options */
  /* ===================== */
  for (i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "-br")) {                        /* Required */
      encPara->bitrate = atoi(argv[++i]);
      required++;
      continue;
    }
    else if (!strcmp(argv[i], "-bw")) {                   /* Optional */
      encPara->bw_limit = atoi(argv[++i]);
      continue;
    }
    else if (!strcmp(argv[i], "-streamID")) {             /* Optional */
      encPara->streamID = atoi(argv[++i]);

      /* Sanity check */
      if ( encPara->streamID > MAX_STREAM_ID ) {
        CommonWarning("main: specified value of stream ID is outside the valid range! Stream ID can take values from 0 to 65535.");
        error_usac = 1;
        break;
      }
      encPara->streamID_present = 1;
      continue;
    }
    else if (!strcmp(argv[i], "-noIPF")) {                /* Optional */
      encPara->embedIPF = 0;
      continue;
    }
    else if (!strcmp(argv[i], "-usac_switched")) {        /* Optional */
      if (codecModeSet == 1) {
        CommonWarning("main: codecMode has already been set. Choose only one of the following options: -usac_switched (default), -usac_fd, -usac_td.");
        error_usac = 1;
        break;
      } else {
        encPara->codecMode = USAC_SWITCHED;
        codecModeSet = 1;
        continue;
      }
    }
    else if (!strcmp(argv[i], "-usac_fd")) {              /* Optional */
      if (codecModeSet == 1) {
        CommonWarning("main: codecMode has already been set. Choose only one of the following options: -usac_switched (default), -usac_fd, -usac_td.");
        error_usac = 1;
        break;
      } else {
        encPara->codecMode = USAC_ONLY_FD;
        codecModeSet = 1;
        continue;
      }
    }
    else if (!strcmp(argv[i], "-usac_td")) {              /* Optional */
      if (codecModeSet == 1) {
        CommonWarning("main: codecMode has already been set. Choose only one of the following options: -usac_switched (default), -usac_fd, -usac_td.");
        error_usac = 1;
        break;
      } else {
        encPara->codecMode = USAC_ONLY_TD;
        codecModeSet = 1;
        continue;
      }
    }
    else if (!strcmp(argv[i], "-tns")) {                  /* Optional */
      encPara->tns_select = TNS_USAC;
      continue;
    }
    else if (!strcmp(argv[i], "-ms")) {                   /* Optional */
      encPara->ms_select = atoi(argv[++i]);
      continue;
    }
    else if (!strcmp(argv[i], "-ep")) {                   /* Optional */
      encPara->ep_config = 1;
      continue;
    }
    else if (!strcmp(argv[i], "-wshape")) {               /* Optional */
      encPara->wshape_flag = 1;
      continue;
    }
    /*else if (!strcmp(argv[i], "-fillElem")) {             
      encPara->bUseFillElement = atoi(argv[++i]);
      continue;
    }*/
    else if (!strcmp(argv[i], "-noSbr")) {                /* Optional */
      encPara->sbrenable = 0;
      continue;
    }
    else if (!strcmp(argv[i], "-usac_tw")) {              /* Optional */
      encPara->flag_twMdct = 1;
      continue;
    }
    else if (!strcmp(argv[i], "-hSBR")) {                 /* Optional */
      encPara->flag_harmonicBE = 1;
      continue;
    }
    else if (!strcmp(argv[i], "-noiseFilling")) {         /* Optional */
      encPara->flag_noiseFilling = 1;
      continue;
    }
    else if (!strcmp(argv[i], "-ipd")) {                  /* Optional */
      encPara->flag_ipd = 1;
      continue;
    }                                                        
    else if (!strcmp(argv[i], "-cplx_pred")) {            /* Optional */
      encPara->flag_cplxPred = 1;
      continue;
    }
    else if (!strcmp(argv[i], "-tsd")) {                  /* Optional */
      strncpy ( encPara->tsdInputFileName, argv[++i], FILENAME_MAX );
      continue;
    }
  }

  /* get sbrRatioIndex */
  encPara->sbrRatioIndex = (encPara->sbrenable) ? 3 : 0; /* set to 2:1 SBR by default if sbr is enabled*/
  for (i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "-sbrRatioIndex")) {             /* Optional */
      encPara->sbrRatioIndex = atoi(argv[++i]);
      if(encPara->sbrRatioIndex == 0){
        encPara->sbrenable = 0;
      }
      if(encPara->sbrRatioIndex == 2){
        encPara->flag_768 = 1;
      }
      break;
    }
  }

  if (encPara->sbrenable == 1) {
    for (i = 1; i < argc; ++i) {
      if (!strcmp(argv[i], "-mps_res")) {                 /* Optional */
        encPara->flag_mpsres = encPara->sbrenable;
        break;
      }
    }
  }

  /* read AAC common command line */
  if ((encPara->codecMode == USAC_SWITCHED) || (encPara->codecMode == USAC_ONLY_FD))
  {
    encPara->aacAllowScalefacs = 1;
    for (i = 1; i < argc; ++i) {
      if (!strcmp(argv[i], "-aac_nosfacs")) {             /* Optional */
        encPara->aacAllowScalefacs = 0;
        break;
      }
    }
  }

  /* read more specific AAC command line */
  if (encPara->ep_config == -1) {
    if (encPara->tns_select != NO_TNS){
      encPara->tns_select = TNS_USAC;
    }
    for (i = 1; i < argc; ++i) {
      if (!strcmp(argv[i], "-tns_lc")) {
        tnsLcSet = 1;
      }
    }
    if (tnsLcSet != 1) {
      encPara->tns_select = TNS_USAC;
    }
  }

  /* validate command line arguments */
  if (required != 1) {
    error_usac = 1;
  }
  
  return error_usac;
}

static int GetCmdline(
            int                               argc,
            char                           ** argv,
            char                            * wav_InputFile,
            char                            * mp4_OutputFile,
            char                            * cfg_InputFile,
            int                             * useGlobalBinariesPath,
            int                             * numChannelOut,
            float                           * fSampleOut,
            HANDLE_ENCPARA                    encPara,
            DRC_PARAMS                      * drcParams,
            int                             * mainDebugLevel,
            VERBOSE_LEVEL                   * verboseLevel,
            RECYCLE_LEVEL                   * recycleLevel
) {
  int error_usac = 0;
  int i                       = 0;
  int required                = 0;

  for (i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "-if")) {                                /* Required */
      strncpy ( wav_InputFile, argv[++i], FILENAME_MAX );
      required++;
      continue;
    }
    else if (!strcmp(argv[i], "-of")) {                           /* Required */
      strncpy ( mp4_OutputFile, argv[++i], FILENAME_MAX );
      required++;
      continue;
    }
    else if (!strcmp(argv[i], "-cfg")) {                          /* Optional */
      strncpy(cfg_InputFile, argv[++i], FILENAME_MAX);
      *useGlobalBinariesPath = 0;
      continue;
    }
    else if (!strcmp(argv[i], "-numChannelOut")) {                /* Optional */
      *numChannelOut = atoi(argv[++i]);
      continue;
    }
    else if (!strcmp(argv[i], "-fSampleOut")) {                   /* Optional */
      *fSampleOut = (float) atof(argv[++i]);
      continue;
    }
    else if (!strcmp(argv[i], "-drcConfigFile")) {                /* Optional */
      strncpy ( drcParams->drcConfigXmlFileName, argv[++i], FILENAME_MAX );
      encPara->enableDrcEnc = 1;
      continue;
    }
    else if (!strcmp(argv[i], "-drcGainFile")) {                  /* Optional */
      strncpy ( drcParams->drcGainFileName, argv[++i], FILENAME_MAX );
      encPara->enableDrcEnc = 1;
      continue;
    }
    else if (!strcmp(argv[i], "-v")) {                            /* Optional */
      *verboseLevel = (*verboseLevel > VERBOSE_LVL1) ? *verboseLevel : VERBOSE_LVL1;
      continue;
    }
    else if (!strcmp(argv[i], "-vv")) {                           /* Optional */
      *verboseLevel = VERBOSE_LVL2;
      continue;
    }
    else if (!strcmp(argv[i], "-deblvl")) {                       /* Optional */
      *mainDebugLevel = atoi(argv[++i]);
      continue;
    }
    else if (!strcmp(argv[i], "-nogc")) {                 /* Optional */
      *recycleLevel = RECYCLE_INACTIVE;
      continue;
  }
  }

  /* validate command line arguments */
  if (required != 2) {
    error_usac = 1;
  }
  
  error_usac = ParseEncoderOptions( argc, argv, encPara, mainDebugLevel );

  return error_usac;
}

static HANDLE_ENCODER newEncoder()
{
  HANDLE_ENCODER ret = (HANDLE_ENCODER)malloc(sizeof(struct tagEncoderData));
  ret->data = NULL;
  ret->getNumChannels = NULL;
  ret->getReconstructedTimeSignal = NULL;
  ret->getEncoderMode = NULL;
  ret->getSbrEnable = NULL;
  ret->getSbrRatioIndex = NULL;
  ret->getBitrate = NULL;
  return ret;
}

/* 20060107 */
#ifdef USE_AFSP
#include <math.h>
#include "libtsp.h"
static double
FIxKaiser (double x, double alpha)

{
  static double alpha_prev = 0.0;
  static double Ia = 1.0;
  double beta, win;

  /* Avoid recalculating the denominator Bessel function */
  if (alpha != alpha_prev) {
    Ia = FNbessI0 (alpha);
    alpha_prev = alpha;
  }

  if (x < -1.0 || x > 1.0)
    win = 0.0;
  else {
    beta = alpha * sqrt (1.0 - x * x);
    win = FNbessI0 (beta) / Ia;
  }

  return win;
}

static void
RSKaiserLPF (float h[], int N, double Fc, double alpha)
{
  int i, k;
  double t, u, T, Ga;

  /* Multiply the Kaiser window by the sin(x)/x response */
  /* Use symmetry to reduce computations */
  Ga = 2.0 * Fc;
  T = 0.5 * (N-1);
  for (i = 0, k = N-1; i <= k; ++i, --k) {
    t = i - T;
    u = k - T;
    h[i] = (float) (Ga * FNsinc (2.0 * Fc * t) * FIxKaiser (t / T, alpha));
    if (-t == u)
      h[k] = h[i];
    else
      h[k] = (float) (Ga * FNsinc (2.0 * Fc * u) * FIxKaiser (u / T, alpha));
  }
}


static void
FIdownsample(const float* input, float* out, int NOut, const float*win, int Ncof, int downSampleRatio)
{
  int i, j, k, l;
  double sum;

  k = Ncof-1;
  for (i=0; i<NOut; i++) {
    sum = 0.0;
    l = k;
    for (j=0; j<Ncof; j++) {
      sum += win[j]*input[l--];
    }

    out[i] = (float)sum;
    k+=downSampleRatio;
  }
}

static void
FIupsample(const float* input, float* out, int NOut, const float*win, int Ncof, int upSampleRatio)
{
  int i, j, k, l;
  double sum;

  for (i=0; i<NOut+Ncof; i++){
    out[i] = 0.f;
  }

  for (i=0; i<(NOut+Ncof)/upSampleRatio; i++){
    out[i*upSampleRatio] = input[i];
  }

  k = Ncof-1;
  for (i=0; i<NOut; i++) {
    sum = 0.0;
    l = k;
    for (j=0; j<Ncof; j++) {
      sum += win[j]*out[l--];
    }

    out[i] = (float)sum;
    k++;
  }
}
#endif

static int checkExistencyAndRemoveNewlineChar(
            char                            * string,
            const VERBOSE_LEVEL               verboseLevel
) {
  FILE* fExists = NULL;
  int len       = (int)strlen(string);

  if (string[len - 1] == '\n') {
    string[len - 1] = '\0';
  }

  fExists = fopen(string, "r");

  if (!fExists) {
    if (VERBOSE_LVL1 <= verboseLevel) {
      fprintf(stderr, "\nBinary %s not present.\nCheck the corresponding config file entry or binary location.\n", string);
    }
    return -1;
  }

  fclose(fExists);

  return 0;
}

static int checkBinaries_module(
            const char                      * module_description,
            char                            * module_binary,
            const VERBOSE_LEVEL               verboseLevel
) {
  int retError = 0;

  if (checkExistencyAndRemoveNewlineChar(module_binary, verboseLevel)) {
    if (VERBOSE_LVL1 <= verboseLevel) {
      fprintf(stderr, "\nWarning: No %s support.\n", module_description);
    }
    retError = -1;
  } else {
    if (VERBOSE_LVL2 <= verboseLevel) {
      fprintf(stderr, "\nUsing %s binary:\n %s\n", module_description, module_binary);
    }
  }

  return retError;
}

static int setBinaries_module(
            const char                      * globalBinariesPath,
            const char                      * module_description,
            const char                      * module_name,
            char                            * module_binary,
            const VERBOSE_LEVEL               verboseLevel
) {
  int retError = 0;

  strcpy(module_binary, globalBinariesPath);
  strcat(module_binary, module_name);

  retError = checkBinaries_module(module_description,
                                  module_binary,
                                  verboseLevel);

  return retError;
}

static int getBinaries_module(
            FILE                            * f,
            const int                         nChars,
            const char                      * module_description,
            char                            * module_binary,
            const VERBOSE_LEVEL               verboseLevel
) {
  int retError = 0;

  if (fgets(module_binary, nChars, f) != 0) {
    retError = checkBinaries_module(module_description,
                                    module_binary,
                                    verboseLevel);
  } else {
    fprintf(stderr, "\nError reading %s binary.\n", module_description);
    retError = -1;
  }

  return retError;
}

static int getBinaries(
            char                            * cfgFile,
            int                             * nBinaries,
            char                            * binary_DrcEncoder,
            const VERBOSE_LEVEL               verboseLevel
) {
  int retError            = 0;
  char line[FILENAME_MAX] = {'\0'};
  FILE* f                 = fopen ( cfgFile, "r" );
  const int len           = sizeof( line );
  int nBinaries_tmp       = 0;

  if (!f) {
    fprintf(stderr, "Unable to open config file containing binaries\n");
    return -1;
  }

  while (fgets(line, len, f) != 0) {
    if (!strncmp(line, "binary_drcToolEncoder:", 21)) {
      retError = getBinaries_module(f,
                                    len,
                                    usac_binaries[0][USAC_DESCRIPTION],
                                    binary_DrcEncoder,
                                    verboseLevel);
      if (0 != retError) {
        return retError;
      }
      nBinaries_tmp++;
    }
  }

  *nBinaries = nBinaries_tmp;

  return 0;
}

static int setBinaries(
            char                            * globalBinariesPath,
            int                             * nBinaries,
            char                            * binary_DrcEncoder,
            const VERBOSE_LEVEL               verboseLevel
) {
  int retError      = 0;
  int nBinaries_tmp = 0;

  /***********************************************
               binary_DrcEncoder
  ***********************************************/
  retError = setBinaries_module(globalBinariesPath,
                                usac_binaries[0][USAC_DESCRIPTION],
                                usac_binaries[0][USAC_EXE],
                                binary_DrcEncoder,
                                verboseLevel);
  if (0 != retError) {
    return retError;
  }
  nBinaries_tmp++;

  *nBinaries = nBinaries_tmp;

  return retError;
}

static int setupUsacEncoder(
            const int                         useGlobalPath,
            char                            * globalPath,
            char                            * cfg_InputFile,
            char                            * binary_DrcEncoder,
            const VERBOSE_LEVEL               verboseLevel
) {
  int error      = 0;
  int nBinaries  = 0;
  char* basename = NULL;

  fprintf(stdout, "\n");
  fprintf(stdout, "::-----------------------------------------------------------------------------\n::    ");
  fprintf(stdout, "Setup encoder...\n");
  fprintf(stdout, "::-----------------------------------------------------------------------------\n");
  fprintf(stdout, "\n");

  if (useGlobalPath) {
#ifdef _WIN32
    GetModuleFileName(NULL, globalPath, 3 * FILENAME_MAX);
#else
#ifdef __linux__
    readlink("/proc/self/exe", globalPath, 3 * FILENAME_MAX);
#endif

#ifdef __APPLE__
    uint32_t tmpStringLength = 3 * FILENAME_MAX;
    if (_NSGetExecutablePath( globalPath, &tmpStringLength)) {
      CommonExit(1, "Error initializing USAC Encoder.");
    }
#endif
#endif

    #ifdef _WIN32
    basename = strstr(globalPath, "usacEnc.exe");
#else
    basename = strstr(globalPath, "usacEnc");
#endif

    if (basename == 0) {
      CommonExit(1, "Error initializing USAC Encoder.");
    } else {
      strcpy(basename, "\0");
   }

    /* set the binary names */
    fprintf(stdout, "Collecting executables...\n");
    error = setBinaries(globalPath,
                        &nBinaries,
                        binary_DrcEncoder,
                        verboseLevel);
    if (error) {
      fprintf(stderr,"Error locating USAC encoder binaries\n");
    }
  } else {
    /* Get the binary names */
    if (VERBOSE_LVL1 <= verboseLevel) {
      fprintf(stdout, "Parsing %s file...\n", cfg_InputFile);
    }

    fprintf(stdout, "Collecting executables...\n");
    error = getBinaries(cfg_InputFile,
                        &nBinaries,
                        binary_DrcEncoder,
                        verboseLevel);
    if (error) {
      fprintf(stderr,"Error locating USAC encoder binaries\n");
    }
  }

  if (VERBOSE_LVL1 <= verboseLevel) {
    fprintf(stdout, "\n>>    %d executables loaded!\n", nBinaries);
  }

  return 0;
}

/* call binary - wrapper for system() */
int callBinary(
            const char                       *exe_2call,
            const char                       *exe_description,
            const VERBOSE_LEVEL               verboseLevel
) {
  int retError = 0;

  fprintf(stdout, "Calling %s...\n", exe_description);
  
  if (VERBOSE_LVL1 <= verboseLevel) {
    fprintf(stdout, ">>     command:\n%s\n\n", exe_2call);
  }

  retError = system(exe_2call);
  if (retError) {
    fprintf(stderr, "\nError running %s\n", exe_description);
    retError = retError;
  }

  return retError;
}

static int execDrc(DRC_PARAMS                 drcParams,
                   int                        numSamples,
                   int                        numChannels,
                   const VERBOSE_LEVEL        verboseLevel)
{
  int error                         = 0;
  int len                           = 0;
  char callDrc[3 * FILENAME_MAX]    = {'\0'};
  char tmpString[3 * FILENAME_MAX]  = {'\0'};

  /* Set input commandline parameters */
  sprintf(callDrc, "%s %s %s", drcParams.binary_DrcEncoder, drcParams.drcConfigXmlFileName, drcParams.drcGainFileName);

  /* Set output commandline parameters */
  sprintf(tmpString, " %s %s %s", drcParams.tmpFile_Drc_config, drcParams.tmpFile_Loudness_config, drcParams.tmpFile_Drc_payload);
  strcat(callDrc, tmpString);

  sprintf(tmpString, " > %s 2>&1", drcParams.logfile);
  strcat(callDrc, tmpString);

  if (0 != callBinary(callDrc, "DRC encoder", verboseLevel)) {
    return -1;
  }

  return 0;

}

static void removeTempFiles(
            const RECYCLE_LEVEL               removeFiles,
            const int                         bUseWindowsCommands,
            const VERBOSE_LEVEL               verboseLevel
) {
  if (RECYCLE_ACTIVE == removeFiles) {
    if (VERBOSE_LVL1 <= verboseLevel) {
      fprintf(stdout,"\n");
      fprintf(stdout, "Deleting intermediate files...\n");
    }

    if (bUseWindowsCommands) {
      system("del tmpFileUsacEnc_*.* 2> NUL");
    } else {
      remove("tmpFileUsacEnc_drc_config_output.bit");
      remove("tmpFileUsacEnc_drc_loudness_output.bit");
      remove("tmpFileUsacEnc_drc_loudness_output.txt");
      remove("tmpFileUsacEnc_drc_payload_output.bit");
      remove("tmpFileUsacEnc_drc_payload_output.txt");
    }
  } else {
    if (VERBOSE_LVL1 <= verboseLevel) {
      fprintf(stdout,"\n");
      fprintf(stdout, "Keeping intermediate files!\n");
    }
  }
}

static int useWindowsCommands(
            void
) {
  int retVal      = 1;

#if defined __linux__ || defined __APPLE__
  retVal = 0; 
#else
  FILE *fp        = NULL;
  char path[1035] = {'\0'};

  /* Open the command for reading. */
  fp = _popen("uname -o >> win_commands.txt 2>&1", "r");
  if (!(fp == NULL)) {
    if (fgets(path, sizeof(path)-1, fp) != NULL) {
      if (!strncmp(path,"Cygwin",6)) {
        retVal = 0; 
      }
    }
  }
  _pclose(fp);

  if (retVal) {
    system("del win_commands.txt 2>  NUL");
  } else {
    system("rm -f win_commands.txt");
  }
#endif

  return retVal;
}


/* 20060107 */
/* Encode() */
/* Encode audio file and generate bit stream. */
/* (This function evaluates the global xxxDebugLevel variables !!!) */

static int Encode (
                   char *audioFileName,          /* in: audio file name */
                   char *bitFileName,            /* in: bit stream file name */
                   DRC_PARAMS *drcParams,
                   char *codecMode,              /* in: codec mode string(s) */
                   HANDLE_ENCPARA encPara,       /* in: encoder parameter struct */
                   float regionStart,            /* in: start time of region */
                   float regionDurat,            /* in: duration of region */
                   int numChannelOut,            /* in: number of channels (0 = as input) */
                   float fSampleOut,             /* in: sampling frequency (0 = as input) */
                   int mainDebugLevel,
                   VERBOSE_LEVEL verboseLevel
/* returns: 0=OK  1=error */
) {
  int err = 0;
  AudioFile *audioFile;
  float **sampleBuf;
  float **reSampleBuf;
  float **encSampleBuf;
  float fSample=0;
  long fileNumSample;
  long totNumSample;
  int numSample;
  int numChannel;
  int frameNumSample;
  long fSampleLong;
  HANDLE_STREAMFILE outfile;
  HANDLE_STREAMPROG outprog;
  HANDLE_BSBITBUFFER *au_buffers;
  int requestIPF = 0;
  int numAPRframes = 0;
  int enableIPF = 0;
  USAC_SYNCFRAME_TYPE usacSyncState = USAC_SYNCFRAME_NOSYNC;
  int startupNumFrame,startupFrame;
  int ch, i, i_enc;
  long startSample;
  long encNumSample;
  int numChannelBS = 0;
  long fSampleLongBS;
  ENC_FRAME_DATA frameData;
  int track_count_total;
  int delayNumSample = 0;

  FIR_FILT *lowpassFilt = NULL;

  /* 20060107 */
  int sbrenable = 0;
  int sbrRatioIndex = 0;
  int usac212enable = 0;
  int downSampleRatio = 1;
  int upSampleRatio   = 1;
  int bitRate;
  int NcoefRS = 0;
  int NcoefUS = 0;
  int delayTotal = 0;
  int delay212 = 0;
  int delayEditlist = 0; 
  int addlCoreDelay = 0;
  int nSamplesProcessed = 0;
  int delayEncSamples   = 0;
  int delayEncFrames    = 0;
  int delayBufferSamples        = 0;
  float *hRS = NULL;
  float *hUS = NULL;
  float **sampleBufRS = NULL;
  float **sampleBufRSold = NULL;
  float **tmpBufRS = NULL;
  float **additionalCoreDelayBuffer = NULL;
  float **sampleBufUS = NULL;
  float **sampleBufUSold = NULL;
  float **tmpBufUS = NULL;
  float lowPassFreq = 0;
  /* 20060107 */
#ifdef I2R_LOSSLESS
  int type_PCM = 0;
  /* long ll_bits_to_use; */
  /* int frameLength = 0; */
  /* int ll_track_count_total; */
#endif
  int frameMaxNumBit; /* SAMSUNG_2005-09-30 : maximum number of bits per frame */

  const int         bitBufSize                    = 0;
  const int         bitDebugLevel                 = 0;
  const int         audioDebugLevel               = 0;
  
  int               frame                         = 0;
  int               osf                           = 1;

  /* ---- init ---- */
  fprintf(stdout, "\n");
  fprintf(stdout, "::-----------------------------------------------------------------------------\n::    ");
  fprintf(stdout, "Encoding audio file...\n" );
  fprintf(stdout, "::-----------------------------------------------------------------------------\n");
  fprintf(stdout, "\n");

  if (verboseLevel >= VERBOSE_LVL1) {
    fprintf(stdout,"audioFileName=\"%s\"\n",audioFileName);
    fprintf(stdout,"bitFileName=\"%s\"\n",bitFileName);
    fprintf(stdout, "\n");
  }
  if (verboseLevel == VERBOSE_LVL2) {
  fprintf(stdout,"codecMode=\"%s\"\n",codecMode);
    fprintf(stdout, "\n");
  }

  SetDebugLevel(mainDebugLevel);
  BsInit(bitBufSize,bitDebugLevel,0);
  AudioInit(getenv(MP4_ORI_RAW_ENV),       /* headerless file format */
            audioDebugLevel);

  /* ---- open audio file ---- */
  audioFile = AudioOpenRead(audioFileName,&numChannel,&fSample,
                            &fileNumSample);
  if (audioFile==NULL)
    CommonExit(1,"Encode: error opening audio file %s "
               "(maybe unknown format)",
               audioFileName);

#ifdef I2R_LOSSLESS
  /******  test osf  */
  fileNumSample /= osf;
  type_PCM = AudioType(audioFile);
  fSample = fSample / osf;
#endif

  startSample = (long)(regionStart*fSample+0.5);

  if (regionDurat >= 0)
    encNumSample = (long)(regionDurat*fSample+0.5);
  else
    if (fileNumSample < 0) /* unkown */
      encNumSample = -1;
    else
      encNumSample = max(0,fileNumSample-startSample);


  /* ---- DRC Encoder  ---- */

   /* run the DRC exec here. */
  if( 1 == encPara->enableDrcEnc ){
    execDrc(*drcParams, fileNumSample, numChannel, verboseLevel);
  }


  /* ---- init encoder ---- */
  fSampleLong = (long)(fSample+0.5);
    numChannelBS = (numChannelOut==0) ? numChannel : numChannelOut;
  if (numChannelBS > numChannel) {
    CommonWarning("Desired number of channels too large, using %i instead of %i\n",numChannel,numChannelBS);
    numChannelBS = numChannel;
  }
  fSampleLongBS = (fSampleOut==0) ? fSampleLong : (long)(fSampleOut+0.5);

  /* ---- get selected codec ---- */
  track_count_total = 0;
  frameData.trackCount = (int*)malloc(sizeof(int));
  frameData.enc_mode = (struct EncoderMode*)malloc(sizeof(EncoderMode));
  frameData.enc_data = (HANDLE_ENCODER*)malloc(sizeof(HANDLE_ENCODER));

  for (i=0; i<MAX_TRACKS; i++) {
    setupDecConfigDescr(&frameData.dec_conf[i]);
    frameData.dec_conf[i].profileAndLevelIndication.value = 0x40;
    frameData.dec_conf[i].streamType.value = 0x05;
    frameData.dec_conf[i].upsteam.value = 0;
  }

  {
    struct EncoderMode *mode;
    HANDLE_BSBITBUFFER asc_buffer = BsAllocBuffer(USAC_MAX_CONFIG_LEN << 3);
    i = 0;

    do {
      mode = &encoderCodecsList[i++];
    } while ((mode->mode != -1) && strcmp(codecMode,mode->modename));
    if (mode->mode == -1)
      CommonExit(1,"Encode: unknown codec mode %s",codecMode);

    *(frameData.enc_mode) = *mode;

    /*varBitRate*/
    *frameData.enc_data = newEncoder();

    err = frameData.enc_mode->EncInit(numChannelBS,
                                      fSample,
                                      encPara,
                                      &frameNumSample,
                                      &frameMaxNumBit,   /* SAMSUNG_2005-09-30 */
                                      &(frameData.dec_conf[track_count_total]),
                                      frameData.trackCount,
                                      &asc_buffer,
                                      &numAPRframes,
#ifdef I2R_LOSSLESS
                                      type_PCM,
                                      osf,
#endif
                                      NULL,
                                      *frameData.enc_data);
    if (0 != err) {
      CommonExit(-1, "EncInit returned %i", err);
    }

    DebugPrintf(1,"initialized encoder: %i tracks",frameData.trackCount);

    for (i=0; i<*frameData.trackCount; i++) {
      DebugPrintf(2,"                   track %i: AOT %i",i,frameData.dec_conf[track_count_total+i].audioSpecificConfig.audioDecoderType.value);
    }
    track_count_total += *frameData.trackCount;

    BsFreeBuffer(asc_buffer);
  }

  /* 20060107 */
  sbrenable = (*frameData.enc_data)->getSbrEnable(*frameData.enc_data, &bitRate);
  if((*frameData.enc_data)->getSbrRatioIndex){
    sbrRatioIndex = (*frameData.enc_data)->getSbrRatioIndex(*frameData.enc_data);
  }

  if(sbrenable && (!sbrRatioIndex) ){
    downSampleRatio = 2;
  } else {
    switch(sbrRatioIndex){
    case 0:
      downSampleRatio = 1;
      break;
    case 1:
      downSampleRatio = 4;
      break;
    case 2:
      downSampleRatio = 8;
      upSampleRatio   = 3;
      break;
    case 3:
      downSampleRatio = 2;
      break;
    default:
      assert(0);
      break;
    }
  }

  if (sbrenable || (sbrRatioIndex > 0)) {
    if (fSample != 44100.0f && fSample != 48000.0f) {
    }
    fSample = (upSampleRatio*fSample)/downSampleRatio;
    lowPassFreq = 5000.0f;
    if (numChannelBS==1) {
      if (bitRate==24000 && fSample==22050)
        lowPassFreq = 5513.0f; /* startFreq 4 @ 22.05kHz */
      if (bitRate==24000 && fSample==24000)
        lowPassFreq = 5625.0f; /* startFreq 4 @ 24kHz */
    }
    if (numChannelBS==2) {
      if (bitRate==48000 && fSample==22050)
        lowPassFreq = 7924.0f; /* startFreq 9 @ 22.05kHz */
      if (bitRate==48000 && fSample==24000)
        lowPassFreq = 8250.0f; /* startFreq 9 @ 24kHz */
    }
    lowPassFreq = fSample/2.0f;

    {
      /* init resampler and buffers for downsampling by 2 */
      float Fc = lowPassFreq / (fSample * downSampleRatio);
      float dF = 0.1f * Fc; /* 10% transition bw */
      float alpha = 7.865f; /* 80 dB attenuation */
      float D = 5.015f; /* transition width ? */
      NcoefRS = (int)ceil(D/dF) + 1;

      delayTotal = addlCoreDelay = (EncUsac_getUsacDelay(*frameData.enc_data) * downSampleRatio/upSampleRatio);
      usac212enable = EncUsac_getusac212enable(*frameData.enc_data);

      /* downsampling filter coefficients */
      if ((hRS = (float *)calloc(NcoefRS, sizeof(float)))==NULL)
        CommonExit(1,"Encode: memory allocation error");
#ifdef USE_AFSP
      RSKaiserLPF (hRS, NcoefRS, (double)Fc, (double)alpha);
#else
      CommonExit(1,"AFSP not available.");
#endif

      Fc = 1.047198f; /* = pi/upsampleRatio = pi/3 */
      dF = 0.1f * Fc;
      NcoefUS = (int)ceil(D/dF) + 1;
      NcoefUS = ((NcoefUS+upSampleRatio-1)/upSampleRatio)*upSampleRatio;

      /* downsampling filter coefficients */
      if ((hUS = (float *)calloc(NcoefUS, sizeof(float)))==NULL)
        CommonExit(1,"Encode: memory allocation error");
#ifdef USE_AFSP
      RSKaiserLPF (hUS, NcoefUS, (double)Fc, (double)alpha);
#else
      CommonExit(1,"AFSP not available.");
#endif

      /* input buffers */
      if ((sampleBufRS=(float**)malloc(numChannel*sizeof(float*)))==NULL)
        CommonExit(1,"Encode: memory allocation error");
      if ((sampleBufRSold=(float**)malloc(numChannel*sizeof(float*)))==NULL)
        CommonExit(1,"Encode: memory allocation error");
      if ((tmpBufRS=(float**)malloc(numChannel*sizeof(float*)))==NULL)
        CommonExit(1,"Encode: memory allocation error");
      if ((additionalCoreDelayBuffer=(float**)malloc(numChannel*sizeof(float*)))==NULL)
        CommonExit(1,"Encode: memory allocation error");
      if ((sampleBufUS=(float**)malloc(numChannel*sizeof(float*)))==NULL)
        CommonExit(1,"Encode: memory allocation error");
      if ((sampleBufUSold=(float**)malloc(numChannel*sizeof(float*)))==NULL)
        CommonExit(1,"Encode: memory allocation error");
      if ((tmpBufUS=(float**)malloc(numChannel*sizeof(float*)))==NULL)
        CommonExit(1,"Encode: memory allocation error");

      for (ch=0; ch<numChannel; ch++) {
        /* additional core encoder delay */
        if ((additionalCoreDelayBuffer[ch]=(float*)calloc(2*downSampleRatio*frameNumSample/upSampleRatio,sizeof(float)))== NULL)
          CommonExit(1,"Encode: memory allocation error");

        /* down sampling */
        if ((sampleBufRS[ch]=(float*)calloc(downSampleRatio*frameNumSample+NcoefUS,sizeof(float)))== NULL)
          CommonExit(1,"Encode: memory allocation error");
        if ((sampleBufRSold[ch]=(float*)calloc(NcoefRS+delayTotal,sizeof(float)))==NULL)
          CommonExit(1,"Encode: memory allocation error");
        if ((tmpBufRS[ch]=(float*)calloc(NcoefRS+delayTotal+downSampleRatio*frameNumSample,sizeof(float)))==NULL)
          CommonExit(1,"Encode: memory allocation error");

        /* up sampling */
        if ((sampleBufUS[ch]=(float*)calloc(downSampleRatio*frameNumSample/upSampleRatio,sizeof(float)))== NULL)
          CommonExit(1,"Encode: memory allocation error");
        if ((sampleBufUSold[ch]=(float*)calloc(NcoefUS,sizeof(float)))==NULL)
          CommonExit(1,"Encode: memory allocation error");
        if ((tmpBufUS[ch]=(float*)calloc(NcoefUS+downSampleRatio*frameNumSample/upSampleRatio,sizeof(float)))==NULL)
          CommonExit(1,"Encode: memory allocation error");
      }
    }
  }

  if (verboseLevel == VERBOSE_LVL2) {
    fprintf(stdout,"fSample=%.3f Hz  (int=%ld)\n",fSample,fSampleLong);
    fprintf(stdout,"frameNumSample=%d  (%.6f sec/frame)\n",
           frameNumSample,frameNumSample/fSample);
    fprintf(stdout,"fileNumSample=%ld  (%.3f sec  %.3f frames)\n",
           fileNumSample,fileNumSample/fSample,
           fileNumSample/(float)frameNumSample);
    fprintf(stdout,"startSample=%ld\n",startSample);
    fprintf(stdout,"encNumSample=%ld  (%.3f sec  %.3f frames)\n",
           encNumSample,encNumSample/fSample,
           encNumSample/(float)frameNumSample);
  }

  /* allocate buffers */
#ifdef EXT2PAR
  if (mode == MODE_SSC)
  {
    bitBuf = BsAllocBuffer((SSC_BSC_MAX_BUFFER_LENGTH*8));
  }
  else
#endif
  {
    au_buffers = (HANDLE_BSBITBUFFER*)malloc(track_count_total*sizeof(HANDLE_BSBITBUFFER));
    for (i=0; i<track_count_total; i++) {
      au_buffers[i] = BsAllocBuffer(frameMaxNumBit); /* SAMSUNG_2005-09-30 : modified for Multichanel */  
    }
  }

  if ((sampleBuf=(float**)malloc(numChannel*sizeof(float*)))==NULL)
    CommonExit(1,"Encode: memory allocation error");
  for (ch=0; ch<numChannel; ch++)
    if ((sampleBuf[ch]=(float*)malloc(MAX_OSF*frameNumSample*sizeof(float)))==NULL)
      CommonExit(1,"Encode: memory allocation error");

  if ((reSampleBuf=(float**)malloc(MAX_OSF*numChannel*sizeof(float*)))==NULL)
    CommonExit(1,"Encode: memory allocation error");
  for (ch=0; ch<numChannel; ch++)
    if ((reSampleBuf[ch]=(float*)malloc(frameNumSample*sizeof(float)))==NULL)
      CommonExit(1,"Encode: memory allocation error");

  /* ---- open bit stream file ---- */
  outfile = StreamFileOpenWrite(bitFileName, FILETYPE_AUTO, NULL);

  if (outfile == NULL) {
    CommonExit(1, "Encode: error opening bit stream file %s",bitFileName);
  }

  outprog             = StreamFileAddProgram(outfile);
  outprog->trackCount = track_count_total;

  if (numAPRframes <= 0 || numAPRframes > 3) {
    CommonExit(1, "Encoder setup does not match the PreRoll requirements.");
  }

  /* editlist-support for USAC */
#ifdef EDITLIST_SUPPORT
  delayEditlist = 0;
  if (0 == strcmp(codecMode, "usac")) {
    StreamFileSetEditlist(outprog,
                          0,
                          1,
                          numAPRframes);
  }
#endif


  for (i = 0; i < track_count_total; i++) {
    outprog->decoderConfig[i] = frameData.dec_conf[i];
    outprog->dependencies[i]  = i - 1;
    outprog->sbrRatioIndex[i] = sbrRatioIndex;

    DebugPrintf(2," track %i: AOT %i", i, outprog->decoderConfig[i].audioSpecificConfig.audioDecoderType.value);
  }

  EncUsac_getUsacEncoderDelay(*frameData.enc_data,
                              &delayEncSamples,
                              &delayEncFrames,
                              downSampleRatio,
                              upSampleRatio,
                              (upSampleRatio > 1)   ? NcoefUS : 1,
                              (downSampleRatio > 1) ? NcoefRS : 1,
                              usac212enable,
                              &delayBufferSamples);

  if (numAPRframes > 0) {
    enableIPF = 1;

    if (delayEncFrames - numAPRframes < 0) {
      CommonExit(1, "Encoder delay line does not match IPF requirements.");
    }
  }

  DebugPrintf(2,"total track count: %i\n",(int)outprog->trackCount);

  /* num frames to start up encoder due to delay compensation */
  startupNumFrame = (delayNumSample+frameNumSample-1)/frameNumSample;

  /* seek to beginning of first (startup) frame (with delay compensation) */
  AudioSeek(audioFile,
            startSample+delayNumSample-startupNumFrame*frameNumSample);

  if (verboseLevel == VERBOSE_LVL2) {
    fprintf(stdout,"startupNumFrame=%d\n",startupNumFrame);
  }

  /* process audio file frame by frame */
  frame = -startupNumFrame;
  totNumSample = 0;

  for (i=0; i<track_count_total; i++) {
    DebugPrintf(2,"                   track %i: AOT %i",i,frameData.dec_conf[i].audioSpecificConfig.audioDecoderType.value);
  }

  /* **************************************************************** */
  /* MAIN ENCODING LOOP */
  /* **************************************************************** */
  do {
#ifdef FHG_DEBUGPLOT
    /*plotInit(framePlot,BsGetFileName(bitStream),1);*/
#endif

    if ((verboseLevel <= VERBOSE_LVL1) && (frame%20 == 0)) {
      fprintf(stdout,"\rframe %4d",frame);
      fflush(stdout);
    }
    if (verboseLevel == VERBOSE_LVL2) {
      fprintf(stdout,"\rframe %4d\n",frame);
      fflush(stdout);
    }

    /* check for startup frame */
    startupFrame = frame < 0;

    /* number of samples processed after this frame */
    nSamplesProcessed = (frame + 1) * (long)frameNumSample * downSampleRatio / upSampleRatio - delayBufferSamples;

    /* 20060107 */
    if(sbrenable){
      /* save last NcoefRS samples */
      for (ch=0; ch<numChannel; ch++)
        for (i=0; i<NcoefRS+delayTotal; i++)
          sampleBufRSold[ch][i] = sampleBufRS[ch][downSampleRatio*frameNumSample-(NcoefRS+delayTotal)+i];

      for (ch=0; ch<numChannel; ch++)
        for (i=0; i<NcoefUS/upSampleRatio; i++)
          sampleBufUSold[ch][i] = sampleBufUS[ch][downSampleRatio*frameNumSample/upSampleRatio-(NcoefUS/upSampleRatio)+i];
    }
    /* 20060107 */
    /* read audio file */
    /* 20060107 */
    if (sbrenable) {
      numSample = AudioReadData(audioFile,sampleBufRS,frameNumSample*downSampleRatio/upSampleRatio);

        /* Consider an additional delay caused by the core encoder */
        for (ch = 0; ch < numChannel; ch++) {
          int coreDelay = delayBufferSamples;
          int nSamples  = frameNumSample * downSampleRatio/upSampleRatio;

          for (i = 0; i < coreDelay; i++) {
            additionalCoreDelayBuffer[ch][i]                        = sampleBufRS[ch][nSamples - coreDelay + i];
          }
          for (i = nSamples - 1; i >= coreDelay; i--) {
            sampleBufRS[ch][i]                                      = sampleBufRS[ch][i - coreDelay];
          }
          for (i = 0; i < coreDelay; i++) {
            sampleBufRS[ch][i]                                      = additionalCoreDelayBuffer[ch][2*nSamples - coreDelay + i];
            additionalCoreDelayBuffer[ch][2*nSamples - coreDelay + i] = additionalCoreDelayBuffer[ch][i];
          }
        }
    }
    /* read audio file */
    else {
      numSample = AudioReadData(audioFile,sampleBuf,osf*frameNumSample);
      numSample /= osf;
    }
    totNumSample += numSample;
    if (numSample != frameNumSample && encNumSample == -1)
      encNumSample = totNumSample;

    /* get input signals for classification, process each frame with length 1024 samples */
    if (classfyData.isSwitchMode) {
      int di;
      for (di = 0; di < numSample; di++) {
        classfyData.input_samples[classfyData.n_buffer_samples + di] = sampleBufRS[0][di];   
      }
      classfyData.n_buffer_samples += numSample;
      classification(&classfyData);
    }

    if (sbrenable){
      int nChannelsResamp = numChannel;

      if (usac212enable){
        float inputBuffer[2 * MAX_BUFFER_SIZE];
        float downmixBuffer[2 * MAX_BUFFER_SIZE];
        Stream bitstream;
        HANDLE_SPATIAL_ENC enc = EncUsac_getSpatialEnc(*frameData.enc_data);
        int indepFlagOffset = enc->nBitstreamDelayBuffer - 1;
        unsigned char *databuf;
        int i, j;
        int size;
        int nchDmx = enc->outputChannels;

        InitStream(&bitstream, NULL, STREAM_WRITE);

        for (i = 0; i < MAX_BUFFER_SIZE; i++) {
          for (j = 0; j < numChannel; j++) {
            /* ote is NcoefUS a mistake for SBR 2:1 and 4:1 */
            if (i < downSampleRatio * frameNumSample + NcoefUS) {
              inputBuffer[i*numChannel+j] = sampleBufRS[j][i];
            } else {
              inputBuffer[i*numChannel+j] = 0.0f;
            }
          }
        }

        SpatialEncApply(enc, inputBuffer, downmixBuffer, &bitstream, EncUsac_getIndependencyFlag( *frameData.enc_data, 0));

        size = GetBitsInStream(&bitstream);

        ByteAlignWrite(&bitstream);

        databuf = bitstream.buffer;

        EncUsac_setSpatialData(*frameData.enc_data, databuf, size);

        /* downmix signal */
        if (nchDmx == 2) {
          float inv_sqrt2 = (float)(1.0/sqrt(2.0));
          for (i = 0; i < 2*frameNumSample; i++) {
            float tmp = inv_sqrt2*(downmixBuffer[2*i] - downmixBuffer[2*i+1]);
            downmixBuffer[2*i]   = inv_sqrt2*(downmixBuffer[2*i] + downmixBuffer[2*i+1]);
            downmixBuffer[2*i+1] = tmp;
          }
        }

        for (i=0; i<downSampleRatio*frameNumSample/upSampleRatio; i++)
          for (ch=0; ch<nchDmx; ch++)
            sampleBufRS[ch][i] = downmixBuffer[nchDmx*i+ch];

        nChannelsResamp = nchDmx;
      }

      if(upSampleRatio > 1){
        for (i=0; i<downSampleRatio*frameNumSample/upSampleRatio; i++)
          for (ch=0; ch<nChannelsResamp; ch++)
            sampleBufUS[ch][i] = sampleBufRS[ch][i];

        for (i=0; i<NcoefUS/upSampleRatio; i++)
          for (ch=0; ch<nChannelsResamp; ch++)
            tmpBufUS[ch][i] = sampleBufUSold[ch][i];
        
        for (i=NcoefUS/upSampleRatio; i<downSampleRatio*frameNumSample/upSampleRatio+(NcoefUS/upSampleRatio); i++)
          for (ch=0; ch<nChannelsResamp; ch++)
            tmpBufUS[ch][i] = sampleBufUS[ch][i-(NcoefUS/upSampleRatio)];

        for (ch=0; ch<nChannelsResamp; ch++)
          FIupsample(tmpBufUS[ch], sampleBufRS[ch], downSampleRatio*frameNumSample, hUS, NcoefUS, upSampleRatio);

      }

      for (i=0; i<(NcoefRS+delayTotal); i++)
        for (ch=0; ch<nChannelsResamp; ch++)
          tmpBufRS[ch][i] = sampleBufRSold[ch][i];
      
      /* Add NcoefRS samples delay to input signal sampleBufRS */
      for (i=(NcoefRS+delayTotal); i<downSampleRatio*frameNumSample+(NcoefRS+delayTotal); i++)
        for (ch=0; ch<nChannelsResamp; ch++)
          tmpBufRS[ch][i] = sampleBufRS[ch][i-(NcoefRS+delayTotal)];
      
#ifdef USE_AFSP
      /* Low-pass filtering and downsampling */
      for (ch=0; ch<nChannelsResamp; ch++)
        FIdownsample(tmpBufRS[ch]+addlCoreDelay, sampleBuf[ch], frameNumSample, hRS, NcoefRS, downSampleRatio);
#else
      CommonExit(1,"AFSP not available.");
#endif
    }

    if (delayEncFrames - numAPRframes == frame) {
      requestIPF = (1 == enableIPF) ? 1 : 0;
    } else {
      requestIPF = 0;
    }

    /* encode one frame */
    track_count_total = 0;

    /* mix down stereo to mono */
    encSampleBuf = sampleBuf;
    if ((numChannelBS==1)&&(numChannel==2)) {
      for (i=0; i<frameNumSample; i++) {
        reSampleBuf[0][i] = (sampleBuf[0][i] + sampleBuf[1][i]) / 2;
      }
      encSampleBuf = reSampleBuf;
    }

    err = frameData.enc_mode->EncFrame(encSampleBuf,
                                       &au_buffers[track_count_total],
                                       requestIPF,
                                       &usacSyncState,
                                       *frameData.enc_data,
                                       sbrenable ? ((upSampleRatio > 1) ? tmpBufUS : tmpBufRS) : NULL);
    if (0 != err) {
      CommonExit(1, "EncFrame returned %i", err);
    }

    track_count_total += *frameData.trackCount;

    if (frame >= delayEncFrames) {
      for (i = 0; i < track_count_total; i++) {
        HANDLE_STREAM_AU au = StreamFileAllocateAU(0);
        au->numBits         = BsBufferNumBit(au_buffers[i]);
        au->data            = BsBufferGetDataBegin(au_buffers[i]);

        switch (usacSyncState) {
          case USAC_SYNCFRAME_IMMEDIATE_PLAY_OUT_FRAME:
            outprog->sampleGroupID[i] = SAMPLE_GROUP_NONE;
            outprog->isIPF[i] = 1;
            break;
          case USAC_SYNCFRAME_INDEPENDENT_FRAME:
            outprog->sampleGroupID[i] = SAMPLE_GROUP_PROL;
            outprog->isIPF[i] = 0;
            break;
          default:
            outprog->sampleGroupID[i] = SAMPLE_GROUP_NONE;
            outprog->isIPF[i] = 0;
        }

        StreamPutAccessUnit(outprog, i, au);
        BsClearBuffer(au_buffers[i]);
        free(au);
      }
    }

    frame++;

#ifdef DEBUGPLOT
    framePlot++;
    plotDisplay(0);
#endif

#ifdef _DEBUG
    assert(nSamplesProcessed == frame*(long)frameNumSample * downSampleRatio / upSampleRatio - delayBufferSamples);
#endif
  } while (encNumSample < 0 || nSamplesProcessed < encNumSample + delayEncSamples
#ifdef I2R_LOSSLESS
           +2*(long)frameNumSample
#endif
           );

  if (verboseLevel <= VERBOSE_LVL1) {
    fprintf(stdout," \n");
  }
  if (verboseLevel == VERBOSE_LVL2) {
    fprintf(stdout,"totNumFrame=%d\n", frame - delayEncFrames);
    fprintf(stdout,"encNumSample=%ld  (%.3f sec  %.3f frames)\n",
           encNumSample,encNumSample/fSample,
           encNumSample/(float)frameNumSample);
    fprintf(stdout,"totNumSample=%ld\n",totNumSample);
  }

  /* free encoder memory */
  if(frameData.enc_data && frameData.enc_mode){
    frameData.enc_mode->EncFree(*frameData.enc_data);
    if(frameData.enc_data){
      free(frameData.enc_data);
    }
    free(frameData.enc_mode);
  }

  if(frameData.trackCount){
    free(frameData.trackCount);
  }

   /* close audio file */
   AudioClose(audioFile);

   /* editlist support for USAC */
#ifdef EDITLIST_SUPPORT
   if (0 == strcmp(codecMode, "usac")) {
     StreamFileSetEditlistValues(outprog,
                                 delayEditlist,
                                 fSampleLong,
                                 totNumSample, 
                                 fSampleLong,
                                 delayEditlist,
                                 frame - delayEncFrames);
   }
#endif

   /* close bit stream file */
   if (StreamFileClose(outfile))
     CommonExit(1,"Encode: error closing bit stream file");

   /* free buffers */
   for (ch=0; ch<numChannel; ch++) {
     free(sampleBuf[ch]);
     free(reSampleBuf[ch]);
   }
   free(sampleBuf);
   free(reSampleBuf);
   if(au_buffers){
     for (i=0; i<track_count_total; i++) {
       BsFreeBuffer(au_buffers[i]);
     }
     free(au_buffers);
   }

   /* free up-/downsampling buffers */
   if(hRS){
     free(hRS);
   }
   
   if(hUS){
     free(hUS);
   }

   if(sampleBufRS){
     for (ch=0; ch<numChannel; ch++) {
       if(sampleBufRS[ch]){
         free(sampleBufRS[ch]);
       }
     }
     free(sampleBufRS);
   }

   if(sampleBufUS){
     for (ch=0; ch<numChannel; ch++) {
       if(sampleBufUS[ch]){
         free(sampleBufUS[ch]);
       }
     }
     free(sampleBufUS);
   }

   if(sampleBufRSold){
     for (ch=0; ch<numChannel; ch++) {
       if(sampleBufRSold[ch]){
         free(sampleBufRSold[ch]);
       }
     }
     free(sampleBufRSold);
   }

   if(sampleBufUSold){
     for (ch=0; ch<numChannel; ch++) {
       if(sampleBufUSold[ch]){
         free(sampleBufUSold[ch]);
       }
     }
     free(sampleBufUSold);
   }

   if(tmpBufRS){
     for (ch=0; ch<numChannel; ch++) {
       if(tmpBufRS[ch]){
         free(tmpBufRS[ch]);
       }
     }
     free(tmpBufRS);
   }

   if(tmpBufUS){
     for (ch=0; ch<numChannel; ch++) {
       if(tmpBufUS[ch]){
         free(tmpBufUS[ch]);
       }
     }
     free(tmpBufUS);
   }

   /* 20060107 */
   return 0;
}



/* ---------- main ---------- */

int main (int argc, char *argv[])
{
  int              error                                   = 0;
  char             wav_InputFile[FILENAME_MAX]             = {0};
  char             mp4_OutputFile[FILENAME_MAX]            = {0};
  char             cfg_FileName[FILENAME_MAX]              = {0};
  char             globalBinariesPath[3 * FILENAME_MAX]    = {0};
  int              useGlobalBinariesPath                   = 1;
  char           * codecMode;
  int              numChannelOut                           = 0;
  float            fSampleOut                              = 0.0f;
  int              mainDebugLevel                          = 0;
  int              regionDuratUsed                         = 0;
  float            regionDurat                             = -1;
  float            regionStart                             = 0;
  HANDLE_ENCPARA   encPara = (HANDLE_ENCPARA)calloc(1, sizeof(ENC_PARA));

  VERBOSE_LEVEL    verboseLevel  = VERBOSE_NONE;  /* defines verbose level */

  RECYCLE_LEVEL    recycleLevel = RECYCLE_ACTIVE;                                                 /* defines the clean-up behavior */
  int              bUseWindowsCommands                     = 0;
  DRC_PARAMS       drcParams = {0};
  sprintf(drcParams.tmpFile_Drc_payload,     "tmpFileUsacEnc_drc_payload_output.bit");
  sprintf(drcParams.tmpFile_Drc_config,      "tmpFileUsacEnc_drc_config_output.bit");
  sprintf(drcParams.tmpFile_Loudness_config, "tmpFileUsacEnc_drc_loudness_output.bit");
  sprintf(drcParams.logfile,                 "drcToolEncoder.log");

  codecMode = MODENAME_USAC;
  
  /* parse command line parameters */
  error = PrintCmdlineHelp(argc, argv);
  if (0 != error) {
    exit(0);
  }

  error = GetCmdline(argc,
                     argv,
                     wav_InputFile,
                     mp4_OutputFile,
                     cfg_FileName,
                     &useGlobalBinariesPath,
                     &numChannelOut,
                     &fSampleOut,
                     encPara,
                     &drcParams,
                     &mainDebugLevel,
                     &verboseLevel,
                     &recycleLevel
                     );

  if (0 != error) {
    CommonExit(1, "Error initializing MPEG USAC Audio decoder. Invalid command line parameters.");
  }

  error = setupUsacEncoder(useGlobalBinariesPath,
                           globalBinariesPath,
                           cfg_FileName,
                           drcParams.binary_DrcEncoder,
                           verboseLevel);
  if (error != 0) {
    return error;
  }

  Encode(wav_InputFile,
         mp4_OutputFile,
         &drcParams,
         codecMode,
         encPara,
         regionStart,
         regionDurat,
         numChannelOut,
         fSampleOut,
         mainDebugLevel,
         verboseLevel);

  /* Check if windows or linux commands should be used */
  bUseWindowsCommands = useWindowsCommands();

  /**********************************/
  /* clean up                       */
  /**********************************/
  removeTempFiles(recycleLevel,
                  bUseWindowsCommands,
                  verboseLevel);

  if( verboseLevel >= VERBOSE_LVL1 ){
    fprintf(stdout,"\n");
    fprintf(stdout,"Finished.\n");
  }

  return 0;
}

/* end of mp4enc.c */


