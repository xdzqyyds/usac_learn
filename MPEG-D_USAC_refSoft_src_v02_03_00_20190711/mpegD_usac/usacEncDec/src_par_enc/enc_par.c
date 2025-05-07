/**********************************************************************
MPEG-4 Audio VM
Encoder core (parametric)



This software module was originally developed by

Heiko Purnhagen (University of Hannover / Deutsche Telekom Berkom)
Bernd Edler (University of Hannover / Deutsche Telekom Berkom)
Masayuki Nishiguchi, Kazuyuki Iijima, Jun Matsumoto (Sony Corporation)

and edited by

Akira Inoue (Sony Corporation) and
Yuji Maeda (Sony Corporation)

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



Source file: enc_par.c


Required libraries:
HVXC.a			HVXC library

Required modules:
common.o		common module
cmdline.o		command line module
bitstream.o		bit stream module
indilinextr.o		indiline extraction module
indilineenc.o		indiline bitstream encoder module
( indilinesyn.o		indiline synthesiser module )

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BE    Bernd Edler, Uni Hannover <edler@tnt.uni-hannover.de>
MN    Masayuki Nishiguchi, Sony IPC <nishi@pcrd.sony.co.jp>

Changes:
14-jun-96   HP    first version (dummy)
18-jun-96   HP    added bit reservoir handling
24-jun-96   HP    implemented "individual spectral lines"
25-jun-96   HP    using CommonFreeAlloc(), small bug fixes
28-jun-96   HP    joined with HVXC code by Sony IPC
01-jul-96   HP    added HVXC header file, included modifications by IPC
02-jul-96   HP    included modifications by IPC
14-aug-96   HP    added EncParInfo(), EncParFree()
15-aug-96   HP    adapted to new enc.h
26-aug-96   HP    CVS
10-sep-96   BE
12-sep-96   HP    incorporated indiline modules as source code
26-sep-96   HP    adapted to new indiline module interfaces
26-sep-96   HP    incorporated changes by Sony IPC
04-dec-96   HP    included ISO copyright
10-apr-97   HP    harmonic stuff ...
22-apr-97   HP    noisy stuff ...
15-may-97   HP    clean up
06-jul-97   HP    switch/mix
17-jul-97   HP    "clas" option
23-oct-97   HP    merged IL/HVXC switching
                  swit mode: 2 HVXC frames / 40 ms
07-nov-97   MN    bug-fixes
11-nov-97   HP    added HVXC variable rate switches
12-nov-97   HP    fixed mixed HVXC/IL mode
05-feb-99   HP    marked HILN as Version 2
10-oct-00   HP    ER_HILN/ER_PARA -mp4ff stuff
29-jan-01   BE    float -> double
12-feb-01   HP    completed epConfig=1 support

VMIL stuff
24-jun-97   HP    adapted env/harm/noise interface
16-nov-97   HP    adapted HILN decoder config header
09-apr-98   HP    ILX/ILE config
02-dec-98   HP    new noise quant as dflt.

**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "common_m4a.h"		/* common module */
#include "cmdline.h"		/* command line module */
#include "bitstream.h"		/* bit stream module */

#include "obj_descr.h"		/* AI 990616 */
#include "flex_mux.h"		/* AI 990616 */

#include "indilinextr.h"	/* indiline extraction module */
#include "indilineenc.h"	/* indiline bitstream encoder module */

#ifdef IL_ENC_SYNTHESIS_NOT_YET_IMPLEMENTED
#include "indilinesyn.h"	/* indiline synthesiser module */
#endif



#include "enc_par.h"		/* encoder cores */

#include "mp4_par.h"		/* parametric core declarations */

#include "hvxc.h"		/* hvxc module */
#include "hvxcEnc.h"		/* hvxcEnc module */

#include "pan_par_const.h"

/* ---------- declarations ---------- */

#define PROGVER "parametric encoder core FDAM1 16-Feb-2001"

#define SEPACHAR " ,="

#define MAXSAMPLE 32768		/* full scale sample value */

#define MAXNUMBITFACTOR 1.2	/* for indiline bit reservoir control */

#define STRLEN 255

/* globals from indilinecom.c */
extern int sampleRateTable[16];


extern int PARusedNumBit;


/* ---------- variables ---------- */

/* init parameters (global) */
static float EPfSample;
static float EPbitRate;
static int EPmode;		/* enum MP4ModePar */
static int EPframeNumSample;
static int EPdelayNumSample;

/* sample buffer (frame-to-frame memory) */
static float *EPsampleBuf = NULL;
static float *EPsampleBuf2 = NULL;
static int EPsampleBufSize;

/* indiline handles */
static ILXstatus *ILX;
static ILEstatus *ILE;

/* extensionFlag for HVXC (AI990209) 0=HVXC 1=ER_HVXC */
static int EPextensionFlag = 0;

/* number of enhancement layers for scalable bitstream (AI 990902) */
static int NumEnhLayers;

/* command line module */

static int EPerPara;
static char *EPparaMode;
static int EPdelayMode;
static int EPvrMode;
static int EPmixMode;
static int EPdebugLevel;
static int EPdebugLevelIl;
static float EPframeDur;
static int EPmaxNumLine;
static int EPmaxNumLineUsed;
static int EPmaxNumLineAdd;
static float EPmaxLineFreq;
static int EPmaxLineFreqUsed;
static int EPmaxNumEnv;
static int EPmaxNumHarm;
static int EPmaxNumHarmLine;
static int EPmaxNumNoisePara;
static float EPprevFrames;
static float EPnextFrames;
static int EPprevSamples;
static int EPnextSamples;

static int EPtest;
static int EPnewEncDelay;
static int EPfree;

static int EPrateMode = ENC4K;	/* for VR scalable mode (YM 990728) */

static int EPpackMode = PM_SHORT;

static int EPscalFlag;	/* for scalable mode (AI 990902) */
static int EPvrScalFlag;	/* for scalable mode (YM 990728) */

int HVXCclassifierMode;

/* HILN encoder control HP971116 */
static int EPnumExtLayer;
static float EPextLayerRate[MAX_LAYER-1];
static int EPextLayerNumBit[MAX_LAYER-1];
static int EPenhaFlag;
static float EPbaseRate;
static int EPbaseRateUsed;
static float EPmaxAvgRatio;
static float EPbufAvgRatio;
static int  EPepConfig;

static ILXconfig ILXcfg;
static ILEconfig ILEcfg;

static int numEsc[MAX_LAYER];	/* number of ESCs for each layer */


  

static int mp4ffFlag;	/* a flag to use system interface(flexmux) (AI 990616) */



static CmdLineSwitch switchList[] = {
  {"h",NULL,NULL,NULL,NULL,"print help"},
  {"dm",&EPdelayMode,"%i","0",NULL,"HVXC delay mode (0=short 1=long)"},
  {"vr",&EPvrMode,"%i","0",NULL,"HVXC variable rate mode (0=fixed 1=var)\n"
   "(set frame work option -vr)"},
  {"sc",&EPscalFlag,"%i","0",NULL,
   "HVXC scalable flag (0=off 1=on)"}, /* scalable encode (YM 990902) */
  {"dl",&EPdebugLevel,"%i","0",NULL,"debug level"},
  {"-mp4ff",&mp4ffFlag,NULL,NULL,NULL,"use system interface(flexmux)"},	/* AI 990616 */
  {"erpara",&EPerPara,NULL,NULL,NULL,"ER_PARA (dflt: ER_HILN)"},
  {"m",&EPparaMode,"%s",MODEPARNAME_IL,NULL,
   "ER_PARA: parametric codec mode (" MODEPARNAME_LIST ")\n\b"},
  {"mm",&EPmixMode,"%i","1",NULL,
   "HVXC/IL mix mode (2ch: L=HVXC R=IL, use with -n 1)\n"
   "(0=HVXC 1=HVXC2+IL 2=HVXC4+IL 3=IL  -=cycle periode)\n\b"},
  {"dli",&EPdebugLevelIl,"%i","0",NULL,
   "debug level HILN (enc + 10*xtr)"},
  {"fd",&EPframeDur,"%f","0.032",NULL,"HILN frame duration [sec]"},
  {"ml",&EPmaxNumLine,"%i",NULL,&EPmaxNumLineUsed,
   "max num individual lines\n"
   "(dflt: auto (bit rate dependent))"},
  {"mlf",&EPmaxLineFreq,"%f",NULL,&EPmaxLineFreqUsed,
   "max line frequency (=bandwidth) (dflt: fs/2)"},
  {"me",&EPmaxNumEnv,"%i","1",NULL,"max num envelopes (1=il/harm 2=+noise)"},
  {"mh",&EPmaxNumHarm,"%i","1",NULL,"max num harmonic tones"},
  {"mhl",&EPmaxNumHarmLine,"%i","100",NULL,"max num lines of harmonic tone"},
  {"mn",&EPmaxNumNoisePara,"%i","12",NULL,"max num noise parameters"},
  {"mabr",&EPmaxAvgRatio,"%f","1.0",NULL,
   "for fl4/mp4: max/avg bitrate ratio"},
  {"babr",&EPbufAvgRatio,"%f","1.0",NULL,
   "for fl4/mp4: buffer size (peak/avg bitrate ratio)\n\b"},
  {"hixl",&EPnumExtLayer,"%i","0",NULL,
   "harm/indi num extension layers"},
  {"hix1",&EPextLayerRate[0],"%f","0",NULL,
   "harm/indi extension layer 1 bit rate"},
  {"hix2",&EPextLayerRate[1],"%f","0",NULL,
   "harm/indi extension layer 2 bit rate"},
  {"hix3",&EPextLayerRate[2],"%f","0",NULL,
   "harm/indi extension layer 3 bit rate"},
  {"hix4",&EPextLayerRate[3],"%f","0",NULL,
   "harm/indi extension layer 4 bit rate"},
  {"hix5",&EPextLayerRate[4],"%f","0",NULL,
   "harm/indi extension layer 5 bit rate"},
  {"hix6",&EPextLayerRate[5],"%f","0",NULL,
   "harm/indi extension layer 6 bit rate"},
  {"hix7",&EPextLayerRate[6],"%f","0",NULL,
   "harm/indi extension layer 7 bit rate"},
  {"hip",&ILEcfg.phaseFlag,"%i","0",NULL,
   "harm/indi phase flag (0=random 1=determ) (needs bsf>=5)\n\b"},
  {"hipi",&ILEcfg.maxPhaseIndi,"%i","0",NULL,
   "max num indi lines with phase (base layer)"},
  {"hipe",&ILEcfg.maxPhaseExt,"%i","0",NULL,
   "max num indi lines with phase (extension layer)"},
  {"hiph",&ILEcfg.maxPhaseHarm,"%i","0",NULL,
   "max num harm lines with phase"},
  {"hie",&EPenhaFlag,"%i","0",NULL,
   "harm/indi enha flag (0=basic 1=enha)"},
  {"hibr",&EPbaseRate,"%f",NULL,&EPbaseRateUsed,
   "harm/indi basic bit rate (for hie=1) (dflt: full rate)"},
  {"hiq",&ILEcfg.quantMode,"%i","-1",NULL,
   "harm/indi quant mode (-1=auto 0..1)"},
  {"hieq",&ILEcfg.enhaQuantMode,"%i","-1",NULL,
   "harm/indi enha quant mode (-1=auto 0..3)"},
  {"hicm",&ILEcfg.contMode,"%i","0",NULL,
   "harm/indi continue mode (0=hi 1=hi&ii 2=none)"},
  {"iecm",&ILEcfg.encContMode,"%i","0",NULL,
   "indi encoder continue mode (0=cont 1=no-cont)"},
  {"bsf",&ILEcfg.bsFormat,"%i","6",NULL,
   "HILN bitstream format\n"
   "(0=VM 1=9705 2=CD 3=CD+nx 4=v2 5=v2phase 6=ER_HILN)\n\b"},
  {"clas",&HVXCclassifierMode,"%i","0",NULL,
   "for test only\n"
   "(classifier mode 0=pause req. 1=no pause req.)"},
  {"test",&EPtest,"%i","0",NULL,
   "for test only\n"
   "(hvxc/il alternate periode +=IL/HVXC/.. -=HVXC/IL/..)\n\b"},
  {"free",&EPfree,"%i",/*"0"*/"1",NULL,"XxxFree()"},
  {"ef",&EPextensionFlag,"%i","0",NULL,
   "extension flag (0=HVXC 1=ER_HVXC)"},  /* for "EP" tool (YM 990616) */
  {"epc",&EPepConfig,"%i","0",NULL,
   "ER_HVXC/ER_HILN/ER_PARA epConfig (0 or 1)"},
  {"ed",&EPnewEncDelay,"%i","0",NULL,  /* MN 971114 */
   "HVXC only mode encoder delay comp.: 0=0/20ms 1=6/26ms\n\b"},
  {NULL,NULL,NULL,NULL,NULL,NULL}
};

