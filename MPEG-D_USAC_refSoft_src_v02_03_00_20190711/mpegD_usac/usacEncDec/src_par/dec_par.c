/**********************************************************************
MPEG-4 Audio VM
Decoder core (parametric)



This software module was originally developed by

Heiko Purnhagen (University of Hannover / Deutsche Telekom Berkom)
Bernd Edler (University of Hannover / Deutsche Telekom Berkom)
Masayuki Nishiguchi, Kazuyuki Iijima, Jun Matsumoto (Sony Corporation)

and edited by

Akira Inoue (Sony Corporation) and
Yuji Maeda (Sony Corporation)
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

$Id: dec_par.c,v 1.1.1.1 2009-05-29 14:10:13 mul Exp $
Parametric decoder module
**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "common_m4a.h"		/* common module */
#include "cmdline.h"		/* command line module */
#include "bitstream.h"		/* bit stream module */

#include "obj_descr.h"		/* AI 990616 */

#include "indilinedec.h"	/* indiline bitstream decoder module */
#include "indilinesyn.h"	/* indiline synthesiser module */


#include "dec_par.h"		/* decoder cores */

#include "dec_hvxc.h"		/* HVXC decoder core */

#include "mp4_par.h"		/* parametric core common declarations */

#include "hvxc.h"		/* hvxcDec module */
#include "hvxcDec.h"		/* hvxcDec module */


/* ---------- declarations ---------- */

#define PROGVER "parametric decoder core  MPEG-4 Audio  03-Mar-2003"

#define SEPACHAR " ,="

#define MAXSAMPLE 32768		/* full scale sample value */

#define STRLEN 255

#define MINSPEEDFACT (0.2f)	/* expected min. value: speedFact=0.2 */
				/* AI 990128 / HP 990422 */


/* ---------- variables ---------- */

static float DPfSample;
static float DPbitRate;
static int DPmode;		/* enum MP4ModePar */
static int DPframeMaxNumSample;
static int DPdelayNumSample;
static int DPframeTypNumSample;
static float speedFactCl;
static float pitchFactCl;


/* indiline handles */
static ILDstatus *ILD;
static ILSstatus *ILS;

extern int samplFreqIndex[];       /* (tf_tables.c) undesired, but ... */

/* command line module */

static int DPdelayMode;
static int DPdebugLevel;
static int DPdebugLevelIl;

/* HILN decoder control HP970514 */
static int DPnoIL;
static int DPnoHarm;
static int DPnoNoise;
static int DPmaxNumLine;
static int DPmaxNumHarm;
static int DPmaxNumHarmLine;
static int DPmaxNumNoisePara;
static int DPmaxNumSynth;
static int DPcgdSynth;
static float DPnoiseParaFact;
static int DPenhaFlag;
static int DPenhaFlagUsed;
static int DPusePhase;
static int DPnumExtLayer;
static int DPnumExtLayerUsed;
static int DPinitPrevNumLine;

/* HVXC decoder configuration (AI 990210) */
static int DPvarMode;
static int DPrateMode;
static int DPextensionFlag;

static int DPnumClass = 1;
/* static int DPpackMode = PM_SHORT; */

static int DPvrScalFlag = SF_OFF;

static int DPdecMode;
static int DPscalFlag;  /* to be set in scalable bitstream decoding(AI 2001/01/31) */
static int DPtestMode;	/* HVXC test_mode for decoder conformance testing */
/* 0000 0000 0000 0000 : normal operation */
/* xxxx xxxx xxxx xxx1 : postfilter and post processing are skipped */
/* xxxx xxxx xxxx xx1x : initial values of harmonic phase are reset to zeros */
/* xxxx xxxx xxxx x1xx : noise component addition are disabled */
/* xxxx xxxx xxxx 1xxx : the output of Time Domain Decoder is disabled */

/* HVXC decoder handle(used in DecParFrame() currently) */
static HvxcDecStatus *decHvxcHandle;

static int outputLayerNumber;
static int numLayer;

static ILDconfig ILDcfg;
static ILSconfig ILScfg;
static int DPtest;
static int DPBSenhaFlag;
static int DPnbs;
static int DPfr0;
static float DPtargetP;

static int DPrndseed;
static int DPrndperiod;
static int DPfree;


/* HILN PCU calc. */
static long DPpcuCnt = 0;
static float DPpcuSum = 0.;
static float DPpcuMax = 0.;
static int DPpcuNs;
static int DPpcuNsPrev = 0;

/* for ESC support */
static int epConfig = 0;
static int numEsc[4+3];           /* ER_HVXC number of ESCs for each layer */
static int numEscPA[MAX_LAYER+1]; /* ER_PARA number of ESCs for each layer */


static CmdLineSwitch switchList[] = {
  {"h",NULL,NULL,NULL,NULL,"print help"},
  {"dm",&DPdelayMode,"%i","1",NULL,"HVXC delay mode (0=short 1=normal)"},
  {"dl",&DPdebugLevel,"%i","0",NULL,"debug level (calc. HILN/Para PCU)"},
  {"tm",&DPtestMode,"%i","0",NULL,"HVXC test_mode (for decoder conformance testing)"},
 {"dli",&DPdebugLevelIl,"%i","0",NULL,
   "debug level HILN (dec + 10*syn)"},
  {"noi",&DPnoIL,NULL,NULL,NULL,"disable indiline synthesis"},
  {"noh",&DPnoHarm,NULL,NULL,NULL,"disable harm synthesis"},
  {"non",&DPnoNoise,NULL,NULL,NULL,"disable noise synthesis"},
  {"ml",&DPmaxNumLine,"%i","999",NULL,"max num individual lines"},
  {"mh",&DPmaxNumHarm,"%i","999",NULL,"max num harmonic tones"},
  {"mhl",&DPmaxNumHarmLine,"%i","999",NULL,"max num lines of harmonic tones"},
  {"mn",&DPmaxNumNoisePara,"%i","999",NULL,"max num noise parameters"},
  {"ms",&DPmaxNumSynth,"%i","999",NULL,"max num lines synthesised"},
  {"cgd",&DPcgdSynth,"%i","0",NULL,"line synthesiser CGD mode (0=tbd)"},
  {"npf",&DPnoiseParaFact,"%f","1.0",NULL,"noise para factor"},
  {"hixl",&DPnumExtLayer,"%i",NULL,&DPnumExtLayerUsed,
   "harm/indi num extension layers\n"
   "(dflt: 0 or controlled by -maxl)"},
  {"hie",&DPenhaFlag,"%i",NULL,&DPenhaFlagUsed,
   "harm/indi enha flag\n"
   "(0=basic 1=enha 2=no-scalable-enha)\n"
   "(dflt: 0 or controlled by -maxl)"},
  {"pnl",&DPinitPrevNumLine,"%i","0",NULL,"initial number of prev. indilines\n"
  "(-1=unknown for start at arbitrary frame)"},
  {"hicm",&ILDcfg.contModeTest,"%i","-1",NULL,
   "harm/indi continue mode\n"
   "(-1=bitstream 0=hi 1=hi&ii 2=none)"},
  {"bsf",&ILDcfg.bsFormat,"%i","6",NULL,
   "HILN bitstream format\n"
   "(0=VM 1=9705 2=CD 3=CD+nx 4=v2 5=v2phase 6=ER_HILN)\n\b"},
  {"nos",&ILDcfg.noStretch,NULL,NULL,NULL,"disable stretching"},
  {"cdf",&ILDcfg.contdf,"%f","1.05",NULL,"line continuation df"},
  {"cda",&ILDcfg.contda,"%f","4.0",NULL,"line continuation da"},
  {"rsp",&ILScfg.rndPhase,"%i","1",NULL,
   "random startphase (0=sin(0) 1=sin(rnd))"},
  {"lsm",&ILScfg.lineSynMode,"%i","2",NULL,
   "line sythesiser mode\n"
   "(1=hard-lpf 2=fast)\n"
   "[0=soft-lpf 3=nosweep-hard 4=nosweep-fast])"},
  {"test",&DPtest,"%i","0",NULL,"for test only (1=hvxc 2=il)"},
  {"rnds",&DPrndseed,"%i","0",NULL,"random generator seed"},
  {"rndp",&DPrndperiod,"%i","0",NULL,"random generator periode (0=+inf)"},
  {"free",&DPfree,"%i",/*"0"*/"1",NULL,"XxxFree()"},
  {"sf",&speedFactCl,"%f","1",NULL,"speed change factor"},
  {"pf",&pitchFactCl,"%f","1",NULL,"pitch change factor"},
  {NULL,NULL,NULL,NULL,NULL,NULL}
};

/* switch/mix stuff */
static float *tmpSampleBuf;
static float *mixSampleBuf;
static int mixSampleBufSize = 0;
static int ILdelayNumSample = 0;
static int HVXdelayNumSample = 0;

static HANDLE_BSBITBUFFER dmyBuf;
static HANDLE_BSBITBUFFER tmpBSbuf;
static HANDLE_BSBITSTREAM tmpBSstream;



/* ---------- internal functions ---------- */



/* DecParInitIl() */
/* Init: individual lines */

static void DecParInitIl (
  HANDLE_BSBITSTREAM hdrStream)	/* in: header for bit stream */
{
  int frameLenQuant;
  double fSampleQuant;
  int maxNumLine;
  int maxNumEnv;
  int maxNumHarm;
  int maxNumHarmLine;
  int maxNumNoisePara;
  int maxNumAllLine;
  int enhaFlag;
  int i;
  HANDLE_BSBITSTREAM dmyStream;


  /* init modules */
  ILD = IndiLineDecodeInit(hdrStream,MAXSAMPLE,DPinitPrevNumLine,
			   &ILDcfg,DPdebugLevelIl%10,
			   &maxNumLine,&maxNumEnv,&maxNumHarm,
			   &maxNumHarmLine,&maxNumNoisePara,&maxNumAllLine,
			   &enhaFlag,&frameLenQuant,&fSampleQuant);

  DPframeTypNumSample = (int)(frameLenQuant*DPfSample/fSampleQuant+0.5);
  DPframeMaxNumSample = (int)(frameLenQuant*DPfSample/fSampleQuant/
			      MINSPEEDFACT+0.5);
  DPdelayNumSample = DPframeTypNumSample/2;
  /* delay for speedFact=1 */
 
  DPBSenhaFlag = enhaFlag;
  if (DPenhaFlag && !enhaFlag) {
    CommonWarning("DecParInitIl: no enhancement bitstream available");
    DPenhaFlag = 0;
  }

  ILS = IndiLineSynthInit(DPframeMaxNumSample,DPfSample,
			  maxNumAllLine,maxNumEnv,maxNumNoisePara,
			  &ILScfg,DPdebugLevelIl/10,ILDcfg.bsFormat);


  DPusePhase = DPenhaFlag || (ILDcfg.bsFormat>=5);

  if (DPdebugLevel >= 1) {
    printf("DecParInitIl: dbgLvlIl=%d\n",
	   DPdebugLevelIl);
    printf("DecParInitIl: maxNumLine=%d\n",
	   maxNumLine);
    printf("DecParInitIl: fSampleQuant=%f  frameLenQuant=%d\n",
	   fSampleQuant,frameLenQuant);
    printf("DecParInitIl: frmNumSmp=%d  dlyNumSmp=%d\n",
	   DPframeMaxNumSample,DPdelayNumSample);
  }
  dmyBuf=BsAllocBuffer(200);
  dmyStream=BsOpenBufferWrite(dmyBuf);
  for (i=0; i<200; i++)		/* make "silenence" bitstream */
    BsPutBit(dmyStream,0,1);
  BsClose(dmyStream);
}


