/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by

Heiko Purnhagen     (University of Hannover / ACTS-MoMuSys)

and edited by

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

$Id: mp4ifc.h,v 1.3 2011-05-28 14:25:18 mul Exp $
Decoder frame work
**********************************************************************/

typedef enum _ELST_MODE {
  ELST_MODE_CORE_LEVEL,
  ELST_MODE_EXTERN_LEVEL
} ELST_MODE_LEVEL;

typedef enum _CORE_MODE {
  CORE_STANDALONE_MODE,
  CORE_FRAMEWORK_MODE
} CORE_MODE_LEVEL;

typedef struct _AUDIOPREROLL_INFO {
  unsigned int nSamplesCoreCoderPreRoll;
  unsigned int numPrerollAU;
} AUDIOPREROLL_INFO;

typedef struct _ELST_INFO {
  unsigned int  truncLeft;
  unsigned int  truncRight;
  unsigned long totalSamplesElst;
  unsigned long totalSamplesFile;
} ELST_INFO;

typedef struct _DRC_APPLY_INFO {
  int uniDrc_extEle_present;
  int loudnessInfo_configExt_present;
  int frameLength;
  int sbr_active;
  char* uniDrc_config_file;
  char* uniDrc_payload_file;
  char* loudnessInfo_config_file;
} DRC_APPLY_INFO;

int MP4Audio_ProbeFile(char *inFile);

int MP4Audio_DecodeFile(char*                 inFile,          /* in: bitstream input file name */
                        char*                 audioFileName,   /* in: audio file name */
                        char*                 audioFileFormat, /* in: audio file format (au, wav, aif, raw) */
                        int                   int24flag,
                        int                   monoFilesOut,
                        int                   maxLayer,        /* in  max layers */
                        char*                 decPara,         /* in: decoder parameter string */
                        int                   mainDebugLevel,
                        int                   audioDebugLevel,
                        int                   epDebugLevel,
                        int                   bitDebugLevel,
                        char*                 aacDebugString,
#ifdef CT_SBR
                        int                   HEaacProfileLevel,
                        int                   bUseHQtransposer,
#endif
#ifdef HUAWEI_TFPP_DEC
                        int                   actATFPP,
#endif
                        int                   programNr,
                        DRC_APPLY_INFO       *drcInfo,
                        AUDIOPREROLL_INFO    *aprInfo,
                        ELST_INFO            *elstInfo,
                        const ELST_MODE_LEVEL elstInfoModeLevel,
                        int                  *streamID,
                        const CORE_MODE_LEVEL decoderMode,
                        const int             verboseLevel
);