/* variables for HVXC */
int ipc_encMode = ENC4K;
int ipc_bitstreamMode = BM_CONSTANT;

int ipc_rateMode = ENC4K; /* YM 020110 */

int ipc_extensionFlag = 0;
int ipc_packMode = PM_SHORT;

int ipc_vrScalFlag = SF_OFF;	

static int rateModeBits[3]; /* HP20010213 */

/* switch/mix stuff */
extern int judge;

#define JUDGE_DLY 124	/* (124+1)*40ms=5s*/
#define HVXCDBLESC 10 /* max #ESCs in HVXC double frame */
static int numBsBuf;
static int actBsBuf;
static HANDLE_BSBITBUFFER bsBuf[JUDGE_DLY+1][HVXCDBLESC];
static HANDLE_BSBITSTREAM bsStream[HVXCDBLESC];
static int ILdelayNumSample = 0;
static int HVXdelayNumSample = 0;
static int ILsampleBufSize = 0;
static int HVXsampleBufSize = 0;
static int ILdelayOff = 0;
static int HVXdelayOff = 0;
static HANDLE_BSBITBUFFER dmyBuf;
static int testFrame;

static HANDLE_BSBITBUFFER hdrBuf;

int ipc_encDelayMode = DM_SHORT;



/* HILN */
static double maxNoiseFreq;


/* ---------- internal functions ---------- */



/* EncParInitIl() */
/* Init: individual lines */

static void EncParInitIl (
  HANDLE_BSBITSTREAM hdrStream)	/* out: header for bit stream */
{
  int maxNumLine;
  int frameMaxNumBit;
  int hdrFSample,hdrFrameNumSample;
  int i;


  /* calc variable default values */
  if (!EPmaxNumLineUsed)
    EPmaxNumLine = -1;
  if (!EPmaxLineFreqUsed)
    EPmaxLineFreq = EPfSample/2;
  if (!EPbaseRateUsed)
    EPbaseRate = EPbitRate;
  else
    if (EPbaseRate > EPbitRate)
      CommonWarning("EncParInitIl: EPbaseRate > EPbitRate");

  /* calc encoder parameters */
  EPframeNumSample = (int)(EPfSample*EPframeDur+0.5);
  EPprevSamples = EPframeNumSample;
  EPnextSamples = EPframeNumSample/2;
  /* EPdelayNumSample = EPframeNumSample/2; */
  EPdelayNumSample = EPnextSamples;	/* HP 990910 */
  frameMaxNumBit = (int)(EPbaseRate*EPframeDur*MAXNUMBITFACTOR+0.5);
  for (i=0; i<EPnumExtLayer; i++)
    EPextLayerNumBit[i] = (int)(EPextLayerRate[i]*EPframeDur+0.5);
  for (; i<MAX_LAYER-1; i++)
    EPextLayerNumBit[i] = 0;
  if (ILEcfg.bsFormat<4) {
    hdrFSample = (int)(2*EPmaxLineFreq+.5);
    hdrFrameNumSample = (int)(2*EPmaxLineFreq*EPframeDur+.5);
  }
  else {
    hdrFSample = (int)(EPfSample+.5);
    hdrFrameNumSample = (int)(EPfSample*EPframeDur+.5);
  }

  /* init modules */
  ILE = IndiLineEncodeInit(hdrFrameNumSample,hdrFSample,MAXSAMPLE,
			   EPenhaFlag,frameMaxNumBit,
			   EPmaxNumLine,EPmaxNumEnv,EPmaxNumHarm,
			   EPmaxNumHarmLine,EPmaxNumNoisePara,
			   &ILEcfg,EPdebugLevelIl%10,hdrStream,&maxNumLine,
			   &maxNoiseFreq);
  if (!EPmaxNumLineUsed)
    EPmaxNumLine = maxNumLine+EPmaxNumLineAdd;



  ILX = IndiLineExtractInit(EPframeNumSample,EPfSample,EPmaxLineFreq,MAXSAMPLE,
			    EPmaxNumLine,EPmaxNumEnv,EPmaxNumHarm,
			    EPmaxNumHarmLine,EPmaxNumNoisePara,maxNoiseFreq,
			    &ILXcfg,EPdebugLevelIl/10,ILEcfg.bsFormat,
			    EPprevSamples,EPnextSamples);

  /* set sample buffer size */
  /*  EPsampleBufSize = (EPframeNumSample*5+1)/2; */
  EPsampleBufSize = EPprevSamples+EPframeNumSample+EPnextSamples;	/* HP 990910 */

  if (EPdebugLevel >= 1) {
    printf("EncParInitIl: frmDur=%.6f  dbgLvlIl=%d\n",
	   EPframeDur,EPdebugLevelIl);
    printf("EncParInitIl: frmNumSmp=%d  frmMaxNumBit=%d  smpBufSize=%d\n",
	   EPframeNumSample,frameMaxNumBit,EPsampleBufSize);
    printf("EncParInitIl: maxNumLine=%d  maxLineFreq=%.3f\n",
	   EPmaxNumLine,EPmaxLineFreq);
    printf("EncParInitIl: hdrFSample=%d  hdrFrameNumSample=%d\n",
	   hdrFSample,hdrFrameNumSample);
  }
  dmyBuf=BsAllocBuffer(200);
}



/* number of bits for each ESC(bps) */
static const int hvxcNumBitsEsc[8][4][5] = {	
  {	/* [mode=0] 2kbps fixed rate/(2+2)kbps fixed rate base layer */
    {22,  4,  4, 10,  0},	/* UV(VUV=0) */
    {22,  4,  4, 10,  0},	/*  V(VUV=1) */
    {22,  4,  4, 10,  0},	/*  V(VUV=2) */
    {22,  4,  4, 10,  0}	/*  V(VUV=3) */
  },
  {	/* [mode=1] 4kbps fixed rate */
    {33, 22,  4,  4, 17},	/* UV(VUV=0) */
    {33, 22,  4,  4, 17},	/*  V(VUV=1) */
    {33, 22,  4,  4, 17},	/*  V(VUV=2) */
    {33, 22,  4,  4, 17}	/*  V(VUV=3) */
  },
  {	/* [mode=2] (2+2)kbps fixed rate ehnacecement layer */
    /* 14496-3 COR1 */
    {12, 19,  9,  0,  0},	/* UV(VUV=0) */
    {12, 19,  9,  0,  0},	/*  V(VUV=1) */
    {12, 19,  9,  0,  0},	/*  V(VUV=2) */
    {12, 19,  9,  0,  0}	/*  V(VUV=3) */
  },
  {	/* [mode=3] 2kbps variable rate */
    {22,  4,  2,  0,  0},	/*  UV(VUV=0) */
    { 2,  0,  0,  0,  0},	/* BGN(VUV=1) */
    {22,  4,  4, 10,  0},	/*   V(VUV=2) */
    {22,  4,  4, 10,  0}	/*   V(VUV=3) */
  },
  {	/* [mode=4] 4kbps variable rate */
    {22, 18,  0,  0,  0},	/*  UV(VUV=0) */
    { 3,  0,  0,  0,  0},	/* BGN(VUV=1,UpdateFlag=0) */
    {33, 22,  4,  4, 17},	/*   V(VUV=2,3) */
    {19,  6,  0,  0,  0}	/* BGN(VUV=1,UpdateFlag=1) */
  },
  {	/* [mode=5] (2+2)kbps variable rate base layer */
    {22,  4,  4, 10,  0},	/*  UV(VUV=0) */
    { 3,  0,  0,  0,  0},	/* BGN(VUV=1,UpdateFlag=0) */
    {22,  4,  4, 10,  0},	/*   V(VUV=2,3) */
    {19,  6,  0,  0,  0}	/* BGN(VUV=1,UpdateFlag=1) */
  },
  {	/* [mode=6] (2+2)kbps variable rate enhancement layer */
    {0,  0,  0,  0,  0},	/*  UV(VUV=0) */
    {0,  0,  0,  0,  0},	/* BGN(VUV=1) */
    {12, 19,  9,  0,  0},	/*   V(VUV=2) */
    {12, 19,  9,  0,  0},	/*   V(VUV=3) */
  },
  {	/* [mode=7] 3.7kbps fixed rate */
    {31, 21,  4,  4, 12},	/*  UV(VUV=0) */
    {32, 17,  4,  4, 17},	/*   V(VUV=1) */
    {32, 17,  4,  4, 17},	/*   V(VUV=2) */
    {32, 17,  4,  4, 17}	/*   V(VUV=3) */
  }
};

static const int hvxcNumEsc[8] = {	/* ER_HVXC: number of ESCs */
  4,/* [mode=0] 2kbps fixed rate */
  5,/* [mode=1] 4kbps fixed rate */
  3,/* [mode=2] (2+2)kbps fixed rate ehnacecement layer */
  4,/* [mode=3] 2kbps variable rate */
  5,/* [mode=4] 4kbps variable rate */
  4,/* [mode=5] (2+2)kbps variable rate base layer */
  3,/* [mode=6] (2+2)kbps variable rate enhancement layer */
  5 /* [mode=7] 3.7kbps fixed rate */
};

static const int hvxcMaxBitRate[8][5] = {  /* ER_HVXC: maximum bit rate for each ESC(bps) */
  {1100,  200,  200,  500,    0},	/* [mode=0] 2kbps fixed rate */
                                        /*          (2+2)kbps fixed rate base layer */
  {1650, 1100,  200,  200,  850},	/* [mode=1] 4kbps fixed rate */
  { 600,  950,  450,    0,    0},	/* [mode=2] (2+2)kbps fixed rate ehnacecement layer */
  {1100,  200,  200,  500,    0},	/* [mode=3] 2kbps variable rate */
  {1650, 1100,  200,  200,  850},	/* [mode=4] 4kbps variable rate */
  {1100,  200,  200,  500,    0},	/* [mode=5] (2+2)kbps variable rate base layer */
  { 600,  950,  450,    0,    0},	/* [mode=6] (2+2)kbps variable rate enhancement layer */
  {1600,  850,  200,  200,  850}	/* [mode=7] 3.7kbps fixed rate */
};