/* DecParInitHvx() */
/* Init: harmonic vector exitation */

static void DecParInitHvx (
  HANDLE_BSBITSTREAM hdrStream)	/* in: header for bit stream */
{
  if (BsGetBitInt(hdrStream,(unsigned int*)&DPvarMode,1))
    CommonExit(1,"DecParInitHvx: error reading bit stream header");
  if (BsGetBitInt(hdrStream,(unsigned int*)&DPrateMode,2))
    CommonExit(1,"DecParInitHvx: error reading bit stream header");
  if (BsGetBitInt(hdrStream,(unsigned int*)&DPextensionFlag,1))
    CommonExit(1,"DecParInitHvx: error reading bit stream header");
  if (DPextensionFlag) {
    /* VR scalable mode (YM 990728) */
    if (BsGetBitInt(hdrStream,(unsigned int*)&DPvrScalFlag,1))
      CommonExit(1,"DecParInitHvx: error reading bit stream header");
  }

  if (DPrateMode == DEC2K) DPnumClass = N_CLS_2K;
  else DPnumClass = N_CLS_4K;

  if (DPvrScalFlag) DPrateMode = ENC4K;
  /*
  DPframeMaxNumSample = 160 * (int) ceil(1.0 / DPspeedFact);
  */
  /* DPframeMaxNumSample = 320;	*//* expected max.value, speedFact=0.5 (AI 990128) */
  DPframeMaxNumSample = 160 * (int) ceil(1.0 / MINSPEEDFACT);
  
  if (DPdelayMode == DM_LONG)	/* HP 971023 */
    DPdelayNumSample = 80;	/* 10 ms */
  else
    DPdelayNumSample = 60;	/* 7.5 ms */

  decHvxcHandle = hvxc_decode_init(DPvarMode,
				   DPrateMode,
				   DPextensionFlag,
				   /* version 2 data (YM 990616) */
				   DPnumClass,
				   /* version 2 data (YM 990728) */
				   DPvrScalFlag,
				   DPdelayMode,
				   DPtestMode);
}

/* DecParFrameIl() */
/* Decode frame: individual lines */

static void DecParFrameIl (
  HANDLE_BSBITSTREAM *stream,	/* in: bit stream */
  int useESC,			/* in: set to use 5 base layer ESC streams */
  float *sampleBuf,		/* out: frameNumSample audio samples */
  int *frameBufNumSample,	/* out: num samples in sampleBuf[] */
  float speedFact,		/* in: speed change factor (HP 990422) */
  float pitchFact)		/* in: pitch change factor (HP 990422) */
{
  int numEnv;
  double **envPara;
  int numLine;
  double *lineFreq;
  double *lineAmpl;
  double *linePhase;
  int *linePhaseValid;
  int *lineEnv;
  int *linePred;
  int numHarm;
  int *numHarmLine;
  double *harmFreq;
  double *harmFreqStretch;
  int *harmLineIdx;
  int *harmEnv;
  int *harmPred;
  int numNoisePara;
  double noiseFreq;
  double *noisePara;
  int noiseEnv;
  int numAllLine;

  int i,il,epcbase;
  HANDLE_BSBITSTREAM dmyStream;
  HANDLE_BSBITSTREAM dmyStream5[5];
  HANDLE_BSBITSTREAM streamBase[5];
  HANDLE_BSBITSTREAM streamEnha;
  HANDLE_BSBITSTREAM streamExt[MAX_LAYER];

  if (speedFact<MINSPEEDFACT) {
    speedFact = MINSPEEDFACT;
    CommonWarning("DecParFrameIl: speedFact below %f",MINSPEEDFACT);
  }
  
  /* call IndiLineDecodeFrame() to update internal frame-to-frame memory */
  dmyStream = BsOpenBufferRead(dmyBuf);
  for (i=0; i<5; i++)
    dmyStream5[i] = dmyStream;

  epcbase = useESC?5:1;
  for (i=0; i<5; i++)
    streamBase[i] = (stream)?stream[useESC?i:0]:NULL; 
  streamEnha = (stream)?stream[epcbase]:NULL;
  for (i=0; i<DPnumExtLayer; i++)
    streamExt[i] = (stream)?stream[epcbase+(DPBSenhaFlag?1:0)+i]:NULL;
  for (; i<MAX_LAYER; i++)
    streamExt[i] = NULL;

  IndiLineDecodeFrame(ILD,(stream)?streamBase:dmyStream5,
		      (DPenhaFlag)?((stream)?streamEnha:dmyStream):
		      (HANDLE_BSBITSTREAM)NULL,
		      DPnumExtLayer,
		      (stream && DPnumExtLayer)?streamExt:(HANDLE_BSBITSTREAM*)NULL,
		      &numEnv,&envPara,&numLine,&lineFreq,
		      &lineAmpl,
		      (DPusePhase)?&linePhase:(double**)NULL,
		      (DPusePhase)?&linePhaseValid:(int**)NULL,
		      &lineEnv,&linePred,
		      &numHarm,&numHarmLine,&harmFreq,&harmFreqStretch,
		      &harmLineIdx,&harmEnv,&harmPred,
		      &numNoisePara,&noiseFreq,&noisePara,&noiseEnv);
  BsClose(dmyStream);


  if (DPdebugLevel >= 2) {
    printf("DecParFrameIl: decoded parameters:\n");
    i = 0;
    for(il=0; il<numLine; il++)
      if (linePred[il])
	i++;
    printf("numLine=%2d (new=%2d cont=%2d)\n",numLine,numLine-i,i);
    if (DPdebugLevel >= 3) {
      for (i=0; i<numEnv; i++)
	printf("env %d: tm=%7.5f atk=%7.5f dec=%7.5f\n",
	       i,envPara[i][0],envPara[i][1],envPara[i][2]);
      for(il=0; il<numLine; il++)
	printf("%2d: f=%7.1f a=%7.1f p=%5.2f e=%1d c=%2d\n",
	       il,lineFreq[il],lineAmpl[il],
	       (DPusePhase && linePhaseValid[il])?linePhase[il]:9.99,
	       lineEnv[il],linePred[il]);
    }
    for (i=0; i<numHarm; i++) {
      printf("harm %d: n=%2d f=%7.1f fs=%10e e=%1d c=%1d\n",
	     i,numHarmLine[i],harmFreq[i],harmFreqStretch[i],
	     harmEnv[i],harmPred[i]);
      if (DPdebugLevel >= 3)
	for(il=harmLineIdx[i]; il<harmLineIdx[i]+numHarmLine[i]; il++)
	  printf("h %2d: a=%7.1f\n",il-harmLineIdx[i]+1,lineAmpl[il]);
    }
    if (numNoisePara) {
      printf("noise: n=%d f=%7.1f e=%1d\n",
	     numNoisePara,noiseFreq,noiseEnv);
      if (DPdebugLevel >= 3)
	for (i=0; i<numNoisePara; i++)
	  printf("noise %d: p=%10e\n",i,noisePara[i]);
    }
  }

  if (DPnoIL)
    numLine = 0;
  if (DPnoHarm)
    numHarm = 0;
  if (DPnoNoise)
    numNoisePara = 0;
  numLine = min(numLine,DPmaxNumLine);
  numHarm = min(numHarm,DPmaxNumHarm);
  for (i=0; i<numHarm; i++)
    numHarmLine[i] = min(numHarmLine[i],DPmaxNumHarmLine);
  numNoisePara = min(numNoisePara,DPmaxNumNoisePara);

  /* enhanced mode synthesiser control */
  if (DPenhaFlag==1) {
    numNoisePara = 0;
    if (numHarm)
      numHarmLine[0] = min(numHarmLine[0],10);
  }

  /* append harm lines and join with individual lines */

  IndiLineDecodeHarm(ILD,numLine,lineFreq,lineAmpl,
		     (DPusePhase)?linePhase:(double*)NULL,
		     (DPusePhase)?linePhaseValid:(int*)NULL,
		     lineEnv,linePred,
		     numHarm,numHarmLine,
		     harmFreq,
		     harmFreqStretch,
		     harmLineIdx,harmEnv,harmPred,
		     &numAllLine);

  /* cal DPpcuNs */
  DPpcuNs = DPpcuNsPrev+numAllLine;
  DPpcuNsPrev = numAllLine;
  if (!DPenhaFlag)
    for (il=0; il<numAllLine; il++)
      if (linePred[il])
	DPpcuNs--;
  
  numAllLine = min(numAllLine,DPmaxNumSynth);


  /* modify frequency if pitch change (only basic mode) */
  for (il=0; il<numAllLine; il++) {
    lineFreq[il] *= (double)pitchFact;
    lineAmpl[il] *= (double)1.;
  }
  if (numNoisePara)
    noiseFreq *= (double)pitchFact;



  if ( ILDcfg.bsFormat<4 )
  {
    for (i=0; i<numNoisePara; i++)
      noisePara[i] *= DPnoiseParaFact;


  }
  else
  {
	noisePara[0]*=DPnoiseParaFact;
  }



  /* enhanced mode synthesiser control */
  if (DPenhaFlag==1)
    for (i=0; i<numAllLine; i++)
      linePred[i] = 0;

  *frameBufNumSample = (int)(DPframeTypNumSample/speedFact+0.5);
  /* synthesise using quantised parameters */
  IndiLineSynth(ILS,(DPusePhase)?1:0,
		*frameBufNumSample,
		numEnv,envPara,
		numAllLine,lineFreq,lineAmpl,
		(DPusePhase)?linePhase:(double*)NULL,
		(DPusePhase)?linePhaseValid:(int*)NULL,
		NULL,lineEnv,linePred,
		numNoisePara,noiseFreq,noisePara,noiseEnv,
		sampleBuf);
}


/* DecParFrameHvx() */
/* Decode frame: harmonic vector exitation */

