/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by

Heiko Purnhagen     (University of Hannover / ACTS-MoMuSys)

and edited by

Naoya Tanaka        (Matsushita Communication Industrial Co., Ltd.)
Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
Ali Nowbakht-Irani  (Fraunhofer Gesellschaft IIS)
Markus Werner       (SEED / Software Development Karlsruhe) 

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

Copyright (c) 2000.

$Id: mp4audec.c,v 1.4 2011-05-28 14:25:18 mul Exp $
Decoder frame work


Source file: mp4dec.c


Required modules:
common.o                common module
cmdline.o               command line module
bitstream.o             bit stream module
audio.o                 audio i/o module
dec_par.o               decoder core (parametric)
dec_lpc.o               decoder core (CELP)
dec_tf.o                decoder core (T/F)

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
NT    Naoya Tanaka, Panasonic <natanaka@telecom.mci.mei.co.jp>
BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>
CCETT N.N., CCETT <@ccett.fr>
OK    Olaf Kaehler, Fhg/IIS <kaehleof@iis.fhg.de>

Changes:
18-jun-96   HP    first version
19-jun-96   HP    added .wav format / using new ComposeFileName()
26-jun-96   HP    improved handling of switch -o
04-jul-96   HP    joined with t/f code by BG
09-aug-96   HP    adapted to new cmdline module, added mp4.h
16-aug-96   HP    adapted to new dec.h
                  added multichannel signal handling
                  added cmdline parameters for numChannel, fSample
26-aug-96   HP    CVS
03-sep-96   HP    added speed change & pitch change for parametric core
30-oct-96   HP    additional frame work options
15-nov-96   HP    adapted to new bitstream module
18-nov-96   HP    changed int to long where required
                  added bit stream header options
10-dec-96   HP    added variable bit rate
10-jan-97   HP    using BsGetSkip()
23-jan-97   HP    added audio i/o module
03-feb-97   HP    audio module bug fix
07-feb-97   NT    added PICOLA speed control
14-mar-97   HP    merged FhG AAC code
21-mar-97   BT    various changes (t/f, AAC)
04-apr-97   HP    new option -r for scalable decoder
26-mar-97   CCETT added G729 decoder
07-apr-97   HP    i/o filename handling improved / "-" supported
05-nov-97   HP    update by FhG/UER
30-mar-98   HP    added ts option
06-may-98   HP    aacEOF
07-jul-98   HP    improved bitrate statistics debug output
02-dec-98   HP    merged most FDIS contributions ...
20-jan-99   HP    due to the nature of some of the modifications merged
                  into this code, I disclaim any responsibility for this
                  code and/or its readability -- sorry ...
21-jan-99   HP	  trying to clean up a bit ...
22-jan-99   HP    stdin/stdout "-" support, -d 1 uses stderr
22-apr-99   HP    merging all contributions
19-nov-99   HP    added MOUSECHANGE
19-jun-01   OK    removed G723/G729 dummy calls
21-jul-03   AR/RJ Added Extension 2 parametric coding
28-jan-04   OK    Added program switch
**********************************************************************/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include "obj_descr.h"          /* structs */
#include "common_m4a.h"         /* common module       */
#include "cmdline.h"            /* command line module */
#include "audio.h"              /* audio i/o module    */
#include "mp4au.h"              /* frame work common declarations */

#include "hvxc_struct.h"        /* struct for HVXC(AI 990616) */
#include "dec_par.h"            /* decoder cores ... */
#include "dec_lpc.h"
#include "dec_tf.h"
#ifdef EXT2PAR
#include "SscDec.h"
#endif

#ifdef MPEG12
#include "dec_mpeg12.h"
#endif

#include "statistics.h"
#include "mp4ifc.h"

#ifdef  DEBUGPLOT
#include "plotmtv.h"
#endif

#ifdef V2_2_LINUX_GET_MODULENAME_FIX
#ifdef __linux__
#include <unistd.h>
#endif
#endif

#include <streamfile.h>
#include <decifc.h>
#include <streamfile_helper.h>
#include <libxaacd_export.h>



/* ###################################################################### */
/* ##                     enums, struct and defines                    ## */
/* ###################################################################### */
#define MPEGD_USAC_VERSION_NUMBER    "02.03.00"
#define MPEGD_USAC_MODULE_NAME       "mpegdUSACrs"

#define MPEGUSAC_ERROR_FIRST -666

#if __STDC_VERSION__ < 199901L
#if __GNUC__ >= 2 || _MSC_VER >= 1300
#define __func__ __FUNCTION__
#else
#define __func__ "<unknown>"
#endif
#else
#define __func__ "<unknown>"
#endif
#define USAC_ERROR_POSITION_INFO __FILE__,__func__,__LINE__

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

typedef enum _RETURN_CODE {
  USAC_ERROR_DRC_INIT                   = MPEGUSAC_ERROR_FIRST,
  USAC_ERROR_DRC_GENERIC,
  USAC_ERROR_WAVCUTTER_CLIPPING,
  USAC_ERROR_WAVCUTTER_FILEIO,
  USAC_ERROR_CORE_CLIPPING,
  USAC_ERROR_CORE_GENERIC,
  USAC_ERROR_FRAMEWORK_PLAYOUT,
  USAC_ERROR_FRAMEWORK_MISSINGEXE,
  USAC_ERROR_FRAMEWORK_INVALIDEXE,
  USAC_ERROR_FRAMEWORK_INVALIDCMD,
  USAC_ERROR_FRAMEWORK_BITDEPTH,
  USAC_ERROR_FRAMEWORK_INIT,
  USAC_ERROR_FRAMEWORK_GENERIC          = -1,
  USAC_OK                               = 0
} USAC_RETURN_CODE;

typedef enum _VERBOSE_LEVEL {
  VERBOSE_NONE,
  VERBOSE_LVL1,
  VERBOSE_LVL2
} VERBOSE_LEVEL;

typedef enum _DEBUG_LEVEL {
  DEBUG_NORMAL,
  DEBUG_LVL1,
  DEBUG_LVL2
} DEBUG_LEVEL;

typedef enum _RECYCLE_LEVEL {
  RECYCLE_INACTIVE,
  RECYCLE_ACTIVE
} RECYCLE_LEVEL;

typedef enum _BIT_DEPTH {
  BIT_DEPTH_DEFAULT = 0,
  BIT_DEPTH_16_BIT  = 16,
  BIT_DEPTH_24_BIT  = 24,
  BIT_DEPTH_32_BIT  = 32
} BIT_DEPTH;

typedef enum _USAC_CONFPOINT {
  USAC_DEFAULT_OUTPUT    = 0,    /* 0 - 127:   ISO use */
  USAC_CORE_OUTPUT       = 128,  /* 128 - ...: proprietary use */
  USAC_CORE_ELST_OUTPUT  = 129,
  USAC_CORE_STANDALONE   = 130
} USAC_CONFPOINT;

typedef enum _USAC_DECMODE {
  USAC_DECMODE_DEFAULT,         /* full USAC decoder framework */
  USAC_DECMODE_STANDALONE       /* core decoder only mode */
} USAC_DECMODE;

typedef struct _DRC_PARAMS {
  int uniDrcInterfacePresent;
  float targetLoudnessLevel;
  int targetLoudnessLevelPresent;
  unsigned long drcEffectTypeRequest;
  int drcEffectTypeRequestPresent;
} DRC_PARAMS;

static const char usac_binaries[][3][FILENAME_MAX] =
{
/*
  module description                      Windows executable                      Unix executable
  USAC_DESCRIPTION                        USAC_WIN_EXE                            USAC_UNIX_EXE
  */
  {{"DRC decoder"},                       {"drcToolDecoder.exe"},                 {"drcToolDecoder"}},
  {{"wavCutter"},                         {"wavCutterCmdl.exe"},                  {"wavCutterCmdl"}}
};

static int gUseFractionalDelayDecor = 0;           /* used as extern int in decoder_usac.c */


struct decode_obj_ {
    DEC_DATA* decData;
    HANDLE_STREAMPROG prog;
    unsigned long framesDone;
    DEC_DEBUG_DATA* decDebugData;
    char* decPara;
    DRC_APPLY_INFO    drcInfo;

    int* streamID;
    HANDLE_DECODER_GENERAL hFault;
    int mainDebugLevel;
    int audioDebugLevel;
    int epDebugLevel;
    char* aacDebugString;
    int HEaacProfileLevel;
    int bUseHQtransposer;

    int numChannelOut;
    float fSampleOut;

    AUDIOPREROLL_INFO* aprInfo;
    int AudioPreRollExisting;

    int verboseLevel;
    char* audioFileName;
    int   int24flag;

    AudioFile* audioFile;
    HANDLE_STREAMFILE stream;

};


/* ###################################################################### */
/* ##                 MPEG USAC decoder static functions               ## */
/* ###################################################################### */
static int PrintCmdlineHelp(
            int                               argc,
            char                             *argv0,
            const int                         versionMajor,
            const int                         versionMinor
) {
  fprintf(stdout, "\n");
  fprintf(stdout, "******************* MPEG-D USAC Audio Coder - Edition %d.%d *********************\n", versionMajor, versionMinor);
  fprintf(stdout, "*                                                                             *\n");
  fprintf(stdout, "*                              USAC Decoder Module                            *\n");
  fprintf(stdout, "*                                                                             *\n");
  fprintf(stdout, "*                                  %s                                *\n", __DATE__);
  fprintf(stdout, "*                                                                             *\n");
  fprintf(stdout, "*    This software may only be used in the development of the MPEG USAC Audio *\n");
  fprintf(stdout, "*    standard, ISO/IEC 23003-3 or in conforming implementations or products.  *\n");
  fprintf(stdout, "*                                                                             *\n");
  fprintf(stdout, "*******************************************************************************\n");

  if (argc < 5) {
    fprintf(stdout, "\n" );
    fprintf(stdout, "Usage: %s -if infile.mp4 -of outfile.wav [options]\n\n", argv0);
    fprintf(stdout, "-if <s>                    Path to input mp4\n");
    fprintf(stdout, "-of <s>                    Path to output wav\n");
    fprintf(stdout, "\n[options]:\n\n");
    fprintf(stdout, "-cfg <s>                   Path to binary configuration file\n");
    fprintf(stdout, "-bitdepth <i>              Quantization depth of the decoder output in bits.\n");
    fprintf(stdout, "-use24bitChain             Reduce the internal processing chain to 24bit.\n");
    fprintf(stdout, "                           <default: N/A>, 32bit chain is used by default.\n");
    fprintf(stdout, "-drcInterface <s>          DRC decoder interface file\n");
    fprintf(stdout, "-targetLoudnessLevel <f>   target loudness level of decoder output in [LKFS].\n");
    fprintf(stdout, "                           Note that the -drcInterface parameter has priority\n");
    fprintf(stdout, "                           if it signals the targetLoudness parameter.\n");
    fprintf(stdout, "                           <default: not present>\n");
    fprintf(stdout, "-drcEffectTypeRequest <i>  DRC effect request index according to Table 11 of\n");
    fprintf(stdout, "                           ISO/IEC 23003-4:2015 in hexadecimal format.\n");
    fprintf(stdout, "                           Note that the -drcInterface parameter has priority\n");
    fprintf(stdout, "                           if it signals the drcEffectTypeRequest parameter.\n");
    fprintf(stdout, "                           <default: not present>\n");
    fprintf(stdout, "-nodrc                     Disable DRC processing\n");
    fprintf(stdout, "-nogc                      Disable the garbage collector\n");
    fprintf(stdout, "-cpo <i>                   conformance point output type\n");
    fprintf(stdout, "                           0:\tfull USAC decoder output\n");
    fprintf(stdout, "                           <default: 0>\n");
    fprintf(stdout, "-deblvl <0,1>              Activate debug level: 0 (default), 1\n");
    fprintf(stdout, "-v                         Activate verbose level 1.\n");
    fprintf(stdout, "-vv                        Activate verbose level 2.\n");
    fprintf(stdout, "\n[formats]:\n\n");
    fprintf(stdout, "<s>                        string\n");
    fprintf(stdout, "<i>                        integer number\n");
    fprintf(stdout, "<f>                        floating point number\n");
    fprintf(stdout, "\n");
    return -1;
  }

  return 0;
}