static int get_hvxc_mode(int layer, int rateMode)
{
  int mode;

  if (!EPvrMode) {
    /* fixed rate mode */
    if (rateMode==ENC4K) {
      mode=1;	/* 4kbps fixed rate */
    }
    else if (rateMode==ENC3K) {
      mode=7;	/* 3.7kbps fixed rate */
    }
    else {
      if (layer==0)
	mode=0;	/* 2kbps fixed rate/(2+2)kbps fixed rate base layer */
      else 
	mode=2;	/* (2+2)kbps fixed rate enhancement layer */
    }
  }
  else {
    /* variable rate mode */
    if (rateMode==ENC4K) {
      mode=4;	/* 4kbps variable rate */
    }
    else {
      if (layer==0) {
	if (EPscalFlag)
	  mode=5;	/* (2+2)kbps variable rate base layer */
	else 
	  mode=3;	/* 2kbps variable rate */
      }
      else {
	mode=6;	/* (2+2)kbps variable rate enhancement layer */
      }
    }
  }
  return mode;
}

/* Akira 980506 */
#ifdef CELP_LPC_TOOL
typedef struct
{
  PHI_PRIV_TYPE *PHI_Priv;
  /* add private data pointers here for other coding varieties */
}
INST_CONTEXT_LPC_ENC_TYPE;

static INST_CONTEXT_LPC_ENC_TYPE *InstCtxt;
#endif


/* EncParInitHvx() */
/* Init: harmonic vector exitation */

static void EncParInitHvx (
  HANDLE_BSBITSTREAM hdrStream,	/* out: header for bit stream */
  int mp4ffFlag) /* HP20001010 */
{
#ifdef CELP_LPC_TOOL
  int i;
  long window_sizes[PAN_NUM_ANA_PAR];
  
  for(i=0;i<PAN_NUM_ANA_PAR;i++)
    window_sizes[i] = PAN_WIN_LEN_PAR;
#endif

  /* HP20010213 */
  rateModeBits[ENC2K] = 40;
  rateModeBits[ENC4K] = 80;
  rateModeBits[ENC3K] = 80-6;

  EPframeNumSample = 160;	/* 20 ms */
  EPsampleBufSize = 160;	/* 20 ms in buffer EPsampleBuf[] */

  if ( EPmode == MODEPAR_HVX && !EPnewEncDelay){ /* MN 971114 */
    if (ipc_encDelayMode == DM_LONG)
      EPdelayNumSample = 160;
    else
      EPdelayNumSample = 0;
  }
  else {			/* these are the correct values! */
    if (ipc_encDelayMode == DM_LONG)	/* HP 971023 */
      EPdelayNumSample = 208; 	/* 46 ms - 20 ms = 26 ms   HP 971111 */
    else
      EPdelayNumSample = 48; 	/* 26 ms - 20 ms = 6 ms */
  }

  if (EPscalFlag) EPrateMode = ENC2K;  /* AI 2001/01/31 */

  if (!mp4ffFlag) {	/* configuration for raw bitstream(AI 990616) */
    if (BsPutBit(hdrStream,ipc_bitstreamMode,1))
      CommonExit(1,"EncParInitHvx: error generating bit stream header");
    /* if (BsPutBit(hdrStream,ipc_rateMode,2)) */
    if (BsPutBit(hdrStream,EPrateMode,2))	/* YM 990728 */
      CommonExit(1,"EncParInitHvx: error generating bit stream header");
    if (BsPutBit(hdrStream,EPextensionFlag,1))	/* AI 990209 */
      CommonExit(1,"EncParInitHvx: error generating bit stream header");
    if (EPextensionFlag) {
      /* bit stream header for version 2 (YM 990728) */
      if (BsPutBit(hdrStream,EPvrScalFlag,1))	
	CommonExit(1,"EncParInitHvx: error generating bit stream header");
    }
  }
  
  IPC_HVXCInit();

#ifdef CELP_LPC_TOOL
  /* Akira 980506 */

  /* -----------------------------------------------------------------*/
  /* Create & initialise private storage for instance context         */
  /* -----------------------------------------------------------------*/
  if (( InstCtxt = (INST_CONTEXT_LPC_ENC_TYPE*)
	malloc(sizeof(INST_CONTEXT_LPC_ENC_TYPE))) == NULL)
    CommonExit(1,"MALLOC FAILURE in celp_initialisation_encoder\n");

  if (( InstCtxt->PHI_Priv = (PHI_PRIV_TYPE*)
	malloc(sizeof(PHI_PRIV_TYPE))) == NULL )
    CommonExit(1,"MALLOC FAILURE in celp_initialisation_encoder\n");
    
  PHI_Init_Private_Data(InstCtxt->PHI_Priv);

  PAN_InitLpcAnalysisEncoder(PAN_LPC_ORDER_PAR, InstCtxt->PHI_Priv);
  /* Akira 980506 */
#endif
}



/* EncParFrameIl() */
/* Encode frame: individual lines */

static void EncParFrameIl (
  float *sampleBuf,
  HANDLE_BSBITSTREAM stream[MAX_LAYER+1+4],	/* out: bit stream(s) */
  int frameAvailNumBit,		/* in: total num bits available for */
				/*     this frame (incl. bit reservoir) */
  int frameNumBit,		/* in: average num bits per frame */
  int frameMaxNumBit)		/* in: max num bits per frame */
{
  int numEnv;
  double **envPara;
  int numLine;
  double *lineFreq;
  double *lineAmpl;
  double *linePhase;
  int *lineEnv;
  int *linePred;
  int numHarm;
  int *numHarmLine;
  double *harmFreq;
  double *harmFreqStretch;
  int *harmLineIdx;
  int *harmEnv;
  int *harmPred;
  double *harmRate;
  int numNoisePara;
  double noiseFreq;
  double *noisePara;
  int noiseEnv;
  double noiseRate;
  int i,il;
  HANDLE_BSBITSTREAM dmyStream;
  HANDLE_BSBITSTREAM dmyStream5[5];
  HANDLE_BSBITSTREAM streamExt[MAX_LAYER-1];
  




  /* extract line parameters */
  IndiLineExtract(ILX,sampleBuf,
		  NULL,
		  NULL,NULL,NULL,
/* 		  (stream)?EPmaxNumLine:0, */
		  (stream)?EPmaxNumLine:-1,
		  &numEnv,&envPara,
		  &numLine,&lineFreq,&lineAmpl,&linePhase,&lineEnv,&linePred,
		  &numHarm,&numHarmLine,&harmFreq,&harmFreqStretch,
		  &harmLineIdx,&harmEnv,&harmPred,&harmRate,
		  &numNoisePara,&noiseFreq,&noisePara,&noiseEnv,&noiseRate);




  if (EPdebugLevel >= 2) {
    printf("EncParFrameIl: extracted parameters:\n");
    i = 0;
    for(il=0; il<numLine; il++)
      if (linePred[il])
	i++;
    printf("numLine=%2d (new=%2d cont=%2d)\n",numLine,numLine-i,i);
    if (EPdebugLevel >= 3) {
      for (i=0; i<numEnv; i++)
	printf("env %d: tm=%7.5f atk=%7.5f dec=%7.5f\n",
	       i,envPara[i][0],envPara[i][1],envPara[i][2]);
      for(il=0; il<numLine; il++)
	printf("%2d: f=%7.1f a=%7.1f p=%5.2f e=%1d c=%2d\n",
	       il,lineFreq[il],lineAmpl[il],linePhase[il],
	       lineEnv[il],linePred[il]);
    }
    for (i=0; i<numHarm; i++) {
      printf("harm %d: n=%2d f=%7.1f fs=%10e e=%1d c=%1d r=%5.3f\n",
	     i,numHarmLine[i],harmFreq[i],harmFreqStretch[i],
	     harmEnv[i],harmPred[i],harmRate[i]);
      if (EPdebugLevel >= 3)
	for(il=harmLineIdx[i]; il<harmLineIdx[i]+numHarmLine[i]; il++)
	  printf("h %2d: a=%7.1f\n",il-harmLineIdx[i]+1,lineAmpl[il]);
    }
    if (numNoisePara) {
      printf("noise: n=%d f=%7.1f e=%1d r=%5.3f\n",
	     numNoisePara,noiseFreq,noiseEnv,noiseRate);
      if (EPdebugLevel >= 3)
	for (i=0; i<numNoisePara; i++)
	  printf("noise %d: p=%10e\n",i,noisePara[i]);
    }
  }

  if (EPbaseRateUsed)
    frameAvailNumBit = min(frameAvailNumBit,
			   (int)(EPbaseRate*EPframeDur+.5));
  /* call IndiLineEncodeFrame() to update internal frame-to-frame memory */
  dmyStream = BsOpenBufferWrite(dmyBuf);
  for (i=0; i<5; i++)
    dmyStream5[i] = dmyStream;
  for (i=0; i<EPnumExtLayer; i++)
    streamExt[i] = stream[5+(EPenhaFlag?1:0)+i];
  for (; i<MAX_LAYER-1; i++)
    streamExt[i] = NULL;
  IndiLineEncodeFrame(ILE,numEnv,envPara,
		      numLine,lineFreq,lineAmpl,linePhase,lineEnv,linePred,
		      numHarm,numHarmLine,harmFreq,harmFreqStretch,
		      harmLineIdx,harmEnv,harmPred,harmRate,
		      numNoisePara,noiseFreq,noisePara,noiseEnv,noiseRate,
		      frameAvailNumBit,frameNumBit,frameMaxNumBit,
		      EPnumExtLayer,
		      (EPnumExtLayer)?EPextLayerNumBit:(int*)NULL,
		      (stream)?stream:dmyStream5,
		      (stream && EPenhaFlag)?stream[5]:(HANDLE_BSBITSTREAM )NULL,
		      (stream && EPnumExtLayer)?streamExt:(HANDLE_BSBITSTREAM *)NULL);
  BsClose(dmyStream);

}



/* EncParFrameHvx() */
/* Encode frame: harmonic vector exitation */