static void DecParFrameHvx (
  HANDLE_BSBITSTREAM stream,	/* in: bit stream */
  float *sampleBuf,		/* out: frameNumSample audio samples */
  int *frameBufNumSample,	/* out: num samples in sampleBuf[] */
  float speedFact,		/* in: speed change factor(AI 990209) */
  float pitchFact,		/* in: pitch change factor(AI 990209) */
  int epFlag,                 /* in: error protection flag */
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData
  )
{
    int	i, j;

    unsigned char tmpIdVUV;
    unsigned char tmpEncBit;

    unsigned char encBit[10];

    if (speedFact<MINSPEEDFACT) {
      speedFact = MINSPEEDFACT;
      CommonWarning("DecParFrameHvx: speedFact below %f",MINSPEEDFACT);
    }


    if(DPvarMode == BM_VARIABLE) {
      if (!stream)
        DPdecMode = DEC0K; 
      else
	DPdecMode = DPrateMode;
    } else

    {
      /* HP 970706 970708 970709 */
      if (!stream){
        DPdecMode = DEC0K; 
      }
      else if (!BsEof(stream,80-1)){  /* MN 971107 */
        DPdecMode = DEC4K; 
      }
      else if (!BsEof(stream,80-6-1)){ /* MN 971107 */
        DPdecMode = DEC3K;
      }
      else{
        DPdecMode = DEC2K;
      }
    }

    if (stream && BsEof(stream,0)) /* HP20010215 */
      DPdecMode = DEC0K;

    if (DPdebugLevel >= 2) {
      printf("DecParFrameHvx: stream=%s\n",stream?"valid":"NULL");
      printf("DecParFrameHvx: decMode=");
      switch (DPdecMode) {
      case DEC0K : printf("DEC0K\n"); break;
      case DEC2K : printf("DEC2K\n"); break;
      case DEC3K : printf("DEC3K\n"); break;
      case DEC4K : printf("DEC4K\n"); break;
      default : printf("ERROR!!!\n");
      }
    }


    for (i = 0; i < 10; i++) encBit[i] = 0;

    
    if(DPdecMode == DEC4K || DPdecMode == DEC3K)
	/* Modified on 07/04/97 by Y.Maeda */
    {

      if(DPvarMode == BM_VARIABLE)
      {
        if(BsGetBitChar(stream, &tmpIdVUV, 2))
          CommonExit(1,"DecParFrameHvx: error reading bit stream");

	encBit[0] = (tmpIdVUV << 6) & 0xc0;

	/* HP 971111 */
	if (DPdebugLevel >= 2)
	  printf("DecParFrameHvx: tmpIdVUV=%d\n", tmpIdVUV);

	switch(tmpIdVUV)
        {
        case 0:
	  if(BsGetBitChar(stream, &tmpEncBit, 6))
            CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  encBit[0] |= (tmpEncBit & 0x3f);

          for(j = 1; j < 5; j++)
            if(BsGetBitChar(stream, &encBit[j], 8))
	      CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  break;
        case 1:
	  if(BsGetBitChar(stream, &tmpEncBit, 1))
	    CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  encBit[0] |= ((tmpEncBit << 5) & 0x20);
	  if(tmpEncBit & 0x01) {
	    if(BsGetBitChar(stream, &tmpEncBit, 5))
	      CommonExit(1,"DecParFrameHvx: error reading bit stream");
	    encBit[0] |= (tmpEncBit & 0x1f);
	    for(j = 1; j < 3; j++)
	      if(BsGetBitChar(stream, &encBit[j], 8))
	        CommonExit(1,"DecParFrameHvx: error reading bit stream");
	    if(BsGetBitChar(stream, &tmpEncBit, 1))
	      CommonExit(1,"DecParFrameHvx: error reading bit stream");
	    encBit[3] = (tmpEncBit & 0x01) << 7;
	  } else {
	    /*** reading padded bits necessary(AI 990528) ***/
	    if(BsGetBitChar(stream, &tmpEncBit, 5))	
	      CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  }
	  break;
	case 2:
	case 3:
	  if(BsGetBitChar(stream, &tmpEncBit, 6))
	     CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  encBit[0] |= (tmpEncBit & 0x3f);
	  for(j = 1; j < 9; j++)
	    if(BsGetBitChar(stream, &encBit[j], 8))
	      CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  if (DPdecMode == DEC4K) {
	      if(BsGetBitChar(stream, &encBit[9], 8)) /* MN 971106 */
	         CommonExit(1,"DecParFrameHvx: error reading bit stream");
          }
          else {
	    /* MN 971107 */
	    if(BsGetBitChar(stream, &tmpEncBit, 8-6))
	    CommonExit(1,"DecParFrameHvx: error reading bit stream");
	    encBit[9] = (tmpEncBit & 0x3) << 6;
          }
	  break;
        }
      }
      else

      {
        for(j = 0; j < 9; j++)
          if(BsGetBitChar(stream, &encBit[j], 8))
	    CommonExit(1,"DecParFrameHvx: error reading bit stream");
        if (DPdecMode == DEC4K) {
	  if(BsGetBitChar(stream, &encBit[9], 8)) /* MN 971106 */
	    CommonExit(1,"DecParFrameHvx: error reading bit stream");
        }
        else {
	  /* MN 971107 */
	  if(BsGetBitChar(stream, &tmpEncBit, 8-6))
	    CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  if (encBit[0]&0xc0) /* HP20010215 */
	    encBit[9] = (tmpEncBit & 0x3) << 6;
	  else
	    encBit[9] = 0; /* 72 bit if VUV=0 */
        }
      }
    }
    else if(DPdecMode == DEC2K)
    {
      if(DPvarMode == BM_VARIABLE)
      {
        if(BsGetBitChar(stream, &tmpIdVUV, 2))
          CommonExit(1,"DecParFrameHvx: error reading bit stream");

	encBit[0] = (tmpIdVUV << 6) & 0xc0;

	/* HP 971111 */
	if (DPdebugLevel >= 2)
	  printf("DecParFrameHvx: tmpIdVUV=%d\n", tmpIdVUV);

	switch(tmpIdVUV)
        {
        case 0:
	  if(BsGetBitChar(stream, &tmpEncBit, 6))
            CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  encBit[0] |= (tmpEncBit & 0x3f);

          for(j = 1; j < 3; j++)
            if(BsGetBitChar(stream, &encBit[j], 8))
	      CommonExit(1,"DecParFrameHvx: error reading bit stream");

	  if(BsGetBitChar(stream, &tmpEncBit, 4))
            CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  encBit[3] = (tmpEncBit & 0xf) << 4;
	  break;
	case 1:
	  /*** reading padded bits necessary(AI 990528) ***/
	  if(BsGetBitChar(stream, &tmpEncBit, 6))	
	    CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  break;
	case 2:
	case 3:
	  if(BsGetBitChar(stream, &tmpEncBit, 6))
            CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  encBit[0] |= (tmpEncBit & 0x3f);

          for(j = 1; j < 5; j++)
            if(BsGetBitChar(stream, &encBit[j], 8))
	      CommonExit(1,"DecParFrameHvx: error reading bit stream");
	  break;
	}
      }
      else
      {
        for(j = 0; j < 5; j++)
          if(BsGetBitChar(stream, &encBit[j], 8))
	    CommonExit(1,"DecParFrameHvx: error reading bit stream");
      }
    }

    /* HP20010215 */
    if (DPdebugLevel >= 2) {
      printf("DecParFrameHvx: encBit[10]");
      for(j = 0; j < 10; j++)
	printf(" %2x",encBit[j]);
      printf(" \n");
    }

    hvxc_decode(decHvxcHandle,
		encBit,
		sampleBuf,
		frameBufNumSample,
		speedFact,
		pitchFact,
		DPdecMode,
	    epFlag, hEscInstanceData
		);
}


/* DecParFreeIl() */
/* Free memory: individual lines */

static void DecParFreeIl ()
{
  IndiLineDecodeFree(ILD);
  IndiLineSynthFree(ILS);
}


/* DecParFreeHvx() */
/* Free memory: harmonic vector exitation */

static void DecParFreeHvx ()
{
  hvxc_decode_free(decHvxcHandle);
}


/* ---------- functions ---------- */


char *DecParInfo (
  FILE *helpStream)		/* in: print decPara help text to helpStream */
				/*     if helpStream not NULL */
				/* returns: core version string */
{
  if (helpStream != NULL) {
    fprintf(helpStream,
	    PROGVER "\n"
	    "decoder parameter string format:\n"
	    "  list of tokens (tokens separated by characters in \"%s\")\n",
	    SEPACHAR);
    CmdLineHelp(NULL,NULL,switchList,helpStream);
  }
  return PROGVER;
}


/*  _decParInit() */
/* Init parametric decoder core. */

static void _decParInit (
  int numChannel,		/* in: num audio channels */
  float fSample,		/* in: sampling frequency [Hz] */
  float bitRate,		/* in: total bit rate [bit/sec] */
  char *decPara,		/* in: decoder parameter string */
  HANDLE_BSBITBUFFER bitHeader,	/* in: header from bit stream */
  int *frameMaxNumSample,	/* out: max num samples per frame */
  int *delayNumSample,		/* out: decoder delay (num samples) */
  int epFlag,                 /* in: error protection flag */
  int epDebugLevel,             /* in: debug level */
  char* infoFileName,           /* in: crc information file name */
  HANDLE_ESC_INSTANCE_DATA* hEscInstanceData,       /* out: ep-information */
  int enhaFlag,			/* in: enhaFlag or -1 for decPara */
  int numExtLayer		/* in: numExtLayer or -1 for decPara */

  )
{
  int parac;
  char **parav;
  int result;
  HANDLE_BSBITSTREAM hdrStream;
  char *paraMode;
  int i;
  int tmpNumSample;


  /* evaluate decoder parameter string */
  parav = CmdLineParseString(decPara,SEPACHAR,&parac);
  result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);
  if (result) {
    if (result==1) {
      DecParInfo(stdout);
      CommonExit(1,"decoder core aborted ...");
    }
    else
      CommonExit(1,"decoder parameter string error");
  }


  /* HP20001201 for -out */
  if (!DPnumExtLayerUsed)
    DPnumExtLayer = 0;
  if (!DPenhaFlagUsed)
    DPenhaFlag = 0;
  if (enhaFlag>=0)
    DPenhaFlag = enhaFlag;
  if (numExtLayer>=0)
    DPnumExtLayer = numExtLayer;




  if (epFlag && DPmode == MODEPAR_HVX) {
    *hEscInstanceData = CreateEscInstanceData( infoFileName, epDebugLevel );
    /* DPnumClass = BsGetNumClassHvx( *hEscInstanceData ); */
  }


  if (DPdebugLevel >= 1) {
    printf("DecParInit: numChannel=%d  fSample=%f  bitRate=%f\n",
	   numChannel,fSample,bitRate);
    printf("DecParInit: decPara=\"%s\"\n",
	   decPara);
    printf("DecParInit: debugLevel=%d\n",
	   DPdebugLevel);

    if ( epFlag )
        printf("DecParInit: numClass=%d\n",
	   DPnumClass);

  }

  if (numChannel != 1)
    CommonExit(1,"DecParInit: audio data has more the one channel (%d)",
	       numChannel);

  CmdLineParseFree(parav);

  DPfSample = fSample;
  DPbitRate = bitRate;

  /* change random number generator seed / period   HP 990701/000215 */
  random1init(DPrndseed,DPrndperiod);

  hdrStream = BsOpenBufferRead(bitHeader);
  if (BsGetBitInt(hdrStream,(unsigned int*)&DPmode,2))
    CommonExit(1,"DecParInit: error reading bit stream header");

  if (DPmode<0 || DPmode>MODEPAR_NUM)
    CommonExit(1,"DecParInit: unknown parametric codec mode %d",DPmode);
  paraMode = MP4ModeParName[DPmode];

  if (DPdebugLevel >= 1)
    printf("DecParInit: modeInt=%d  mode=\"%s\"\n",DPmode,paraMode);

  tmpBSbuf = BsAllocBuffer(80);
  switch (DPmode) {
  case MODEPAR_HVX:
    DecParInitHvx(hdrStream);
    break;

  case MODEPAR_IL:
    DecParInitIl(hdrStream);
    break;
  case MODEPAR_MIX:
    /* HP20010125   "predict" hvxcFrameBufNumSample */
    DPnbs = 0;
    DPfr0 = 0;
    DPtargetP = 0.0;
  case MODEPAR_SWIT:
    DecParInitHvx(hdrStream);
    HVXdelayNumSample = DPdelayNumSample;
    tmpNumSample = DPframeMaxNumSample;
    DecParInitIl(hdrStream);
    ILdelayNumSample = DPdelayNumSample;
    DPframeMaxNumSample = max(tmpNumSample,DPframeMaxNumSample);
    DPdelayNumSample = max(HVXdelayNumSample,ILdelayNumSample);
    break;

  }
  BsClose(hdrStream);

  *frameMaxNumSample = DPframeMaxNumSample;
  *delayNumSample = DPdelayNumSample;
  mixSampleBufSize = DPframeMaxNumSample+
    max(DPdelayNumSample-ILdelayNumSample,DPdelayNumSample-HVXdelayNumSample);

  if (CommonFreeAlloc((void**)&tmpSampleBuf,
		      *frameMaxNumSample*sizeof(float)) == NULL)
    CommonExit(1,"DecParInit: memory allocation error");
  if (CommonFreeAlloc((void**)&mixSampleBuf,
		      mixSampleBufSize*sizeof(float)) == NULL)
    CommonExit(1,"DecParInit: memory allocation error");
  for (i=0; i<mixSampleBufSize; i++)
    mixSampleBuf[i] = 0;

  if (DPdebugLevel >= 1) {
    printf("DecParInit: encDlyMode=%d  vrMode=%d  rateMode=%d\n",
	   DPdelayMode,DPvarMode,DPrateMode);
    printf("DecParInit: dly=%d  frm=%d  mixBuf=%d  ILdly=%d  HVXdly=%d\n",
	   DPframeMaxNumSample,DPdelayNumSample,
	   mixSampleBufSize,ILdelayNumSample,HVXdelayNumSample);
  }
}