static USAC_RETURN_CODE GetCmdline(
            int                               argc,
            char                            **argv,
            char                             *mp4_InputFile,
            char                             *wav_OutputFile,
            char                             *cfg_InputFile,
            int                              *useGlobalBinariesPath,
            DRC_PARAMS                       *drcParams,
            char                             *inputFile_drcInterface,
            int                              *mainDebugLevel,
            BIT_DEPTH                        *outputBitDepth,
            BIT_DEPTH                        *mpegUsacBitDepth,
            USAC_CONFPOINT                   *mpegUsacCpoType,
            USAC_DECMODE                     *mpegUsacDecMode,
            RECYCLE_LEVEL                    *recycleLevel,
            VERBOSE_LEVEL                    *verboseLevel,
            DEBUG_LEVEL                      *debugLevel
) {
  USAC_RETURN_CODE error_usac = USAC_OK;
  int i                       = 0;
  int required                = 0;

  for (i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "-if")) {                                /* Required */
      strncpy(mp4_InputFile, argv[++i], FILENAME_MAX) ;
      required++;
      continue;
    }
    else if (!strcmp(argv[i], "-of")) {                           /* Required */
      strncpy(wav_OutputFile, argv[++i], FILENAME_MAX) ;
      required++;
      continue;
    }
    else if (!strcmp(argv[i], "-cfg")) {                          /* Optional */
      strncpy(cfg_InputFile, argv[++i], FILENAME_MAX);
      *useGlobalBinariesPath = 0;
      continue;
    }
    else if (!strcmp(argv[i], "-bitdepth")) {                     /* Optional */
      int tmp = atoi(argv[++i]);

      switch (tmp) {
        case 16:
        case 24:
        case 32:
          *outputBitDepth = (BIT_DEPTH)tmp;
          break;
        default:
          *outputBitDepth = BIT_DEPTH_DEFAULT;
      }
      continue;
    }
    else if (!strcmp(argv[i], "-use24bitChain")) {                /* Optional */
      *mpegUsacBitDepth = BIT_DEPTH_24_BIT;
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
    else if (!strcmp(argv[i], "-nogc")) {                         /* Optional */
      *recycleLevel = RECYCLE_INACTIVE;
      continue;
    }
    else if (!strcmp(argv[i], "-cpo")) {                          /* Optional */
      int tmp = atoi(argv[++i]);

      switch (tmp) {
        case 128:
          *mpegUsacCpoType = USAC_CORE_OUTPUT;
          break;
        case 129:
          *mpegUsacCpoType = USAC_CORE_ELST_OUTPUT;
          break;
        case 130:
          *mpegUsacCpoType = USAC_CORE_ELST_OUTPUT;
          *mpegUsacDecMode = USAC_DECMODE_STANDALONE;
          break;
        default:
          *mpegUsacCpoType = USAC_DEFAULT_OUTPUT;
      }
      continue;
    }
    else if (!strcmp(argv[i], "-drcInterface")) {                 /* Optional */
      strncpy(inputFile_drcInterface, argv[++i], FILENAME_MAX);
      drcParams->uniDrcInterfacePresent = 1;
      continue;
    }
    else if (!strcmp(argv[i], "-targetLoudnessLevel")) {          /* Optional */
      drcParams->targetLoudnessLevel = (float) atof(argv[++i]);
      drcParams->targetLoudnessLevelPresent = 1;
      continue;
    }
    else if (!strcmp(argv[i], "-drcEffectTypeRequest")) {         /* Optional */
      drcParams->drcEffectTypeRequest = (unsigned long)strtol(argv[++i], NULL, 16);
      drcParams->drcEffectTypeRequestPresent = 1;
      continue;
    }
    else if (!strcmp(argv[i], "-nodrc")) {                        /* Optional */
      *mpegUsacCpoType = USAC_CORE_ELST_OUTPUT;
      continue;
    }
    else if (!strcmp(argv[i], "-deblvl")) {                       /* Optional */
      int tmp = atoi(argv[++i]);

      switch (tmp) {
        case 1:
        case 2:
          *debugLevel = (DEBUG_LEVEL)tmp;
          break;
        default:
          *debugLevel = DEBUG_NORMAL;
      }
      *mainDebugLevel = tmp;
      continue;
    }
  }

  /* Sanity check for interface vs. target loudness + effect type. */
  if (drcParams->uniDrcInterfacePresent && (drcParams->targetLoudnessLevelPresent || drcParams->drcEffectTypeRequestPresent) ){
    /* Give the interface file priority */
    drcParams->targetLoudnessLevelPresent  = 0;
    drcParams->drcEffectTypeRequestPresent = 0;
  }

  /* validate command line arguments */
  if (required != 2) {
    error_usac = USAC_ERROR_FRAMEWORK_INVALIDCMD;
  }
  if (drcParams->targetLoudnessLevel > 0.0f) {
    error_usac = USAC_ERROR_FRAMEWORK_INVALIDCMD;
  }
  
  return error_usac;
}

/* print USAC error code */
static USAC_RETURN_CODE PrintErrorCode(
            const USAC_RETURN_CODE            mpegUsac_code,
            int                              *error_depth,
            const char                       *error_msg,
            const char                       *error_hlp,
            const VERBOSE_LEVEL               verboseLvl,
            const char                       *file,
            const char                       *func,
            const int                         line
) {
  USAC_RETURN_CODE error = mpegUsac_code;
  int error_hlp_present  = 0;

  if (NULL != error_hlp) {
    if (strncmp("\0", error_hlp, 1)) {
      error_hlp_present = 1;
    }
  }

  if (NULL != error_msg && NULL != error_depth && USAC_OK != mpegUsac_code) {
    /* only print deepest error */
    if (*error_depth == 0) {
      if (0 == error_hlp_present) {
        /* print error message: */
        fprintf(stderr, "\n%s (Code: %d)\n\tTry -v for more help.\n\n", error_msg, error);
      } else {
        /* print error message + extended help: */
        fprintf(stderr, "\n%s (Code: %d)\n\t%s\n\n", error_msg, error, error_hlp);
      }
      switch (verboseLvl) {
        case VERBOSE_LVL1:
          /* add some more information */
          fprintf(stderr, "Error information:\n\tCode: %d\n\tfunc: %s()\n\tline: %d\n\n", error, func, line);
          (*error_depth)++;
          break;
        case VERBOSE_LVL2:
          /* add even more information */
          fprintf(stderr, "Error information:\n\tCode: %d\n\tfile: %s\n\tfunc: %s()\n\tline: %d\n\n", error, file, func, line);
          break;
        default:
          (*error_depth)++;
      }
    }
  }

  return error;
}

/* evaluate version string */
static int getVersion_evalString(
            const char*                       versionString,
            char*                             versionSubString,
            const int                         maxSubStringLength,
            const int                         offset,
            int*                              versionNumber
) {
  int i = 0;
  int j = 0;

  for (i = 0; i < maxSubStringLength; i++) {
    versionSubString[i] = versionString[offset + j++];

    if ((versionSubString[i] < '0') || (versionSubString[i] > '9')) {
      return 1;
    }
  }

  *versionNumber = atoi(versionSubString);

  return 0;
}

/* get software version number from string */
static int getVersion(
            const char*                       versionString,
            int*                              versionMajor,
            int*                              versionMinor,
            int*                              versionRMbuild
) {
  int error         = 0;
  int i             = 0;
  int j             = 0;
  char subString[2] = {'\0'};

  error = getVersion_evalString(versionString,
                                subString,
                                2,
                                0,
                                versionMajor);
  if (0 != error) {
    return error;
  }

  error = getVersion_evalString(versionString,
                                subString,
                                2,
                                3,
                                versionMinor);
  if (0 != error) {
    return error;
  }

  error = getVersion_evalString(versionString,
                                subString,
                                2,
                                6,
                                versionRMbuild);

  return error;
}

static int checkExistencyAndRemoveNewlineChar(
            char                             *string,
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
            const char                       *module_description,
            char                             *module_binary,
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
            const char                       *globalBinariesPath,
            const char                       *module_description,
            const char                       *module_name,
            char                             *module_binary,
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
            FILE                             *f,
            const int                         nChars,
            const char                       *module_description,
            char                             *module_binary,
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
            char                             *cfgFile,
            int                              *nBinaries,
            char                             *binary_DrcDecoder,
            char                             *binary_WavCutter,
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
    if (!strncmp(line, "binary_drcToolDecoder:", 21)) {
      retError = getBinaries_module(f,
                                    len,
                                    usac_binaries[0][USAC_DESCRIPTION],
                                    binary_DrcDecoder,
                                    verboseLevel);
      if (0 != retError) {
        return retError;
      }
      nBinaries_tmp++;
    }

    if (!strncmp(line, "binary_WavCutter:", 16)) {
      retError = getBinaries_module(f,
                                    len,
                                    usac_binaries[1][USAC_DESCRIPTION],
                                    binary_WavCutter,
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
            char                             *globalBinariesPath,
            int                              *nBinaries,
            char                             *binary_DrcDecoder,
            char                             *binary_WavCutter,
            const VERBOSE_LEVEL               verboseLevel
) {
  int retError      = 0;
  int nBinaries_tmp = 0;

  /***********************************************
               binary_DrcDecoder
  ***********************************************/
  retError = setBinaries_module(globalBinariesPath,
                                usac_binaries[0][USAC_DESCRIPTION],
                                usac_binaries[0][USAC_EXE],
                                binary_DrcDecoder,
                                verboseLevel);
  if (0 != retError) {
    return retError;
  }
  nBinaries_tmp++;

  /***********************************************
               binary_WavCutter
  ***********************************************/
  retError = setBinaries_module(globalBinariesPath,
                                usac_binaries[1][USAC_DESCRIPTION],
                                usac_binaries[1][USAC_EXE],
                                binary_WavCutter,
                                verboseLevel);
  if (0 != retError) {
    return retError;
  }
  nBinaries_tmp++;

  *nBinaries = nBinaries_tmp;

  return retError;
}

static int setupUsacDecoder(
            const int                         useGlobalPath,
            char                             *globalPath,
            char                             *cfg_InputFile,
            char                             *binary_DrcDecoder,
            char                             *binary_WavCutter,
            int                              *error_depth,
            const VERBOSE_LEVEL               verboseLevel
) {
  int error      = 0;
  int nBinaries  = 0;
  char* basename = NULL;

  fprintf(stdout, "\n");
  fprintf(stdout, "::-----------------------------------------------------------------------------\n::    ");
  fprintf(stdout, "Setup decoder...\n");
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
      CommonExit(1, "Error initializing USAC Decoder.");
    }
#endif
#endif

    #ifdef _WIN32
  //  basename = strstr(globalPath, "usacDec.exe");
#else
    basename = strstr(globalPath, "usacDec");
#endif

  //  if (basename == 0) {
  //    CommonExit(1, "Error initializing USAC Decoder.");
  //  } else {
  //    strcpy(basename, "\0");
  // }

    /* set the binary names */
    fprintf(stdout, "Collecting executables...\n");
    error = setBinaries(globalPath,
                        &nBinaries,
                        binary_DrcDecoder,
                        binary_WavCutter,
                        verboseLevel);
    if (error) {
      fprintf(stderr,"Error locating USAC decoder binaries\n");
    }
  } else {
    /* Get the binary names */
    if (VERBOSE_LVL1 <= verboseLevel) {
      fprintf(stdout, "Parsing %s file...\n", cfg_InputFile);
    }

    fprintf(stdout, "Collecting executables...\n");
    error = getBinaries(cfg_InputFile,
                        &nBinaries,
                        binary_DrcDecoder,
                        binary_WavCutter,
                        verboseLevel);
    if (error) {
      fprintf(stderr,"Error locating USAC decoder binaries\n");
    }
  }

  if (VERBOSE_LVL1 <= verboseLevel) {
    fprintf(stdout, "\n>>    %d executables loaded!\n", nBinaries);
  }

  return 0;
}