static void EncParFrameHvx (
  float *sampleBuf,
  HANDLE_BSBITSTREAM bitStreamLay[],	/* out: bit stream(multi layers) */
  int frameAvailNumBit,		/* in: total num bits available for */
				/*     this frame (incl. bit reservoir) */
  int frameNumBit,		/* in: average num bits per frame */
  int frameMaxNumBit)		/* in: max num bits per frame */
{
    int		i;

    short	frmBuf[FRM];
    IdLsp	idLsp;
    int		idVUV;
    IdCelp	idCelp;
    float	mfdpch;
    IdAm	idAm;

    unsigned char encBit[10];	/* for 4kbps(80bits) */
    unsigned long bit;

    int rateMode;

    HANDLE_BSBITSTREAM stream = NULL;	/* AI 990902 */

    int num, layer, esc, mode;
    int numBits;
    int numLayer;
    unsigned long UpdateFlag = 0;
    
    HANDLE_BSBITBUFFER tmpBitBuffer;	/* temporary bit buffer */
    HANDLE_BSBITSTREAM tmpStream;	/* temporary stream */

    /* HP 970706 */
    if (frameAvailNumBit >= 80-6) {  /* MN 971106 */
      ipc_encMode = ENC4K;
      if (EPscalFlag) 
	rateMode = ENC2K;
      else 
	rateMode = ENC4K;

      if (frameAvailNumBit < 80) {
	rateMode = ENC3K;
      }
    }
    else {
      ipc_encMode = ENC2K;
      rateMode = ENC2K;
    }

    ipc_rateMode = rateMode;  /* YM 020110 */

    if (EPdebugLevel >= 2) {
      printf("EncParFrameHvx: stream=%s\n",stream?"valid":"NULL");
      printf("EncParFrameHvx: availBit=%d\n",frameAvailNumBit);
      printf("EncParFrameHvx: encMode=");
      switch (rateMode) {
      case ENC2K : printf("ENC2K\n"); break;
      case ENC3K : printf("ENC3K\n"); break;
      case ENC4K : printf("ENC4K\n"); break;
      default : printf("ERROR!!!\n");
      }	
    }

    for(i = 0; i < FRM; i++)
    {
	frmBuf[i] = (short) sampleBuf[i];
    }
    
    IPC_HVXCEncParFrm(frmBuf, &idLsp, &idVUV, &idCelp, &mfdpch, &idAm);

    IPC_PackPrm2Bit(&idLsp, idVUV, &idCelp, mfdpch, &idAm, encBit);

    if (!EPepConfig) {  /* epConfig=0 */
    stream = bitStreamLay[0];	/* base layer */
    
    if(ipc_encMode == ENC4K)
    {
      if(ipc_bitstreamMode == BM_VARIABLE)
      {
	/* HP 971111 */
	if (EPdebugLevel >= 2)
	  printf("EncParFrameHvx: idVUV=%d  bit=%d\n",
		 idVUV,(encBit[0] & 0xc0) >> 6);

        switch(idVUV)
	{
	case 0:	
          for(i = 0; i < 5; i++) {
	    bit = encBit[i];
	    if(BsPutBit(stream, bit, 8))
	      CommonExit(1,"EncParFrameHvx: error generating bit stream");
          }
	  /* no enhacement layer in scalable mode */
	  break;
        case 1:
	  bit = (encBit[0] & 0xe0) >> 5;	/* UpdateFlag */
	  if(BsPutBit(stream, bit, 3))
	    CommonExit(1,"EncParFrameHvx: error generating bit stream");
          if (bit & 0x1) {
	    bit = (encBit[0] & 0x1f);
	    if(BsPutBit(stream, bit, 5))
	      CommonExit(1,"EncParFrameHvx: error generating bit stream");
	      for (i = 1; i < 3; i++) {
	        bit = encBit[i];
	        if(BsPutBit(stream, bit, 8))
	          CommonExit(1,"EncParFrameHvx: error generating bit stream");
	      }
	      bit = (encBit[3] & 0x80) >> 7;
	      if(BsPutBit(stream, bit, 1))
	         CommonExit(1,"EncParFrameHvx: error generating bit stream");
	  }
	  /* no enhacement layer in scalable mode */
	  break;
        case 2:
	case 3:
	  for(i = 0; i < 5; i++) {
	    bit = encBit[i];
	    if(BsPutBit(stream, bit, 8))
	      CommonExit(1,"EncParFrameHvx: error generating bit stream");
	  }
	  if (EPscalFlag) {	
	    stream = bitStreamLay[1];	/* enhancement layer in scalable mode(AI 990902) */
	  }
	  for(; i < 9; i++) {
	    bit = encBit[i];
	    if(BsPutBit(stream, bit, 8))
	      CommonExit(1,"EncParFrameHvx: error generating bit stream");
	  }
          if (rateMode!=ENC3K) {
	    bit = encBit[9]; /* MN 971106 */
	    if(BsPutBit(stream, bit, 8))
	      CommonExit(1,"EncParFrameHvx: error generating bit stream");
          }
          else {
	    /* MN 971107 */
  	    bit = (encBit[9] & 0xc0) >> 6;
	    if(BsPutBit(stream, bit, 8-6))
	      CommonExit(1,"EncParFrameHvx: error generating bit stream");
          }
	  break;
	}
      }
      else
      {
        for(i = 0; i < 5; i++) {
	  bit = encBit[i];
	  if(BsPutBit(stream, bit, 8))
	    CommonExit(1,"EncParFrameHvx: error generating bit stream");
        }
	if (EPscalFlag) {	
	  stream = bitStreamLay[1];	/* enhancement layer in scalable mode(AI 990902) */
	}
        for(; i < 9; i++) {
	  bit = encBit[i];
	  if(BsPutBit(stream, bit, 8))
	    CommonExit(1,"EncParFrameHvx: error generating bit stream");
        }
        if (rateMode!=ENC3K) {
	  bit = encBit[9]; /* MN 971106 */
	  if(BsPutBit(stream, bit, 8))
	    CommonExit(1,"EncParFrameHvx: error generating bit stream");
        }
        else {
	  /* MN 971107 */
  	  bit = (encBit[9] & 0xc0) >> 6;
	  if(BsPutBit(stream, bit, 8-6))
	    CommonExit(1,"EncParFrameHvx: error generating bit stream");
        }

      }
    }
    else if(ipc_encMode == ENC2K)
    {
      if(ipc_bitstreamMode == BM_VARIABLE)
      {
	/* HP 971111 */
	if (EPdebugLevel >= 2)
	  printf("EncParFrameHvx: idVUV=%d  bit=%d\n",
		 idVUV,(encBit[0] & 0xc0) >> 6);

        switch(idVUV)
        {
        case 0:
          for(i = 0; i < 3; i++)
          {
  	      bit = encBit[i];
	      if(BsPutBit(stream, bit, 8))
	        CommonExit(1,"EncParFrameHvx: error generating bit stream");
          }

  	  bit = (encBit[3] & 0xf0) >> 4;
	  if(BsPutBit(stream, bit, 4))
	    CommonExit(1,"EncParFrameHvx: error generating bit stream");
          break;
        case 1:
  	  bit = (encBit[0] & 0xc0) >> 6;
	  if(BsPutBit(stream, bit, 2))
	    CommonExit(1,"EncParFrameHvx: error generating bit stream");
          break;
        case 2:
        case 3:
          for(i = 0; i < 5; i++)
          {
  	      bit = encBit[i];
	      if(BsPutBit(stream, bit, 8))
	        CommonExit(1,"EncParFrameHvx: error generating bit stream");
          }
          break;
        }
      }
      else
      {
        for(i = 0; i < 5; i++)
        {
  	    bit = encBit[i];
	    if(BsPutBit(stream, bit, 8))
	      CommonExit(1,"EncParFrameHvx: error generating bit stream");
        }
      }
    }
    }  /* if (!EPepConfig) { */
    else {  /* epConfig=1 */

      tmpBitBuffer = BsAllocBuffer(80);

      if (EPscalFlag) 
	numLayer=2;
      else
	numLayer=1;

      /* firstly, write whole bitstream to temporary bit buffer */
      tmpStream = BsOpenBufferWrite(tmpBitBuffer);
      for (i=0; i<10; i++) {
	bit = encBit[i];
	if(BsPutBit(tmpStream, bit, 8))
	  CommonExit(1,"EncParFrameHvx: error generating bit stream");
      }
      BsClose(tmpStream);

      /* secondary, read bits for every ESC in temporary bit buffer 
	 and write them to output stream */
      tmpStream = BsOpenBufferRead(tmpBitBuffer);
      num=0;
      for (layer=0; layer<numLayer; layer++) {
	/* set mode(bitrate,fixed/variable,base/enhancement layer) */
	mode=get_hvxc_mode(layer,rateMode);

	/* mode can be variable for ER_PARA mixed mode  HP20010213 */
	numEsc[layer]=hvxcNumEsc[mode];

	if (mode!=4 && mode!=5) {
	  
	  for (esc=0; esc<numEsc[layer]; esc++, num++) {
	    stream=bitStreamLay[num];
	    numBits=hvxcNumBitsEsc[mode][idVUV][esc];
	    if (EPdebugLevel>=3)
	      printf("layer=%d,esc%d(num=%d,mode=%d,idVUV=%2d): %d bits\n", layer, esc, num, mode, idVUV, numBits);
	    while(numBits>0) {
	      /* read/write 32bits at maximum */
	      BsGetBit(tmpStream,&bit,min(32,numBits));
	      BsPutBit(stream,bit,min(32,numBits));
	      numBits -= 32;
	    }
	  }
	}
	else {	/* mode=4,5(UpdateFlag is necessary to parse bitstream) */
	  for (esc=0; esc<numEsc[layer]; esc++, num++) {
	    stream=bitStreamLay[num];

	    numBits=0;
	    if (layer==0 && esc==0) {
	      /* read/write 2bits for VUV */
	      BsGetBit(tmpStream,&bit,2);
	      BsPutBit(stream,bit,2);
	      numBits -= 2;
	      idVUV=bit;
	      if (EPdebugLevel>=3)
		printf("\tVUV=%d(2bits are already read)\n", idVUV);
	      if (idVUV==1) {
		/* read/write 1bit for UpdateFlag */
		BsGetBit(tmpStream,&bit,1);
		BsPutBit(stream,bit,1);
		numBits -= 1;
		UpdateFlag=bit;
		if (EPdebugLevel>=3)
		  printf("\tUpdateFlag=%ld(1bit are already read)\n", UpdateFlag);
	      }
	    }
	    
	    if (idVUV==3) {
	      numBits += hvxcNumBitsEsc[mode][2][esc];
	    }
	    else if (idVUV==1 && UpdateFlag==1) {	/* UpdateFlag is a bit in esc0 */
	      numBits += hvxcNumBitsEsc[mode][3][esc];
	    }
	    else {
	      numBits += hvxcNumBitsEsc[mode][idVUV][esc];
	    }
	    if (EPdebugLevel>=3)
	      printf("layer=%d,esc%d(num=%d,mode=%d,idVUV=%2d): %d bits\n", layer, esc, num, mode, idVUV, numBits);
	    while(numBits>0) {
	      /* read/write 32bits at maximum */
	      BsGetBit(tmpStream,&bit,min(32,numBits));
	      BsPutBit(stream,bit,min(32,numBits));
	      numBits -= 32;
	    }
	  }
	}
      }
      BsClose(tmpStream);

      BsFreeBuffer(tmpBitBuffer);	/* free temporary bit buffer */
    }
}



/* EncParFreeIl() */
/* Free memory: individual lines */

static void EncParFreeIl ()
{
  IndiLineExtractFree(ILX);
  IndiLineEncodeFree(ILE);
}



/* EncParFreeHvx() */
/* Free memory: harmonic vector exitation */

static void EncParFreeHvx ()
{
  /* Akira 980506 */
  PAN_FreeLpcAnalysisEncoder( InstCtxt->PHI_Priv);
  IPC_HVXCFree();
}


/* ---------- functions ---------- */


char *EncParInfo (
  FILE *helpStream)		/* in: print encPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL) {
    fprintf(helpStream,
	    PROGVER "\n"
	    "encoder parameter string format:\n"
	    "  list of tokens (tokens separated by characters in \"%s\")\n",
	    SEPACHAR);
    CmdLineHelp(NULL,NULL,switchList,helpStream);
  }
  return PROGVER;
}


/* EncParInit() */
/* Init parametric encoder core. */

void EncParInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *encPara,		/* in: encoder parameter string */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample,		/* out: encoder delay (num samples) */
  HANDLE_BSBITBUFFER bitHeader,	/* out: header for bit stream */
  ENC_FRAME_DATA* frameData,	/* out: system interface */
  int mainDebugLevel)
{
  int parac;
  char **parav;
  int result;
  HANDLE_BSBITSTREAM hdrStream;
  HANDLE_BSBITSTREAM tmpStream;
  int i,j;
  int maxCh;

  int layer, numLayer;
  int bitRateLay;
  unsigned long fakeFlag;
  int esidx;
  int addESC = 0;
  int paraRate = 0;


  /* evalute encoder parameter string */
  parav = CmdLineParseString(encPara,SEPACHAR,&parac);
  result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);
  if (result) {
    if (result==1) {
      EncParInfo(stdout);
      CommonExit(1,"encoder core aborted ...");
    }
    else
      CommonExit(1,"encoder parameter string error");
  }

  /* HP20001013 this is V2 only: ER_PARA uses ER_HVXC, not HVXC syntax */
  EPextensionFlag = 1;

  EPmode = -1;
  do
    EPmode++;
  while (EPmode < MODEPAR_NUM &&
	 strcmp(EPparaMode,MP4ModeParName[EPmode]) != 0);
  if (EPmode >= MODEPAR_NUM)
    CommonExit(1,"EncParInit: unknown parametric codec mode %s",EPparaMode);
  if (!EPerPara && EPmode!=MODEPAR_IL)
    CommonExit(1,"EncParInit: mode %s requires ER_PARA",EPparaMode);
  if (EPerPara && 8000 != (int)(fSample+.5))
    CommonExit(1,"EncParInit: ER_PARA requires 8 kHz (%f)",fSample);

  maxCh = (EPmode == MODEPAR_MIX) ? 2 : 1;
  if (numChannel != maxCh)
    CommonExit(1,"EncParInit: audio data must have %d channel(s) (%d)",
	       maxCh,numChannel);

    
  
  if (EPdelayMode)
    ipc_encDelayMode = DM_LONG;
  else
    ipc_encDelayMode = DM_SHORT;

  if (EPvrMode)
    ipc_bitstreamMode = BM_VARIABLE;
  else
    ipc_bitstreamMode = BM_CONSTANT;

  ipc_extensionFlag = EPextensionFlag;

  if (EPextensionFlag) {
    ipc_packMode = PM_SHORT;
  
    if (EPvrMode) EPvrScalFlag = EPscalFlag;	/* AI 990902 */
    if (EPscalFlag)
      ipc_vrScalFlag = SF_ON;
    else
      ipc_vrScalFlag = SF_OFF;
  }


  /* HP20010213 */
  switch (EPmode) {
  case MODEPAR_HVX:
    paraRate = 0;
    break;
  case MODEPAR_IL:
    paraRate = 0;
    break;
  case MODEPAR_SWIT:
    paraRate = 25; /* 1bit/40ms */
    break;
  case MODEPAR_MIX:
    paraRate = 50; /* 2bit/40ms */
    break;
  }

  /* AI 990209 */
  EPrateMode = ENC4K;	/* 4000 kbps */
  if (bitRate <= 3999.5+paraRate)
    EPrateMode = ENC3K;	/* 3850 kbps */
  if (bitRate <= 3849.5+paraRate)
    EPrateMode = ENC2K;	/* 2000 kbps */

  if (EPscalFlag) EPrateMode = ENC2K;/* AI 2001/01/31 */

  if (EPdebugLevel >= 1) {
    printf("EncParInit: numChannel=%d  fSample=%f  bitRate=%f  "
	   "encPara=\"%s\"\n",
	   numChannel,fSample,bitRate,encPara);
    printf("EncParInit: debugLevel=%d  erpara=%d mode=\"%s\"\n",
	   EPdebugLevel,EPerPara,EPparaMode);
    printf("EncParInit: encDlyMode=%d  vrMode=%d  rateMode=%d\n",
	   ipc_encDelayMode,ipc_bitstreamMode,EPrateMode);
    printf("EncParInit: mp4ffFlag=%d\n",
	   mp4ffFlag);
  }

  CmdLineParseFree(parav);

  EPfSample = fSample;
  EPbitRate = bitRate;

  hdrBuf = BsAllocBuffer(100);

  hdrStream = BsOpenBufferWrite(hdrBuf);
  if (BsPutBit(hdrStream,EPmode,2))
    CommonExit(1,"EncParInit: error generating bit stream header");
  switch (EPmode) {
  case MODEPAR_HVX:
    EncParInitHvx(hdrStream,0);
    break;
  case MODEPAR_IL:
    EncParInitIl(hdrStream);
    break;
  case MODEPAR_SWIT:
    EncParInitHvx(hdrStream,0);
    HVXdelayNumSample = EPdelayNumSample;
    HVXsampleBufSize = EPsampleBufSize;
    EPframeDur = 0.040;
    EncParInitIl(hdrStream);
    ILdelayNumSample = EPdelayNumSample;
    ILsampleBufSize = EPsampleBufSize;
    /* HP 971023   swit mode: 2 HVXC frames / 40 ms */
    EPframeNumSample = 320; /* 40 ms */
    /* HP 971111 */
    EPsampleBufSize =
      max(320+ILdelayNumSample,
	  EPframeNumSample*JUDGE_DLY+320+HVXdelayNumSample)-
      min(320+ILdelayNumSample-ILsampleBufSize,
	  EPframeNumSample*JUDGE_DLY+160+HVXdelayNumSample-HVXsampleBufSize);
    EPdelayNumSample =
      max(ILdelayNumSample,
	  EPframeNumSample*JUDGE_DLY+HVXdelayNumSample);
    HVXdelayOff = ILdelayOff = 
      -min(320+ILdelayNumSample-ILsampleBufSize,
	   EPframeNumSample*JUDGE_DLY+160+HVXdelayNumSample-HVXsampleBufSize);
    HVXdelayOff +=
      EPframeNumSample*JUDGE_DLY+160+HVXdelayNumSample-HVXsampleBufSize;
    ILdelayOff +=
      320+ILdelayNumSample-ILsampleBufSize;
    actBsBuf = 0;
    numBsBuf = JUDGE_DLY+1;
    for (i=0; i<numBsBuf; i++)
      for (j=0; j<HVXCDBLESC; j++)
	bsBuf[i][j] = BsAllocBuffer(160);
    break;
  case MODEPAR_MIX:
    EncParInitHvx(hdrStream,0);
    HVXdelayNumSample = EPdelayNumSample;
    HVXsampleBufSize = EPsampleBufSize;
    EPframeDur = 0.040;
    EncParInitIl(hdrStream);
    ILdelayNumSample = EPdelayNumSample;
    ILsampleBufSize = EPsampleBufSize;
    /* HP 971023   swit mode: 2 HVXC frames / 40 ms */
    EPframeNumSample = 320; /* 40 ms */
    EPsampleBufSize =
      max(320+ILdelayNumSample,
	  320+HVXdelayNumSample)-
      min(320+ILdelayNumSample-ILsampleBufSize,
	  160+HVXdelayNumSample-HVXsampleBufSize);
    EPdelayNumSample =
      max(ILdelayNumSample,
	  HVXdelayNumSample);
    HVXdelayOff = ILdelayOff = 
      -min(320+ILdelayNumSample-ILsampleBufSize,
	   160+HVXdelayNumSample-HVXsampleBufSize);
    HVXdelayOff +=
      160+HVXdelayNumSample-HVXsampleBufSize;
    ILdelayOff +=
      320+ILdelayNumSample-ILsampleBufSize;
    break;
  }
  BsClose(hdrStream);

  hdrStream = BsOpenBufferWrite(bitHeader);
  if (!mp4ffFlag) {
    /* leave hdrStream as is ... */
    BsPutBuffer(hdrStream,hdrBuf);
  }
  else {
    /* flexmux interface HP20001010 */
    NumEnhLayers = 0;
    if (EPenhaFlag)
      NumEnhLayers++;
    NumEnhLayers += EPnumExtLayer;
    
    if (EPscalFlag)
      CommonExit(1,"EncParInit: ER_PARA supports no HVXC enhancement");

    numLayer = 1+NumEnhLayers;
    initObjDescr(frameData->od,mainDebugLevel);

    if (EPepConfig) {
      switch (EPmode) {
      case MODEPAR_HVX:
      case MODEPAR_IL:
	addESC = 5-1;
	break;
      case MODEPAR_SWIT:
      case MODEPAR_MIX:
	addESC = 15-1;
	break;
      }
    }
    else
      addESC = 0; /* epConfig==0: base layer not splitted in ESC instances */

    presetObjDescr(frameData->od, addESC+numLayer-1);
    frameData->od->streamCount.value=addESC+numLayer;

    tmpStream = BsOpenBufferRead(hdrBuf);
    layer = 0;
    for (esidx=0; esidx<addESC+numLayer; esidx++) {
      layer = max(0,esidx-addESC);
      initESDescr(&(frameData->od->ESDescriptor[esidx]));
      presetESDescr(frameData->od->ESDescriptor[esidx], esidx);

      /* elementary stream configuration */
      /* DecConfigDescriptor configuration */
      if (layer==2 && EPenhaFlag)
	/* 1st extension layer dependency if enhancemet present */
        frameData->od->ESDescriptor[esidx]->dependsOn_Es_number.value 
	  = frameData->od->ESDescriptor[0+addESC]->ESNumber.value;
      else if (esidx>0)
	/* enhancement/extension layer dependency */
        frameData->od->ESDescriptor[esidx]->dependsOn_Es_number.value 
	  = frameData->od->ESDescriptor[esidx-1]->ESNumber.value;
      /* presetESDescr() sets streamDependence.value = (esidx)?1:0 */

      bitRateLay = (int)(bitRate+.5);
      for (i=0; i<EPnumExtLayer; i++)
	bitRateLay -= (int)(EPextLayerRate[i]+.5);
      if (layer==0 && EPenhaFlag)
	bitRateLay = (int)(EPbaseRate+.5);
      else if (layer==1 && EPenhaFlag)
	bitRateLay -= (int)(EPbaseRate+.5);
      else if (layer>0)
	bitRateLay = (int)(EPextLayerRate[layer-1-(EPenhaFlag?1:0)]+.5);
      /* lazy ... epConfig==1: all HILN ESCs have full base layer bitrate */

      frameData->layer[esidx].bitRate = bitRateLay;
      frameData->od->ESDescriptor[esidx]->DecConfigDescr.avgBitrate.value = frameData->layer[esidx].bitRate;

      /* HP20001031 */
      frameData->od->ESDescriptor[esidx]->DecConfigDescr.maxBitrate.value = (int)(frameData->layer[esidx].bitRate*EPmaxAvgRatio+.5);
      frameData->od->ESDescriptor[esidx]->DecConfigDescr.bufferSizeDB.value = (int)(frameData->layer[esidx].bitRate*EPbufAvgRatio*EPframeDur+.5+7)/8;

      if (layer==0) {
	/* base layer */
	switch (EPmode) {
	  case MODEPAR_HVX:
	    frameData->od->ESDescriptor[esidx]->DecConfigDescr.specificInfoLength.value = 5+1; /* 15+24+ 8 bits (+1) */
	    break;
	  case MODEPAR_IL:
	    frameData->od->ESDescriptor[esidx]->DecConfigDescr.specificInfoLength.value = 5+4; /* 15+24+ 31 bits (+1) */
	    break;
	default:
	    frameData->od->ESDescriptor[esidx]->DecConfigDescr.specificInfoLength.value = 5+5; /* 15+24+ 35 bits (+1) */
	}
      }
      else {
	/* enhancement/extension layer */
	frameData->od->ESDescriptor[esidx]->DecConfigDescr.specificInfoLength.value = 5+1; /* 15+24+ 1 or 3 bits (+1) */
      }

      /* ALConfigDescriptor configuration */
#ifdef AL_CONFIG        
      frameData->od->ESDescriptor[esidx]->ALConfigDescriptor.useAccessUnitStartFlag.value=1;
      frameData->od->ESDescriptor[esidx]->ALConfigDescriptor.useAccessUnitEndFlag.value=1;
      frameData->od->ESDescriptor[esidx]->ALConfigDescriptor.useRandomAccessPointFlag.value=0;
      frameData->od->ESDescriptor[esidx]->ALConfigDescriptor.usePaddingFlag.value=0;
      frameData->od->ESDescriptor[esidx]->ALConfigDescriptor.seqNumLength.value=0;
#endif
      /* AudioSpecificConfig configuration */
      if (EPerPara) {
	frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value = ER_PARA;
      }
      else {
	frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value = ER_HILN;
      }
      frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.epConfig.value = EPepConfig;
      i = 0;
      while ((int)(fSample+.5)!=sampleRateTable[i] && sampleRateTable[i])
	i++;
      if (!sampleRateTable[i])
	i = 0xf;
      frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value = i;
      if (i==0xf)
	frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.samplingFrequency.value = (int)(fSample+.5);
      frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value = 1;	/* mono */
      
      /* ParametricSpecificConfig */
      if  (layer == 0) {
	/* base layer */
#ifdef CORRIGENDUM1
	frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.isBaseLayer.value = 1;	/* isBaseLayer */
#endif
	BsGetBit(tmpStream,&(frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.PARAmode.value),2);
	if (EPmode != 1) {
	BsGetBit(tmpStream,&(frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HVXCvarMode.value),1);
	BsGetBit(tmpStream,&(frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HVXCrateMode.value),2);
	BsGetBit(tmpStream,&(frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.extensionFlag.value),1);
	if (frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.extensionFlag.value)
	  BsGetBit(tmpStream,&(frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.vrScalFlag.value),1);
	}
	if (EPmode != 0) {
	BsGetBit(tmpStream,&(frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNquantMode.value),1);
	BsGetBit(tmpStream,&(frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNmaxNumLine.value),8);
	BsGetBit(tmpStream,&(frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNsampleRateCode.value),4);
	BsGetBit(tmpStream,&(frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNframeLength.value),12);
	BsGetBit(tmpStream,&(frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNcontMode.value),2);
	}
	frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.PARAextensionFlag.value = 0;
      }
      else if (layer == 1 && EPenhaFlag) {
	/* enhancement layer */
#ifdef CORRIGENDUM1
	frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.isBaseLayer.value = 0;	/* isBaseLayer */
#endif
	frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNenhaLayer.value = 1;
	BsGetBit(tmpStream,&fakeFlag,1); /* read enhaLayer flag */
	if (!fakeFlag)
	  CommonExit(-1,"enhaLayer should have been 1");
	BsGetBit(tmpStream,&(frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNenhaQuantMode.value),2);
      }
      else {
	/* extension layer */
	if (layer == 1) {
	  BsGetBit(tmpStream,&fakeFlag,1); /* read enhaLayer flag */
	  if (fakeFlag)
	    CommonExit(-1,"enhaLayer should have been 0");
	}
#ifdef CORRIGENDUM1
	frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.isBaseLayer.value = 0;	/* isBaseLayer */
#endif
	frameData->od->ESDescriptor[esidx]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNenhaLayer.value = 0;
      }

      if (addESC && esidx<addESC) {
	/* re-open hdrBuf for all base layer ESCs */
	BsClose(tmpStream);
	tmpStream = BsOpenBufferRead(hdrBuf);
      }
    } /* for (esidx=0; esidx<addESC+numLayer; esidx++) */
    BsClose(tmpStream);
    
    advanceODescr(hdrStream, frameData->od, 1);

    for (esidx=0; esidx<addESC+numLayer; esidx++){ 
      advanceESDescr(hdrStream, frameData->od->ESDescriptor[esidx], 1);
      /* HP20010131  i = buffer size, see EPbufAvgRatio */
      i = frameData->od->ESDescriptor[esidx]->DecConfigDescr.bufferSizeDB.value*8;
      frameData->layer[esidx].bitBuf = BsAllocBuffer(i);
    }

  } /* if (!mp4ffFlag) {} else */
  BsClose(hdrStream);

  testFrame = 0;	/* for EPtest!=0 or EPmixMode<0 */

  /* init sample buffer */
  if (CommonFreeAlloc((void**)&EPsampleBuf,
		      EPsampleBufSize*sizeof(float)) == NULL)
    CommonExit(1,"EncParInit: memory allocation error");
  for (i=0; i<EPsampleBufSize; i++)
    EPsampleBuf[i] = 0;
  if (EPmode == MODEPAR_MIX) {
    if (CommonFreeAlloc((void**)&EPsampleBuf2,
			EPsampleBufSize*sizeof(float)) == NULL)
      CommonExit(1,"EncParInit: memory allocation error");
    for (i=0; i<EPsampleBufSize; i++)
      EPsampleBuf2[i] = 0;
  }

  *frameNumSample = EPframeNumSample;
  *delayNumSample = EPdelayNumSample;

  if (EPdebugLevel >= 1) {
    printf("EncParInit: modeInt=%d\n",EPmode);
    printf("EncParInit: frmNumSmp=%d  dlyNumSmp=%d  hdrNumBit=%ld\n",
	   *frameNumSample,*delayNumSample,BsBufferNumBit(bitHeader));
    printf("EncParInit: dly=%d  buf=%d  frm=%d\n",
	   EPdelayNumSample,EPsampleBufSize,EPframeNumSample);
    printf("EncParInit: ILdly=%d  ILbuf=%d  ILoff=%d\n",
	   ILdelayNumSample,ILsampleBufSize,ILdelayOff);
    printf("EncParInit: HVXdly=%d  HVXbuf=%d  HVXoff=%d\n",
	   HVXdelayNumSample,HVXsampleBufSize,HVXdelayOff);
  }
}