/* HP20010215 */
static void ErHvxcConv (HANDLE_BSBITSTREAM *streamESC,
			HANDLE_BSBITBUFFER tmpBuf,
			int rateMode);

/* DecParFrame() */
/* Decode one bit stream frame into one audio frame with */
/* parametric decoder core. */

static void _decParFrame (
  HANDLE_BSBITBUFFER bitBuf,	/* in: bit stream frame or NULL */
  HANDLE_BSBITSTREAM *stream,   /* in: NULL or bit stream(s) */
  float **sampleBuf,		/* out: audio frame samples */
				/*     sampleBuf[numChannel][frameNumSample] */
  int *usedNumBit,		/* out: num bits used for this frame */
  int *frameBufNumSample,	/* out: num samples in sampleBuf[][] */
  float speedFact,		/* in: speed change factor (AI 990209) */
  float pitchFact,		/* in: pitch change factor (AI 990209) */
  int epFlag,                 /* in: error protection flag */
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData
  )
{

  HANDLE_BSBITSTREAM tmpStream[MAX_LAYER+1+14];
  int i;
  float pcu;


  unsigned int il_hvxc;
  int tmpNumSample;
  int hilnFrameBufNumSample;
  float tmpSpeedFact;
  int hvxcFrameBufNumSample;
  int dcfrm;
  float ratio;


  if (DPdebugLevel > 1) {
    if (bitBuf)
      printf("DecParFrame: availNum=%ld\n",BsBufferNumBit(bitBuf));
    printf("DecParFrame: speedFact=%f  pitchFact=%f\n",
	   speedFact,pitchFact);
  }

  if (bitBuf) {
    stream = &(tmpStream[0]);
    stream[0] = BsOpenBufferRead(bitBuf);
    for (i=1; i<MAX_LAYER+1+14; i++)
      stream[i] = stream[0];
  }

  DPpcuNs = -1;
  pcu = 0.;
  switch (DPmode) {
  case MODEPAR_HVX:
    ErHvxcConv(stream,tmpBSbuf,DPrateMode);
    tmpBSstream = BsOpenBufferRead(tmpBSbuf);
    DecParFrameHvx(tmpBSstream,sampleBuf[0],frameBufNumSample,speedFact,pitchFact, epFlag, hEscInstanceData);
    BsClose(tmpBSstream);
    pcu = 2.;
    break;
  case MODEPAR_IL:
    DecParFrameIl(stream,
		             epConfig,
		             sampleBuf[0],frameBufNumSample,speedFact,pitchFact);
    break;
  case MODEPAR_SWIT:
    BsGetBitInt(stream[0],&il_hvxc,1);
    if (DPdebugLevel >= 1)
      printf("switch: %s\n",(il_hvxc)?"il":"hvxc");
      /* HP 971023   swit mode: 2 HVXC frames / 40 ms */
    if (il_hvxc) {
      DecParFrameHvx((HANDLE_BSBITSTREAM)NULL,tmpSampleBuf,&tmpNumSample,speedFact,pitchFact,
		     epFlag, hEscInstanceData);
      DecParFrameHvx((HANDLE_BSBITSTREAM)NULL,tmpSampleBuf+tmpNumSample,frameBufNumSample,
		     speedFact,pitchFact, epFlag, hEscInstanceData );

      *frameBufNumSample += tmpNumSample;
      for (i=0; i<*frameBufNumSample; i++)
	mixSampleBuf[DPdelayNumSample-HVXdelayNumSample+i] += tmpSampleBuf[i];
      DecParFrameIl((DPtest==1)?(HANDLE_BSBITSTREAM*)NULL:stream,
        epConfig,
		    tmpSampleBuf,&tmpNumSample,speedFact,pitchFact);
      if (DPdebugLevel >= 2)
	printf("hvxcSamples=%d hilnSamples=%d\n",*frameBufNumSample,tmpNumSample);
      *frameBufNumSample = tmpNumSample;
      for (i=0; i<*frameBufNumSample; i++)
	mixSampleBuf[DPdelayNumSample-ILdelayNumSample+i] += tmpSampleBuf[i];
    }
    else {
      ErHvxcConv(stream+(epConfig?5:0),tmpBSbuf,(DPrateMode==DEC2K)?DEC2K:DEC4K);
      tmpBSstream = BsOpenBufferRead(tmpBSbuf);
      DecParFrameHvx((DPtest==2)?(HANDLE_BSBITSTREAM)NULL:tmpBSstream,
		     tmpSampleBuf,&tmpNumSample,speedFact,pitchFact,	
		     epFlag, hEscInstanceData);
      BsClose(tmpBSstream);
      ErHvxcConv(stream+(epConfig?10:0),tmpBSbuf,DPrateMode);
      tmpBSstream = BsOpenBufferRead(tmpBSbuf);
      DecParFrameHvx((DPtest==2)?(HANDLE_BSBITSTREAM)NULL:tmpBSstream,
		     tmpSampleBuf+tmpNumSample,frameBufNumSample,speedFact,pitchFact,
		     epFlag, hEscInstanceData);
      BsClose(tmpBSstream);
      *frameBufNumSample += tmpNumSample;
      for (i=0; i<*frameBufNumSample; i++)
	mixSampleBuf[DPdelayNumSample-HVXdelayNumSample+i] += tmpSampleBuf[i];
       DecParFrameIl((HANDLE_BSBITSTREAM*)NULL,
		    epConfig,
		    tmpSampleBuf,&tmpNumSample,
		    320.0/(*frameBufNumSample) /*speedFact*/ ,pitchFact);
      if (DPdebugLevel >= 2)
	printf("hvxcSamples=%d hilnSamples=%d\n",*frameBufNumSample,tmpNumSample);
      for (i=0; i<tmpNumSample; i++)
	mixSampleBuf[DPdelayNumSample-ILdelayNumSample+i] += tmpSampleBuf[i];
      pcu = 2.;
    }
    for (i=0; i<*frameBufNumSample; i++)
      sampleBuf[0][i] = mixSampleBuf[i];
    for (i=0; i<mixSampleBufSize-*frameBufNumSample; i++)
      mixSampleBuf[i] = mixSampleBuf[i+*frameBufNumSample];
    for (i=0; i<*frameBufNumSample; i++)
      mixSampleBuf[i+mixSampleBufSize-*frameBufNumSample] = 0;
    break;
  case MODEPAR_MIX:
    BsGetBitInt(stream[0],&il_hvxc,2);
    if (DPdebugLevel >= 1)
      switch (il_hvxc) {
      case 0: printf("mix: hvxc\n"); break;
      case 1: printf("mix: hvxc2 + il\n"); break;
      case 2: printf("mix: hvxc4 + il\n"); break;
      case 3: printf("mix: il\n"); break;
      }

    /* HP20010125   "predict" hvxcFrameBufNumSample */
    dcfrm = 0;
    while(DPnbs == DPfr0) {
      ratio = DPtargetP - (float)DPfr0 + 1;
      DPtargetP += speedFact;
      DPfr0 = (int)ceil(DPtargetP);
      dcfrm++;
    }
    DPnbs++;
    hvxcFrameBufNumSample = dcfrm*160;
    dcfrm = 0;
    while(DPnbs == DPfr0) {
      ratio = DPtargetP - (float)DPfr0 + 1;
      DPtargetP += speedFact;
      DPfr0 = (int)ceil(DPtargetP);
      dcfrm++;
    }
    DPnbs++;
    hvxcFrameBufNumSample += dcfrm*160;
    tmpSpeedFact = 320.0/hvxcFrameBufNumSample;
    if (il_hvxc==3)
      tmpSpeedFact = speedFact;
    if (il_hvxc != 0) {
       DecParFrameIl((DPtest==1)?(HANDLE_BSBITSTREAM*)NULL:stream,
		    epConfig,
		    tmpSampleBuf,frameBufNumSample,tmpSpeedFact,pitchFact);
    }
    else {
      DecParFrameIl(NULL,
		    epConfig,
		    tmpSampleBuf,frameBufNumSample,tmpSpeedFact,pitchFact);
    }
    for (i=0; i<*frameBufNumSample; i++)
      mixSampleBuf[DPdelayNumSample-ILdelayNumSample+i] += tmpSampleBuf[i];
    hilnFrameBufNumSample = *frameBufNumSample;

    switch (il_hvxc) {
    case 0:
      ErHvxcConv(stream+(epConfig?5:0),tmpBSbuf,(DPrateMode==DEC2K)?DEC2K:DEC4K);
      tmpBSstream = BsOpenBufferRead(tmpBSbuf);
      DecParFrameHvx((DPtest==2)?(HANDLE_BSBITSTREAM)NULL:tmpBSstream,
		     tmpSampleBuf,&tmpNumSample,speedFact,pitchFact,
		     epFlag, hEscInstanceData);
      BsClose(tmpBSstream);
      ErHvxcConv(stream+(epConfig?10:0),tmpBSbuf,DPrateMode);
      tmpBSstream = BsOpenBufferRead(tmpBSbuf);
      DecParFrameHvx((DPtest==2)?(HANDLE_BSBITSTREAM)NULL:tmpBSstream,
		     tmpSampleBuf+tmpNumSample,frameBufNumSample,speedFact,pitchFact,
		     epFlag, hEscInstanceData);
      BsClose(tmpBSstream);
      *frameBufNumSample += tmpNumSample;
      pcu = 2.;
      break;
    case 1:
      ErHvxcConv(stream+(epConfig?5:0),tmpBSbuf,DEC2K);
      tmpBSstream = BsOpenBufferRead(tmpBSbuf);
      DecParFrameHvx((DPtest==2)?(HANDLE_BSBITSTREAM)NULL:tmpBSstream,
		     tmpSampleBuf,&tmpNumSample,speedFact,pitchFact,
		     epFlag, hEscInstanceData);
      BsClose(tmpBSstream);
      ErHvxcConv(stream+(epConfig?10:0),tmpBSbuf,DEC2K);
      tmpBSstream = BsOpenBufferRead(tmpBSbuf);
      DecParFrameHvx((DPtest==2)?(HANDLE_BSBITSTREAM)NULL:tmpBSstream,
		     tmpSampleBuf+tmpNumSample,frameBufNumSample,speedFact,pitchFact,
		     epFlag, hEscInstanceData);
      BsClose(tmpBSstream);
      *frameBufNumSample += tmpNumSample;
      pcu = 2.;
      break;
    case 2:
      ErHvxcConv(stream+(epConfig?5:0),tmpBSbuf,DEC4K);
      tmpBSstream = BsOpenBufferRead(tmpBSbuf);
      DecParFrameHvx((DPtest==2)?(HANDLE_BSBITSTREAM)NULL:tmpBSstream,
		     tmpSampleBuf,&tmpNumSample,speedFact,pitchFact,
		     epFlag, hEscInstanceData);
      BsClose(tmpBSstream);
      ErHvxcConv(stream+(epConfig?10:0),tmpBSbuf,DEC4K);
      tmpBSstream = BsOpenBufferRead(tmpBSbuf);
      DecParFrameHvx((DPtest==2)?(HANDLE_BSBITSTREAM)NULL:tmpBSstream,
		     tmpSampleBuf+tmpNumSample,frameBufNumSample,speedFact,pitchFact,
		     epFlag, hEscInstanceData);
      BsClose(tmpBSstream);
      *frameBufNumSample += tmpNumSample;
      pcu = 2.;
      break;
    case 3:
      DecParFrameHvx((HANDLE_BSBITSTREAM)NULL,tmpSampleBuf,&tmpNumSample,
		     speedFact,pitchFact,epFlag, hEscInstanceData);
      DecParFrameHvx((HANDLE_BSBITSTREAM)NULL,tmpSampleBuf+tmpNumSample,frameBufNumSample,
		     speedFact,pitchFact, epFlag, hEscInstanceData);
      *frameBufNumSample += tmpNumSample;
      break;
    }
    for (i=0; i<*frameBufNumSample; i++)
      mixSampleBuf[DPdelayNumSample-HVXdelayNumSample+i] += tmpSampleBuf[i];
    if (DPdebugLevel >= 2)
      printf("hvxcSamples=%d hilnSamples=%d\n",*frameBufNumSample,hilnFrameBufNumSample);
    *frameBufNumSample = hilnFrameBufNumSample;

    for (i=0; i<*frameBufNumSample; i++)
      sampleBuf[0][i] = mixSampleBuf[i];
    for (i=0; i<mixSampleBufSize-*frameBufNumSample; i++)
      mixSampleBuf[i] = mixSampleBuf[i+*frameBufNumSample];
    for (i=0; i<*frameBufNumSample; i++)
      mixSampleBuf[i+mixSampleBufSize-*frameBufNumSample] = 0;
    break;

  }
  *usedNumBit = BsCurrentBit(stream[0]);
  if (bitBuf)
    BsCloseRemove(stream[0],1);

  DPpcuCnt++;
  if (DPpcuNs>=0)
    pcu += (1.0+DPpcuNs*0.15)*DPfSample/16000.;
  DPpcuSum += pcu;
  if (DPpcuMax < pcu)
    DPpcuMax = pcu;

  if (DPdebugLevel >= 1)
    printf("DecParFrame: "
	   "bits=%d smpls=%d ns=%d actPCU=%.2f avgPCU=%.2f maxPCU=%.2f\n",
	   *usedNumBit,*frameBufNumSample,
	   DPpcuNs,pcu,DPpcuSum/DPpcuCnt,DPpcuMax);

}