static void MakeLogFileNames(
            char                             *wav_OutputFile,
            char                             *logFileNames,
            const int                         bUseWindowsCommands,
            const VERBOSE_LEVEL               verboseLevel
) {
  char tmp[3 * FILENAME_MAX]  = {"\0"};
  char tmp2[3 * FILENAME_MAX] = {"\0"};

  if (VERBOSE_LVL1 <= verboseLevel) {
    strncpy(tmp2, wav_OutputFile, strlen(wav_OutputFile) - 4);
    tmp2[strlen(wav_OutputFile) - 4] = '\0';

    strncpy(tmp, logFileNames, strlen(logFileNames));
    sprintf(logFileNames, "%s_%s", tmp2, tmp);
  } else {
    if (1 == bUseWindowsCommands) {
      sprintf(logFileNames, "nul");
    } else {
      sprintf(logFileNames, "/dev/null");
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

static int checkIfFileExists(
            char                             *string,
            char                             *suffix
) {
  char name[FILENAME_MAX] = {'\0'};
  FILE* fExists           = NULL;

  if (suffix != NULL) {
    sprintf(name, "%s.%s", string, suffix);
  } else {
    sprintf(name, "%s", string);
  }

  fExists = fopen ( name, "r" );

  if (!fExists) {
    return 0;
  }

  fclose(fExists);

  return 1;
}

/* removes .wav suffix from filename */
static int removeWavExtension(
            char                             *filename
) {
  char* wavSuffix = strstr(filename, ".wav");

  if (wavSuffix != NULL) {
    /* remove the file extension */
    strcpy(wavSuffix, "\0");
  } else {
    return -1;
  }

  return 0;
}

/* call binary - wrapper for system() */
static int callBinary(
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

static void removeTempFiles(
            const RECYCLE_LEVEL               removeFiles,
            const int                         bUseWindowsCommands,
            const VERBOSE_LEVEL               verboseLevel
) {
  if (RECYCLE_ACTIVE == removeFiles) {
    if (VERBOSE_LVL1 <= verboseLevel) {
      fprintf(stdout, "Deleting intermediate files...\n");
    }

    if (bUseWindowsCommands) {
      system("del tmpFileUsacDec*.* 2> NUL");
    } else {
      system("rm -f tmpFileUsacDec*.*");
    }
  } else {
    if (VERBOSE_LVL1 <= verboseLevel) {
      fprintf(stdout, "Keeping intermediate files!\n");
    }
  }
}

static USAC_RETURN_CODE copyToOutputFile(
            char                             *inputFileName,
            char                             *outputFileName,
            char                             *logFileNames,
            char                             *binary_WavCutter,
            const int                         forceCopy,
            const BIT_DEPTH                   bitdepth,
            const int                         delay,
            const int                         truncation,
            const VERBOSE_LEVEL               verboseLevel,
            const DEBUG_LEVEL                 debugLevel
) {
  int error                        = 0;
  int truncatedSamples             = truncation;
  char callCmd[3 * FILENAME_MAX]   = {'\0'};
  char tmpString[3 * FILENAME_MAX] = {'\0'};

  if (BIT_DEPTH_DEFAULT == bitdepth) {
    sprintf(callCmd, "%s -if %s -of %s -delay %d -truncate %d -bitdepth %d -deblvl %d", binary_WavCutter, inputFileName, outputFileName, delay, truncatedSamples, 24, (int)debugLevel);
  } else {
    sprintf(callCmd, "%s -if %s -of %s -delay %d -truncate %d -bitdepth %d -deblvl %d", binary_WavCutter, inputFileName, outputFileName, delay, truncatedSamples, (int)bitdepth, (int)debugLevel);
  }

  sprintf(tmpString, " >> %s 2>&1", logFileNames);
  strcat(callCmd, tmpString);

  error = callBinary(callCmd, "WavCutter", verboseLevel);
  if (0 != error) {
    switch (error) {
      case -2:            /* can not open input/output file */
        return USAC_ERROR_WAVCUTTER_FILEIO;
        break;
      case -41:           /* -41 = WAVIO_ERROR_CLIPPED */
        return USAC_ERROR_WAVCUTTER_CLIPPING;
        break;
      default:
        return USAC_ERROR_CORE_GENERIC;
    }
  }

  return USAC_OK;
}

static USAC_RETURN_CODE playOutCpo(
            const USAC_CONFPOINT              mpegUsacCpoType,
            const int                         delay,
            const int                         truncation,
            char                             *inputFileName,
            char                             *outputFileName,
            char                             *logFileNames,
            char                             *binary_WavCutter,
            char                             *signalPathID,
            const VERBOSE_LEVEL               verboseLevel,
            const DEBUG_LEVEL                 debugLevel
) {
  USAC_RETURN_CODE error_usac   = USAC_OK;
  char tmpName[FILENAME_MAX]    = {'\0'};
  char tmpExt[15]               = {'\0'};

  if (USAC_DEFAULT_OUTPUT != mpegUsacCpoType) {
    if (NULL != signalPathID) {
      strcpy(tmpName, outputFileName);
      removeWavExtension(tmpName);

      sprintf(tmpExt, "_%s.wav", signalPathID);
      strcat(tmpName, tmpExt);
    } else {
      strcpy(tmpName, outputFileName);
    }

    error_usac = copyToOutputFile(inputFileName,
                                  tmpName,
                                  logFileNames,
                                  binary_WavCutter,
                                  1,
                                  BIT_DEPTH_24_BIT,
                                  delay,
                                  truncation,
                                  verboseLevel,
                                  debugLevel);
    if (USAC_OK != error_usac) {
      return error_usac;
    }
  }

  return error_usac;
}

static int generateDRCeffectFallback(
            const unsigned long               drcEffectTypeRequestCmdl,
            int                              *numDrcEffectTypeRequests,
            int                              *drcEffectTypeRequest,
            const int                         maxNumDrcEffectTypes
) {
  int retError                 = 0;
  int i                        = 0;
  int drcEffectTypeRequested   = 0;
  unsigned long tmp            = drcEffectTypeRequestCmdl;

  if (8 != maxNumDrcEffectTypes) {
    return -1;
  }

  *numDrcEffectTypeRequests = 0;

  for (i = 0; i < maxNumDrcEffectTypes; i++) {
    drcEffectTypeRequested = tmp & 15;
    if (i == 0 || drcEffectTypeRequested != 0) {
      *numDrcEffectTypeRequests += 1;
      drcEffectTypeRequest[i] = drcEffectTypeRequested;
    } else {
      break;
    }
    tmp >>= 4;
  }

  /* recommended DRC fallback effect according to ISO/IEC 23003-4:2015, Annex E.2.2 */
  if (*numDrcEffectTypeRequests == 1 && drcEffectTypeRequest[0] > 0 && drcEffectTypeRequest[0] < 7) {
    *numDrcEffectTypeRequests = 6;
    switch (drcEffectTypeRequest[0]) {
      case 1:
        drcEffectTypeRequest[1] = 6;
        drcEffectTypeRequest[2] = 2;
        drcEffectTypeRequest[3] = 3;
        drcEffectTypeRequest[4] = 4;
        drcEffectTypeRequest[5] = 5;
        break;
      case 2:
        drcEffectTypeRequest[1] = 6;
        drcEffectTypeRequest[2] = 1;
        drcEffectTypeRequest[3] = 3;
        drcEffectTypeRequest[4] = 4;
        drcEffectTypeRequest[5] = 5;
        break;
      case 3:
        drcEffectTypeRequest[1] = 6;
        drcEffectTypeRequest[2] = 1;
        drcEffectTypeRequest[3] = 2;
        drcEffectTypeRequest[4] = 4;
        drcEffectTypeRequest[5] = 5;
        break;
      case 4:
        drcEffectTypeRequest[1] = 6;
        drcEffectTypeRequest[2] = 2;
        drcEffectTypeRequest[3] = 1;
        drcEffectTypeRequest[4] = 3;
        drcEffectTypeRequest[5] = 5;
        break;
      case 5:
        drcEffectTypeRequest[1] = 6;
        drcEffectTypeRequest[2] = 1;
        drcEffectTypeRequest[3] = 2;
        drcEffectTypeRequest[4] = 3;
        drcEffectTypeRequest[5] = 4;
        break;
      case 6:
        drcEffectTypeRequest[1] = 1;
        drcEffectTypeRequest[2] = 2;
        drcEffectTypeRequest[3] = 3;
        drcEffectTypeRequest[4] = 4;
        drcEffectTypeRequest[5] = 5;
        break;
    }
  }

  return retError;
}

static int setDrcParams(
            DRC_PARAMS                       *drcParams
) {
  int retError = 0;

  drcParams->drcEffectTypeRequest        = 0;
  drcParams->drcEffectTypeRequestPresent = 0;
  drcParams->targetLoudnessLevel         = 0;
  drcParams->targetLoudnessLevelPresent  = 0;
  drcParams->uniDrcInterfacePresent      = 0;

 return retError;
}

static USAC_RETURN_CODE execDrc(
            DRC_APPLY_INFO                   *drcInfo,
            char                             *audioIn,
            char                             *audioOut,
            char                             *inputFile_drcInterface,
            char                             *logFileNames,
            DRC_PARAMS                       *drcParams,
            char                             *binary_DrcDecoder,
            int                              *error_depth,
            const VERBOSE_LEVEL               verboseLevel
) {
  int error                        = 0;
  int i                            = 0;
  char callDrc[3 * FILENAME_MAX]   = {'\0'};
  char tmpString[3 * FILENAME_MAX] = {'\0'};
  int drcEffectTypeRequest[8]      = {0};
  int numDrcEffectTypeRequests     = 0;

  sprintf(callDrc, "%s -if %s -of %s -af %d -pl", binary_DrcDecoder, audioIn, audioOut, drcInfo->frameLength);

  if (1 == drcInfo->uniDrc_extEle_present) {
    sprintf(tmpString, " -ic %s -ig %s", drcInfo->uniDrc_config_file, drcInfo->uniDrc_payload_file);
    strcat(callDrc, tmpString);
  }

  if (1 == drcInfo->loudnessInfo_configExt_present) {
    sprintf(tmpString, " -il %s", drcInfo->loudnessInfo_config_file);
    strcat(callDrc, tmpString);
  }

  if (1 == drcInfo->sbr_active) {
    sprintf(tmpString, " -gd 257");
    strcat(callDrc, tmpString);
  }

  if (!drcParams->uniDrcInterfacePresent && !drcParams->targetLoudnessLevel && !drcParams->drcEffectTypeRequest) {
    return PrintErrorCode(USAC_ERROR_DRC_INIT, error_depth, "Error initializing DRC decoder.", "Invalid/missing command line parameters.", verboseLevel, USAC_ERROR_POSITION_INFO);
  }

  if (drcParams->uniDrcInterfacePresent) {
    sprintf(tmpString, " -ii %s", inputFile_drcInterface);
    strcat(callDrc, tmpString);
  } else {
    if (drcParams->targetLoudnessLevelPresent) {
      sprintf(tmpString, " -tl %f", drcParams->targetLoudnessLevel);
      strcat(callDrc, tmpString);
    }

    if (drcParams->drcEffectTypeRequestPresent) {
      sprintf(tmpString, " -drcEffect ");
      strcat(callDrc, tmpString);

      error = generateDRCeffectFallback(drcParams->drcEffectTypeRequest,
                                        &numDrcEffectTypeRequests,
                                        drcEffectTypeRequest,
                                        8);

      for (i = numDrcEffectTypeRequests; i > 0; i--) {
        sprintf(tmpString, "%d",drcEffectTypeRequest[i - 1]);
        strcat(callDrc, tmpString);
      }
    } else {
      sprintf(tmpString, " -drcEffect 0");
      strcat(callDrc, tmpString);
    }
  }

  sprintf(tmpString, " >> %s 2>&1", logFileNames);
  strcat(callDrc, tmpString );

  if (0 != callBinary(callDrc, "DRC decoder", verboseLevel)) {
    return USAC_ERROR_DRC_GENERIC;
  }

  return USAC_OK;
}

static USAC_RETURN_CODE execFullDecode(
            char                             *mp4_InputFile,
            char                             *outputFile_CoreDecoder,
            char                             *outputFile_Drc,
            char                             *wav_OutputFile,
            char                             *inputFile_drcInterface,
            char                             *tmpFile_Drc_payload,
            char                             *tmpFile_Drc_config,
            char                             *tmpFile_Loudness_config,
            char                             *binary_DrcDecoder,
            char                             *binary_WavCutter,
            char                             *logFileNames,
            int                              *error_depth,
            DRC_PARAMS                       *drcParams,
            const int                         mainDebugLevel,
            const BIT_DEPTH                   mpegUsacBitDepth,
            const BIT_DEPTH                   outputBitDepth,
            const USAC_CONFPOINT              mpegUsacCpoType,
            const USAC_DECMODE                mpegUsacDecMode,
            const VERBOSE_LEVEL               verboseLevel,
            const DEBUG_LEVEL                 debugLevel
) {
  USAC_RETURN_CODE  error_usac                    = USAC_OK;
  ELST_INFO         elstInfo;
  AUDIOPREROLL_INFO aprInfo;
  DRC_APPLY_INFO    drcInfo;
  const int         monoFilesOut                  = 0;
  const int         maxLayer                      = -1;
  const int         audioDebugLevel               = 0;
  const int         epDebugLevel                  = 0;
  const int         bitDebugLevel                 = 0;
#ifdef HUAWEI_TFPP_DEC
  const int         actATFPP                      = 0;
#endif
#ifdef CT_SBR
  const int         HEaacProfileLevel             = 5;
  const int         bUseHQtransposer              = 0;
#endif
  const int         programNr                     = -1;
  int               streamID                      = -1;
  char             *decPara                       = "";
  char             *aacDebugString                = "";

  /* set temporary DRC file names */
  drcInfo.uniDrc_config_file       = tmpFile_Drc_config;
  drcInfo.uniDrc_payload_file      = tmpFile_Drc_payload;
  drcInfo.loudnessInfo_config_file = tmpFile_Loudness_config;

  if (MP4Audio_ProbeFile(mp4_InputFile)) {
    fprintf(stdout, "Calling core decoder...\n");
    MP4Audio_DecodeFile(mp4_InputFile,
                        (USAC_DECMODE_STANDALONE == mpegUsacDecMode) ? wav_OutputFile : outputFile_CoreDecoder,
                        "wav",
                        (mpegUsacBitDepth == BIT_DEPTH_24_BIT) ? 1 : 0,
                        monoFilesOut,
                        maxLayer,
                        decPara,
                        mainDebugLevel,
                        audioDebugLevel,
                        epDebugLevel,
                        bitDebugLevel,
                        aacDebugString,
#ifdef CT_SBR
                        HEaacProfileLevel,
                        bUseHQtransposer,
#endif
#ifdef HUAWEI_TFPP_DEC
                        actATFPP,
#endif
                        programNr,
                        &drcInfo,
                        &aprInfo,
                        &elstInfo,
                        (mpegUsacCpoType == USAC_CORE_ELST_OUTPUT) ? ELST_MODE_CORE_LEVEL : ELST_MODE_EXTERN_LEVEL,
                        &streamID,
                        (USAC_DECMODE_STANDALONE == mpegUsacDecMode) ? CORE_STANDALONE_MODE : CORE_FRAMEWORK_MODE,
                        (int)verboseLevel);
  } else {
    CommonWarning("error decoding audio file %s", mp4_InputFile);
  }

  if (VERBOSE_LVL1 <= verboseLevel) {
    fprintf(stdout, "::-----------------------------------------------------------------------------\n::    ");
    fprintf(stdout, "Stream ID: %d\n", streamID);
    fprintf(stdout, "::-----------------------------------------------------------------------------\n");
    fprintf(stdout, "\n");
  }

  if (USAC_CORE_OUTPUT == mpegUsacCpoType || USAC_CORE_ELST_OUTPUT == mpegUsacCpoType) {
    error_usac = playOutCpo((USAC_DECMODE_STANDALONE == mpegUsacDecMode) ? USAC_DEFAULT_OUTPUT : mpegUsacCpoType,
                            0,
                            0,
                            outputFile_CoreDecoder,
                            wav_OutputFile,
                            logFileNames,
                            binary_WavCutter,
                            NULL,
                            verboseLevel,
                            debugLevel);
    return PrintErrorCode(error_usac, error_depth, "Error executing MPEG-D USAC core decoder.", NULL, verboseLevel, USAC_ERROR_POSITION_INFO);
  }

  if (drcInfo.loudnessInfo_configExt_present || drcInfo.uniDrc_extEle_present) {
    error_usac = execDrc(&drcInfo,
                         outputFile_CoreDecoder,
                         outputFile_Drc,
                         inputFile_drcInterface,
                         logFileNames,
                         drcParams,
                         binary_DrcDecoder,
                         error_depth,
                         verboseLevel);

    if (USAC_OK != error_usac) {
      return PrintErrorCode(error_usac, error_depth, "Error running DRC decoder.", NULL, verboseLevel, USAC_ERROR_POSITION_INFO);
    }
  } else {
    strcpy(outputFile_Drc, outputFile_CoreDecoder);
  }

  /* copy to output */
  error_usac = copyToOutputFile(outputFile_Drc,
                                wav_OutputFile,
                                logFileNames,
                                binary_WavCutter,
                                0,
                                outputBitDepth,
                                aprInfo.nSamplesCoreCoderPreRoll + elstInfo.truncLeft,
                                elstInfo.truncRight,
                                verboseLevel,
                                debugLevel);
  if (USAC_OK != error_usac) {
    return PrintErrorCode(error_usac, error_depth, "Error in reproduction chain.", NULL, verboseLevel, USAC_ERROR_POSITION_INFO);
  }

  return error_usac;
}

static int setAprInfo(AUDIOPREROLL_INFO* aprInfo
) {
    int retVal = 0;

    if (NULL == aprInfo) {
        retVal = 1;
    }
    else {
        aprInfo->numPrerollAU = 0;
        aprInfo->nSamplesCoreCoderPreRoll = 0;
    }

    return retVal;
}

static int setElstInfo(ELST_INFO* elstInfo,
    const double             startOffset,
    const double             durationTotal,
    const long               numOutSamples,
    const unsigned long      framesDone,
    const float              fSampleOut,
    const int                useEditlist
) {
    int retVal = 0;

    if (NULL == elstInfo) {
        retVal = 1;
    }
    else {
        if (useEditlist == 1) {
            unsigned long tmpFileLength = numOutSamples * framesDone;
            double tmpStartOffset = startOffset * fSampleOut + 0.5;
            double tmpPlayTime = durationTotal * fSampleOut + 0.5;

            elstInfo->truncLeft = (long)(tmpStartOffset);
            elstInfo->totalSamplesElst = (long)(tmpPlayTime);
            elstInfo->totalSamplesFile = tmpFileLength;

            if (elstInfo->totalSamplesElst <= elstInfo->totalSamplesFile) {
                elstInfo->truncRight = ((elstInfo->totalSamplesFile - elstInfo->truncLeft) - elstInfo->totalSamplesElst);
            }
            else {
                /* warning: edit list ist longer than actual file length */
                elstInfo->truncRight = 0;
            }
        }
        else {
            elstInfo->truncLeft = 0;
            elstInfo->truncRight = 0;
            elstInfo->totalSamplesElst = 0;
            elstInfo->totalSamplesFile = 0;
        }
    }

    return retVal;
}

static int frameDataAddAccessUnit(DEC_DATA* decData,
    HANDLE_STREAM_AU au,
    int track)
{
    unsigned long unitSize, unitPad;
    HANDLE_BSBITBUFFER bb;
    HANDLE_BSBITSTREAM bs;
    HANDLE_BSBITBUFFER AUBuffer = NULL;
    int idx = track;

    unitSize = (au->numBits + 7) / 8;
    unitPad = (unitSize * 8) - au->numBits;

    bb = BsAllocPlainDirtyBuffer(au->data, /*au->numBits*/ unitSize << 3);
    bs = BsOpenBufferRead(bb);

    AUBuffer = decData->frameData->layer[idx].bitBuf;
    if (AUBuffer != 0) {
        if ((BsBufferFreeBits(AUBuffer)) > (long)unitSize * 8) {
            int auNumber = decData->frameData->layer[idx].NoAUInBuffer;
            BsGetBufferAppend(bs, AUBuffer, 1, unitSize * 8);
            decData->frameData->layer[idx].AULength[auNumber] = unitSize * 8;
            decData->frameData->layer[idx].AUPaddingBits[auNumber] = unitPad;
            decData->frameData->layer[idx].NoAUInBuffer++;/* each decoder must decrease this by the number of decoded AU */
            /* each decoder must remove the first element in the list  by shifting the elements down */
            DebugPrintf(6, "AUbuffer %2i: %i AUs, last size %i, lastPad %i\n", idx, decData->frameData->layer[idx].NoAUInBuffer, decData->frameData->layer[idx].AULength[auNumber], decData->frameData->layer[idx].AUPaddingBits[auNumber]);
        }
        else {
            BsGetSkip(bs, unitSize * 8);
            CommonWarning(" input buffer overflow for layer %d ; skipping next AU", idx);
        }
    }
    else {
        BsGetSkip(bs, unitSize * 8);
    }
    BsCloseRemove(bs, 0);
    free(bb);
    return 0;
}

static int MP4Audio_SetupDecoders(DEC_DATA* decData,
    DEC_DEBUG_DATA* decDebugData,
    int                     streamCount,     /* in: number of streams decoded */
    char* decPara,         /* in: decoder parameter string */
    char* drc_payload_fileName,
    char* drc_config_fileName,
    char* loudness_config_fileName,
    int* streamID,
    HANDLE_DECODER_GENERAL  hFault,
    int                     mainDebugLevel,
    int                     audioDebugLevel,
    int                     epDebugLevel,
    char* aacDebugString
#ifdef CT_SBR
    , int                    HEaacProfileLevel
    , int                    bUseHQtransposer
#endif
#ifdef HUAWEI_TFPP_DEC
    , int                    actATFPP
#endif
) {
    int delayNumSample = 0;

    AudioInit(NULL, audioDebugLevel);

    /*
      init decoders
    */

    decDebugData->aacDebugString = aacDebugString;
    decDebugData->decPara = decPara;
    decDebugData->infoFileName = NULL;
    decDebugData->mainDebugLevel = mainDebugLevel;
    decDebugData->epDebugLevel = epDebugLevel;

    delayNumSample = audioDecInit(hFault,
        decData,
        decDebugData,
        drc_payload_fileName,
        drc_config_fileName,
        loudness_config_fileName,
        streamID,
#ifdef CT_SBR
        HEaacProfileLevel,
        bUseHQtransposer,
#endif
        streamCount
#ifdef HUAWEI_TFPP_DEC
        , actATFPP
#endif
    );

    return delayNumSample;
}

static int usac_core_parsePrerollData(HANDLE_STREAM_AU        au,
    HANDLE_STREAM_AU* prerollAU,
    unsigned int* numPrerollAU,
    AUDIO_SPECIFIC_CONFIG* prerollASC,
    unsigned int* prerollASCLength,
    unsigned int* applyCrossFade
) {
    HANDLE_BSBITBUFFER bb = NULL;
    HANDLE_BSBITSTREAM bs;

    int retVal = 1;
    int nLFE = 0;
    int ascBitCounter = 0;
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned long indepFlag = 0;
    unsigned long extPayloadPresentFlag = 0;
    unsigned long useDefaultLengthFlag = 0;
    unsigned long dummy = 0;
    unsigned long auByte = 0;
    unsigned int ascSize = 0;
    unsigned int auLength = 0;
    unsigned int totalPayloadLength = 0;
    unsigned int nBitsToSkip = 0;

    bb = BsAllocPlainDirtyBuffer(au->data, au->numBits);
    bs = BsOpenBufferRead(bb);

    *applyCrossFade = 0;

    /* Indep flag must be one */
    BsGetBit(bs, &indepFlag, 1);
    if (!indepFlag) {
        goto freeMem;
    }

    /* Payload present flag must be one */
    BsGetBit(bs, &extPayloadPresentFlag, 1);
    if (!extPayloadPresentFlag) {
        goto freeMem;
    }

    /* Default length flag must be zero */
    BsGetBit(bs, &useDefaultLengthFlag, 1);
    if (useDefaultLengthFlag) {
        goto freeMem;
    }

    /* Seems to be a valid preroll extension */
    retVal = 0;

    /* read overall ext payload length */
    UsacConfig_ReadEscapedValue(bs, &totalPayloadLength, 8, 16, 0);

    /* read ASC size */
    UsacConfig_ReadEscapedValue(bs, &ascSize, 4, 4, 8);
    *prerollASCLength = ascSize;

    /* read ASC */
    if (ascSize) {
        ascBitCounter = UsacConfig_Advance(bs, &(prerollASC->specConf.usacConfig), 0);
        prerollASC->channelConfiguration.value = usac_get_channel_number(&prerollASC->specConf.usacConfig, &nLFE);

        /* Skip remaining bits from ASC that were not parsed */
        nBitsToSkip = ascSize * 8 - ascBitCounter;

        while (nBitsToSkip) {
            int nMaxBitsToRead = min(nBitsToSkip, LONG_NUMBIT);
            BsGetBit(bs, &dummy, nMaxBitsToRead);
            nBitsToSkip -= nMaxBitsToRead;
        }
    }

    BsGetBit(bs, (unsigned long*)applyCrossFade, 1);
    BsGetBit(bs, &dummy, 1);

    /* read num preroll AU's */
    UsacConfig_ReadEscapedValue(bs, numPrerollAU, 2, 4, 0);
    assert(*numPrerollAU <= MPEG_USAC_CORE_MAX_AU);

    for (i = 0; i < *numPrerollAU; ++i) {
        /* For every AU get length and allocate memory  to hold the data */
        UsacConfig_ReadEscapedValue(bs, &auLength, 16, 16, 0);
        prerollAU[i] = StreamFileAllocateAU(auLength * 8);

        for (j = 0; j < auLength; ++j) {
            /* Read AU bytewise and copy into AU data buffer */
            BsGetBit(bs, &auByte, 8);
            memcpy(&(prerollAU[i]->data[j]), &auByte, 1);
        }
    }

freeMem:
    BsCloseRemove(bs, 0);
    if (bb) {
        free(bb);
    }

    return retVal;
}

int16_t** ConvertFloatToInt16(float** floatData, int numChannels, long numSamples) {
    int c = 0;
    long s = 0;
    int i = 0;
    float sample;
    int16_t** intData = (int16_t**)malloc(numChannels * sizeof(int16_t*));
    if (!intData) return NULL;

    for (c = 0; c < numChannels; c++) {
        intData[c] = (int16_t*)malloc(numSamples * sizeof(int16_t));
        if (!intData[c]) {
            // �����ڴ����ʧ��
            for (i = 0; i < c; i++) free(intData[i]);
            free(intData);
            return NULL;
        }

        for (s = 0; s < numSamples; s++) {
            sample = floatData[c][s];
            // ���ʹ�����ת��
            if (sample > 32767.0f) {
                intData[c][s] = 32767;
            }
            else if (sample < -32768.0f) {
                intData[c][s] = -32768;
            }
            else {
                intData[c][s] = (int16_t)((sample >= 0) ? (sample + 0.5f) : (sample - 0.5f));
            }
        }
    }
    return intData;
}


decode_obj* xheaacd_create(decode_para* decode_para) {
    int argc = 11;
    char* argv[] = {
        "usacDec",
        "-if", "encode.mp4",
        "-of", "output.wav",
        "-bitdepth", "16",
        "-nodrc",
        "-cpo", "0",
        "-v"
    };
    decode_obj* ctx = (decode_obj*)malloc(sizeof(struct decode_obj_));
    unsigned long framesDone = 0;
    char* decPara = "";
    static int streamID = -1;
    int audioDebugLevel = 0;
    int epDebugLevel = 0;
    int HEaacProfileLevel = 5;
    int bUseHQtransposer = 0;
    int AudioPreRollExisting = 0;

    /* ###################################################################### */
    /* ##                          main parameter                          ## */
    /* ###################################################################### */

    USAC_RETURN_CODE  error_usac = USAC_OK;
    int               error_depth = 0;
    int               versionMajor = 0;
    int               versionMinor = 0;
    int               versionRMbuild = 0;
    char              mp4_InputFile[FILENAME_MAX] = { '\0' };
    char              wav_OutputFile[FILENAME_MAX] = { '\0' };
    char              cfg_FileName[FILENAME_MAX] = { '\0' };
    int               error = 0;
    char              inputFile_drcInterface[FILENAME_MAX] = { '\0' };
    DRC_PARAMS        drcParams;
    BIT_DEPTH         mpegUsacBitDepth = BIT_DEPTH_32_BIT;                       /* bitdepth of the MPEG-D USAC processing chain */
    BIT_DEPTH         outputBitDepth = BIT_DEPTH_DEFAULT;                      /* bitdepth of output file */
    USAC_CONFPOINT    mpegUsacCpoType = USAC_DEFAULT_OUTPUT;                    /* output mode type of the MPEG-D USAC decoder */
    USAC_DECMODE      mpegUsacDecMode = USAC_DECMODE_DEFAULT;                   /* operation mode of the MPEG-D USAC decoder */
    int               UseFftHarmonicTransposer = 0;
    int               USACDecMode = 0;
    int               epFlag = 0;
    int               mainDebugLevel = 0;
    int               useGlobalBinariesPath = 1;
    RECYCLE_LEVEL     recycleLevel = RECYCLE_ACTIVE;                                                    /* defines the clean-up behavior */
    VERBOSE_LEVEL     verboseLevel = VERBOSE_NONE;                                                      /* defines verbose level */
    DEBUG_LEVEL       debugLevel = DEBUG_NORMAL;                                                        /* defines debuging level */
    int               bUseWindowsCommands = 0;
    char              binary_DrcDecoder[FILENAME_MAX] = { '\0' };                                 /* location: DRC module */
    char              binary_WavCutter[FILENAME_MAX] = { '\0' };                                 /* location: wavCutter module */
    char              outputFile_CoreDecoder[FILENAME_MAX] = "tmpFileUsacDec_core_output.wav";
    char              outputFile_Drc[FILENAME_MAX] = "tmpFileUsacDec_drc.wav";
    char              tmpFile_Drc_payload[FILENAME_MAX] = "tmpFileUsacDec_drc_payload_output.bit";
    char              tmpFile_Drc_config[FILENAME_MAX] = "tmpFileUsacDec_drc_config_output.bit";
    char              tmpFile_Loudness_config[FILENAME_MAX] = "tmpFileUsacDec_drc_loudness_output.bit";
    char              logFileNames[FILENAME_MAX] = "framework.log";
    char              globalBinariesPath[3 * FILENAME_MAX] = { '\0' };


    /* ###################################################################### */
    /* ##               MP4Audio_DecodeFile    parameter                   ## */
    /* ###################################################################### */
#define MAX_TRACKS_PER_LAYER 50

    int bWriteIEEEFloat;
    AUDIOPREROLL_INFO aprInfo;
    ELST_INFO         elstInfo;
    static T_DECODERGENERAL faultData[MAX_TF_LAYER];
    static HANDLE_DECODER_GENERAL hFault = &faultData[0];
    const int         bitDebugLevel = 0;
    HANDLE_STREAMFILE stream = NULL;
    int         programNr = -1;
    int progStart, progStop, progCnt;
    HANDLE_STREAMPROG prog = NULL;
    int  suitableTracks = 0;
    AUDIO_SPECIFIC_CONFIG* asc;
    int channelNum , tmp;
    int numFC = 0;
    int fCenter = 0;
    int numSC = 0;
    int numBC = 0;
    int bCenter = 0;
    int numLFE = 0;
    char* aacDebugString = "";
    int    monoFilesOut = 0;
    DEC_DATA* decData = NULL;
    DEC_DEBUG_DATA* decDebugData = NULL;
    int         maxLayer = -1;
    int  trackIdx;
    int useEditlist[MAX_TRACKS_PER_LAYER] = { 0 };
    double startOffset[MAX_TRACKS_PER_LAYER] = { 0.0 };
    double durationTotal[MAX_TRACKS_PER_LAYER] = { 0.0 };

    int numChannelOut = 0;
    float fSampleOut = 0.0f;

    DRC_APPLY_INFO    drcInfo;

    drcInfo.uniDrc_config_file = tmpFile_Drc_config;
    drcInfo.uniDrc_payload_file = tmpFile_Drc_payload;
    drcInfo.loudnessInfo_config_file = tmpFile_Loudness_config;


    /* ###################################################################### */
    /* ##                          main function                           ## */
    /* ###################################################################### */
    /* select correct mp4_header */
    if (decode_para->bitrate >= 64000) {
        if (decode_para->num_chan == 1) {
            switch (decode_para->samp_freq)
            {
            case 48000: argv[2] = "sequence/64/48000_1.mp4"; break;
            case 44100: argv[2] = "sequence/64/44100_1.mp4"; break;
            case 32000: argv[2] = "sequence/64/32000_1.mp4"; break;
            default:
                break;
            }
        }
        else {
            switch (decode_para->samp_freq)
            {
            case 48000: argv[2] = "sequence/64/48000_2.mp4"; break;
            case 44100: argv[2] = "sequence/64/44100_2.mp4"; break;
            case 32000: argv[2] = "sequence/64/32000_2.mp4"; break;
            default:
                break;
            }
        }
   
    }
    else {
        if (decode_para->num_chan == 1) {
            switch (decode_para->samp_freq)
            {
            case 48000: argv[2] = "sequence/20/48000_1.mp4"; break;
            case 44100: argv[2] = "sequence/20/44100_1.mp4"; break;
            case 32000: argv[2] = "sequence/20/32000_1.mp4"; break;
            default:
                break;
            }
        }
        else {
            switch (decode_para->samp_freq)
            {
            case 48000: argv[2] = "sequence/20/48000_2.mp4"; break;
            case 44100: argv[2] = "sequence/20/44100_2.mp4"; break;
            case 32000: argv[2] = "sequence/20/32000_2.mp4"; break;
            default:
                break;
            }
        }

    }



    /* get version number */
    error = getVersion(MPEGD_USAC_VERSION_NUMBER,
        &versionMajor,
        &versionMinor,
        &versionRMbuild);
    if (0 != error) {
        //return PrintErrorCode(USAC_ERROR_FRAMEWORK_GENERIC, &error_depth, "Error intializing decoder.", NULL, verboseLevel, USAC_ERROR_POSITION_INFO);
    }

    /* parse command line parameters */
    error = PrintCmdlineHelp(argc,
        argv[0],
        versionMajor,
        versionMinor);
    if (error) {
        //return PrintErrorCode(USAC_OK, &error_depth, NULL, NULL, verboseLevel, USAC_ERROR_POSITION_INFO);
    }

    /* init DRC params */
    error = setDrcParams(&drcParams);
    if (error) {
        //return PrintErrorCode(USAC_ERROR_DRC_INIT, &error_depth, "Error initializing DRC parameters.", NULL, verboseLevel, USAC_ERROR_POSITION_INFO);
    }

    error = GetCmdline(argc,
        argv,
        mp4_InputFile,
        wav_OutputFile,
        cfg_FileName,
        &useGlobalBinariesPath,
        &drcParams,
        inputFile_drcInterface,
        &mainDebugLevel,
        &outputBitDepth,
        &mpegUsacBitDepth,
        &mpegUsacCpoType,
        &mpegUsacDecMode,
        &recycleLevel,
        &verboseLevel,
        &debugLevel);
    if (error) {
        //return PrintErrorCode(USAC_ERROR_FRAMEWORK_INVALIDCMD, &error_depth, "Error initializing MPEG USAC Audio decoder.", "Invalid command line parameters.", verboseLevel, USAC_ERROR_POSITION_INFO);
    }

    if (decode_para->pcm_wd_sz == 16) {
        outputBitDepth = BIT_DEPTH_16_BIT;
    }
    else if (decode_para->pcm_wd_sz == 24) {
        outputBitDepth = BIT_DEPTH_24_BIT;
    }
    else if (decode_para->pcm_wd_sz == 32) {
        outputBitDepth = BIT_DEPTH_32_BIT;
    }

    if (VERBOSE_LVL1 <= verboseLevel) {
        fprintf(stdout, "\n");
        fprintf(stdout, "::-----------------------------------------------------------------------------\n::    ");
        fprintf(stdout, "Initializing I/O files...\n");
        fprintf(stdout, "::-----------------------------------------------------------------------------\n");
        fprintf(stdout, "\n");
        fprintf(stdout, "Input bitstream:\n      %s\n\n", mp4_InputFile);
        fprintf(stdout, "Output audio file:\n      %s\n", wav_OutputFile);

        if (checkIfFileExists(wav_OutputFile, NULL)) {
            fprintf(stderr, "\nWarning: Output File already exists: %s\n", wav_OutputFile);
        }
        fprintf(stdout, "\n");
    }

    /* Check if windows or linux commands should be used */
    bUseWindowsCommands = useWindowsCommands();

    MakeLogFileNames(wav_OutputFile,
        logFileNames,
        bUseWindowsCommands,
        verboseLevel);

    /* Remove all old temp files */
    removeTempFiles(RECYCLE_ACTIVE,
        bUseWindowsCommands,
        verboseLevel);

    SetDebugLevel(mainDebugLevel);

    if (USAC_DECMODE_DEFAULT == mpegUsacDecMode) {
        error = setupUsacDecoder(useGlobalBinariesPath,
            globalBinariesPath,
            cfg_FileName,
            binary_DrcDecoder,
            binary_WavCutter,
            &error_depth,
            verboseLevel);
        if (error != 0) {
            return error;
        }
    }

    /* Set USAC Decoder Mode */
    gUseFractionalDelayDecor = (USACDecMode & 1);
    UseFftHarmonicTransposer = ((USACDecMode >> 1) & 1);

    fprintf(stdout, "\n");
    fprintf(stdout, "::-----------------------------------------------------------------------------\n::    ");
    fprintf(stdout, "Decoding bit stream...\n");
    fprintf(stdout, "::-----------------------------------------------------------------------------\n");
    fprintf(stdout, "\n");





    /* ###################################################################### */
    /* ##                   MP4Audio_DecodeFile function                   ## */
    /* ###################################################################### */

    /* reduce internal processing chain to 24 bit resolution if requested*/
    if (((mpegUsacBitDepth == BIT_DEPTH_24_BIT) ? 1 : 0 == 1)) {
        bWriteIEEEFloat = 0;
    }

    /* initialize AudioPreRoll Info struct */
    setAprInfo(&aprInfo);

    /* initialize Elst Info struct */
    setElstInfo(&elstInfo,
        0.0,
        0.0,
        0,
        0,
        0.0f,
        0);

    /* initialize hFault */
    memset(hFault, 0, MAX_TF_LAYER * sizeof(T_DECODERGENERAL));

    BsInit(0, bitDebugLevel, 0);

    stream = StreamFileOpenRead(mp4_InputFile, FILETYPE_AUTO); if (stream == NULL) goto bail;

    stream->prog[0].decoderConfig->avgBitrate.value = decode_para->bitrate;


    if (programNr != -1) {      /* specific program selected */
        progStart = programNr;
        progStop = programNr + 1;
        DebugPrintf(1, "Selected program: %d", progStart);
    }
    else {                      /* no specific program selected -> loop over all programs */
        progStart = 0;
        progStop = StreamFileGetProgramCount(stream);
        if (progStop > 1) {       /* more than one program in stream */
            DebugPrintf(1, "No specific program selected -> looping over all programs");
        }
        DebugPrintf(1, "\nNumber of programs: %d", progStop);
    }

    //for (progCnt = progStart; progCnt < progStop; progCnt++) {
    progCnt = progStart;
    programNr = progCnt;
    DebugPrintf(1, "Decoding program: %d", programNr);

    prog = StreamFileGetProgram(stream, programNr); if (prog == NULL) goto bail;
    suitableTracks = prog->trackCount;

    DebugPrintf(1, "\nFound MP4 file with  %d suitable Tracks  ", suitableTracks);

    if (!suitableTracks) {
        return -1;
    }

    asc = &(prog->decoderConfig[0].audioSpecificConfig);

    switch (asc->audioDecoderType.value) {
#ifdef AAC_ELD
    case ER_AAC_ELD:
        channelNum = asc->channelConfiguration.value;
        break;
#endif

    case USAC:
        channelNum = get_channel_number_USAC(&asc->specConf.usacConfig,
            &numFC, &fCenter, &numSC, &numBC, &bCenter, &numLFE, NULL);
        break;
    default:
        if (asc->channelConfiguration.value == 0) {
            select_prog_config(asc->specConf.TFSpecificConfig.progConfig,
                (asc->specConf.TFSpecificConfig.frameLength.value ? 960 : 1024),
                hFault->hResilience,
                hFault->hEscInstanceData,
                aacDebugString['v']);
        }
        channelNum = get_channel_number(asc->channelConfiguration.value,
            asc->specConf.TFSpecificConfig.progConfig,
            &numFC, &fCenter, &numSC, &numBC, &bCenter, &numLFE, NULL);
        break;
    }

    if (channelNum > 2) {
        CommonWarning("No standards for putting %ld Channels into 1 File -> Using multiple files\n", channelNum);
        monoFilesOut = 1;
    }

    /*
      instantiate proper decoder for decoderconfig
    */

    decData = (DEC_DATA*)calloc(1, sizeof(DEC_DATA));
    decDebugData = (DEC_DEBUG_DATA*)calloc(1, sizeof(DEC_DEBUG_DATA));

    /* suitableTracks was initally total no of tracks
       this function stores total no of tracks in decData and
       returns no of tracks needed for decoding up to mayLayer */
    tmp = asc->channelConfiguration.value;
    asc->channelConfiguration.value = channelNum;

    suitableTracks = frameDataInit(prog->allTracks,
        suitableTracks,
        maxLayer,
        prog->decoderConfig,
        decData);
    asc->channelConfiguration.value = tmp;

    if (suitableTracks <= 0) {
        CommonWarning("Error during framework initialisation");
        goto bail;
    }

    /* get editlist info */
    for (trackIdx = 0; trackIdx < suitableTracks; trackIdx++) {
        double tmpStart, tmpDuration;

        useEditlist[trackIdx] = 0;
        if (0 == StreamFileGetEditlist(prog, trackIdx, &tmpStart, &tmpDuration)) {
            startOffset[trackIdx] = tmpStart;
            durationTotal[trackIdx] = tmpDuration;
            if (tmpDuration > -1) {
                useEditlist[trackIdx] = 1;
            }
        }
    }

    if (verboseLevel > 0) {
        fprintf(stdout, "\nDecoding ...\n");
    }



    ctx->decData = decData;
    ctx->prog = prog;
    ctx->framesDone = framesDone;
    ctx->decDebugData = decDebugData;
    ctx->decPara = decPara;
    ctx->drcInfo = drcInfo;
    ctx->streamID = &streamID;
    ctx->hFault = hFault;
    ctx->mainDebugLevel = mainDebugLevel;
    ctx->audioDebugLevel = audioDebugLevel;
    ctx->epDebugLevel = epDebugLevel;
    ctx->aacDebugString = aacDebugString;
    ctx->HEaacProfileLevel = HEaacProfileLevel;
    ctx->bUseHQtransposer = bUseHQtransposer;
    ctx->numChannelOut = numChannelOut;
    ctx->fSampleOut = fSampleOut;
    ctx->aprInfo = &aprInfo;
    ctx->AudioPreRollExisting = AudioPreRollExisting;
    ctx->verboseLevel = verboseLevel;
    ctx->audioFile = NULL;
    ctx->stream = NULL;

    return ctx;


bail:

    return 0;

}

int xheaacd_decode_frame(decode_obj* ctx, void* audioframe, int i_bytes_to_read) {
    float** outSamples;
    int16_t** int16Samples;
    long  numOutSamples;
    int nAudioPreRollSamplesWrittenPerChannel = 0;
    int frameCntAudioPreRoll = 0;
    long startOffsetInSamples[MAX_TRACKS_PER_LAYER] = { 0 };

    float* ch0;
    int streamID = -1;

    char buffer_empty = 0;
    int tracksForDecoder;
    AUDIO_SPECIFIC_CONFIG prerollASC;
    HANDLE_STREAM_AU inputAUs[MAX_TRACKS_PER_LAYER];
    HANDLE_STREAM_AU decoderAUs[MAX_TRACKS_PER_LAYER];
    HANDLE_STREAM_AU prerollAU[MPEG_USAC_CORE_MAX_AU];
    unsigned int prerollASCLength = 0;
    unsigned int applyCrossFade = 0;
#ifdef DEBUGPLOT
    plotInit(framePlot, audioFileName, 0);
#endif

    {
        int layer, track;
        int firstTrackInLayer, resultTracksInLayer;
        int err = 0;

        ctx->streamID = &streamID;

        for (track = 0; track < MAX_TRACKS_PER_LAYER; track++) {
            inputAUs[track] = StreamFileAllocateAU(0);
            decoderAUs[track] = StreamFileAllocateAU(0);
        }
        firstTrackInLayer = tracksForDecoder = 0;
        /* go through each decoded layer */
        for (layer = 0; layer < (signed)ctx->decData->frameData->scalOutSelect + 1; layer++) {
            /* go through all frames in the superframe */
            resultTracksInLayer = EPconvert_expectedOutputClasses(ctx->decData->frameData->ep_converter[layer]);
            while ((signed)ctx->decData->frameData->layer[tracksForDecoder].NoAUInBuffer < StreamAUsPerFrame(ctx->prog, firstTrackInLayer)) {
                int epconv_input_tracks;
                if (EPconvert_numFramesBuffered(ctx->decData->frameData->ep_converter[layer])) {
                    epconv_input_tracks = 0;
                }
                else {
                    epconv_input_tracks = ctx->decData->frameData->tracksInLayer[layer];
                }

                /* go through all tracks in this layer */
                //for (track = 0; track < epconv_input_tracks; track++) {
                //    err = StreamGetAccessUnit(ctx->prog, firstTrackInLayer + track, inputAUs[track]);
                //    if (err) break;
                //}
                inputAUs[0]->numBits = i_bytes_to_read * 8;
                inputAUs[0]->data = audioframe;
                ctx->prog->programData->status = STATUS_READING;
                ctx->prog->programData->timeThisFrame[0] = ctx->prog->programData->timePerAU[0];

                if (err) break;

                /* decode with epTool if necessary */
                err = EPconvert_processAUs(ctx->decData->frameData->ep_converter[layer],
                    inputAUs, epconv_input_tracks,
                    decoderAUs, MAX_TRACKS_PER_LAYER);

                if (resultTracksInLayer != err) {
                    CommonWarning("Expected %i AUs after ep-conversion, but got %i AUs", resultTracksInLayer, err);
                    /*resultTracksInLayer=err;*/
                }
                err = 0;

                /* save the tracks */
                for (track = 0; track < resultTracksInLayer; track++) {
                    frameDataAddAccessUnit(ctx->decData, decoderAUs[track], tracksForDecoder + track);
                }
            }
            if (err) break;
            firstTrackInLayer += ctx->decData->frameData->tracksInLayer[layer];
            tracksForDecoder += resultTracksInLayer;
        }
        buffer_empty = 1;
        for (track = 0; track < (signed)ctx->decData->frameData->od->streamCount.value; track++) {
            if (ctx->decData->frameData->layer[track].NoAUInBuffer > 0) buffer_empty = 0;
        }
        //if (buffer_empty)
        //    break;
    }

    if (ctx->framesDone == 0) {
        int delay, track;
        char              tmpFile_Drc_payload[FILENAME_MAX] = "tmpFileUsacDec_drc_payload_output.bit";
        char              tmpFile_Drc_config[FILENAME_MAX] = "tmpFileUsacDec_drc_config_output.bit";
        char              tmpFile_Loudness_config[FILENAME_MAX] = "tmpFileUsacDec_drc_loudness_output.bit";

        ctx->drcInfo.uniDrc_payload_file = tmpFile_Drc_payload;
        ctx->drcInfo.uniDrc_config_file = tmpFile_Drc_config;
        ctx->drcInfo.loudnessInfo_config_file = tmpFile_Loudness_config;

        //initESDescr(&(ctx->decData->frameData->od->ESDescriptor[0]));

        delay = MP4Audio_SetupDecoders(ctx->decData,
            ctx->decDebugData,
            tracksForDecoder,
            ctx->decPara,
            ctx->drcInfo.uniDrc_payload_file,
            ctx->drcInfo.uniDrc_config_file,
            ctx->drcInfo.loudnessInfo_config_file,
            ctx->streamID,
            ctx->hFault,
            ctx->mainDebugLevel,
            ctx->audioDebugLevel,
            ctx->epDebugLevel,
            ctx->aacDebugString
#ifdef CT_SBR
            , ctx->HEaacProfileLevel
            , ctx->bUseHQtransposer
#endif
#ifdef HUAWEI_TFPP_DEC
            , actATFPP
#endif
        );

        /* decoder initialisation will update streamCount in decData and
           print an error if decoder initialisation does not agree with track count
           determined earlier by frameDataInit() and stored in suitableTracks */

        ctx->fSampleOut = (float)ctx->decData->frameData->scalOutSamplingFrequency;
        ctx->numChannelOut = ctx->decData->frameData->scalOutNumChannels;

        ctx->audioFile = AudioOpenWrite("tmpFileUsacDec_core_output.wav",
                                   "wav",
                                   ctx->numChannelOut,
                                   ctx->fSampleOut,
                                   0);

//        if (ELST_MODE_CORE_LEVEL == elstInfoModeLevel) {
//            for (track = 0; track < (signed)ctx->decData->frameData->od->streamCount.value; track++) {
//                if (useEditlist[track]) {
//#ifndef ALIGN_PRECISION_NOFIX
//                    double tmpStartOffset = startOffset[track] * fSampleOut + 0.5;
//                    double tmpPlayTime = durationTotal[track] * fSampleOut + 0.5;
//
//                    startOffsetInSamples[track] = (long)(tmpStartOffset);
//                    playTimeInSamples[track] = (long)(tmpPlayTime);
//#else
//                    startOffsetInSamples[track] = (long)(startOffset[track] * fSampleOut + 0.5);
//                    playTimeInSamples[track] = (long)(durationTotal[track] * fSampleOut + 0.5);
//#endif
//                }
//            }
//        }

        /* open audio file */
        /* (sample format: 16 bit twos complement, uniform quantisation) */
//        if (fileOpened == 0) {
//
//            if (monoFilesOut == 0) {
//                audioFile = AudioOpenWrite(audioFileName,
//                    audioFileFormat,
//                    numChannelOut,
//                    fSampleOut,
//                    int24flag);
//            }
//            else {
//                audioFile = AudioOpenWriteMC(audioFileName,
//                    audioFileFormat,
//                    fSampleOut,
//                    int24flag,
//                    numFC,
//                    fCenter,
//                    numSC,
//                    bCenter,
//                    numBC,
//                    numLFE);
//            }
//
//            if (audioFile == NULL)
//                CommonExit(1, "Decode: error opening audio file %s "
//                    "(maybe unknown format \"%s\")",
//                    audioFileName, audioFileFormat);
//
//            /* seek to beginning of first frame (with delay compensation) */
//            AudioSeek(audioFile, -delay);
//
//            fileOpened = 1;
//        }
    }

    if ((ctx->mainDebugLevel == 1) && (ctx->framesDone % 20 == 0)) {
        fprintf(stderr, "\rFrame [%ld] ", ctx->framesDone);
        fflush(stderr);
    }
    if (ctx->mainDebugLevel >= 2 && ctx->mainDebugLevel <= 3) {
        printf("\rFrame [%ld] ", ctx->framesDone);
        fflush(stdout);
    }
    if (ctx->mainDebugLevel > 3)
        printf("Frame [%ld]\n", ctx->framesDone);

    if (ctx->framesDone == 0) {
        int i = 0;
        int retVal = 0;
        int instance = 0;                                               /* AudioPreRoll() shall be the first element in the AU! */
        int track = ctx->decData->frameData->od->streamCount.value - 1;   /* USAC only supports one track! */
        AUDIO_SPECIFIC_CONFIG* pUsacASC = &ctx->decData->frameData->od->ESDescriptor[track]->DecConfigDescr.audioSpecificConfig;
        USAC_DECODER_CONFIG* pUsacConfig = &pUsacASC->specConf.usacConfig.usacDecoderConfig;
        USAC_EXT_CONFIG* pUsacExtConfig = &pUsacConfig->usacElementConfig[instance].usacExtConfig;

        if (USAC_ID_EXT_ELE_AUDIOPREROLL == pUsacExtConfig->usacExtElementType) {
            /* Prepare preroll-ASC layout - needed to have the correct number of bits for every entry */
            memcpy(&prerollASC, pUsacASC, sizeof(AUDIO_SPECIFIC_CONFIG));

            /* Parse preroll data if present */
            retVal = usac_core_parsePrerollData(decoderAUs[track],
                prerollAU,
                &ctx->aprInfo->numPrerollAU,
                &prerollASC,
                &prerollASCLength,
                &applyCrossFade);

            if (0 != retVal) {
                ctx->AudioPreRollExisting = 0;
            }
            else {
                ctx->AudioPreRollExisting = 1;

                /* decoder Pre Roll AU's  */
                for (i = 0; i < ctx->aprInfo->numPrerollAU; i++) {
                    BsBufferSetData(&ctx->decData->frameData->layer[0],
                        prerollAU[i]->data,
                        prerollAU[i]->numBits);

                    audioDecFrame(ctx->decData,
                        ctx->hFault,
                        &outSamples,
                        &numOutSamples,
                        &ctx->numChannelOut);

                    //AudioWriteDataTruncat(audioFile, outSamples, numOutSamples, 0);

                    nAudioPreRollSamplesWrittenPerChannel += numOutSamples;
                    frameCntAudioPreRoll++;
                }

                /* Put the IPF AU back into our decData instance and continue decoding as normal. */
                BsBufferSetData(&ctx->decData->frameData->layer[0],
                    decoderAUs[track]->data,
                    decoderAUs[track]->numBits);

                ctx->aprInfo->nSamplesCoreCoderPreRoll = nAudioPreRollSamplesWrittenPerChannel;
            }
        }

        startOffsetInSamples[0] += ctx->aprInfo->nSamplesCoreCoderPreRoll;
    }

    audioDecFrame(ctx->decData,
        ctx->hFault,
        &outSamples,
        &numOutSamples,
        &ctx->numChannelOut);
    

    int16Samples = ConvertFloatToInt16(outSamples, ctx->numChannelOut, numOutSamples);
    ch0 = int16Samples[0];

//   if (numChannelOut != decData->frameData->scalOutNumChannels && firstDecdoeFrame)
//   {
//       AudioClose(audioFile);
//       /*  fixing 7.1 channel error */
//       if (decData->frameData->scalOutObjectType == ER_BSAC)
//       {
//           channelNum = get_channel_number_BSAC(numChannelOut, /* SAMSUNG_2005-09-30 : Warnning ! it may not be same with asc->channelConfiguration.value. */
//               asc->specConf.TFSpecificConfig.progConfig,
//               &numFC, &fCenter, &numSC, &numBC, &bCenter, &numLFE, NULL);
//
//       }
//       else
//       {
//           switch (asc->audioDecoderType.value) {
//ifdef AAC_ELD
//           case ER_AAC_ELD:
//               channelNum = asc->channelConfiguration.value;
//               break;
//endif
//           default:
//               channelNum = get_channel_number(numChannelOut, /* SAMSUNG_2005-09-30 : Warnning ! it may not be same with asc->channelConfiguration.value. */
//                   asc->specConf.TFSpecificConfig.progConfig,
//                   &numFC, &fCenter, &numSC, &numBC, &bCenter, &numLFE, NULL);
//               break;
//           }
//       }
//
//       if (channelNum > 2) {
//           CommonWarning("No standards for putting %ld Channels into 1 File -> Using multiple files\n", channelNum);
//           monoFilesOut = 1;
//       }
//
//       if (monoFilesOut == 0) {
//           audioFile = AudioOpenWrite(audioFileName,
//               audioFileFormat,
//               numChannelOut,
//               fSampleOut,
//               int24flag);
//       }
//       else {
//           /* SAMSUNG 2005-10-18 */
//           if (decData->frameData->scalOutObjectType == ER_BSAC)
//               audioFile = AudioOpenWriteMC_BSAC(audioFileName,
//                   audioFileFormat,
//                   fSampleOut,
//                   int24flag,
//                   numFC,
//                   fCenter,
//                   numSC,
//                   bCenter,
//                   numBC,
//                   numLFE);
//           else
//               audioFile = AudioOpenWriteMC(audioFileName,
//                   audioFileFormat,
//                   fSampleOut,
//                   int24flag,
//                   numFC,
//                   fCenter,
//                   numSC,
//                   bCenter,
//                   numBC,
//                   numLFE);
//       }
//       firstDecdoeFrame = 0;
//   }


#ifdef CT_SBR
      /* The implicit signalling....*/

 //   if (decData->tfData != NULL) {
 //       static int reInitAudioOutput = 1;
 //
 //       if (decData->tfData->runSbr == 1 &&             /* If zero, we know that no implicit signalling was detected*/
 //           decData->tfData->sbrPresentFlag != 0 &&     /* If zero, we know no SBR is present.*/
 //           decData->tfData->sbrPresentFlag != 1 &&     /* If one , we know for sure SBR is present, and already set up the output correctly.*/
 //           reInitAudioOutput) {
 //
 //           /* check sampling frequency just for AAC_LC and AAC_SCAL ( other aot are not supported at the moment ) */
 //           /* 20070326 BSAC Ext.*/
 //           if ((decData->frameData->scalOutObjectType == AAC_LC) || (decData->frameData->scalOutObjectType == AAC_SCAL) || (decData->frameData->scalOutObjectType == ER_BSAC)) {
 //               /* 20070326 BSAC Ext.*/
 //
 //                 /* get current output sampling frequency */
 //               float outputSamplingFreq = AudioGetSamplingFreq(audioFile);
 //
 //               /* calculate favoured output sbr frequency */
 //               float sbrSamplingFreq;
 //
 //               if (decData->tfData->bDownSampleSbr) {
 //                   sbrSamplingFreq = (float)prog->decoderConfig[suitableTracks - 1].audioSpecificConfig.samplingFrequency.value;
 //               }
 //               else {
 //                   sbrSamplingFreq = (float)prog->decoderConfig[suitableTracks - 1].audioSpecificConfig.samplingFrequency.value * 2;
 //               }
 //
 //
 //               if (outputSamplingFreq != sbrSamplingFreq) {
 //
 //                   reInitAudioOutput = 0;
 //
 //                   /* reinit audio writeout */
 //                   AudioClose(audioFile);
 //                   if (monoFilesOut == 0) {
 //                       audioFile = AudioOpenWrite(audioFileName,
 //                           audioFileFormat,
 //                           channelNum,
 //                           sbrSamplingFreq,
 //                           int24flag);
 //                   }
 //                   else if (decData->frameData->scalOutObjectType == ER_BSAC) {
 //                       audioFile = AudioOpenWriteMC_BSAC(audioFileName,
 //                           audioFileFormat,
 //                           sbrSamplingFreq,
 //                           int24flag,
 //                           numFC,
 //                           fCenter,
 //                           numSC,
 //                           bCenter,
 //                           numBC,
 //                           numLFE);
 //                   }
 //                   else {
 //                       audioFile = AudioOpenWriteMC(audioFileName,
 //                           audioFileFormat,
 //                           sbrSamplingFreq,
 //                           int24flag,
 //                           numFC,
 //                           fCenter,
 //                           numSC,
 //                           bCenter,
 //                           numBC,
 //                           numLFE);
 //                   }
 //               }
 //           }
 //       }
 //   }
#endif


//if (ELST_MODE_CORE_LEVEL == elstInfoModeLevel)
//{
//    int writeoutOn = 0;
//
//#ifdef I2R_LOSSLESS
//    if ((decData->frameData->scalOutObjectType == SLS) || (decData->frameData->scalOutObjectType == SLS_NCORE)) {
//        if (framesDone > 1) {
//            writeoutOn = 1;      /* for SLS skip 1st 2 frames (to chk why?) */
//        }
//    }
//    else
//#endif
//    {
//        writeoutOn = 1;      /* normally write out all frames */
//    }
//
//    if (writeoutOn == 1) {
//        long skipSamples = 0;
//
//        if (useEditlist[0]) {
//            if (numOutSamples < startOffsetInSamples[0]) {
//                skipSamples = numOutSamples;
//            }
//            else {
//                skipSamples = startOffsetInSamples[0];
//
//                if (numOutSamples > playTimeInSamples[0]) {
//                    numOutSamples = playTimeInSamples[0];
//                }
//            }
//
//            startOffsetInSamples[0] -= skipSamples;
//            playTimeInSamples[0] -= (numOutSamples - skipSamples);
//        }
//
//        if (decData->frameData->scalOutObjectType == USAC) {
//            AudioWriteDataTruncat(audioFile, outSamples, numOutSamples, skipSamples);
//        }
//        else {
//            AudioWriteData(audioFile, outSamples, numOutSamples, skipSamples);
//        }
//    }
//}
//else {
//    if (decData->frameData->scalOutObjectType == USAC) {
//        AudioWriteDataTruncat(audioFile, outSamples, numOutSamples, 0);
//    }
//    else {
//        AudioWriteData(audioFile, outSamples, numOutSamples, 0);
//    }
//}
// 
    if (ctx->decData->frameData->scalOutObjectType == USAC) {
        //AudioWriteDataTruncat(ctx->audioFile, outSamples, numOutSamples, 0);
        AudioWriteDataTruncatInt16(ctx->audioFile, int16Samples, numOutSamples, 0);
    }

    ctx->framesDone = ctx->framesDone + 1;


#ifdef DEBUGPLOT
    framePlot++;
    plotDisplay(0);
#endif
    if (ctx->verboseLevel > 0) {
        fprintf(stdout, "Decoding frame %3d\n", ctx->framesDone);
    }

 //  if (ELST_MODE_EXTERN_LEVEL == elstInfoModeLevel) {
 //      setElstInfo(elstInfo,
 //          startOffset[0],
 //          durationTotal[0],
 //          numOutSamples,
 //          framesDone,
 //          fSampleOut,
 //          useEditlist[0]);
 //  }
 //
 //  if (verboseLevel > 0) {
 //      if ((frameCntAudioPreRoll > 0) && (0 == discardAudioPreRoll)) {
 //          fprintf(stdout, "Finished - (%d frames + %d Audio Pre-Roll frames)\n", frameCnt++, frameCntAudioPreRoll++);
 //      }
 //      else {
 //          fprintf(stdout, "Finished - (%d frames)\n\n", frameCnt++);
 //      }
 //  }
 //
 //  /* transport MPEG-D DRC data */
 //  if (decData->usacData != NULL) {
 //      drcInfo->uniDrc_extEle_present = decData->usacData->drcInfo.uniDrc_extEle_present;
 //      drcInfo->loudnessInfo_configExt_present = decData->usacData->drcInfo.loudnessInfo_configExt_present;
 //      drcInfo->frameLength = decData->usacData->drcInfo.frameLength;
 //      drcInfo->sbr_active = decData->usacData->drcInfo.sbr_active;
 //  }

}

int xheaacd_delete(decode_obj* ctx) {
    audioDecFree(ctx->decData, ctx->hFault);
    free(ctx->decData);
    free(ctx->decDebugData);
    if (ctx->audioFile) {
        AudioClose(ctx->audioFile);
    }
    if (ctx->stream) {
        StreamFileClose(ctx->stream);
    }
}




/* ###################################################################### */
/* ##                  MPEG USAC decoder main function                 ## */
/* ###################################################################### */

#define USE_SIMPLE_MAIN 1

#define AUDIO_BITRATE        64000
#define AUDIO_SAMPLE_RATE    44100
#define AUDIO_CHANNELS       2
#define AUDIO_PCM_WIDTH      16
#define AUDIO_SBR_FLAG       0
#define AUDIO_MPS_FLAG       0
#define AUDIO_SUPERFRAME     400

#define Handle_frame_length  256*AUDIO_CHANNELS*AUDIO_PCM_WIDTH 
#define Super_frame_length   AUDIO_BITRATE*AUDIO_SUPERFRAME/8000

#if USE_SIMPLE_MAIN

int main() {
    decode_para dec_para;
    decode_obj* ctx;
    HANDLE_STREAM_AU au;
    void* audioframe;
    int i_bytes_to_read;

    memset(&dec_para, 0, sizeof(decode_para));
    dec_para.bitrate = AUDIO_BITRATE;
    dec_para.pcm_wd_sz = AUDIO_PCM_WIDTH;
    dec_para.samp_freq = AUDIO_SAMPLE_RATE;
    dec_para.num_chan = AUDIO_CHANNELS;
    dec_para.sbr_flag = AUDIO_SBR_FLAG;
    dec_para.mps_flag = AUDIO_MPS_FLAG;
    dec_para.Super_frame_mode = AUDIO_SUPERFRAME;

    ctx = xheaacd_create(&dec_para);

    while (1) {
        xheaacd_decode_frame(ctx, audioframe, i_bytes_to_read);
    }


}

#endif