/* EncParFrame() */
/* Encode one audio frame into one bit stream frame with */
/* parametric encoder core. */

void EncParFrame (
  float **sampleBuf,		/* in: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  HANDLE_BSBITBUFFER bitBuf,	/* out: bit stream frame */
				/*      or NULL during encoder start up */
  int frameAvailNumBit,		/* in: total num bits available for */
				/*     this frame (incl. bit reservoir) */
  int frameNumBit,		/* in: average num bits per frame */
  int frameMaxNumBit,		/* in: max num bits per frame */
  ENC_FRAME_DATA* frameData)	/* in/out: system interface */
{
  HANDLE_BSBITSTREAM stream[MAX_LAYER+1+14]; /* multi layer bitstreams + ESCs */
  int i;
  int hvxcFrame1;
  int mixMode;
  int ilBits;

  int layer;
  int numLayer = 0;
  int addESC = 0;
  HANDLE_BSBITSTREAM bitStreamHvxc[4+3]; /* multi layer / ESC bitstreams for HVXC */
  
  if (EPdebugLevel >= 1)
    printf("EncParFrame: %s  availNum=%d  num=%d  maxNum=%d\n",
	   (bitBuf==NULL)?"start up":"normal",
	   frameAvailNumBit,frameNumBit,frameMaxNumBit);

  /* update sample buffer */
  for (i=0; i<EPsampleBufSize-EPframeNumSample; i++)
    EPsampleBuf[i] = EPsampleBuf[i+EPframeNumSample];
  for (i=0; i<EPframeNumSample; i++)
    EPsampleBuf[EPsampleBufSize-EPframeNumSample+i] = sampleBuf[0][i];
  if (EPmode == MODEPAR_MIX) {
    for (i=0; i<EPsampleBufSize-EPframeNumSample; i++)
      EPsampleBuf2[i] = EPsampleBuf2[i+EPframeNumSample];
    for (i=0; i<EPframeNumSample; i++)
      EPsampleBuf2[EPsampleBufSize-EPframeNumSample+i] = sampleBuf[1][i];
  }

  if (mp4ffFlag) {	/* for flexmux bitstream, use writeFlexMuxPDU() */
    if (!EPepConfig) {
      switch (EPmode) {
      case MODEPAR_HVX:
      case MODEPAR_IL:
	addESC = 5-1;
	break;
      case MODEPAR_SWIT:
      case MODEPAR_MIX:
	addESC = 15-1;
	break;
      }
    }
    else
      addESC = 0;
    numLayer = frameData->od->streamCount.value;
    for (layer = 0; layer < numLayer; layer++) {
      if (bitBuf)
	stream[addESC+layer] = BsOpenBufferWrite(frameData->layer[layer].bitBuf);
      else
	stream[addESC+layer] = NULL;
    }
    if (addESC)
      for (i=0; i<addESC; i++)
	/* all ESC instances in single base layer bitstream */
	stream[i] = stream[addESC];
  }
  else {
    if (bitBuf)
      stream[0] = BsOpenBufferWrite(bitBuf);
    else
      stream[0] = NULL;
    for (i=1; i<MAX_LAYER+1+4; i++)
      stream[i] = stream[0];
  }

  switch (EPmode) {
  case MODEPAR_HVX:
    if (bitBuf) {
      bitStreamHvxc[0] = stream[0];	/* AI 990902 */
      if (EPepConfig)
	for (i=1; i<5; i++) /* for ESCs */
	  bitStreamHvxc[i] = stream[i];	/* AI 990902 / HP20010212 */
      if (frameAvailNumBit < rateModeBits[EPrateMode])
	CommonWarning("EncParFrame: not enough bits for HVXC");
      EncParFrameHvx(EPsampleBuf,bitStreamHvxc,
		     rateModeBits[EPrateMode],frameNumBit,frameMaxNumBit);
    }
    break;
  case MODEPAR_IL:
    if (bitBuf)
      EncParFrameIl(EPsampleBuf,stream,
		    frameAvailNumBit,frameNumBit,frameMaxNumBit);
    break;
  case MODEPAR_SWIT:
    for (i=0; i<(EPepConfig?HVXCDBLESC:1); i++)
      bsStream[i]=BsOpenBufferWrite(bsBuf[actBsBuf][i]);
    /* HP 971023   swit mode: 2 HVXC frames / 40 ms */
    /* if (frameNumBit-1 >= 40+80) */
    if (EPrateMode != ENC2K)
      hvxcFrame1 = 80;
    else
      hvxcFrame1 = 40;
    if (bitBuf && frameAvailNumBit < 1+hvxcFrame1+rateModeBits[EPrateMode])
      CommonWarning("EncParFrame: not enough bits for HVXC");
    bitStreamHvxc[0] = bsStream[0];	/* AI 990902 */
    if (EPepConfig)
      for (i=1; i<5; i++) /* for ESCs */
	bitStreamHvxc[i] = bsStream[i];	/* AI 990902 / HP20010212 */
    EncParFrameHvx(EPsampleBuf+HVXdelayOff,bitStreamHvxc,
		   hvxcFrame1,80,80);
    if (EPepConfig)
      for (i=0; i<5; i++) /* for ESCs */
	bitStreamHvxc[i] = bsStream[5+i];	/* AI 990902 / HP20010212 */
    EncParFrameHvx(EPsampleBuf+HVXdelayOff+160,bitStreamHvxc,
		   rateModeBits[EPrateMode],80,80);
    for (i=0; i<(EPepConfig?HVXCDBLESC:1); i++)
      BsClose(bsStream[i]);
    if (EPdebugLevel >= 1)
      printf("judge = %d\n",judge);
    if (bitBuf) {
      if (judge==0)
	CommonWarning("EncParFrame: judge==0");
      if (EPtest) {
	judge = (testFrame%max(EPtest,-EPtest) < max(EPtest,-EPtest)/2)?1:2;
	if (EPtest<0)
	  judge = 3-judge;
	testFrame++;
	if (EPdebugLevel >= 1)
	  printf("testjudge = %d\n",judge);
      }

      if (EPdebugLevel >= 1)
	printf("switch: %s\n",(judge==1)?"il":"hvxc");
      if (judge==1) {
	/* music */
	BsPutBit(stream[0],1,1);

	EncParFrameIl(EPsampleBuf+ILdelayOff,stream,
		      frameAvailNumBit-1,frameNumBit,frameMaxNumBit);
      }
      else {
	/* speech & dflt */
	BsPutBit(stream[0],0,1);

	if (EPepConfig)
	  for (i=0; i<HVXCDBLESC; i++)
	    BsPutBuffer(stream[5+i],bsBuf[(actBsBuf+1)%numBsBuf][i]);
	else
	  BsPutBuffer(stream[0],bsBuf[(actBsBuf+1)%numBsBuf][0]);
	if (EPdebugLevel >= 1) {
	  if (EPepConfig)
	    for (i=0; i<HVXCDBLESC; i++)
	      printf("hvxcbufnumbit[%d]=%ld\n",
		     i,BsBufferNumBit(bsBuf[(actBsBuf+1)%numBsBuf][i]));
	  else
	    printf("hvxcbufnumbit=%ld\n",
		   BsBufferNumBit(bsBuf[(actBsBuf+1)%numBsBuf][0]));
	}

	EncParFrameIl(EPsampleBuf+ILdelayOff,NULL,
		      frameAvailNumBit-1,frameNumBit,frameMaxNumBit);
      }
    }
    actBsBuf=(actBsBuf+1)%numBsBuf;  
    break;
  case MODEPAR_MIX:
    mixMode = EPmixMode;
    if (EPmixMode < 0) {
      mixMode = (testFrame%(-EPmixMode))/(-EPmixMode/4);
      testFrame++;
    }
    if (EPdebugLevel >= 1)
      printf("mix: %d\n",mixMode);
    if (bitBuf) {
      BsPutBit(stream[0],mixMode,2);
      ilBits = frameAvailNumBit-2;
      switch (mixMode) {
      case 1:
	ilBits -= 80;
	break;
      case 2:
	ilBits -= 160;
	break;
      }
      EncParFrameIl(EPsampleBuf2+ILdelayOff,(mixMode==0) ?
		    (HANDLE_BSBITSTREAM *)NULL : stream,
		    ilBits,frameNumBit,frameMaxNumBit);
      switch (mixMode) {
      case 0:
	/* if (frameNumBit-2 >= 40+80) */
	if (EPrateMode != ENC2K)
	  hvxcFrame1 = 80;
	else
	  hvxcFrame1 = 40;
	if (frameAvailNumBit < 2+hvxcFrame1+rateModeBits[EPrateMode])
	  CommonWarning("EncParFrame: not enough bits for HVXC");
	if (EPepConfig)
	  for (i=0; i<5; i++) /* for ESCs */
	    bitStreamHvxc[i] = stream[5+i];	/* AI 990902 / HP20010212 */
	else
	  bitStreamHvxc[0] = stream[0];	/* AI 990902 */
	EncParFrameHvx(EPsampleBuf+HVXdelayOff,bitStreamHvxc,
		       hvxcFrame1,80,80);
	if (EPepConfig)
	  for (i=0; i<5; i++) /* for ESCs */
	    bitStreamHvxc[i] = stream[10+i];	/* AI 990902 / HP20010212 */
	EncParFrameHvx(EPsampleBuf+HVXdelayOff+160,bitStreamHvxc,
		       rateModeBits[EPrateMode],80,80);
	break;
      case 1:
	if (EPepConfig)
	  for (i=0; i<5; i++) /* for ESCs */
	    bitStreamHvxc[i] = stream[5+i];	/* AI 990902 / HP20010212 */
	else
	  bitStreamHvxc[0] = stream[0];	/* AI 990902 */
	EncParFrameHvx(EPsampleBuf+HVXdelayOff,bitStreamHvxc,
		       40,40,40);
	if (EPepConfig)
	  for (i=0; i<5; i++) /* for ESCs */
	    bitStreamHvxc[i] = stream[10+i];	/* AI 990902 / HP20010212 */
	EncParFrameHvx(EPsampleBuf+HVXdelayOff+160,bitStreamHvxc,
		       40,40,40);
	break;
      case 2:
	if (EPepConfig)
	  for (i=0; i<5; i++) /* for ESCs */
	    bitStreamHvxc[i] = stream[5+i];	/* AI 990902 / HP20010212 */
	else
	  bitStreamHvxc[0] = stream[0];	/* AI 990902 */
	EncParFrameHvx(EPsampleBuf+HVXdelayOff,bitStreamHvxc,
		       80,80,80);
	if (EPepConfig)
	  for (i=0; i<5; i++) /* for ESCs */
	    bitStreamHvxc[i] = stream[10+i];	/* AI 990902 / HP20010212 */
	EncParFrameHvx(EPsampleBuf+HVXdelayOff+160,bitStreamHvxc,
		       80,80,80);
	break;
      }
    }
    break;
  }
  if (bitBuf==NULL)
    /* encoder start up */
    return;
  if (mp4ffFlag) {
    PARusedNumBit = 0; /* HP20010212 */
    for (layer = 0; layer < numLayer; layer++)
      BsClose(stream[addESC+layer]);
    stream[0] = BsOpenBufferWrite(bitBuf);
    for (layer = 0; layer < numLayer; layer++) {
      PARusedNumBit += BsBufferNumBit(frameData->layer[layer].bitBuf);
      if (EPdebugLevel>1)
	printf("EncParFrame: lay=%d curbit=%ld numbit=%ld\n",
	       layer,BsCurrentBit(stream[0]),
	       BsBufferNumBit(frameData->layer[layer].bitBuf)); 
      writeFlexMuxPDU(layer, stream[0], frameData->layer[layer].bitBuf);
    }
    if (EPdebugLevel>1)
      printf("EncParFrame: ... curbit=%ld PARusedNumBit=%d\n",
	     BsCurrentBit(stream[0]),PARusedNumBit); 
    BsClose(stream[0]);
  }
  else
    BsClose(stream[0]);

  if (EPdebugLevel >= 1)
    printf("EncParFrame: numBit=%ld\n",BsBufferNumBit(bitBuf));

}