/* DecParFree() */
/* Free memory allocated by parametric decoder core. */

void DecParFree (ParaDecStatus *paraD)
{
  if (DPdebugLevel >= 1)
    printf("DecParFree: ...\n");

  if (!DPfree)
    return;
  if (DPmode != MODEPAR_IL)
    DecParFreeHvx();
  if (DPmode != MODEPAR_HVX)
    DecParFreeIl();
}

/* for ESC support */
static int get_hvxc_mode(int layer, int rateMode)
{
  int mode;

  if (!DPvarMode) {
    /* fixed rate mode */
    if (rateMode==DEC4K) {
      mode=1;	/* 4kbps fixed rate */
    }
    else if (rateMode==DEC3K) {
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
    if (rateMode==DEC4K) {
      mode=4;	/* 4kbps variable rate */
    }
    else {
      if (layer==0) {
	if (rateMode==DEC2K && DPvrScalFlag)
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

/* DecHvxcInit() */
/* Init HVXC decoder core for system interface(AI 99060X) */

HvxcDecStatus *DecHvxcInit (
  char *decPara,		/* in: decoder parameter string */
  FRAME_DATA *fD,		/* in: system interface */
  int layer,			/* in: number of layer */
  int epFlag,                 /* in: error protection flag */
  int epDebugLevel,             /* in: debug level */
  char* infoFileName,           /* in: crc information file name */
  HANDLE_ESC_INSTANCE_DATA* hEscInstanceData       /* out: ep-information */
  )
{
  int i;
  int parac;
  char **parav;
  int result;

  AUDIO_SPECIFIC_CONFIG* audSpC;

  int numChannel = 0;
  float fSample  = 0;
  float bitRate;

  HvxcDecStatus *hvxcD;


  int num, esc;
  int mode;
  static int hvxcNumEsc[8] = {	/* ER_HVXC: number of ESCs */
    4,  /* [mode=0] 2kbps fixed rate */
    5,  /* [mode=1] 4kbps fixed rate */
    3,  /* [mode=2] (2+2)kbps fixed rate ehnacecement layer */
    4,  /* [mode=3] 2kbps variable rate */
    5,  /* [mode=4] 4kbps variable rate */
    4,  /* [mode=5] (2+2)kbps variable rate base layer */
    3,  /* [mode=6] (2+2)kbps variable rate enhancement layer */
    5   /* [mode=7] 3.7kbps fixed rate */
  };


  /*  mp4ffFlag = 1;	 use system interface(flexmux) */

  audSpC = &fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig;
  
  DPvarMode = audSpC->specConf.hvxcSpecificConfig.HVXCvarMode.value;
  DPrateMode = audSpC->specConf.hvxcSpecificConfig.HVXCrateMode.value;
  DPextensionFlag = audSpC->specConf.hvxcSpecificConfig.extensionFlag.value;
  if (DPextensionFlag) {
    /* bit stream header for version 2 (AI 990616) */
    DPvrScalFlag = audSpC->specConf.hvxcSpecificConfig.vrScalFlag.value;
  }
  epConfig = audSpC->epConfig.value;

  /* _sps-note_2006-04-03_: With respect to the subsequent code, epConfig=0 and epConfig=2 are treated equal */
  if ( epConfig == 2 ) epConfig=0;

  /* _sps-note_2006-04-03_: epConfig == 3 is currently not relevant for HVXC
     most likely somethin like
     if ( epConfig == 3 ) epConfig = 1;
     should work 
  */ 

  if ( epConfig < 0 || epConfig >= 3 ) CommonExit(1,"epConfig %d not supported",epConfig); 

  if (DPextensionFlag && epConfig ) {	/* ER_HVXC(epConfig=1) */
    for (i=0; i<2; i++) {
      mode=get_hvxc_mode(i,DPrateMode);
      numEsc[i]=hvxcNumEsc[mode];
    }
  }
  else {	/* HVXC,ER_HVXC(epConfig=0) */
    for (i=0; i<2; i++) {
      numEsc[i]=1;
    }
  }
  numLayer=fD->scalOutSelect+1;
  
#ifndef CORRIGENDUM1
  if (numLayer > 2) {
    CommonExit(1,"wrong number of layer, maximum 2(base + 1 enhancement)");
  }
#else
  /* HP20010214 */
  if (fD->scalOutSelect) {
    if (fD->od->ESDescriptor[fD->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.specConf.hvxcSpecificConfig.isBaseLayer.value)
      fD->scalOutSelect = 0;
    else
      fD->scalOutSelect = 1;
    numLayer=fD->scalOutSelect+1;
  }
#endif
  outputLayerNumber = fD->scalOutSelect;
  if (outputLayerNumber >= numLayer) {
    CommonExit(1,"wrong fD->scalOutSelect");	
  }
  
  /* evalute decoder parameter string */
  parav = CmdLineParseString(decPara,SEPACHAR,&parac);
  result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);
  if (result) {
    if (result==1) {
      DecParInfo(stdout);
      exit (1);
    }
    else
      CommonExit(1,"decoder parameter string error");
  }

  /* change random number generator seed / period   HP 990701/000215 */
  random1init(DPrndseed,DPrndperiod);

  switch (audSpC->channelConfiguration.value) {
  case 1 :numChannel=1;       
    break;
  default: CommonExit(1,"wrong channel config");
  }
  
  bitRate = 0.0;
  num=0;
  for (i=0; i<numLayer; i++) {
    for (esc=0; esc<numEsc[i]; esc++, num++) {
      bitRate += fD->od->ESDescriptor[num]->DecConfigDescr.avgBitrate.value;
    }
  }
  /* modify streamCount value(number of tracks to parse actually) */
  if (DPdebugLevel>=3)
    if (fD->od->streamCount.value!=(UINT32)num) 
      printf("fD->od->streamCount.value modified: %ld -> %d\n", fD->od->streamCount.value, num);
  
  fD->od->streamCount.value = (UINT32)num;
  
  if (fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value != 11) {
    CommonExit(1,"illegal sampling frequency config");
  }

  else {
    fSample = 8000.0;
  }

  if (DPdebugLevel >= 1) {
    printf("DecHvxcInitNew: numChannel=%d  fSample=%f  bitRate=%f\n",
	   numChannel,fSample,bitRate);
    printf("DecHvxcInitNew: decPara=\"%s\"\n",
	   decPara);
    printf("DecHvxcInitNew: debugLevel=%d\n",
	   DPdebugLevel);
    printf("DecHvxcInitNew: numLayer=%d, outputLayerNumber=%d\n",
	   numLayer, outputLayerNumber);

    if (DPtestMode != TM_NORMAL) {
      printf("DecHvxcInit: test_mode=%x(for HVXC decoder conformance testing)\n", DPtestMode);
      if (DPtestMode & TM_POSFIL_DISABLE) {
	printf("  - postfilter and post processing are skipped.\n");
      }
      if (DPtestMode & TM_INIT_PHASE_ZERO) {
	printf("  - initial values of harmonic phase are reset to zeros.\n");
      }
      if (DPtestMode & TM_NOISE_ADD_DISABLE) {
	printf("  - noise component addition are disabled.\n");
      }
      if (DPtestMode & TM_VXC_DISABLE) {
	printf("  - the output of Time Domain Decoder is disabled.\n");
      }
    }
  }

  if (numChannel != 1)
    CommonExit(1,"DecHvxcInit: audio data has more the one channel (%d)",
	       numChannel);

  CmdLineParseFree(parav);

  DPfSample = fSample;
  DPbitRate = bitRate;

  DPvarMode = audSpC->specConf.hvxcSpecificConfig.HVXCvarMode.value;
  DPrateMode = audSpC->specConf.hvxcSpecificConfig.HVXCrateMode.value;
  DPextensionFlag = audSpC->specConf.hvxcSpecificConfig.extensionFlag.value;
  if (DPextensionFlag) {
    /* bit stream header for version 2 (AI 990616) */
    DPvrScalFlag = audSpC->specConf.hvxcSpecificConfig.vrScalFlag.value;

    if (DPvrScalFlag || outputLayerNumber>=1)  /* AI 2001/01/31 */
      DPscalFlag = SF_ON;
  }

  DPframeMaxNumSample = 160 * (int) ceil(1.0 / MINSPEEDFACT);
  
  if (DPdelayMode == DM_LONG)	/* HP 971023 */
    DPdelayNumSample = 80;	/* 10 ms */
  else
    DPdelayNumSample = 60;	/* 7.5 ms */

  if ( epFlag ) {
    *hEscInstanceData = CreateEscInstanceData( infoFileName, epDebugLevel );
    /* DPnumClass = BsGetNumClassHvx( *hEscInstanceData ); */
    if (DPrateMode == DEC2K) DPnumClass = N_CLS_2K;
    else DPnumClass = N_CLS_4K;
  }

  hvxcD = hvxc_decode_init(DPvarMode,
			   DPrateMode,
			   DPextensionFlag,
			   /* version 2 data (YM 990616) */
			   DPnumClass,
			   /* version 2 data (YM 990728) */
			   DPscalFlag,  /* AI 2001/01/31 */
			   DPdelayMode,
			   DPtestMode);

  hvxcD->frameNumSample = DPframeMaxNumSample;
  hvxcD->delayNumSample = DPdelayNumSample;
  hvxcD->speedFact =      speedFactCl;
  hvxcD->pitchFact =      pitchFactCl;

  if (DPdebugLevel >= 1) {
    printf("DecHvxcInitNew: decDlyMode=%d  vrMode=%d  rateMode=%d\n",
	   DPdelayMode,DPvarMode,DPrateMode);
  }
  
  if ((hvxcD->sampleBuf=(float**)malloc(numChannel*sizeof(float*)))==NULL)
    CommonExit(1,"DecHvxcInitNew: memory allocation error");
  for (i=0; i<numChannel; i++)
    if ((hvxcD->sampleBuf[i]=(float*)malloc(DPframeMaxNumSample*sizeof(float)))==NULL)
      CommonExit(1,"DecHvxcInitNew: memory allocation error");

  return(hvxcD);
}

/* for ESC support */
/* number of bits for each ESC(bps) */
static const int numBitsEsc[8][4][5] = {	
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


/* HP20010215 */
/* merge ER_HVXC from multiple streams (epConfig=1) to buffer */
static void ErHvxcConv (HANDLE_BSBITSTREAM *streamESC,
			HANDLE_BSBITBUFFER tmpBuf,
			int rateMode)
{
  HANDLE_BSBITSTREAM tmpStream;
  HANDLE_BSBITSTREAM stream;
  unsigned long bit;
  unsigned long VUV, UpdateFlag;
  int num=0;
  int esc, layer;
  int mode;
  int numBits;
  static int hvxcNumEsc[8] = {	/* ER_HVXC: number of ESCs */
    4,  /* [mode=0] 2kbps fixed rate */
    5,  /* [mode=1] 4kbps fixed rate */
    3,  /* [mode=2] (2+2)kbps fixed rate ehnacecement layer */
    4,  /* [mode=3] 2kbps variable rate */
    5,  /* [mode=4] 4kbps variable rate */
    4,  /* [mode=5] (2+2)kbps variable rate base layer */
    3,  /* [mode=6] (2+2)kbps variable rate enhancement layer */
    5   /* [mode=7] 3.7kbps fixed rate */
  };

  if (!epConfig) {
    switch (rateMode) {
    case DEC0K : num =  0; break;
    case DEC2K : num = 40; break;
    case DEC3K : num = 74; break; /* only 72 valid if VUV=0 */
    case DEC4K : num = 80; break;
    }
    if (BsGetBuffer(streamESC[0],tmpBuf,num))
      CommonWarning("ErHvxcConv: not enough bits");
  }
  else if (rateMode != DEC0K) {
    tmpStream = BsOpenBufferWrite(tmpBuf);
    num=0;
    VUV=0;
    UpdateFlag=0;
    for (layer=0; layer<=0 /*outputLayerNumber*/; layer++) {

      mode = get_hvxc_mode(layer,rateMode);
      numEsc[layer] = hvxcNumEsc[mode];

      if (mode!=4 && mode!=5) {
	for (esc=0; esc<numEsc[layer]; esc++, num++) {
	  stream = streamESC[num];
	  
	  numBits=0;
	  if (layer==0 && esc==0) {
	    /* read/write 2bits for VUV */
	    BsGetBit(stream,&bit,2);
	    BsPutBit(tmpStream,bit,2);
	    numBits -= 2;
	    VUV=bit;
	    if (DPdebugLevel>=3)
	      printf("\tVUV=%ld(2bits are already read)\n", VUV);
	  }

	  numBits += numBitsEsc[mode][VUV][esc];
	  if (DPdebugLevel>=3)
	    printf("layer=%d,esc%d(num=%d,mode=%d,VUV=%2ld): %d bits\n", layer, esc, num, mode, VUV, numBits);
	  while(numBits>0) {
	    /* read/write 32bits at maximum */
	    BsGetBit(stream,&bit,min(32,numBits));
	    BsPutBit(tmpStream,bit,min(32,numBits));
	    numBits -= 32;
	  }
	}
	if (mode==7 && layer==0 && VUV==0)
	  BsPutBit(tmpStream,0,2); /* pad from 72 to 74 bits */
      }
      else {	/* mode=4,5(UpdateFlag is necessary to parse bitstream) */
	for (esc=0; esc<numEsc[layer]; esc++, num++) {
	  stream = streamESC[num];

	  numBits=0;
	  if (layer==0 && esc==0) {
	    /* read/write 2bits for VUV */
	    BsGetBit(stream,&bit,2);
	    BsPutBit(tmpStream,bit,2);
	    numBits -= 2;
	    VUV=bit;
	    if (DPdebugLevel>=3)
	      printf("\tVUV=%ld(2bits are already read)\n", VUV);
	    if (VUV==1) {
	      /* read/write 1bit for UpdateFlag */
	      BsGetBit(stream,&bit,1);
	      BsPutBit(tmpStream,bit,1);
	      numBits -= 1;
	      UpdateFlag=bit;
	      if (DPdebugLevel>=3)
		printf("\tUpdateFlag=%ld(1bit are already read)\n", UpdateFlag);
	    }
	  }
	  
	  if (VUV==3) {
	    numBits += numBitsEsc[mode][2][esc];
	  }
	  else if (VUV==1 && UpdateFlag==1) {	/* UpdateFlag is a bit in esc0 */
	    numBits += numBitsEsc[mode][3][esc];
	  }
	  else {
	    numBits += numBitsEsc[mode][VUV][esc];
	  }
	  if (DPdebugLevel>=3)
	    printf("layer=%d,esc%d(num=%d,mode=%d,VUV=%2ld): %d bits\n", layer, esc, num, mode, VUV, numBits);
	  while(numBits>0) {
	    /* read/write 32bits at maximum */
	    BsGetBit(stream,&bit,min(32,numBits));
	    BsPutBit(tmpStream,bit,min(32,numBits));
	    numBits -= 32;
	  }
	}
      }
    }

    BsClose(tmpStream);
  }
  else
    BsClearBuffer(tmpBuf);
}


/* 
  DecHvxcFrame() 
*/
void DecHvxcFrame(
  FRAME_DATA*  fD,
  HvxcDecStatus* hvxcD,		/* in: HVXC decoder status handle */
  int *usedNumBit,	        /* out: num bits used for this frame         */
  int epFlag,                 /* in: error protection flag */
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData
  )
{
  int i, j;
  HANDLE_BSBITSTREAM stream;
  int num, mode, esc, layer;
  int numBits;
  HANDLE_BSBITSTREAM tmpStream;
  HANDLE_BSBITBUFFER tmpBitBuffer;
  unsigned long bit;
  unsigned long VUV, UpdateFlag;
  HANDLE_BSBITSTREAM bitStreamLay[4+3];	/* multi bitstream layer (AI/HP 20010209) */

  int tempNumBit, hvxc_sys_align;
  
  unsigned char tmpIdVUV;
  unsigned char tmpEncBit;

  unsigned char encBit[10];
  
  if (hvxcD->speedFact<MINSPEEDFACT) {
    hvxcD->speedFact = MINSPEEDFACT;
    CommonWarning("DecHvxcFrame: speedFact below %f",MINSPEEDFACT);
  }

  if (DPextensionFlag && epConfig) {	/* ER_HVXC(epConfig=1) */

    tmpBitBuffer = BsAllocBuffer(80);

    num=0;
    for (layer=0; layer<=outputLayerNumber; layer++) {
      for (esc=0; esc<numEsc[layer]; esc++, num++) {
	bitStreamLay[num] = BsOpenBufferRead(fD->layer[num].bitBuf); 
      }
    }

    /* firstly, write bits for every ESC to temporary bit buffer */
    tmpStream = BsOpenBufferWrite(tmpBitBuffer);

    num=0;
    VUV=0;
    UpdateFlag=0;
    for (layer=0; layer<=outputLayerNumber; layer++) {

      mode = get_hvxc_mode(layer,DPrateMode);

      if (mode!=4 && mode!=5) {
	for (esc=0; esc<numEsc[layer]; esc++, num++) {
	  stream=bitStreamLay[num];
	  
	  numBits=0;
	  if (layer==0 && esc==0) {
	    /* read/write 2bits for VUV */
	    BsGetBit(stream,&bit,2);
	    BsPutBit(tmpStream,bit,2);
	    numBits -= 2;
	    VUV=bit;
	    if (DPdebugLevel>=3)
	      printf("\tVUV=%ld(2bits are already read)\n", VUV);
	  }

	  numBits += numBitsEsc[mode][VUV][esc];
	  if (DPdebugLevel>=3)
	    printf("layer=%d,esc%d(num=%d,mode=%d,VUV=%2ld): %d bits\n", layer, esc, num, mode, VUV, numBits);
	  while(numBits>0) {
	    /* read/write 32bits at maximum */
	    BsGetBit(stream,&bit,min(32,numBits));
	    BsPutBit(tmpStream,bit,min(32,numBits));
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
	    BsGetBit(stream,&bit,2);
	    BsPutBit(tmpStream,bit,2);
	    numBits -= 2;
	    VUV=bit;
	    if (DPdebugLevel>=3)
	      printf("\tVUV=%ld(2bits are already read)\n", VUV);
	    if (VUV==1) {
	      /* read/write 1bit for UpdateFlag */
	      BsGetBit(stream,&bit,1);
	      BsPutBit(tmpStream,bit,1);
	      numBits -= 1;
	      UpdateFlag=bit;
	      if (DPdebugLevel>=3)
		printf("\tUpdateFlag=%ld(1bit are already read)\n", UpdateFlag);
	    }
	  }
	  
	  if (VUV==3) {
	    numBits += numBitsEsc[mode][2][esc];
	  }
	  else if (VUV==1 && UpdateFlag==1) {	/* UpdateFlag is a bit in esc0 */
	    numBits += numBitsEsc[mode][3][esc];
	  }
	  else {
	    numBits += numBitsEsc[mode][VUV][esc];
	  }
	  if (DPdebugLevel>=3)
	    printf("layer=%d,esc%d(num=%d,mode=%d,VUV=%2ld): %d bits\n", layer, esc, num, mode, VUV, numBits);
	  while(numBits>0) {
	    /* read/write 32bits at maximum */
	    BsGetBit(stream,&bit,min(32,numBits));
	    BsPutBit(tmpStream,bit,min(32,numBits));
	    numBits -= 32;
	  }
	}
      }
    }
    /* fill bits in tmpBitBuffer up to 80bits(10bytes) */
    numBits=80-BsCurrentBit(tmpStream);
    while(numBits>0) {
      BsPutBit(tmpStream,0,min(32,numBits));
      numBits -= 32;
    }
    BsClose(tmpStream);

    /* secondly, write temporary bit buffer to encBit[] array */
    tmpStream = BsOpenBufferRead(tmpBitBuffer);
    for (i=0; i<10; i++) {
      if(BsGetBit(tmpStream, &bit, 8))
	CommonExit(1,"DecHvxcFrame: error reading bit stream");
      encBit[i] = bit;
    }
    BsClose(tmpStream);

    BsFreeBuffer(tmpBitBuffer);	/* free temporary bit buffer */

  }	/* if (DPextensionFlag && epConfig) */
  else {	/* HVXC,ER_HVXC(epConfig=0) */

  for ( i = 0; i <= outputLayerNumber; i++ ) {
    bitStreamLay[i] = BsOpenBufferRead(fD->layer[i].bitBuf) ; 
  }

  stream = bitStreamLay[0];	/* base layer (AI 990903) */

  if (DPdebugLevel >= 2) {
    printf("DecHvxcFrame: stream=%s\n",stream?"valid":"NULL");
    printf("DecHvxcFrame: HVXCvarMode=%d\n", DPvarMode);
    printf("DecHvxcFrame: HVXCrateMode=");
    switch (DPrateMode) {
    case DEC0K : printf("DEC0K\n"); break;
    case DEC2K : printf("DEC2K\n"); break;
    case DEC3K : printf("DEC3K\n"); break;
    case DEC4K : printf("DEC4K\n"); break;
    default : printf("ERROR!!!\n");
    }
  }

  for (i = 0; i < 5; i++) encBit[i] = 0;

  if (DPrateMode == DEC4K || 
      DPrateMode == DEC3K || 
     (DPrateMode == DEC2K && DPvrScalFlag) ||
     (DPrateMode == DEC2K && outputLayerNumber == 1)) 
  {
    if(DPvarMode == BM_VARIABLE)
    {
      if(BsGetBitChar(stream, &tmpIdVUV, 2))
        CommonExit(1,"DecHvxcFrame: error reading bit stream");

      encBit[0] = (tmpIdVUV << 6) & 0xc0;

      /* HP 971111 */
      if (DPdebugLevel >= 2)
        printf("DecHvxcFrame: tmpIdVUV=%d\n", tmpIdVUV);

      switch(tmpIdVUV)
      {
      case 0:
        if(BsGetBitChar(stream, &tmpEncBit, 6))
          CommonExit(1,"DecHvxcFrame: error reading bit stream");
        encBit[0] |= (tmpEncBit & 0x3f);

        for(j = 1; j < 5; j++)
          if(BsGetBitChar(stream, &encBit[j], 8))
            CommonExit(1,"DecHvxcFrame: error reading bit stream");
	break;
      case 1:
        if(BsGetBitChar(stream, &tmpEncBit, 1))
          CommonExit(1,"DecHvxcFrame: error reading bit stream");
	encBit[0] |= ((tmpEncBit << 5) & 0x20);
	if(tmpEncBit & 0x01) {
	  if(BsGetBitChar(stream, &tmpEncBit, 5))
	    CommonExit(1,"DecHvxcFrame: error reading bit stream");
	  encBit[0] |= (tmpEncBit & 0x1f);
	  for(j = 1; j < 3; j++)
	    if(BsGetBitChar(stream, &encBit[j], 8))
	      CommonExit(1,"DecHvxcFrame: error reading bit stream");
	  if(BsGetBitChar(stream, &tmpEncBit, 1))
	    CommonExit(1,"DecHvxcFrame: error reading bit stream");
	  encBit[3] = (tmpEncBit & 0x01) << 7;
	} else {
	  /*** reading padded bits necessary(AI 990528) ***/
	  if(BsGetBitChar(stream, &tmpEncBit, 5))	
	    CommonExit(1,"DecHvxcFrame: error reading bit stream");
	}
	break;
      case 2:
      case 3:
        if(BsGetBitChar(stream, &tmpEncBit, 6))
          CommonExit(1,"DecHvxcFrame: error reading bit stream");
	encBit[0] |= (tmpEncBit & 0x3f);
	for(j = 1; j < 5; j++)
	  if(BsGetBitChar(stream, &encBit[j], 8))
	    CommonExit(1,"DecHvxcFrame: error reading bit stream");

	/* if there exists enhancement layer, parse bitStreamLay[1] (990903) */
	if (outputLayerNumber == 1)    
	  stream = bitStreamLay[1];	

	if ((DPrateMode == DEC4K) || (outputLayerNumber == 1)) {
	  /* parse the rest of 4kbps base layer(numLayer=1)
	     or enhancement layer(outputLayerNumber=1) (AI 990903) */
	  for(j = 5; j < 9; j++)
	    if(BsGetBitChar(stream, &encBit[j], 8))
	      CommonExit(1,"DecHvxcFrame: error reading bit stream");

	  if (DPrateMode == DEC3K) {
	    if(BsGetBitChar(stream, &tmpEncBit, 8-6))	/* MN 971107 */
	      CommonExit(1,"DecHvxcFrame: error reading bit stream");
	    encBit[9] = (tmpEncBit & 0x3) << 6;
	  }
	  else {
	    if(BsGetBitChar(stream, &encBit[9], 8)) /* MN 971106 */
	      CommonExit(1,"DecHvxcFrame: error reading bit stream");
	  }
	}
	break;
      }
    }
    else

    {
      for(j = 0; j < 5; j++)
        if(BsGetBitChar(stream, &encBit[j], 8))
	  CommonExit(1,"DecHvxcFrame: error reading bit stream");

      /* if there exists enhancement layer, parse bitStreamLay[1] (990903) */
      if (outputLayerNumber == 1)	
	stream = bitStreamLay[1];	

      if ((DPrateMode == DEC4K) || (DPrateMode == DEC3K) || (outputLayerNumber == 1)) {
	/* parse the rest of 4kbps base layer(numLayer=1)
	   or enhancement layer(outputLayerNumber=1) (AI 990903) */
	for(j = 5; j < 9; j++)
	  if(BsGetBitChar(stream, &encBit[j], 8))
	    CommonExit(1,"DecHvxcFrame: error reading bit stream");

	  if (DPrateMode == DEC3K) {
	    if(BsGetBitChar(stream, &tmpEncBit, 8-6))	/* MN 971107 */
	      CommonExit(1,"DecHvxcFrame: error reading bit stream");
	    encBit[9] = (tmpEncBit & 0x3) << 6;
	  }
	  else {
	    if(BsGetBitChar(stream, &encBit[9], 8)) /* MN 971106 */
	      CommonExit(1,"DecHvxcFrame: error reading bit stream");
	  }
      }
    }
  }
  else if(DPrateMode == DEC2K)
  {
    if(DPvarMode == BM_VARIABLE)
    {
      if(BsGetBitChar(stream, &tmpIdVUV, 2))
	CommonExit(1,"DecHvxcFrame: error reading bit stream");

      encBit[0] = (tmpIdVUV << 6) & 0xc0;

      /* HP 971111 */
      if (DPdebugLevel >= 2)
	printf("DecHvxcFrame: tmpIdVUV=%d\n", tmpIdVUV);

      switch(tmpIdVUV)
      {
      case 0:
	if(BsGetBitChar(stream, &tmpEncBit, 6))
	  CommonExit(1,"DecHvxcFrame: error reading bit stream");
	encBit[0] |= (tmpEncBit & 0x3f);

	for(j = 1; j < 3; j++)
	  if(BsGetBitChar(stream, &encBit[j], 8))
	    CommonExit(1,"DecHvxcFrame: error reading bit stream");

	if(BsGetBitChar(stream, &tmpEncBit, 4))
	  CommonExit(1,"DecHvxcFrame: error reading bit stream");
	encBit[3] = (tmpEncBit & 0xf) << 4;
	break;
      case 1:
	/*** reading padded bits necessary(AI 990528) ***/
	if(BsGetBitChar(stream, &tmpEncBit, 6))	
	  CommonExit(1,"DecHvxcFrame: error reading bit stream");
	break;
      case 2:
      case 3:
	if(BsGetBitChar(stream, &tmpEncBit, 6))
	  CommonExit(1,"DecHvxcFrame: error reading bit stream");
	encBit[0] |= (tmpEncBit & 0x3f);

	for(j = 1; j < 5; j++)
	  if(BsGetBitChar(stream, &encBit[j], 8))
	    CommonExit(1,"DecHvxcFrame: error reading bit stream");
	break;
      }
    }
    else	/* DPvarMode == BM_CONSTANT */
    {
      for(j = 0; j < 5; j++)
	if(BsGetBitChar(stream, &encBit[j], 8))
	  CommonExit(1,"DecHvxcFrame: error reading bit stream");
    }
  }

  }	/* if (DPextensionFlag && epConfig) */

      
  if (DPdebugLevel >= 3) {
    printf("DecHvxcFrame: DPbitRate=%.2f\n", DPbitRate);
    printf("DecHvxcFrame: DPrateMode=%d\n", DPrateMode);
    printf("DecHvxcFrame: outputLayerNumber=%d\n", outputLayerNumber);
  }

  if (DPrateMode == DEC2K && outputLayerNumber == 1) {
    /* 4kbps decode(base + enhancement layer) in scalable mode */
    DPdecMode = DEC4K;
  }
  else {
    DPdecMode = DPrateMode;
  }

  hvxc_decode(hvxcD, 
	      encBit, 
	      hvxcD->sampleBuf[0], 
	      &hvxcD->frameNumSample,
	      hvxcD->speedFact,
	      hvxcD->pitchFact,
	      DPdecMode,	/* DPrateMode : scalability (YM 990730) */
	      epFlag, 
	      hEscInstanceData
	      );

  *usedNumBit = 0;
  num = 0;
  for (layer=0; layer<=outputLayerNumber; layer++) {
    for (esc=0; esc<numEsc[layer]; esc++, num++) {
      /* byte alignment */
      tempNumBit = BsCurrentBit(bitStreamLay[num]);
      hvxc_sys_align = 8 - (tempNumBit % 8);
      if (hvxc_sys_align == 8) hvxc_sys_align = 0;
      BsGetSkip(bitStreamLay[num],hvxc_sys_align);
      
      tempNumBit = BsCurrentBit(bitStreamLay[num]);
      *usedNumBit += tempNumBit;
      BsCloseRemove(bitStreamLay[num],1); 
    
      fD->layer[num].NoAUInBuffer--;
    }
  }
  if (DPdebugLevel >= 2)
    printf("DecHvxcFrame: numBit=%d  numSample=%d\n",
	   *usedNumBit,hvxcD->frameNumSample);
}



/* DecParFree() */
/* Free memory allocated by parametric decoder core. */

void DecHvxcFree(
HvxcDecStatus *hvxcD		/* in: HVXC decoder status handle */
) 
{
  if (DPdebugLevel >= 1)
    printf("DecHvxcFree: ...\n");

  free( hvxcD->sampleBuf[0] );
  free( hvxcD->sampleBuf );
  hvxc_decode_free(hvxcD);
}




/* DecParInitNew() */
/* Init parametric decoder core for system interface   HP 20000330 */

ParaDecStatus *DecParInit(
  char *decPara,		/* in: decoder parameter string */
  FRAME_DATA *fD,		/* in: system interface */
  int layer,			/* in: number of layer */
  int epFlag,			/* in: error protection flag */
  int epDebugLevel,		/* in: debug level */
  char* infoFileName,		/* in: crc information file name */
  HANDLE_ESC_INSTANCE_DATA* hEscInstanceData 	/* out: ep-information */
  )
{
  ParaDecStatus *paraD;

  AUDIO_SPECIFIC_CONFIG* audSpC;

  HANDLE_BSBITSTREAM tmpStream;
  HANDLE_BSBITBUFFER bitHeader;

  int numExtLayer,enhaFlag;
  int i;
  int num,lay,esc;
  int numChannel = 0;
  float fSample;
  float bitRate;

  
  audSpC = &fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig;
  epConfig = audSpC->epConfig.value;

  if (epConfig) {
    switch (audSpC->specConf.paraSpecificConfig.PARAmode.value) {
    case 0:
    case 1:
      numEscPA[0] = 5;
      break;
    case 2:
    case 3:
      numEscPA[0] = 15;
      break;
    }
  }
  else
    numEscPA[0] = 1;
  for (i=1; i<MAX_LAYER+1; i++)
    numEscPA[i] = 1;

  numLayer = fD->od->streamCount.value-numEscPA[0]+1;
  if (numLayer > MAX_LAYER+1)
    CommonExit(1,"DecParInitNew: wrong number of layer, maximum 9 (base+enha+7ext)");
  if (fD->scalOutSelect==fD->od->streamCount.value-1)
    /* default value found, limit to numLayer-1 */
    fD->scalOutSelect = numLayer-1;

  outputLayerNumber = fD->scalOutSelect;
  if (outputLayerNumber >= numLayer)
    CommonExit(1,"DecParInitNew: wrong fD->scalOutSelect");	

  /* modify streamCount value (number of tracks to parse actually) */
  num = fD->scalOutSelect+numEscPA[0];
  if (DPdebugLevel>=1)
    if (fD->od->streamCount.value!=(UINT32)num) 
      printf("fD->od->streamCount.value modified: %ld -> %d\n",
	     fD->od->streamCount.value, num);
  fD->od->streamCount.value = (UINT32)num;
  
  switch (audSpC->channelConfiguration.value) {
  case 1:
    numChannel = 1;       
    break;
  default:
    CommonExit(1,"DecParInitNew: wrong channel config");
  }
  
  bitRate = 0.0;
  num = 0;
  for (lay=0; lay<=outputLayerNumber; lay++)
    for (esc=0; esc<numEscPA[lay]; esc++, num++) {
      bitRate += fD->od->ESDescriptor[num]->DecConfigDescr.avgBitrate.value;
  }
  /* doesn't make sense, but anyway nobody cares ... */

  if (fD->od->ESDescriptor[fD->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value == 0xf)
    fSample = fD->od->ESDescriptor[fD->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.samplingFrequency.value;
  else
    fSample = samplFreqIndex[fD->od->ESDescriptor[fD->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value];
  if (fSample<0)
    CommonExit(1,"DecParInitNew: illegal sampling frequency config");
  
  if ((paraD = (ParaDecStatus*)malloc(sizeof(ParaDecStatus)))==NULL)
    printf("DecParInitNew: memory allocation error");
  
  bitHeader = BsAllocBuffer(8192);
  tmpStream = BsOpenBufferWrite(bitHeader);
  num = 0;
  for (lay=0; lay<=outputLayerNumber; lay++)
    for (esc=0; esc<numEscPA[lay]; esc++, num++) {
      /* ParametricSpecificConfig */
      if  (num == 0) {
	/* base layer */
#ifdef CORRIGENDUM1
	if (fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.isBaseLayer.value != 1)	/* isBaseLayer */
	  CommonExit(1,"DecParInitNew: paraSpecificConfig error isBaseLayer!=1");
#endif
	BsPutBit(tmpStream,fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.PARAmode.value,2);
	if (fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.PARAmode.value != 1) {
	  BsPutBit(tmpStream,fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HVXCvarMode.value,1);
	  BsPutBit(tmpStream,fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HVXCrateMode.value,2);
	  BsPutBit(tmpStream,fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.extensionFlag.value,1);
	  if (fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.extensionFlag.value)
	    BsPutBit(tmpStream,fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.vrScalFlag.value,1);
	}
	if (fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.PARAmode.value != 0) {
	  BsPutBit(tmpStream,fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNquantMode.value,1);
	  BsPutBit(tmpStream,fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNmaxNumLine.value,8);
	  BsPutBit(tmpStream,fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNsampleRateCode.value,4);
	  BsPutBit(tmpStream,fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNframeLength.value,12);
	  BsPutBit(tmpStream,fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNcontMode.value,2);
	}
	if (fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.PARAextensionFlag.value != 0)
	  CommonExit(1,"DecParInitNew: paraSpecificConfig error PARAextensionFlag!=0");
      }
      else if (lay != 0) {
#ifdef CORRIGENDUM1
	if (fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.isBaseLayer.value != 0)	/* isBaseLayer */
	  CommonExit(1,"DecParInitNew: paraSpecificConfig error isBaseLayer!=0");
#endif
	/* enhancement/extension layer */
	if (fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNenhaLayer.value == 1) {
	  /* enhancement layer */
	  if (lay != 1)
	    CommonExit(1,"DecParInitNew: paraSpecificConfig error HILNenhaLayer=1 for lay!=1");
	  BsPutBit(tmpStream,1,1); /* fake enhaFlag=1 for IndiLineDecodeInit() */
	  BsPutBit(tmpStream,fD->od->ESDescriptor[num]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNenhaQuantMode.value,2);
	}
      }
    }
  if ( outputLayerNumber==0 ||
     ( outputLayerNumber>0 && fD->od->ESDescriptor[numEscPA[0]]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNenhaLayer.value!=1 ) ) {
    BsPutBit(tmpStream,0,1); /* fake enhaFlag=0 for IndiLineDecodeInit() */
  }
  BsClose(tmpStream);

  numExtLayer = outputLayerNumber;
  if (outputLayerNumber>=1 && fD->od->ESDescriptor[numEscPA[0]]->DecConfigDescr.audioSpecificConfig.specConf.paraSpecificConfig.HILNenhaLayer.value==1) {
    enhaFlag = 1;
    numExtLayer--;
  }
  else
    enhaFlag = 0;
  
  
  _decParInit(
	     numChannel,
	     fSample,
	     bitRate,
	     decPara,
	     bitHeader,
	     &(paraD->frameMaxNumSample),
	     &(paraD->delayNumSample),
	     epFlag,
	     epDebugLevel,
	     infoFileName,
	     hEscInstanceData,
	     enhaFlag,
	     numExtLayer
	     );

  paraD->speedFact =      speedFactCl;
  paraD->pitchFact =      pitchFactCl;

  
  BsFreeBuffer(bitHeader);

  if (DPdebugLevel >= 1) {
    printf("DecParInitNew: numChannel=%d  fSample=%f  bitRate=%f\n",
	   numChannel,fSample,bitRate);
    printf("DecParInitNew: decPara=\"%s\"\n",
	   decPara);
    printf("DecParInitNew: debugLevel=%d\n",
	   DPdebugLevel);
    printf("DecParInitNew: numLayer=%d, outputLayerNumber=%d\n",
	   numLayer, outputLayerNumber);
    printf("DecParInitNew: enhaFlag=%d, numExtLayer=%d numEscPA[0]=%d\n",
	   enhaFlag, numExtLayer, numEscPA[0]);

    if (DPtestMode != TM_NORMAL) {
      printf("DecParInitNew: test_mode=%x(for HVXC decoder conformance testing)\n", DPtestMode);
      if (DPtestMode & TM_POSFIL_DISABLE) {
	printf("  - postfilter and post processing are skipped.\n");
      }
      if (DPtestMode & TM_INIT_PHASE_ZERO) {
	printf("  - initial values of harmonic phase are reset to zeros.\n");
      }
      if (DPtestMode & TM_NOISE_ADD_DISABLE) {
	printf("  - noise component addition are disabled.\n");
      }
      if (DPtestMode & TM_VXC_DISABLE) {
	printf("  - the output of Time Domain Decoder is disabled.\n");
      }
    }
  }

  if ((paraD->sampleBuf=(float**)malloc(numChannel*sizeof(float*)))==NULL)
    CommonExit(1,"DecParInitNew: memory allocation error");
  for (i=0; i<numChannel; i++)
    if ((paraD->sampleBuf[i]=(float*)malloc(paraD->frameMaxNumSample*sizeof(float)))==NULL)
      CommonExit(1,"DecParInitNew: memory allocation error");

  return(paraD);
}		

/* DecParFrame() */
/* Decode one bit stream frame into one audio frame with */
/* parametric decoder core - system interface   HP 20000330 */

void DecParFrame(
  FRAME_DATA*  fD,
  ParaDecStatus* paraD,		/* in: para decoder status handle */
  int *usedNumBit,	        /* out: num bits used for this frame         */
   int epFlag,                   /* in: error protection flag */
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData
  )
{
  int num,lay,esc;
  HANDLE_BSBITSTREAM stream[MAX_LAYER+1+14];
  int tempNumBit, sysAlign;
  unsigned int i;

  num = 0;
  for (lay=0; lay<=outputLayerNumber; lay++)
    for (esc=0; esc<numEscPA[lay]; esc++, num++)
      stream[num] = BsOpenBufferRead(fD->layer[num].bitBuf); 

  _decParFrame(
	      (HANDLE_BSBITBUFFER)NULL,
	      stream,
	      paraD->sampleBuf,
	      usedNumBit,
	      &(paraD->frameNumSample),
	      paraD->speedFact,
	      paraD->pitchFact,
	      epFlag,
	      hEscInstanceData
	      );

  *usedNumBit = 0;

  num = 0;
  for (lay=0; lay<=outputLayerNumber; lay++)
    for (esc=0; esc<numEscPA[lay]; esc++, num++) {
      /* skip padding bits & bytes in AU   HP 20011010 */
      tempNumBit = BsCurrentBit(stream[num]);
      sysAlign = fD->layer[num].AULength[0]-tempNumBit;
      if (DPdebugLevel >= 2)
	printf("layer=%d NoAU=%d AULen[0]=%d numBit=%d padBit=%d\n",
	       num,fD->layer[num].NoAUInBuffer,fD->layer[num].AULength[0],
	       tempNumBit,sysAlign);
      BsGetSkip(stream[num],sysAlign);
      for (i=1; i<fD->layer[num].NoAUInBuffer; i++)
	fD->layer[num].AULength[i-1] = fD->layer[num].AULength[i];

      tempNumBit = BsCurrentBit(stream[num]);
      *usedNumBit += tempNumBit;
      BsCloseRemove(stream[num],1); 
    
      fD->layer[num].NoAUInBuffer--;
    }
}


/* end of dec_par.c */