/* EncParFree() */
/* Free memory allocated by parametric encoder core. */

void EncParFree ()
{
  if (EPdebugLevel >= 1)
    printf("EncParFree: ...\n");

  if (!EPfree)
    return;
  if (EPmode != MODEPAR_IL)
    EncParFreeHvx();
  if (EPmode != MODEPAR_HVX)
    EncParFreeIl();
}


/* EncHvxcInit() */
/* Init HVXC encoder core. */

void EncHvxcInit(
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *encPara,		/* in: encoder parameter string */
  int *frameNumSample,		/* out: num samples per frame */
  int *delayNumSample,		/* out: encoder delay (num samples) */
  HANDLE_BSBITBUFFER bitHeader,	/* out: header for bit stream */
  ENC_FRAME_DATA* frameData,	/* out: system interface(AI 990616) */
  int mainDebugLevel  )
{
  int parac;
  char **parav;
  int result;
  HANDLE_BSBITSTREAM hdrStream;
  int i;

  int layer, numLayer;	/* AI 990616 */
  int bitRateLay;	/* AI 990902 */

  int num;	/* counter for Elementary Stream */
  int esc, mode;
  
  /* evalute encoder parameter string */
  parav = CmdLineParseString(encPara,SEPACHAR,&parac);
  result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);
  if (result) {
    if (result==1) {
      EncParInfo(stdout);
      exit (1);
    }
    else
      CommonExit(1,"encoder parameter string error");
  }

  
  EPparaMode = MP4ModeParName[MODEPAR_HVX];

  if (EPdelayMode)
    ipc_encDelayMode = DM_LONG;
  else
    ipc_encDelayMode = DM_SHORT;

  if (EPvrMode)
    ipc_bitstreamMode = BM_VARIABLE;
  else
    ipc_bitstreamMode = BM_CONSTANT;

  ipc_extensionFlag = EPextensionFlag;
  
  if (EPextensionFlag) {
    ipc_packMode = PM_SHORT;

    /* in only variable mode, var_SclableFlag is set(AI 2001/01/31) */
    if (EPvrMode) EPvrScalFlag = EPscalFlag;	/* AI 990902 */
  /* for VR scalable mode (YM 990728) */
    if (EPscalFlag)
      ipc_vrScalFlag = SF_ON;
    else
      ipc_vrScalFlag = SF_OFF;
  }

  EPrateMode = ENC4K;	/* 4000 kbps */
  if (bitRate <= 3999.5)
    EPrateMode = ENC3K;	/* 3700 kbps */ /* 3850->3700  HP20010213 */
  if (bitRate <= 3699.5)
    EPrateMode = ENC2K;	/* 2000 kbps */


  if (EPscalFlag) EPrateMode = ENC2K;/* AI 2001/01/31 */

  if (EPdebugLevel >= 1) {
    printf("EncHvxcInit: numChannel=%d  fSample=%f  bitRate=%f  "
	   "encPara=\"%s\"\n",
	   numChannel,fSample,bitRate,encPara);
    printf("EncHvxcInit: debugLevel=%d  mode=\"%s\"\n",
	   EPdebugLevel,EPparaMode);
    printf("EncHvxcInit: encDlyMode=%d  vrMode=%d  rateMode=%d\n",
	   ipc_encDelayMode,ipc_bitstreamMode,EPrateMode);
    printf("EncHvxcInit: mp4ffFlag=%d\n",
	   mp4ffFlag);
  }

  EPmode = 0;	/* HVXC only */
  
  CmdLineParseFree(parav);

  EPfSample = fSample;
  EPbitRate = bitRate;

  if (EPepConfig && !EPextensionFlag) /* HP20010214 */
    CommonExit(1,"EncHvxcInit: epc=1 requires ef=1 (ER_HVXC)");
  
  hdrStream = BsOpenBufferWrite(bitHeader);

  EncParInitHvx(hdrStream,mp4ffFlag);

  /* system interface(flexmux) configuration (AI 990616) */
  if (mp4ffFlag) {
    if (EPscalFlag) {
      NumEnhLayers = 1;	/* base layer + 1 enhancement layer */
    }
    else {
      NumEnhLayers = 0;	/* only base layer */
    }
    numLayer = 1+NumEnhLayers;
    initObjDescr(frameData->od,mainDebugLevel);

    if (!EPextensionFlag || !EPepConfig) {  /* HVXC,ER_HVXC(epConfig=0) */
    presetObjDescr(frameData->od, numLayer-1);
    frameData->od->streamCount.value=numLayer;
    
    for (layer=0; layer<numLayer; layer++) {
      initESDescr(&(frameData->od->ESDescriptor[layer]));
      presetESDescr(frameData->od->ESDescriptor[layer], layer);

      /* elementary stream configuration */
      /* DecConfigDescriptor configuration */
      if (layer>0)
	/* enahanement layer dependency(AI 991129) */
        frameData->od->ESDescriptor[layer]->dependsOn_Es_number.value 
	  = frameData->od->ESDescriptor[0]->ESNumber.value;

      if (layer == 0) {
	bitRateLay = (int)(bitRate+.5) - (int)2000*NumEnhLayers;
      }
      else {
	bitRateLay = 2000;
      }
      frameData->layer[layer].bitRate = bitRateLay;
      frameData->od->ESDescriptor[layer]->DecConfigDescr.avgBitrate.value = frameData->layer[layer].bitRate;
      frameData->od->ESDescriptor[layer]->DecConfigDescr.maxBitrate.value = frameData->layer[layer].bitRate; /* AI 991129 */
      frameData->od->ESDescriptor[layer]->DecConfigDescr.bufferSizeDB.value = (int)(frameData->layer[layer].bitRate*0.020+.5+7)/8;  /* AI 2001/02/01 */
      if (layer==0) {
	/* base layer */
	frameData->od->ESDescriptor[layer]->DecConfigDescr.specificInfoLength.value = 2+1;	/* 15+ 6 bits at maximum(AI 991129) */
      }
      else {
	/* enhancement layer */
#ifdef CORRIGENDUM1
	frameData->od->ESDescriptor[layer]->DecConfigDescr.specificInfoLength.value = 1+1;	
	/* 15+ 1 bit for "isBaseLayer"bit(AI 001012) */
#else
	/* no specificInfo required(AI 991129) */
	frameData->od->ESDescriptor[layer]->DecConfigDescr.specificInfoFlag.value = 0;
	frameData->od->ESDescriptor[layer]->DecConfigDescr.specificInfoLength.value = 2+0;
#endif
      }

      /* ALConfigDescriptor configuration */
#ifdef AL_CONFIG        
      frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useAccessUnitStartFlag.value=1;
      frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useAccessUnitEndFlag.value=1;
      frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useRandomAccessPointFlag.value=0;
      frameData->od->ESDescriptor[layer]->ALConfigDescriptor.usePaddingFlag.value=0;
      frameData->od->ESDescriptor[layer]->ALConfigDescriptor.seqNumLength.value=0;
#endif
      /* AudioSpecificConfig configuration */
      if (EPextensionFlag) {
	frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value = ER_HVXC;	/* Error Resilient HVXC core */
      }
      else {
	frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value = HVXC;	/* HVXC core */
      }
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value = 0xb;	/* 8000Hz */
      frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value = 1;	/* mono */
      
      /* hvxcSpecificConfig configuration */
      if  (layer == 0) {
	/* HVXC base layer configuration */
#ifdef CORRIGENDUM1
	frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.isBaseLayer.value = 1;	/* isBaseLayer */
#endif
	frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCvarMode.value = ipc_bitstreamMode;	/* HVXCvarMode */
	if (EPscalFlag) {	/* base layer configuration in scalable mode */
	  frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCrateMode.value = ENC2K;	/* HVXCrateMode */
	}
	else {
	  frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCrateMode.value = EPrateMode;	/* HVXCrateMode */
	}
	frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.extensionFlag.value = EPextensionFlag;	/* extensionFlag */
	if (EPextensionFlag) {
	  frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.vrScalFlag.value = EPvrScalFlag;	/*YM 990728  */
	}
      }
      else {
	/* HVXC enhancement layer configuration */
#ifdef CORRIGENDUM1
	frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.isBaseLayer.value = 0;	/* isBaseLayer */
#endif
	/* no speific configuration required */
      }
    }

    advanceODescr(hdrStream, frameData->od, 1);

    for (layer=0; layer<numLayer; layer++){ 
      advanceESDescr(hdrStream, frameData->od->ESDescriptor[layer], 1,0);
      frameData->layer[layer].bitBuf = BsAllocBuffer(80);
    }
    }  /* if (!EPextensionFlag || !epConfig) { */
    else {  /* EPepConfig=1 */

      frameData->od->streamCount.value = 0;
      for (layer=0; layer<numLayer; layer++) {
	mode=get_hvxc_mode(layer,EPrateMode);
	numEsc[layer]=hvxcNumEsc[mode];
	frameData->od->streamCount.value += numEsc[layer];
      }
      presetObjDescr(frameData->od, frameData->od->streamCount.value);

      num=0;
      for (layer=0; layer<numLayer; layer++) {
	mode=get_hvxc_mode(layer,EPrateMode);

	for (esc=0; esc<numEsc[layer]; esc++, num++) {

	  initESDescr(&(frameData->od->ESDescriptor[num]));
	  presetESDescr(frameData->od->ESDescriptor[num], num);

	  /* elementary stream configuration */
	  /* DecConfigDescriptor configuration */
	  if (num>0)
	    /* enhancement layer dependency(AI 991129) */
	    frameData->od->ESDescriptor[num]->dependsOn_Es_number.value 
	      = frameData->od->ESDescriptor[num-1]->ESNumber.value;

	  frameData->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.epConfig.value = EPepConfig;
	  bitRateLay = hvxcMaxBitRate[mode][esc];
	
	  frameData->layer[num].bitRate = bitRateLay;
	  frameData->od->ESDescriptor[num]->DecConfigDescr.avgBitrate.value = frameData->layer[num].bitRate;
	  frameData->od->ESDescriptor[num]->DecConfigDescr.maxBitrate.value = frameData->layer[num].bitRate; /* AI 991129 */
	  frameData->od->ESDescriptor[num]->DecConfigDescr.bufferSizeDB.value = (int)(frameData->layer[num].bitRate*0.020+.5+7)/8;  /* AI 2001/02/01 */
	  if (layer==0) {
	    /* base layer */
	    frameData->od->ESDescriptor[num]->DecConfigDescr.specificInfoLength.value = 1;	/* 6 bits at maximum(AI 991129) */
	  }
	  else {
	    /* enhancement layer */
#ifdef CORRIGENDUM1
	    frameData->od->ESDescriptor[num]->DecConfigDescr.specificInfoLength.value = 1;	
	    /* 1 bit for "isBaseLayer"bit(AI 001012) */
#else
	    /* no specificInfo required(AI 991129) */
	    frameData->od->ESDescriptor[num]->DecConfigDescr.specificInfoFlag.value = 0;
	    frameData->od->ESDescriptor[num]->DecConfigDescr.specificInfoLength.value = 0;
#endif
	  }

	  /* ALConfigDescriptor configuration */
#ifdef AL_CONFIG        
	  frameData->od->ESDescriptor[num]->ALConfigDescriptor.useAccessUnitStartFlag.value=1;
	  frameData->od->ESDescriptor[num]->ALConfigDescriptor.useAccessUnitEndFlag.value=1;
	  frameData->od->ESDescriptor[num]->ALConfigDescriptor.useRandomAccessPointFlag.value=0;
	  frameData->od->ESDescriptor[num]->ALConfigDescriptor.usePaddingFlag.value=0;
	  frameData->od->ESDescriptor[num]->ALConfigDescriptor.seqNumLength.value=0;
#endif
	  /* AudioSpecificConfig configuration */
	  frameData->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value = ER_HVXC;	/* Error Resilient HVXC core */
	  frameData->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value = 0xb;	/* 8000Hz */
	  frameData->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value = 1;	/* mono */
      
	  /* hvxcSpecificConfig configuration */
	  if  (layer == 0) {
	    /* ER_HVXC base layer configuration */
#ifdef CORRIGENDUM1
	    frameData->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.isBaseLayer.value = 1;	/* isBaseLayer */
#endif
	    frameData->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCvarMode.value = ipc_bitstreamMode;	/* HVXCvarMode */
	    if (EPscalFlag) {	/* base layer configuration in scalable mode */
	      frameData->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCrateMode.value = ENC2K;	/* HVXCrateMode */
	    }
	    else {
	      frameData->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.HVXCrateMode.value = EPrateMode;	/* HVXCrateMode */
	    }
	    frameData->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.extensionFlag.value = EPextensionFlag;	/* extensionFlag */

	    frameData->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.vrScalFlag.value = EPvrScalFlag;	/*YM 990728  */
	  }
	  else {
	    /* ER_HVXC enhancement layer configuration */
#ifdef CORRIGENDUM1
	    frameData->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.isBaseLayer.value = 0;	/* isBaseLayer */
#endif
	    /* no speific configuration required */
	  }
	}	/* for (esc=0; esc<numEsc[layer]; esc++, num++) { */
      }	/* for (layer=0, num=0; layer<numLayer; layer++) { */

      advanceODescr(hdrStream, frameData->od, 1);

      for (layer=0, num=0; layer<numLayer; layer++) {
	for (esc=0; esc<numEsc[layer]; esc++, num++) {
	  advanceESDescr(hdrStream, frameData->od->ESDescriptor[num], 1);
	  frameData->layer[num].bitBuf = BsAllocBuffer(80);
	}
      }
    }

  } /* if (mp4ffFlag) */

  BsClose(hdrStream);

  /* init sample buffer */
  if (CommonFreeAlloc((void**)&EPsampleBuf,
		      EPsampleBufSize*sizeof(float)) == NULL)
    CommonExit(1,"EncHvxcInit: memory allocation error");
  for (i=0; i<EPsampleBufSize; i++)
    EPsampleBuf[i] = 0;

  *frameNumSample = EPframeNumSample;
  *delayNumSample = EPdelayNumSample;

  if (EPdebugLevel >= 1) {
    printf("EncHvxcInit: modeInt=%d\n",EPmode);
    printf("EncHvxcInit: frmNumSmp=%d  dlyNumSmp=%d  hdrNumBit=%ld\n",
	   *frameNumSample,*delayNumSample,BsBufferNumBit(bitHeader));
    printf("EncHvxcInit: dly=%d  buf=%d  frm=%d\n",
	   EPdelayNumSample,EPsampleBufSize,EPframeNumSample);
    printf("EncHvxcInit: HVXdly=%d  HVXbuf=%d  HVXoff=%d\n",
	   HVXdelayNumSample,HVXsampleBufSize,HVXdelayOff);
  }
}

/* EncHvxcFrame() */
/* Encode one audio frame into one bit stream frame with */
/* HVXC encoder core. */

void EncHvxcFrame (
  float **sampleBuf,		/* in: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  HANDLE_BSBITBUFFER bitBuf,		/* out: bit stream frame */
				/*      or NULL during encoder start up */
  int frameAvailNumBit,		/* in: total num bits available for */
				/*     this frame (incl. bit reservoir) */
  int frameNumBit,		/* in: average num bits per frame */
  int frameMaxNumBit,		/* in: max num bits per frame */
  ENC_FRAME_DATA* frameData	/* in/out: system interface(AI 990616) */
  )
{
  HANDLE_BSBITSTREAM stream;
  int i;
  int layer, numLayer;
  int num;
  int esc;
  HANDLE_BSBITSTREAM bitStreamLay[4+3];	/* at maximum */

  if (EPdebugLevel >= 1)
    printf("EncParFrame: %s  availNum=%d  num=%d  maxNum=%d\n",
	   (bitBuf==NULL)?"start up":"normal",
	   frameAvailNumBit,frameNumBit,frameMaxNumBit);

  /* update sample buffer */
  for (i=0; i<EPsampleBufSize-EPframeNumSample; i++)
    EPsampleBuf[i] = EPsampleBuf[i+EPframeNumSample];
  for (i=0; i<EPframeNumSample; i++)
    EPsampleBuf[EPsampleBufSize-EPframeNumSample+i] = sampleBuf[0][i];

  if (mp4ffFlag) {	/* for flexmux bitstream, use writeFlexMuxPDU() */
    numLayer = frameData->od->streamCount.value;
    for (layer = 0; layer < numLayer; layer++) {
      if (bitBuf)
	bitStreamLay[layer] = BsOpenBufferWrite(frameData->layer[layer].bitBuf);
      else
	bitStreamLay[layer] = NULL;
    }
  }
  else {
    numLayer = 1;
    if (bitBuf)
      bitStreamLay[0] = BsOpenBufferWrite(bitBuf);
    else
      bitStreamLay[0] = NULL;
  }
  
  if (bitBuf)		/* HP20001012 */
    EncParFrameHvx(EPsampleBuf,bitStreamLay,
		   frameAvailNumBit,frameNumBit,frameMaxNumBit);
    
  if (mp4ffFlag) {
    if (bitBuf) {
      for (layer = 0; layer < numLayer; layer++) {
        BsClose(bitStreamLay[layer]);
      }
      bitStreamLay[0] = BsOpenBufferWrite(bitBuf);
      for (layer = 0; layer < numLayer; layer++) {
        writeFlexMuxPDU(layer, bitStreamLay[0], frameData->layer[layer].bitBuf);
      }
      BsClose(bitStreamLay[0]);
    }
  } else {
    if (bitBuf)
      BsClose(bitStreamLay[0]);
  }

  if (bitBuf && EPdebugLevel >= 1)
    printf("EncParFrame: numBit=%ld\n",BsBufferNumBit(bitBuf));
}

/* EncParFree() */
/* Free memory allocated by parametric encoder core. */

void EncHvxcFree ()
{
  if (EPdebugLevel >= 1)
    printf("EncHvxcFree: ...\n");

  EncParFreeHvx();
}



/* end of enc_par.c */

