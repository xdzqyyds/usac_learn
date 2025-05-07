/**********************************************************************
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by

Bodo Teichmann (Fraunhofer IIS-A tmn@iis.fhg.de)

and edited by

Heiko Purnhagen (University of Hannover)
Olaf Kaehler (Fraunhofer IIS-A)
Manuela Schinn (Fraunhofer IIS)

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

Copyright (c) 1998.



Source file: flex_mux.c


Required modules:
common.o		common module
bitstream.o             bitstream handling module

BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

 **********************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "allHandles.h"

#include "obj_descr.h"           /* structs */
#include "lpc_common.h"          /* structs */
#include "sac_dec_interface.h"
#include "spatial_bitstreamreader.h"

#include "bitstream.h"
#include "common_m4a.h"
#include "flex_mux.h"
#include "usac_config.h"

#include "all.h"

const int samplFreqIndex[] = {
    96000,  88200,  64000,  48000,  44100,  32000,  24000,  22050,  16000,  12000,  11025,  8000,  7350,
    -1,
    -1,
    -1,
    -1
};



static const unsigned long initialObjectDescrTag = 0x02 ;
static const unsigned long elementStreamDescrTag = 0x03 ;
static const unsigned long decoderConfigDescrTag = 0x04 ;
static const unsigned long decSpecificInfoTag    = 0x05 ;
static const unsigned long slConfigTag           = 0x06 ;


static const char *codecNames [] =
{
    "Null Object - Linear",
    "AAC Main Profile",
    "AAC Low Complexity Profile",
    "AAC Scalable Sampling Rate Profile",
    "AAC Long Term Predictor (LTP)",
#ifdef CT_SBR
    "SBR",
#else
    "5",
#endif
    "AAC Scalable",
    "TwinVQ",
    "CELP",
    "HVXC",
    "10",
    "11",
    "TTSI (not supported)",
    "Main Synthetic (not supported)",
    "Wavetable Synthesis (not supported)",
    "General MIDI (not supported)",
    "Algorithmic Synthesis and Audio FX (not supported)",
    "Error Resilient AAC Low Complexity",
    "18",
    "Error Resilient AAC Long Term Predictor",
    "Error Resilient AAC Scalable Object",
    "Error Resilient TwinVQ Object",
    "Error Resilient BSAC Object",
    "Error Resilient AAC LD Object",
    "Error Resilient CELP Object",
    "Error Resilient HVXC Object",
    "Error Resilient HILN Object",
    "Error Resilient Parametric Object",
#ifdef EXT2PAR
    "SSC Parametric Object",
#else
    "28",
#endif
#ifdef PARAMETRICSTEREO
    "PS",
#else
    "29",
#endif
    "30",
    "<escape-aot>",
#ifdef MPEG12
    "Layer-1",
    "Layer-2",
    "Layer-3",
#else
    "32",
    "33",
    "34",
#endif
    "35",
    "36",
#ifdef I2R_LOSSLESS
    "SLS",
    "SLS nocore",
#else
    "37",
    "38",
#endif
#ifdef AAC_ELD
    "ER AAC ELD",
#else
    "39",
#endif
    "--"
} ;


void ObjDescPrintf(int WriteFlag, char *message, ...)
{
  if ((WriteFlag==0)||(WriteFlag==1)) {
    va_list args;
    va_start(args,message);
    DebugVPrintf(3,message,args);
    va_end(args);
  }
}


void initObjDescr(OBJECT_DESCRIPTOR *od)
{
  od->ODLength.length=32;
  od->ODescrId.length=10;
  od->streamCount.length=5;
  od->extensionFlag.length=1;
}

void presetObjDescr(OBJECT_DESCRIPTOR *od)
{

  od->ODLength.value=0;
  od->ODescrId.value=1;
  od->streamCount.value=0;
  od->extensionFlag.value=0;

}

#ifdef AAC_ELD
static void initELDspecConf ( ELD_SPECIFIC_CONFIG *eldConf )
{
  eldConf->frameLengthFlag.length = 1;

  eldConf->aacSectionDataResilienceFlag.length     = 1;
  eldConf->aacScalefactorDataResilienceFlag.length = 1;
  eldConf->aacSpectralDataResilienceFlag.length    = 1;

  eldConf->ldSbrPresentFlag.length   = 1;
  eldConf->ldSbrSamplingRate.length  = 1;
  eldConf->ldSbrCrcFlag.length       = 1;

  memset(eldConf->sbrHeaderData, 0, sizeof(UINT8)*MAX_SBR_HEADER_SIZE);

  return;
}
#endif

static void initTFspecConf ( TF_SPECIFIC_CONFIG *tfConf )
{

  /*   tfConf->TFCodingType.length=2 ; */
  tfConf->frameLength.length=1 ;
  tfConf->dependsOnCoreCoder.length=1 ;
  tfConf->coreCoderDelay.length=14 ;
  tfConf->extension.length=1 ;
  tfConf->layerNr.length=3 ;

  tfConf->numOfSubFrame.length=5;
  tfConf->layer_length.length=11;

  tfConf->aacSectionDataResilienceFlag.length     = 1;
  tfConf->aacScalefactorDataResilienceFlag.length = 1;
  tfConf->aacSpectralDataResilienceFlag.length    = 1;

  tfConf->extension3.length=1 ;

  if (tfConf->progConfig == NULL) {
    tfConf->progConfig = (ProgConfig*) malloc(sizeof(ProgConfig));
    if (tfConf->progConfig==NULL) CommonExit(-1,"no mem");
    memset (tfConf->progConfig, 0, sizeof (ProgConfig)) ;
  }

  return;
}


static void initCelpSpecConf (CELP_SPECIFIC_CONFIG *celpConf )
{
  celpConf->excitationMode.length = 1;
  celpConf->sampleRateMode.length = 1;
  celpConf->fineRateControl.length = 1;

  celpConf->silenceCompressionSW.length = 1;

  /* RPE mode */
  celpConf->RPE_Configuration.length = 3;

  /* MPE mode */
  celpConf->MPE_Configuration.length = 5;
  celpConf->numEnhLayers.length = 2;
  celpConf->bandwidthScalabilityMode.length = 1;
#ifdef CORRIGENDUM1
  celpConf->isBaseLayer.length = 1;	/* 14496-3 COR1 */
  celpConf->isBWSLayer.length = 1;	/* 14496-3 COR1 */
  celpConf->CELP_BRS_id.length = 2;	/* 14496-3 COR1 */
  celpConf->BWS_Configuration.length = 2;
#endif
}

#ifndef CORRIGENDUM1
void initCelpEnhSpecConf (CELP_ENH_SPECIFIC_CONFIG *celpEnhConf)
{
  celpEnhConf->BWS_Configuration.length = 2;
}
#endif

/* AI 990616 */
static void initHvxcSpecConf (HVXC_SPECIFIC_CONFIG *hvxcConf)
{
#ifdef CORRIGENDUM1
  hvxcConf->isBaseLayer.length = 1;	/* 14496-3 COR1 (AI 2000/10) */
#endif
  hvxcConf->HVXCvarMode.length = 1;
  hvxcConf->HVXCrateMode.length = 2;
  hvxcConf->extensionFlag.length = 1;
  hvxcConf->vrScalFlag.length = 1;    /* VR scalable mode (YM 990728) */
  return;
}

/* HP20001009 */
static void initParaSpecConf (PARA_SPECIFIC_CONFIG *paraConf)
{
#ifdef CORRIGENDUM1
  paraConf->isBaseLayer.length = 1;	/* 14496-3 COR1 */
#endif

  /* base layer */
  paraConf->PARAmode.length = 2;

  paraConf->HVXCvarMode.length = 1;
  paraConf->HVXCrateMode.length = 2;
  paraConf->extensionFlag.length = 1;
  paraConf->vrScalFlag.length = 1;

  paraConf->HILNquantMode.length = 1;
  paraConf->HILNmaxNumLine.length = 8;
  paraConf->HILNsampleRateCode.length = 4;
  paraConf->HILNframeLength.length = 12;
  paraConf->HILNcontMode.length = 2;

  paraConf->PARAextensionFlag.length = 1;

  /* enha/ext layer(s) */
  paraConf->HILNenhaLayer.length = 1;
  paraConf->HILNenhaQuantMode.length = 2;
  return;
}

#ifdef EXT2PAR
static void initSSCSpecConf (SSC_SPECIFIC_CONFIG *sscConf)
{

  sscConf->DecoderLevel.length           = 2;
  sscConf->UpdateRate.length             = 4;
  sscConf->SynthesisMethod.length        = 2;
  sscConf->ModeExt.length                = 2;
  sscConf->PsReserved.length             = 2;

  return;
}
#endif

#ifdef I2R_LOSSLESS
static void initSLSSpecConf(SLS_SPECIFIC_CONFIG* slsConf)
{
  slsConf->SLSpcmWordLength.length       = 3;
  slsConf->SLSaacCorePresent.length      = 1;
  slsConf->SLSlleMainStream.length       = 1;
  slsConf->SLSreserved.length            = 1;
  slsConf->SLSframeLength.length         = 3;

  if (slsConf->progConfig == NULL) {
    slsConf->progConfig = (ProgConfig*) malloc(sizeof(ProgConfig));
    if (slsConf->progConfig==NULL) CommonExit(-1,"no mem");
    memset (slsConf->progConfig, 0, sizeof (ProgConfig)) ;
  }

  return;
}
#endif

#ifdef MPEG12
static void initMPEG_1_2_SpecConf (MPEG_1_2_SPECIFIC_CONFIG *MPEG_1_2_Conf)
{

  MPEG_1_2_Conf->Extension.value = 0;    /* shall be zero according to Standard */
  MPEG_1_2_Conf->Extension.length = 1;
  return;
}
#endif

void initEpSpecConf (EP_SPECIFIC_CONFIG *esc)
{
  esc->numberOfPredifinedSet.length = 8;
  esc->interleaveType.length = 2;
  esc->bitStuffing.length = 3;
  esc->numberOfConcatenatedFrame.length = 3;

  esc->headerProtection.length = 1;
  esc->headerRate.length = 5 ;
  esc->headerCrclen.length =5;
  return;
}
static void initEpPredSetConf (PRED_SET_CONFIG *psc)
{
  psc->numberOfClass.length = 6;
  psc->classReorderedOutput.length =1;
  return;
}
static void initEpClassConf (EP_CLASS_CONFIG *cc)
{
  cc->lengthEscape.length = 1;
  cc->rateEscape.length = 1;
  cc->crclenEscape.length = 1;
  cc->concatenateFlag.length = 1;
  cc->fecType.length = 2;
  cc->terminationSwitch.length = 1;
  cc->interleaveSwitch.length = 2;
  cc->classOptional.length = 1;
  cc->numberOfBitsForLength.length = 4;
  cc->classLength.length = 16;
  /*cc->classRate.length = 5;*/
  cc->classCrclen.length = 5;
  cc->classOutputOrder.length = 6;
  return;
}

void setupDecConfigDescr( DEC_CONF_DESCRIPTOR *decConfigDescr )
{
  decConfigDescr->profileAndLevelIndication.length=8 ;
  decConfigDescr->streamType.length=6 ;
  decConfigDescr->upsteam.length=1 ;
  decConfigDescr->specificInfoFlag.length=1 ;
  decConfigDescr->bufferSizeDB.length=24 ;
  decConfigDescr->maxBitrate.length=32 ;
  decConfigDescr->avgBitrate.length=32 ;
  decConfigDescr->specificInfoLength.length=8 ;
  decConfigDescr->audioSpecificConfig.audioDecoderType.length=5 ;
  decConfigDescr->audioSpecificConfig.samplingFreqencyIndex.length= 4;
  decConfigDescr->audioSpecificConfig.samplingFrequency.length= 24;
  decConfigDescr->audioSpecificConfig.channelConfiguration.length=4 ;
  /* 20070326 BSAC Ext.*/
  decConfigDescr->audioSpecificConfig.extensionChannelConfiguration.length=4 ;
  /* 20070326 BSAC Ext.*/
#ifndef EP_CONFING_MISSING
  decConfigDescr->audioSpecificConfig.epConfig.length=2 ;
  decConfigDescr->audioSpecificConfig.directMapping.length=1 ;
#endif
#ifdef CT_SBR
  decConfigDescr->audioSpecificConfig.extensionAudioDecoderType.length = 5;
  decConfigDescr->audioSpecificConfig.extensionSamplingFrequencyIndex.length = 4;
  decConfigDescr->audioSpecificConfig.extensionSamplingFrequency.length= 24;
  decConfigDescr->audioSpecificConfig.sbrPresentFlag.length = 1;
  decConfigDescr->audioSpecificConfig.syncExtensionType.length = 11;
#ifdef PARAMETRICSTEREO
  decConfigDescr->audioSpecificConfig.psPresentFlag.length = 1;
#endif
#endif
}

void initESDescr( ES_DESCRIPTOR **es)
{

  *es = NULL;
  *es = (ES_DESCRIPTOR*) malloc(sizeof(ES_DESCRIPTOR));


  if (*es==NULL) CommonExit(-1,"no mem");

  memset (*es, 0, sizeof (ES_DESCRIPTOR)) ;

  (*es)->ESNumber.length=16  /*5*/;
  (*es)->streamDependence.length=1;
  (*es)->URLFlag.length=1;
  (*es)->OCRFlag.length=1;
  (*es)->streamPriority.length=5;
  (*es)->dependsOn_Es_number.length=16/*5*/;
  (*es)->URLlength.length=8;
  (*es)->URLstring.length=8;
  (*es)->OCR_ES_id.length=16;

  setupDecConfigDescr( &((*es)->DecConfigDescr) );

  (*es)->ALConfigDescriptor.useAccessUnitStartFlag.length = 1;
  (*es)->ALConfigDescriptor.useAccessUnitEndFlag.length = 1;
  (*es)->ALConfigDescriptor.useRandomAccessPointFlag.length = 1;
  (*es)->ALConfigDescriptor.usePaddingFlag.length = 1;
  (*es)->ALConfigDescriptor.seqNumLength.length = 4;

}
void presetDecConfigDescr( DEC_CONF_DESCRIPTOR *DecConfigDescr )
{
  DecConfigDescr->profileAndLevelIndication.value=0x40;
  DecConfigDescr->streamType.value=5 ; /* audio stream */
  DecConfigDescr->upsteam.value=0;
  DecConfigDescr->specificInfoFlag.value=1 ;
  DecConfigDescr->bufferSizeDB.value=6144;
  DecConfigDescr->maxBitrate.value=0;
  DecConfigDescr->avgBitrate.value=0;
  DecConfigDescr->specificInfoLength.value=2; /* 16 bits if TFcoding */
}
void presetESDescr( ES_DESCRIPTOR *es,int layer)
{
  es->ESNumber.value=layer;

  /* if this is the first layer, there is no dependence */
  es->streamDependence.value=((layer==0)?0:1);
  es->URLFlag.value=0;
  es->dependsOn_Es_number.value=(layer>0)?layer:0;

  presetDecConfigDescr(&es->DecConfigDescr);

  /* es->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value= 0x6;*/  /*24 kHz */ /* cause this is patched afterward */
  es->DecConfigDescr.audioSpecificConfig.channelConfiguration.value=1 ;
#ifndef EP_CONFING_MISSING
  es->DecConfigDescr.audioSpecificConfig.epConfig.value=0;	/* epConfig=0 at default */
  es->DecConfigDescr.audioSpecificConfig.directMapping.value=1;	/* directMapping=1 at default */
#endif
}


/* ---- GA specific config ---- */


static int advanceEleList(HANDLE_BSBITSTREAM bs,
                          EleList* p,
                          int WriteFlag,
                          int enable_cpe)
{
  int i;
  unsigned long ultmp;
  int bitCount = 0;

  for (i=0; i<p->num_ele; i++) {
    if (enable_cpe) {
      ultmp=p->ele_is_cpe[i];
      bitCount+=BsRWBitWrapper (bs, &ultmp, LEN_ELE_IS_CPE, WriteFlag);
      p->ele_is_cpe[i]=ultmp;
    }
    ultmp=p->ele_tag[i];
    bitCount+=BsRWBitWrapper (bs, &ultmp, LEN_TAG, WriteFlag);
    p->ele_tag[i]=ultmp;
  }

  return bitCount;
}

static int advanceProgConfig (HANDLE_BSBITSTREAM bs,
                              ProgConfig* p,
                              int WriteFlag,
                              int ascBits)
{
  int i;
  unsigned long ultmp = 0, j;
  int bitCount = 0;

  ultmp=p->tag;
  bitCount+=BsRWBitWrapper (bs, &ultmp, LEN_TAG, WriteFlag);
  p->tag=ultmp;          ultmp=p->profile;
  bitCount+=BsRWBitWrapper (bs, &ultmp, LEN_PROFILE, WriteFlag);
  p->profile=ultmp;      ultmp=p->sampling_rate_idx;
  bitCount+=BsRWBitWrapper(bs, &ultmp, LEN_SAMP_IDX, WriteFlag);
  p->sampling_rate_idx=ultmp;
  ObjDescPrintf(WriteFlag,"   pce->tag              : %i",p->tag);
  ObjDescPrintf(WriteFlag,"   pce->profile          : %i",p->profile);
  ObjDescPrintf(WriteFlag,"   pce->sampling_rate_idx: %i",p->sampling_rate_idx);
  ultmp=p->front.num_ele;
  bitCount+=BsRWBitWrapper (bs, &ultmp, LEN_NUM_ELE, WriteFlag);
  p->front.num_ele=ultmp;ultmp=p->side.num_ele;
  bitCount+=BsRWBitWrapper (bs, &ultmp, LEN_NUM_ELE, WriteFlag);
  p->side.num_ele=ultmp; ultmp=p->back.num_ele;
  bitCount+=BsRWBitWrapper (bs, &ultmp, LEN_NUM_ELE, WriteFlag);
  p->back.num_ele=ultmp; ultmp=p->lfe.num_ele;
  bitCount+=BsRWBitWrapper (bs, &ultmp, LEN_NUM_LFE, WriteFlag);
  p->lfe.num_ele=ultmp;  ultmp=p->data.num_ele;
  bitCount+=BsRWBitWrapper (bs, &ultmp, LEN_NUM_DAT, WriteFlag);
  p->data.num_ele=ultmp; ultmp=p->coupling.num_ele;
  bitCount+=BsRWBitWrapper (bs, &ultmp, LEN_NUM_CCE, WriteFlag);
  p->coupling.num_ele=ultmp;ultmp=p->mono_mix.present;
  bitCount+=BsRWBitWrapper (bs, &ultmp, LEN_MIX_PRES, WriteFlag);
  p->mono_mix.present=ultmp;
  if (p->mono_mix.present == 1) {
    ultmp=p->mono_mix.ele_tag;
    bitCount+=BsRWBitWrapper (bs, &ultmp, LEN_TAG, WriteFlag);
    p->mono_mix.ele_tag=ultmp;
  }
  ultmp=p->stereo_mix.present;
  bitCount+=BsRWBitWrapper (bs, &ultmp, LEN_MIX_PRES, WriteFlag);
  p->stereo_mix.present=ultmp;
  if (p->stereo_mix.present == 1) {
    ultmp=p->stereo_mix.ele_tag;
    bitCount+=BsRWBitWrapper (bs, &ultmp, LEN_TAG, WriteFlag);
    p->stereo_mix.ele_tag=ultmp;
  }
  ultmp=p->matrix_mix.present;
  bitCount+=BsRWBitWrapper (bs, &ultmp, LEN_MIX_PRES, WriteFlag);
  p->matrix_mix.present=ultmp;
  if (p->matrix_mix.present == 1) {
    ultmp=p->matrix_mix.ele_tag;
    bitCount+=BsRWBitWrapper (bs, &ultmp, LEN_MMIX_IDX, WriteFlag);
    p->matrix_mix.ele_tag=ultmp;ultmp=p->matrix_mix.pseudo_enab;
    bitCount+=BsRWBitWrapper (bs, &ultmp, LEN_PSUR_ENAB, WriteFlag);
    p->matrix_mix.pseudo_enab=ultmp;
  }
  bitCount+=advanceEleList(bs, &p->front, WriteFlag, 1);
  bitCount+=advanceEleList(bs, &p->side, WriteFlag, 1);
  bitCount+=advanceEleList(bs, &p->back, WriteFlag, 1);
  bitCount+=advanceEleList(bs, &p->lfe, WriteFlag, 0);
  bitCount+=advanceEleList(bs, &p->data, WriteFlag, 0);
  bitCount+=advanceEleList(bs, &p->coupling, WriteFlag, 1);

  ultmp=j=0;

  i = (ascBits + bitCount) % 8;
  if (i) bitCount+=BsRWBitWrapper(bs, &ultmp, 8-i, WriteFlag);

  j = /*strlen(p->comments)*/0;
  bitCount+=BsRWBitWrapper (bs, &(j), LEN_COMMENT_BYTES, WriteFlag);
  for (i=0; (unsigned)i<j; i++) {
    ultmp = p->comments[i];
    bitCount+=BsRWBitWrapper (bs, &(ultmp), LEN_BYTE, WriteFlag);
    p->comments[i] = (char) ultmp;
  }
  p->comments[i] = 0; /* null terminator for string */
  return bitCount;
}

int usac_get_channel_number(USAC_CONFIG* pUsacConfig, int* nLfe) {

  int nChannels = -1;

  if(nLfe) *nLfe = 0;

  if(pUsacConfig){
    nChannels = pUsacConfig->usacChannelConfig.numOutChannels;
  }

  return nChannels;
}

int get_channel_number(int channelConfig,
                       ProgConfig* p,
                       int *numFC,   /* number of front channels */
                       int *fCenter, /* 1 if decoder has front center channel */
                       int *numSC,   /* number of side channels */
                       int *numBC,   /* number of back channels */
                       int *bCenter, /* 1 if decoder has back center channel */
                       int *numLFE,  /* number of LFE channels */
                       int *numIndCCE) /* number of individually switched coupling channels */
{
  int fc = 0, sc = 0, bc = 0, lfe = 0, indCCE = 0;

  switch (channelConfig) {
    case 1:
    case 2:
    case 3:
      fc=channelConfig;
      break;
    case 4:
      fc=3;
      bc=1;
      break;
    case 6:
      lfe=1;
    case 5:
      fc=3;
      bc=2;
      break;
    case 7:
      lfe=1;
      fc=3;
      sc=2;
      bc=2;
  }

  if (channelConfig==0) {
    int c;
    for (c = 0; c < p->front.num_ele ; c++) {
      if (p->front.ele_is_cpe[c]) fc +=2;
      else fc +=1;
    }
    for (c = 0; c < p->side.num_ele ; c++) {
      if (p->side.ele_is_cpe[c]) sc +=2;
      else sc +=1;
    }
    for (c = 0; c < p->back.num_ele ; c++) {
      if (p->back.ele_is_cpe[c]) bc +=2;
      else bc +=1;
    }
    lfe = p->lfe.num_ele;
    for (c = 0; c < p->coupling.num_ele ; c++) {
      if (p->coupling.ele_is_cpe[c]) indCCE++;
    }
  } else if (channelConfig>7) return channelConfig;

  if (numFC) *numFC = fc;
  if (numSC) *numSC = sc;
  if (numBC) *numBC = bc;
  if (numLFE) *numLFE = lfe;
  if (fCenter) *fCenter = fc % 2;
  if (bCenter) *bCenter = bc % 2;
  if (numIndCCE) *numIndCCE = indCCE;

  return fc + sc + bc + lfe;
}

int get_channel_number_USAC(USAC_CONFIG *pUsacConfig, /* USAC configuration*/
                       int *numFC,   /* number of front channels */
                       int *fCenter, /* 1 if decoder has front center channel */
                       int *numSC,   /* number of side channels */
                       int *numBC,   /* number of back channels */
                       int *bCenter, /* 1 if decoder has back center channel */
                       int *numLFE,  /* number of LFE channels */
                       int *numIndCCE) /* number of individually switched coupling channels */
{
  unsigned int nChannels = 0;
  
  if(numFC) *numFC = 0;
  if(fCenter) *fCenter = 0;
  if(numSC) *numSC = 0;
  if(numBC) *numBC = 0;
  if(bCenter) *bCenter = 0;
  if(numLFE) *numLFE = 0;
  if(numIndCCE) *numIndCCE = 0;

  switch(pUsacConfig->usacChannelConfig.numOutChannels) {
    case 1:
      if(numFC) *numFC = 1;
      if(fCenter) *fCenter = 1;
      nChannels = 1;
      break;
    case 2:
      if(numFC) *numFC = 2;
      nChannels = 2;
      break;
    case 3:
      if(numFC) *numFC = 3;
      if(fCenter) *fCenter = 1;
      nChannels = 3;
      break;
    case 4:
      if(numFC) *numFC = 2;
      if(fCenter) *fCenter = 0;
      if(numBC) *numBC = 2;
      nChannels = 4;
      break;
    case 5:
      if(numFC) *numFC = 3;
      if(fCenter) *fCenter = 1;
      if(numBC) *numBC = 2;
      nChannels = 5;
      break;
    case 6:
      if(numFC) *numFC = 3;
      if(fCenter) *fCenter = 1;
      if(numSC) *numSC = 0;
      if(numBC) *numBC = 2;
      if(bCenter) *bCenter = 0;
      if(numLFE) *numLFE = 1;
      if(numIndCCE) *numIndCCE = 0;
      nChannels = 6;
      break;
  }

  return (int)nChannels;
}

/* fixing 7.1channel BSAC error */
int get_channel_number_BSAC(int channelConfig,
                            ProgConfig* p,
                            int *numFC,   /* number of front channels */
                            int *fCenter, /* 1 if decoder has front center channel */
                            int *numSC,   /* number of side channels */
                            int *numBC,   /* number of back channels */
                            int *bCenter, /* 1 if decoder has back center channel */
                            int *numLFE,  /* number of LFE channels */
                            int *numIndCCE) /* number of individually switched coupling channels */
{
  int fc = 0, sc = 0, bc = 0, lfe = 0, indCCE = 0;

  switch (channelConfig) {
    case 1:
    case 2:
    case 3:
      fc=channelConfig;
      break;
    case 4:
      fc=3;
      bc=1;
      break;
    case 6:
      lfe=1;
    case 5:
      fc=3;
      bc=2;
      break;
    case 7:
      lfe=1;
      fc=3;
      sc=2;
      bc=2;
    case 8:
      lfe=1;
      fc=3;
      sc=2;
      bc=2;
  }

  if (channelConfig==0) {
    int c;
    for (c = 0; c < p->front.num_ele ; c++) {
      if (p->front.ele_is_cpe[c]) fc +=2;
      else fc +=1;
    }
    for (c = 0; c < p->side.num_ele ; c++) {
      if (p->side.ele_is_cpe[c]) sc +=2;
      else sc +=1;
    }
    for (c = 0; c < p->back.num_ele ; c++) {
      if (p->back.ele_is_cpe[c]) bc +=2;
      else bc +=1;
    }
    lfe = p->lfe.num_ele;
    for (c = 0; c < p->coupling.num_ele ; c++) {
      if (p->coupling.ele_is_cpe[c]) indCCE++;
    }
  } else if (channelConfig>8) return channelConfig;

  if (numFC) *numFC = fc;
  if (numSC) *numSC = sc;
  if (numBC) *numBC = bc;
  if (numLFE) *numLFE = lfe;
  if (fCenter) *fCenter = fc % 2;
  if (bCenter) *bCenter = bc % 2;
  if (numIndCCE) *numIndCCE = indCCE;

  return fc + sc + bc + lfe;
}

#ifdef AAC_ELD

static int get_ld_sbr_header( HANDLE_BSBITSTREAM bitStream,
                              unsigned char sbrHeaderData[MAX_SBR_HEADER_SIZE],
                              int WriteFlag
)
{

  /* quick parser to get size of header */

  /* quick parser to get size of header */
  int header_extra_1, header_extra_2, sbrHeaderLengthExt, i;
  UINT8  bits2parse = 16;
  UINT32 tmp, bitCount = 0;

  bitCount+=BsRWBitWrapper(bitStream, &tmp, bits2parse, WriteFlag);

  header_extra_1 = (tmp & 2) >> 1;
  header_extra_2 = tmp & 1;
  sbrHeaderLengthExt = header_extra_1*(2+1+2) + header_extra_2*(2+2+1+1);

  /* copy sbr header into char buffer */
  sbrHeaderData[0] = (tmp & (0xFF00)) >> 8; /* 1st byte */
  sbrHeaderData[1] =  tmp & (0xFF);         /* 2nd byte */

  for (i=0; i<(sbrHeaderLengthExt/8); i++) {
    bitCount+=BsRWBitWrapper(bitStream, &tmp, 8, WriteFlag);
    sbrHeaderData[i+2] = tmp;
  }
  if ((sbrHeaderLengthExt % 8) != 0) {
    int rest = (sbrHeaderLengthExt % 8);
    bitCount+=BsRWBitWrapper(bitStream, &tmp, rest, WriteFlag);
    sbrHeaderData[i+2] = tmp << (8 - rest);
  }
  return (bitCount);
}

static int  ld_sbr_header( const int channelConfiguration,
                           HANDLE_BSBITSTREAM bitStream,
                           unsigned char m_sbrHeaderData[MAX_SBR_ELEMENTS][MAX_SBR_HEADER_SIZE],
                           int WriteFlag
)
{
  int numSbrHeader, el, bitCount=0;
  switch ( channelConfiguration ) {
    default:
      numSbrHeader = 0;
      break;
    case 1:
    case 2:
      numSbrHeader = 1;
      break;
    case 3:
      numSbrHeader = 2;
      break;
    case 4:
    case 5:
    case 6:
      numSbrHeader = 3;
      break;
    case 7:
      numSbrHeader = 4;
      break;
  }
  for (el=0; el<numSbrHeader; el++) {
    bitCount += get_ld_sbr_header(bitStream, m_sbrHeaderData[el],WriteFlag);
  }
  return (bitCount);
}

static int advanceELDspecConf (HANDLE_BSBITSTREAM bitStream,
                               ELD_SPECIFIC_CONFIG *eldConf,
                               int channelConfiguration,
                               int WriteFlag)
{
  int bitCount = 0;

  bitCount+=BsRWBitWrapper(bitStream, &(eldConf->frameLengthFlag.value), eldConf->frameLengthFlag.length, WriteFlag);
  bitCount+=BsRWBitWrapper(bitStream, &(eldConf->aacSectionDataResilienceFlag.value), eldConf->aacSectionDataResilienceFlag.length, WriteFlag);
  bitCount+=BsRWBitWrapper(bitStream, &(eldConf->aacScalefactorDataResilienceFlag.value), eldConf->aacScalefactorDataResilienceFlag.length, WriteFlag);
  bitCount+=BsRWBitWrapper(bitStream, &(eldConf->aacSpectralDataResilienceFlag.value), eldConf->aacSpectralDataResilienceFlag.length, WriteFlag);
  bitCount+=BsRWBitWrapper(bitStream, &(eldConf->ldSbrPresentFlag.value), eldConf->ldSbrPresentFlag.length, WriteFlag);

  if (eldConf->ldSbrPresentFlag.value) {
    bitCount+=BsRWBitWrapper(bitStream, &(eldConf->ldSbrSamplingRate.value), eldConf->ldSbrSamplingRate.length, WriteFlag);
    bitCount+=BsRWBitWrapper(bitStream, &(eldConf->ldSbrCrcFlag.value), eldConf->ldSbrCrcFlag.length, WriteFlag);

    bitCount += ld_sbr_header(channelConfiguration, bitStream, eldConf->sbrHeaderData, WriteFlag);
  }
  {
    UINT32 len, cnt, eldExtType, eldExtLen, eldExtLenAdd, eldExtLenAddAdd, other;
    /* parse ExtTypeConfigData */
    bitCount+=BsRWBitWrapper(bitStream, &eldExtType, 4, WriteFlag);
    while (eldExtType != ELDEXT_TERM) {
      bitCount+=BsRWBitWrapper(bitStream, &eldExtLen, 4, WriteFlag);
      len = eldExtLen;
      if ( eldExtLen == 0xf ) {
        bitCount+=BsRWBitWrapper(bitStream, &eldExtLenAdd, 8, WriteFlag);
        len += eldExtLenAdd;

        if ( eldExtLenAdd == 0xff ) {
          bitCount+=BsRWBitWrapper(bitStream, &eldExtLenAddAdd, 16, WriteFlag);
          len += eldExtLenAddAdd;
        }
      }

      switch (eldExtType) {
        default:
          for(cnt=0; cnt<len; cnt++) {
            bitCount+=BsRWBitWrapper(bitStream, &other, 8, WriteFlag);
          }
          break;
          /* add future eld extension configs here */
      }
      bitCount+=BsRWBitWrapper(bitStream, &eldExtType, 4, WriteFlag);
    }
  }

  return bitCount;
}
#endif


static int advanceTFspecConf (HANDLE_BSBITSTREAM bitStream, TF_SPECIFIC_CONFIG *tfConf, int WriteFlag,
                              int audioObjectType, int channelConfig, int ascBits)
{
  int bitCount = 0;
  /* bitCount+=BsRWBitWrapper(bitStream, &(tfConf->TFCodingType.value), tfConf->TFCodingType.length,WriteFlag); */

  bitCount+=BsRWBitWrapper(bitStream, &(tfConf->frameLength.value), tfConf->frameLength.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "   audioSpC->ga.frameLengthFlag   : %ld",tfConf->frameLength.value);

  bitCount+=BsRWBitWrapper(bitStream, &(tfConf->dependsOnCoreCoder.value), tfConf->dependsOnCoreCoder.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "   audioSpC->ga.dependsOnCoreCoder: %ld",tfConf->dependsOnCoreCoder.value);
  if (tfConf->dependsOnCoreCoder.value != 0) {
    bitCount+=BsRWBitWrapper(bitStream, &(tfConf->coreCoderDelay.value), tfConf->coreCoderDelay.length,WriteFlag);
    ObjDescPrintf(WriteFlag, "   audioSpC->ga.coreCoderDelay    : %ld",tfConf->coreCoderDelay.value);
  }

  bitCount+=BsRWBitWrapper(bitStream, &(tfConf->extension.value), tfConf->extension.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "   audioSpC->ga.extensionFlag     : %ld",tfConf->extension.value);

  if (channelConfig == 0) {
    bitCount+=advanceProgConfig (bitStream, tfConf->progConfig, WriteFlag, ascBits + bitCount );
  }

#ifdef CORRIGENDUM1
  if (audioObjectType == AAC_SCAL || audioObjectType == ER_AAC_SCAL) {
    bitCount+=BsRWBitWrapper (bitStream, &(tfConf->layerNr.value), tfConf->layerNr.length, WriteFlag ) ;
    ObjDescPrintf(WriteFlag, "   audioSpC->ga.layerNr           : %ld",tfConf->layerNr.value);
  }
#endif

  if (tfConf->extension.value != 0) {

    if (audioObjectType == ER_BSAC) { /* ER_BSAC */
      bitCount+=BsRWBitWrapper(bitStream, &(tfConf->numOfSubFrame.value), tfConf->numOfSubFrame.length,WriteFlag);
      bitCount+=BsRWBitWrapper(bitStream, &(tfConf->layer_length.value), tfConf->layer_length.length,WriteFlag);
      ObjDescPrintf(WriteFlag, "   audioSpC->ga.numOfSubFrame     : %ld",tfConf->numOfSubFrame.value);
      ObjDescPrintf(WriteFlag, "   audioSpC->ga.layer_length      : %ld",tfConf->layer_length.value);
    }

    if (audioObjectType == ER_AAC_LC   || audioObjectType == ER_AAC_LTP ||
        audioObjectType == ER_AAC_SCAL || audioObjectType == ER_AAC_LD) {    /* ER AAC */
      bitCount+=BsRWBitWrapper(bitStream, &(tfConf->aacSectionDataResilienceFlag.value), tfConf->aacSectionDataResilienceFlag.length,WriteFlag);
      bitCount+=BsRWBitWrapper(bitStream,&(tfConf->aacScalefactorDataResilienceFlag.value), tfConf->aacScalefactorDataResilienceFlag.length,WriteFlag);
      bitCount+=BsRWBitWrapper(bitStream, &(tfConf->aacSpectralDataResilienceFlag.value), tfConf->aacSpectralDataResilienceFlag.length,WriteFlag);
      ObjDescPrintf(WriteFlag, "   audioSpC->ga.aacSectionRF      : %ld",tfConf->aacSectionDataResilienceFlag.value);
      ObjDescPrintf(WriteFlag, "   audioSpC->ga.aacScalefactRF    : %ld",tfConf->aacScalefactorDataResilienceFlag.value);
      ObjDescPrintf(WriteFlag, "   audioSpC->ga.aacSpectralRF     : %ld",tfConf->aacSpectralDataResilienceFlag.value);
    }

    bitCount+=BsRWBitWrapper(bitStream, &(tfConf->extension3.value), tfConf->extension3.length,WriteFlag);
    ObjDescPrintf(WriteFlag, "   audioSpC->ga.extension3Flag    : %ld",tfConf->extension3.value);
  }

  return bitCount;
}



/* ---- CELP specific config ---- */


static int advanceCelpSpecConf ( HANDLE_BSBITSTREAM bitStream,CELP_SPECIFIC_CONFIG *celpConf, int WriteFlag, int audioObjectType)
{
  int bitCount = 0;
#ifdef CORRIGENDUM1
  bitCount+=BsRWBitWrapper(bitStream, &(celpConf->isBaseLayer.value), celpConf->isBaseLayer.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "   audioSpC->celp.isBaseLayer     : %ld",celpConf->isBaseLayer.value);
  if (celpConf->isBaseLayer.value == 1)  { /* CELP Base Layer */
#endif

    bitCount+=BsRWBitWrapper(bitStream, &(celpConf->excitationMode.value), celpConf->excitationMode.length,WriteFlag);
    bitCount+=BsRWBitWrapper(bitStream, &(celpConf->sampleRateMode.value), celpConf->sampleRateMode.length,WriteFlag);
    bitCount+=BsRWBitWrapper(bitStream, &(celpConf->fineRateControl.value), celpConf->fineRateControl.length,WriteFlag);
    ObjDescPrintf(WriteFlag, "   audioSpC->celp.excitationMode  : %ld",celpConf->excitationMode.value);
    ObjDescPrintf(WriteFlag, "   audioSpC->celp.sampleRateMode  : %ld",celpConf->sampleRateMode.value);
    ObjDescPrintf(WriteFlag, "   audioSpC->celp.fineRateControl : %ld",celpConf->fineRateControl.value);

    if (audioObjectType==CELP)
      celpConf->silenceCompressionSW.value=0;
    else {
      bitCount+=BsRWBitWrapper(bitStream, &(celpConf->silenceCompressionSW.value), celpConf->silenceCompressionSW.length,WriteFlag);
      ObjDescPrintf(WriteFlag, "   audioSpC->celp.silenceCompSW   : %ld",celpConf->silenceCompressionSW.value);
    }

    if (celpConf->excitationMode.value == RegularPulseExc)
    {
      bitCount+=BsRWBitWrapper(bitStream, &(celpConf->RPE_Configuration.value), celpConf->RPE_Configuration.length,WriteFlag);
      ObjDescPrintf(WriteFlag, "   audioSpC->celp.RPE_config      : %ld",celpConf->RPE_Configuration.value);
    }

    if (celpConf->excitationMode.value == MultiPulseExc)
    {
      bitCount+=BsRWBitWrapper(bitStream, &(celpConf->MPE_Configuration.value), celpConf->MPE_Configuration.length,WriteFlag);
      bitCount+=BsRWBitWrapper(bitStream, &(celpConf->numEnhLayers.value), celpConf->numEnhLayers.length,WriteFlag);
      bitCount+=BsRWBitWrapper(bitStream, &(celpConf->bandwidthScalabilityMode.value), celpConf->bandwidthScalabilityMode.length,WriteFlag);
      ObjDescPrintf(WriteFlag, "   audioSpC->celp.MPE_config      : %ld",celpConf->MPE_Configuration.value);
      ObjDescPrintf(WriteFlag, "   audioSpC->celp.numEnhLayers    : %ld",celpConf->numEnhLayers.value);
      ObjDescPrintf(WriteFlag, "   audioSpC->celp.bandwidthScal   : %ld",celpConf->bandwidthScalabilityMode.value);
    }
#ifdef CORRIGENDUM1
  } else {
    bitCount+=BsRWBitWrapper(bitStream, &(celpConf->isBWSLayer.value), celpConf->isBWSLayer.length,WriteFlag);
    if (celpConf->isBWSLayer.value == 1) { /* CELP BWS Layer */
      bitCount+=BsRWBitWrapper(bitStream, &(celpConf->BWS_Configuration.value), celpConf->BWS_Configuration.length,WriteFlag);
      ObjDescPrintf(WriteFlag, "   audioSpC->celp.BWS_config      : %ld",celpConf->BWS_Configuration.value);
    } else { /* CELP BRS Layer */
      bitCount+=BsRWBitWrapper(bitStream, &(celpConf->CELP_BRS_id.value), celpConf->CELP_BRS_id.length,WriteFlag);
      ObjDescPrintf(WriteFlag, "   audioSpC->celp.CELP_BRS_id     : %ld",celpConf->CELP_BRS_id.value);
    }
  }
#endif
  return bitCount;
}

#ifndef CORRIGENDUM1
static int advanceCelpEnhSpecConf ( HANDLE_BSBITSTREAM  bitStream,CELP_ENH_SPECIFIC_CONFIG *celpEnhConf, int WriteFlag)
{
  int bitCount = 0;
  bitCount+=BsRWBitWrapper(bitStream, &(celpEnhConf->BWS_Configuration.value), celpEnhConf->BWS_Configuration.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "   audioSpC->celpEnh.BWS_config   : %ld",celpEnhConf->BWS_Configuration.value);
  return bitCount;
}
#endif


/* ---- HVXC specific config ---- */


/* AI 990616 */
static int advanceHvxcSpecConf(HANDLE_BSBITSTREAM  bitStream, HVXC_SPECIFIC_CONFIG *hvxcConf, int WriteFlag)
{
  int bitCount = 0;
#ifdef CORRIGENDUM1
  bitCount+=BsRWBitWrapper(bitStream, &(hvxcConf->isBaseLayer.value), hvxcConf->isBaseLayer.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "   audioSpC->hvxc.isBaseLayer     : %ld",hvxcConf->isBaseLayer.value);

  if (hvxcConf->isBaseLayer.value)
#endif
  {
    bitCount+=BsRWBitWrapper(bitStream, &(hvxcConf->HVXCvarMode.value), hvxcConf->HVXCvarMode.length,WriteFlag);
    bitCount+=BsRWBitWrapper(bitStream, &(hvxcConf->HVXCrateMode.value), hvxcConf->HVXCrateMode.length,WriteFlag);
    bitCount+=BsRWBitWrapper(bitStream, &(hvxcConf->extensionFlag.value), hvxcConf->extensionFlag.length,WriteFlag);
    ObjDescPrintf(WriteFlag, "   audioSpC->hvxc.HVXCvarMode     : %ld",hvxcConf->HVXCvarMode.value);
    ObjDescPrintf(WriteFlag, "   audioSpC->hvxc.HVXCrateMode    : %ld",hvxcConf->HVXCrateMode.value);
    ObjDescPrintf(WriteFlag, "   audioSpC->hvxc.extensionFlag   : %ld",hvxcConf->extensionFlag.value);
    if (hvxcConf->extensionFlag.value != 0) {	/* for "EP" tool (AI 990616) */
      /* for VR scalable mode (YM 990728) */
      bitCount+=BsRWBitWrapper(bitStream, &(hvxcConf->vrScalFlag.value), hvxcConf->vrScalFlag.length,WriteFlag);
      ObjDescPrintf(WriteFlag, "   audioSpC->hvxc.vrScalFlag      : %ld",hvxcConf->vrScalFlag.value);
    }
  }
  return bitCount;
}


/* ---- PARA specific config ---- */


/* HP20001009 */
static int advanceParaEnhSpecConf(HANDLE_BSBITSTREAM bitStream, PARA_SPECIFIC_CONFIG *paraConf, int WriteFlag)
{
  int bitCount = 0;
  bitCount+=BsRWBitWrapper(bitStream, &(paraConf->HILNenhaLayer.value), paraConf->HILNenhaLayer.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "   audioSpC->para.HILNenhaLayer   : %ld",paraConf->HILNenhaLayer.value);
  if (paraConf->HILNenhaLayer.value) {
    bitCount+=BsRWBitWrapper(bitStream, &(paraConf->HILNenhaQuantMode.value), paraConf->HILNenhaQuantMode.length,WriteFlag);
    ObjDescPrintf(WriteFlag, "   audioSpC->para.HILNQuantMode   : %ld",paraConf->HILNenhaQuantMode.value);
  }
  return bitCount;
}

static int advanceParaSpecConf(HANDLE_BSBITSTREAM  bitStream, PARA_SPECIFIC_CONFIG *paraConf, int WriteFlag)
{
  int bitCount = 0;
#ifdef CORRIGENDUM1
  bitCount+=BsRWBitWrapper(bitStream, &(paraConf->isBaseLayer.value), paraConf->isBaseLayer.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "   audioSpC->para.isBaseLayer    : %ld",paraConf->isBaseLayer.value);
  if (paraConf->isBaseLayer.value==0)
    bitCount+=advanceParaEnhSpecConf(bitStream,paraConf,WriteFlag);
  else
#endif
  {
    bitCount+=BsRWBitWrapper(bitStream, &(paraConf->PARAmode.value), paraConf->PARAmode.length,WriteFlag);
    ObjDescPrintf(WriteFlag, "   audioSpC->para.PARAmode        : %ld",paraConf->PARAmode.value);
    if (paraConf->PARAmode.value != 1) {
      bitCount+=BsRWBitWrapper(bitStream, &(paraConf->HVXCvarMode.value), paraConf->HVXCvarMode.length,WriteFlag);
      bitCount+=BsRWBitWrapper(bitStream, &(paraConf->HVXCrateMode.value), paraConf->HVXCrateMode.length,WriteFlag);
      bitCount+=BsRWBitWrapper(bitStream, &(paraConf->extensionFlag.value), paraConf->extensionFlag.length,WriteFlag);
      ObjDescPrintf(WriteFlag, "   audioSpC->para.HVXCvarMode     : %ld",paraConf->HVXCvarMode.value);
      ObjDescPrintf(WriteFlag, "   audioSpC->para.HVXCrateMode    : %ld",paraConf->HVXCrateMode.value);
      ObjDescPrintf(WriteFlag, "   audioSpC->para.extensionFlag   : %ld",paraConf->extensionFlag.value);
      if (paraConf->extensionFlag.value != 0) {	/* for "EP" tool (AI 990616) */
        bitCount+=BsRWBitWrapper(bitStream, &(paraConf->vrScalFlag.value), paraConf->vrScalFlag.length,WriteFlag);
        ObjDescPrintf(WriteFlag, "   audioSpC->para.vrScalFlag      : %ld",paraConf->vrScalFlag.value);
      }
    }
    if (paraConf->PARAmode.value != 0) {
      bitCount+=BsRWBitWrapper(bitStream, &(paraConf->HILNquantMode.value), paraConf->HILNquantMode.length,WriteFlag);
      bitCount+=BsRWBitWrapper(bitStream, &(paraConf->HILNmaxNumLine.value), paraConf->HILNmaxNumLine.length,WriteFlag);
      bitCount+=BsRWBitWrapper(bitStream, &(paraConf->HILNsampleRateCode.value), paraConf->HILNsampleRateCode.length,WriteFlag);
      bitCount+=BsRWBitWrapper(bitStream, &(paraConf->HILNframeLength.value), paraConf->HILNframeLength.length,WriteFlag);
      bitCount+=BsRWBitWrapper(bitStream, &(paraConf->HILNcontMode.value), paraConf->HILNcontMode.length,WriteFlag);
      ObjDescPrintf(WriteFlag, "   audioSpC->para.HILNQuantMode   : %ld",paraConf->HILNquantMode.value);
      ObjDescPrintf(WriteFlag, "   audioSpC->para.HILNmaxNumLine  : %ld",paraConf->HILNmaxNumLine.value);
      ObjDescPrintf(WriteFlag, "   audioSpC->para.HILNsampleRtCode: %ld",paraConf->HILNsampleRateCode.value);
      ObjDescPrintf(WriteFlag, "   audioSpC->para.HILNframeLength : %ld",paraConf->HILNframeLength.value);
      ObjDescPrintf(WriteFlag, "   audioSpC->para.HILNcontMode    : %ld",paraConf->HILNcontMode.value);
    }
    bitCount+=BsRWBitWrapper(bitStream, &(paraConf->PARAextensionFlag.value), paraConf->PARAextensionFlag.length,WriteFlag);
    ObjDescPrintf(WriteFlag, "   audioSpC->para.PARAextension   : %ld",paraConf->PARAextensionFlag.value);
  }
  return bitCount;
}

#ifdef EXT2PAR
/* ---- SSC specific config ---- */

static int advanceSSCSpecConf(HANDLE_BSBITSTREAM  bitStream, SSC_SPECIFIC_CONFIG *sscConf, int WriteFlag, int channelConfig)
{
  int bitCount = 0;

  bitCount+=BsRWBitWrapper(bitStream, &(sscConf->DecoderLevel.value)   , sscConf->DecoderLevel.length,WriteFlag);
  bitCount+=BsRWBitWrapper(bitStream, &(sscConf->UpdateRate.value)     , sscConf->UpdateRate.length,WriteFlag);
  bitCount+=BsRWBitWrapper(bitStream, &(sscConf->SynthesisMethod.value), sscConf->SynthesisMethod.length,WriteFlag);

  ObjDescPrintf(WriteFlag, "   audioSpC->ssc.DecoderLevel         : %ld",sscConf->DecoderLevel.value);
  ObjDescPrintf(WriteFlag, "   audioSpC->ssc.UpdateRate           : %ld",sscConf->UpdateRate.value);
  ObjDescPrintf(WriteFlag, "   audioSpC->ssc.SynthesisMethod      : %ld",sscConf->SynthesisMethod.value);

  if (channelConfig != 1)
  {
    bitCount+=BsRWBitWrapper(bitStream, &(sscConf->ModeExt.value)       , sscConf->ModeExt.length,WriteFlag);
    ObjDescPrintf(WriteFlag, "   audioSpC->ssc.ModeExt              : %ld",sscConf->ModeExt.value);

    if ((channelConfig == 2)&&(sscConf->ModeExt.value == 1))
    {
      bitCount+=BsRWBitWrapper(bitStream, &(sscConf->PsReserved.value), sscConf->PsReserved.length,WriteFlag);
      ObjDescPrintf(WriteFlag, "   audioSpC->ssc.PsReserved: %ld",sscConf->PsReserved.value);
    }
  }

  return bitCount;
}
#endif

#ifdef MPEG12
/* ---- MPEG12 specific config ---- */

static int advanceMPEG_1_2_SpecConf(HANDLE_BSBITSTREAM  bitStream, MPEG_1_2_SPECIFIC_CONFIG *MPEG_1_2_Conf, int WriteFlag)
{
  int bitCount = 0;

  bitCount+=BsRWBitWrapper(bitStream, &(MPEG_1_2_Conf->Extension.value)   , MPEG_1_2_Conf->Extension.length,WriteFlag);

  ObjDescPrintf(WriteFlag, "   audioSpC->MPEG_1_2_Conf.Extension  : %ld",MPEG_1_2_Conf->Extension.value);

  return bitCount;
}
#endif

#ifdef I2R_LOSSLESS
static int advanceSLSspecConf (HANDLE_BSBITSTREAM bitStream, SLS_SPECIFIC_CONFIG *slsConf, int WriteFlag, int channelConfig, int ascBits)
{
  int bitCount = 0;

  bitCount+=BsRWBitWrapper(bitStream, &(slsConf->SLSpcmWordLength.value), slsConf->SLSpcmWordLength.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "   audioSpC->sls.SLSpcmWordLength : %ld",slsConf->SLSpcmWordLength.value);

  bitCount+=BsRWBitWrapper(bitStream, &(slsConf->SLSaacCorePresent.value), slsConf->SLSaacCorePresent.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "   audioSpC->sls.SLSaacCorePresent: %ld",slsConf->SLSaacCorePresent.value);

  bitCount+=BsRWBitWrapper(bitStream, &(slsConf->SLSlleMainStream.value), slsConf->SLSlleMainStream.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "   audioSpC->sls.SLSlleMainStream : %ld",slsConf->SLSlleMainStream.value);

  bitCount+=BsRWBitWrapper(bitStream, &(slsConf->SLSreserved.value), slsConf->SLSreserved.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "   audioSpC->sls.SLSreserved      : %ld",slsConf->SLSreserved.value);

  bitCount+=BsRWBitWrapper(bitStream, &(slsConf->SLSframeLength.value), slsConf->SLSframeLength.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "   audioSpC->sls.SLSframeLength   : %ld",slsConf->SLSframeLength.value);

  if (channelConfig == 0) {
    bitCount+=advanceProgConfig (bitStream, slsConf->progConfig, WriteFlag, ascBits + bitCount );
  }

  return bitCount;
}
#endif


/* ---- EP specific config ---- */

static int advanceEpClassConf(HANDLE_BSBITSTREAM  bitStream, EP_CLASS_CONFIG *classConf, int WriteFlag, int nocf, int it)
{
  int bitCount = 0;

  bitCount+=BsRWBitWrapper(bitStream, &(classConf->lengthEscape.value), classConf->lengthEscape.length ,WriteFlag);
  bitCount+=BsRWBitWrapper(bitStream, &(classConf->rateEscape.value), classConf->rateEscape.length ,WriteFlag);
  bitCount+=BsRWBitWrapper(bitStream, &(classConf->crclenEscape.value), classConf->crclenEscape.length ,WriteFlag);
  if (nocf != 1)
    bitCount+=BsRWBitWrapper(bitStream, &(classConf->concatenateFlag.value), classConf->concatenateFlag.length ,WriteFlag);
  bitCount+=BsRWBitWrapper(bitStream, &(classConf->fecType.value), classConf->fecType.length ,WriteFlag);
  if (classConf->fecType.value == 0)
    bitCount+=BsRWBitWrapper(bitStream, &(classConf->terminationSwitch.value), classConf->terminationSwitch.length ,WriteFlag);
  if (it ==2)
    bitCount+=BsRWBitWrapper(bitStream, &(classConf->interleaveSwitch.value), classConf->interleaveSwitch.length ,WriteFlag);
  bitCount+=BsRWBitWrapper(bitStream, &(classConf->classOptional.value), classConf->classOptional.length ,WriteFlag);
  if (classConf->lengthEscape.value == 1)
    bitCount+=BsRWBitWrapper(bitStream, &(classConf->numberOfBitsForLength.value), classConf->numberOfBitsForLength.length ,WriteFlag);
  else{
    bitCount+=BsRWBitWrapper(bitStream, &(classConf->classLength.value), classConf->classLength.length ,WriteFlag);
  }
  if (classConf->rateEscape.value != 1){
    if (classConf->fecType.value)
      classConf->classRate.length = 7;
    else
      classConf->classRate.length = 5;
    bitCount+=BsRWBitWrapper(bitStream, &(classConf->classRate.value), classConf->classRate.length ,WriteFlag);
  }
  if (classConf->crclenEscape.value != 1)
    bitCount+=BsRWBitWrapper(bitStream, &(classConf->classCrclen.value), classConf->classCrclen.length ,WriteFlag);

  return bitCount;
}

static int advanceEpPredSetConf(HANDLE_BSBITSTREAM  bs, PRED_SET_CONFIG *predSetConf, int WriteFlag, int nocf, int it)
{
  int bitCount = 0;
  unsigned long i;

  bitCount+=BsRWBitWrapper(bs, &(predSetConf->numberOfClass.value), predSetConf->numberOfClass.length ,WriteFlag);
  if(!WriteFlag)
    predSetConf->epClassConfig = (EP_CLASS_CONFIG*) malloc((predSetConf->numberOfClass.value)*(sizeof(predSetConf->epClassConfig[0])));
  for (i=0; i<predSetConf->numberOfClass.value; i++) {
    initEpClassConf (&predSetConf->epClassConfig[i]);
    bitCount+=advanceEpClassConf(bs, &(predSetConf->epClassConfig[i]), WriteFlag, nocf, it);
  }
  bitCount+=BsRWBitWrapper(bs, &(predSetConf->classReorderedOutput.value), predSetConf->classReorderedOutput.length ,WriteFlag);
  if (predSetConf->classReorderedOutput.value == 1)
    for (i=0; i<predSetConf->numberOfClass.value; i++) {
      bitCount+=BsRWBitWrapper(bs, &(predSetConf->epClassConfig[i].classOutputOrder.value), predSetConf->epClassConfig[i].classOutputOrder.length ,WriteFlag);
    }
  return bitCount;
}


int advanceEpSpecConf(HANDLE_BSBITSTREAM  bs, EP_SPECIFIC_CONFIG *epConf, int WriteFlag)
{
  int bitCount = 0;
  unsigned long i;

  bitCount+=BsRWBitWrapper(bs, &(epConf->numberOfPredifinedSet.value), epConf->numberOfPredifinedSet.length ,WriteFlag);
  if (!WriteFlag) {
    epConf->predSetConfig = (PRED_SET_CONFIG*) malloc((epConf->numberOfPredifinedSet.value)*(sizeof(epConf->predSetConfig[0])));
  }
  ObjDescPrintf(WriteFlag, "   epConfig->numPredSets          : %ld",epConf->numberOfPredifinedSet.value);
  bitCount+=BsRWBitWrapper(bs, &(epConf->interleaveType.value), epConf->interleaveType.length ,WriteFlag);
  ObjDescPrintf(WriteFlag, "   epConfig->interleaveType       : %ld",epConf->interleaveType.value);
  bitCount+=BsRWBitWrapper(bs, &(epConf->bitStuffing.value), epConf->bitStuffing.length ,WriteFlag);
  ObjDescPrintf(WriteFlag, "   epConfig->bitStuffing          : %ld",epConf->bitStuffing.value);
  bitCount+=BsRWBitWrapper(bs, &(epConf->numberOfConcatenatedFrame.value), epConf->numberOfConcatenatedFrame.length ,WriteFlag);
  ObjDescPrintf(WriteFlag, "   epConfig->numConcatenatedFrame : %ld",epConf->numberOfConcatenatedFrame.value);
  for (i=0; i<epConf->numberOfPredifinedSet.value; i++) {
    initEpPredSetConf (&(epConf->predSetConfig[i]));
    bitCount+=advanceEpPredSetConf(bs, &(epConf->predSetConfig[i]), WriteFlag, epConf->numberOfConcatenatedFrame.value, epConf->interleaveType.value );
  }

  bitCount+=BsRWBitWrapper(bs, &(epConf->headerProtection.value), epConf->headerProtection.length ,WriteFlag);
  ObjDescPrintf(WriteFlag, "   epConfig->headerProtection     : %ld",epConf->headerProtection.value);
  if (epConf->headerProtection.value == 1){
    bitCount+=BsRWBitWrapper(bs, &(epConf->headerRate.value), epConf->headerRate.length ,WriteFlag);
    ObjDescPrintf(WriteFlag, "   epConfig->headerRate           : %ld",epConf->headerRate.value);
    bitCount+=BsRWBitWrapper(bs, &(epConf->headerCrclen.value), epConf->headerCrclen.length ,WriteFlag);
    ObjDescPrintf(WriteFlag, "   epConfig->headerCRClength      : %ld",epConf->headerCrclen.value);
  }

  /*bitCount+=BsRWBitWrapper(bs, &(epConf->rsFecCapability.value), epConf->rsFecCapability.length ,WriteFlag);*/
  return bitCount;
}

/* ---- */


/*public*/
int advanceAudioSpecificConfig(HANDLE_BSBITSTREAM  bs,
                               AUDIO_SPECIFIC_CONFIG *audioSpecificConfig,
                               int WriteFlag,
                               int SystemsFlag
#ifndef CORRIGENDUM1
                               ,int isEnhLayer
#endif
)
{
  int bitCount = 0;

  unsigned long tag = decSpecificInfoTag, tmp;
  signed long len;
  unsigned long sizeOfClass = 0;
  unsigned long startOfData = 0;
  DESCR_ELE  audioDecoderTypeExt;
  audioDecoderTypeExt.length = 6;     /* Number of bits for Decoder Type Extension */


  if (SystemsFlag) {  /* read class tag and length */
    if (WriteFlag == 0) {
      BsGetBitAhead (bs, &tmp, 8);
      if (tmp != tag) {
        return 1;
      }
    }

    BsRWBitWrapper (bs, &tag, 8, WriteFlag);

    switch (WriteFlag) {
      case 0: /* read length */
        for (sizeOfClass = 0, tmp = 0x80 ; tmp &= 0x80 ;
        sizeOfClass = (sizeOfClass << 7) | (tmp & 0x7F))
          BsRWBitWrapper (bs, &tmp, 8, WriteFlag);
        startOfData = BsCurrentBit( bs );
        break;
      case 1: /* write length */
      case 2: /* calculate length */
        sizeOfClass = advanceAudioSpecificConfig(bs, audioSpecificConfig, 2/*get length*/,0
#ifndef CORRIGENDUM1
                                                 ,isEnhLayer
#endif
        );
        tmp = 0;
        while (sizeOfClass >> (7*tmp)) tmp++;

        if (WriteFlag == 2) return sizeOfClass + ((tmp+1)<<3);

        while (tmp) {
          unsigned long tmp1;
          tmp--;
          tmp1 = ((sizeOfClass+7) >> (7*tmp +3)) + (tmp?0x80:0);
          BsRWBitWrapper (bs, &tmp1, 8, WriteFlag);
        }
        break;
    }
  }


  if (WriteFlag == 0) {
    /* Read AOT*/
    /* Read or Write first 5 bits of audioDecoderType */
    bitCount+=BsRWBitWrapper(bs, &(audioSpecificConfig->audioDecoderType.value), audioSpecificConfig->audioDecoderType.length, WriteFlag);

    if (audioSpecificConfig->audioDecoderType.value == AOT_ESC) {

      /* Read next 6 bits */
      bitCount+=BsRWBitWrapper(bs, &(audioDecoderTypeExt.value), audioDecoderTypeExt.length, WriteFlag);
      /* Add 32 to the value of audioDecoderTypeExt */
      audioSpecificConfig->audioDecoderType.value = 32 + audioDecoderTypeExt.value;
    }
  } else {
    /* Write AOT*/
    if (audioSpecificConfig->audioDecoderType.value > AOT_ESC) {
      /* get value for next 6 bits by subtracting 32 from audioDecoderType variable */
      audioDecoderTypeExt.value = audioSpecificConfig->audioDecoderType.value - 32;
      audioSpecificConfig->audioDecoderType.value = AOT_ESC;
    }
    /* Write first 5 bits of audioDecoderType */
    bitCount+=BsRWBitWrapper(bs, &(audioSpecificConfig->audioDecoderType.value), audioSpecificConfig->audioDecoderType.length, WriteFlag);
    if (audioSpecificConfig->audioDecoderType.value == AOT_ESC) {
      /* write content of audioDecoderTypeExt to the next 6 bits */
      bitCount+=BsRWBitWrapper(bs, &(audioDecoderTypeExt.value), audioDecoderTypeExt.length, WriteFlag);
      /* restore value of audioDecoderType */
      audioSpecificConfig->audioDecoderType.value = audioDecoderTypeExt.value + 32;
    }
  }

#ifdef CT_SBR
  if (audioSpecificConfig->audioDecoderType.value == SBR){
    ObjDescPrintf(WriteFlag,"   audioSpC->audioObjType         : %s", codecNames [audioSpecificConfig->audioDecoderType.value] );
    ObjDescPrintf(WriteFlag,"   This is the hierarchical SBR signalling. The above AOT, will be the extensionAudioObjectType.");
  }
#ifdef PARAMETRICSTEREO
  else if (audioSpecificConfig->audioDecoderType.value == PS){
    ObjDescPrintf(WriteFlag,"   audioSpC->audioObjType         : %s", codecNames [audioSpecificConfig->audioDecoderType.value] );
    ObjDescPrintf(WriteFlag,"   This is the hierarchical SBR+PS signalling. The above AOT, will be the extensionAudioObjectType.");
  }
#endif
  else{
#endif
    if (audioSpecificConfig->audioDecoderType.value < (int)(sizeof (codecNames) / sizeof (codecNames [0])))
      ObjDescPrintf(WriteFlag,"   audioSpC->audioObjType         : %s", codecNames [audioSpecificConfig->audioDecoderType.value] );
    else
      ObjDescPrintf(WriteFlag,"   audioSpC->audioObjType         : %d", audioSpecificConfig->audioDecoderType.value );
#ifdef CT_SBR
  }
#endif

  /* Read samplingFrequencyIndex*/
  bitCount+=BsRWBitWrapper(bs, &(audioSpecificConfig->samplingFreqencyIndex.value), audioSpecificConfig->samplingFreqencyIndex.length,WriteFlag);
  if (audioSpecificConfig->samplingFreqencyIndex.value == 0xf)
    /* Read samplingFrequencyValue*/
    bitCount+=BsRWBitWrapper(bs, &(audioSpecificConfig->samplingFrequency.value), audioSpecificConfig->samplingFrequency.length,WriteFlag);
  else {
    static const unsigned long samplingRate [] = {
        96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0
    };
    audioSpecificConfig->samplingFrequency.value = samplingRate [audioSpecificConfig->samplingFreqencyIndex.value] ;
  }
  ObjDescPrintf(WriteFlag, "   audioSpC->samplingFrequencyIdx : %ld",audioSpecificConfig->samplingFreqencyIndex.value);
  ObjDescPrintf(WriteFlag, "   audioSpC->samplingFrequency    : %ld",audioSpecificConfig->samplingFrequency.value);

  /* Read channelConfiguration*/
  bitCount+=BsRWBitWrapper(bs, &(audioSpecificConfig->channelConfiguration.value), audioSpecificConfig->channelConfiguration.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "   audioSpC->channelConfiguration : %ld",audioSpecificConfig->channelConfiguration.value);
  /*show avgbitrate*/


#ifdef CT_SBR
  if ( WriteFlag == 0 ) {
    /*This is the non-backwards compatible explicit signalling of SBR.*/
    audioSpecificConfig->sbrPresentFlag.value = 2;    /* Should be -1 according to the standard, however it is just an
                                                         arbitrary number apart from zero or one, and since the type is
                                                         an unsigned type -1 would be a confusing choice here. */
#ifdef PARAMETRICSTEREO
    audioSpecificConfig->psPresentFlag.value = 2;     /* same comment as for sbrPresentFlag */
#endif

    /* In order not to try to detect implicit signalling of SBR for any other AOTs than the AAC,
       we will here set the sbrPresentFlag to zero if the AOT that we have just read is not AAC or SBR.
       This is just an implementational issue and is thus not reflected in the table in the standard describing
       the audioSpecificConfig.
     */
    if(audioSpecificConfig->audioDecoderType.value != SBR        &&
#ifdef PARAMETRICSTEREO
        audioSpecificConfig->audioDecoderType.value != PS         &&
#endif
#ifdef AAC_ELD
        audioSpecificConfig->audioDecoderType.value != ER_AAC_ELD &&
#endif
        audioSpecificConfig->audioDecoderType.value != AAC_MAIN   &&
        audioSpecificConfig->audioDecoderType.value != AAC_LC     &&
        audioSpecificConfig->audioDecoderType.value != AAC_SSR    &&
        audioSpecificConfig->audioDecoderType.value != AAC_LTP    &&
        audioSpecificConfig->audioDecoderType.value != AAC_SCAL   &&
        audioSpecificConfig->audioDecoderType.value != ER_AAC_LC  &&
        audioSpecificConfig->audioDecoderType.value != ER_AAC_LTP &&
        /* 20070326 BSAC Ext.*/
        audioSpecificConfig->audioDecoderType.value != ER_BSAC	 &&
        /* 20070326 BSAC Ext.*/
        audioSpecificConfig->audioDecoderType.value != ER_AAC_SCAL){
      audioSpecificConfig->sbrPresentFlag.value = 0;
#ifdef PARAMETRICSTEREO
      audioSpecificConfig->psPresentFlag.value = 0;
#endif
    }



    if(audioSpecificConfig->audioDecoderType.value == SBR
#ifdef PARAMETRICSTEREO
        || audioSpecificConfig->audioDecoderType.value == PS
#endif
    ){
      audioSpecificConfig->extensionAudioDecoderType.value = SBR;
      audioSpecificConfig->sbrPresentFlag.value = 1;
#ifdef PARAMETRICSTEREO
      if(audioSpecificConfig->audioDecoderType.value == PS){
        audioSpecificConfig->psPresentFlag.value = 1;
      }
#endif

      /* Read extensionSamplingFrequencyIndex*/
      bitCount+=BsRWBitWrapper(bs,
                               &(audioSpecificConfig->extensionSamplingFrequencyIndex.value),
                               audioSpecificConfig->extensionSamplingFrequencyIndex.length,
                               WriteFlag);

      if (audioSpecificConfig->extensionSamplingFrequencyIndex.value == 0xf)

        /* Read extensionSamplingFrequencyValue*/
        bitCount+=BsRWBitWrapper(bs,
                                 &(audioSpecificConfig->extensionSamplingFrequency.value),
                                 audioSpecificConfig->extensionSamplingFrequency.length,
                                 WriteFlag);

      else {
        static const unsigned long samplingRate [] = { 96000, 88200, 64000, 48000, 44100,
            32000, 24000, 22050, 16000, 12000,
            11025, 8000, 7350, 0, 0, 0 };

        audioSpecificConfig->extensionSamplingFrequency.value =
          samplingRate [audioSpecificConfig->extensionSamplingFrequencyIndex.value] ;
      }
      ObjDescPrintf(WriteFlag, "   audioSpC->extensionSamplingFrequencyIdx : %ld",audioSpecificConfig->extensionSamplingFrequencyIndex.value);
      ObjDescPrintf(WriteFlag, "   audioSpC->extensionSamplingFrequency    : %ld",audioSpecificConfig->extensionSamplingFrequency.value);

      /* Read the underlying AOT.*/
      bitCount+=BsRWBitWrapper(bs,
                               &(audioSpecificConfig->audioDecoderType.value),
                               audioSpecificConfig->audioDecoderType.length,
                               WriteFlag);

      if (audioSpecificConfig->audioDecoderType.value >= AOT_ESC)
      {
        /* If the 5 bit audioDecoderType = 0x1f (=0b11111),
         * we have to read the next 6 bits and add both values for the real Type
         */
        if (WriteFlag == 0) {
          /* Read next 6 bits */
          bitCount+=BsRWBitWrapper(bs, &(audioSpecificConfig->audioDecoderType.value), audioDecoderTypeExt.length, WriteFlag);
          /* Add 32 to the value of audioDecoderType */
          audioSpecificConfig->audioDecoderType.value = 32 + audioSpecificConfig->audioDecoderType.value;

        } else {
          /* get value for next 6 bits by subtracting 32 from audioDecoderType variable */
          audioSpecificConfig->audioDecoderType.value = audioSpecificConfig->audioDecoderType.value - 32;
          /* write content of audioDecoderType to the next 6 bits */
          bitCount+=BsRWBitWrapper(bs, &(audioSpecificConfig->audioDecoderType.value), audioDecoderTypeExt.length, WriteFlag);
        }
      }

      if (audioSpecificConfig->extensionAudioDecoderType.value < (int)(sizeof (codecNames) / sizeof (codecNames [0])))
        ObjDescPrintf(WriteFlag,"   audioSpC->extensionAudioObjType         : %s", codecNames [audioSpecificConfig->extensionAudioDecoderType.value] );
      else
        ObjDescPrintf(WriteFlag,"   audioSpC->extensionAudioObjType         : %d", audioSpecificConfig->extensionAudioDecoderType.value );

      if (audioSpecificConfig->audioDecoderType.value < (int)(sizeof (codecNames) / sizeof (codecNames [0])))
        ObjDescPrintf(WriteFlag,"   audioSpC->audioObjType         : %s", codecNames [audioSpecificConfig->audioDecoderType.value] );
      else
        ObjDescPrintf(WriteFlag,"   audioSpC->audioObjType         : %d", audioSpecificConfig->audioDecoderType.value );

      /* 20070326 BSAC Ext.*/
      if(audioSpecificConfig->audioDecoderType.value == ER_BSAC) {
        bitCount+=BsRWBitWrapper(bs,
                                 &(audioSpecificConfig->extensionChannelConfiguration.value),
                                 audioSpecificConfig->extensionChannelConfiguration.length,
                                 WriteFlag);
        ObjDescPrintf(WriteFlag,"   audioSpC->extensionChannelConfiguration         : %d", audioSpecificConfig->extensionChannelConfiguration.value );
      }
      /* 20070326 BSAC Ext.*/
    }
    else{
      audioSpecificConfig->extensionAudioDecoderType.value = 0;
    }
  } else { /* WriteFlag != 0 */
    if (audioSpecificConfig->audioDecoderType.value == SBR
#ifdef PARAMETRICSTEREO
        || audioSpecificConfig->audioDecoderType.value == PS
#endif
    ) {
      unsigned long audioObjectType = AAC_LC;

      /* Write extensionSamplingFrequencyIndex*/
      bitCount+=BsRWBitWrapper(bs,
                               &(audioSpecificConfig->extensionSamplingFrequencyIndex.value),
                               audioSpecificConfig->extensionSamplingFrequencyIndex.length,
                               WriteFlag);

      if (audioSpecificConfig->extensionSamplingFrequencyIndex.value == 0xf)

        /* Write extensionSamplingFrequencyValue*/
        bitCount+=BsRWBitWrapper(bs,
                                 &(audioSpecificConfig->extensionSamplingFrequency.value),
                                 audioSpecificConfig->extensionSamplingFrequency.length,
                                 WriteFlag);

      ObjDescPrintf(WriteFlag, "   audioSpC->extensionSamplingFrequencyIdx : %ld",audioSpecificConfig->extensionSamplingFrequencyIndex.value);
      ObjDescPrintf(WriteFlag, "   audioSpC->extensionSamplingFrequency    : %ld",audioSpecificConfig->extensionSamplingFrequency.value);

      /* Write the underlying AOT.*/
      bitCount+=BsRWBitWrapper(bs,
                               &audioObjectType,
                               audioSpecificConfig->audioDecoderType.length,
                               WriteFlag);
    }
  }
#endif


  switch (audioSpecificConfig->audioDecoderType.value)
  {
    case AAC_MAIN : /* AAC Main */
    case AAC_LC :   /* AAC LC */
    case AAC_LTP :
    case AAC_SCAL :
#ifdef CT_SBR
    case SBR:
#ifdef PARAMETRICSTEREO
    case PS:
#endif
#endif
    case TWIN_VQ :
    case ER_AAC_LC:
    case ER_AAC_LTP:
    case ER_AAC_LD:
    case ER_AAC_SCAL:
    case ER_TWIN_VQ :
    case ER_BSAC :
      initTFspecConf ( &(audioSpecificConfig->specConf.TFSpecificConfig));
      if(audioSpecificConfig->extensionAudioDecoderType.value != ER_BSAC)
        bitCount+=advanceTFspecConf(bs, &(audioSpecificConfig->specConf.TFSpecificConfig), WriteFlag,
                                    audioSpecificConfig->audioDecoderType.value, audioSpecificConfig->channelConfiguration.value,
                                    bitCount);
      /* 20070326 BSAC Ext.*/
      else
        bitCount+=advanceTFspecConf(bs, &(audioSpecificConfig->specConf.TFSpecificConfig), WriteFlag,
                                    audioSpecificConfig->extensionAudioDecoderType.value, audioSpecificConfig->extensionChannelConfiguration.value,
                                    bitCount);
      /* 20070326 BSAC Ext.*/
      break;
#ifdef AAC_ELD
    case ER_AAC_ELD:
      initELDspecConf ( &(audioSpecificConfig->specConf.eldSpecificConfig));
      bitCount+=advanceELDspecConf(bs, &(audioSpecificConfig->specConf.eldSpecificConfig), audioSpecificConfig->channelConfiguration.value, WriteFlag);

      
      audioSpecificConfig->sbrPresentFlag   = audioSpecificConfig->specConf.eldSpecificConfig.ldSbrPresentFlag;
      if (audioSpecificConfig->sbrPresentFlag.value) {
        audioSpecificConfig->extensionAudioDecoderType.value  = SBR;
        audioSpecificConfig->extensionSamplingFrequency.value = audioSpecificConfig->samplingFrequency.value * (audioSpecificConfig->specConf.eldSpecificConfig.ldSbrSamplingRate.value ? 2 : 1);
      }
      break;
#endif
    case CELP :
    case ER_CELP :
#ifndef CORRIGENDUM1
      if ( isEnhLayer == 0 ) { /*CELP BaseLayer*/
#endif
        initCelpSpecConf (&(audioSpecificConfig->specConf.celpSpecificConfig));
        bitCount+=advanceCelpSpecConf(bs, &(audioSpecificConfig->specConf.celpSpecificConfig), WriteFlag,
                                      audioSpecificConfig->audioDecoderType.value);
#ifndef CORRIGENDUM1
      } else if ( audioSpecificConfig->BWS_on==1 ) {
        /*CELP BWS Enh.Layer*/
        initCelpEnhSpecConf (&(audioSpecificConfig->specConf.celpEnhSpecificConfig));
        bitCount+=advanceCelpEnhSpecConf(bs,&(audioSpecificConfig->specConf.celpEnhSpecificConfig),WriteFlag);
      } /* No SpecConf for CELP BRS Enh.Layer */
#endif
      break;
    case HVXC:	/* AI 990616 */
    case ER_HVXC :
#ifndef CORRIGENDUM1
      if ( isEnhLayer == 0 ) { /* HVXC BaseLayer(AI 991129) */
#endif
        initHvxcSpecConf (&(audioSpecificConfig->specConf.hvxcSpecificConfig));
        bitCount+=advanceHvxcSpecConf(bs,&(audioSpecificConfig->specConf.hvxcSpecificConfig),WriteFlag);
#ifndef CORRIGENDUM1
      } else {	/* HVXC Enh.Layer(AI 991129) */
        /* No SpecConf */
      }
#endif
      break;
    case ER_HILN :
    case ER_PARA :
      initParaSpecConf (&(audioSpecificConfig->specConf.paraSpecificConfig));
#ifndef CORRIGENDUM1
      if ( isEnhLayer == 0 ) /* BaseLayer */
#endif
        bitCount+=advanceParaSpecConf(bs,&(audioSpecificConfig->specConf.paraSpecificConfig),WriteFlag);
#ifndef CORRIGENDUM1
      else
        bitCount+=advanceParaEnhSpecConf(bs,&(audioSpecificConfig->specConf.paraSpecificConfig),WriteFlag);
#endif
      break;
#ifdef EXT2PAR
    case SSC:
      initSSCSpecConf(&(audioSpecificConfig->specConf.sscSpecificConfig));
      bitCount+=advanceSSCSpecConf(bs,&(audioSpecificConfig->specConf.sscSpecificConfig),
                                   WriteFlag,audioSpecificConfig->channelConfiguration.value);
      break;
#endif
#ifdef MPEG12
    case LAYER_1:
    case LAYER_2:
    case LAYER_3:
      initMPEG_1_2_SpecConf (&(audioSpecificConfig->specConf.MPEG_1_2_SpecificConfig));
      bitCount+=advanceMPEG_1_2_SpecConf(bs,&(audioSpecificConfig->specConf.MPEG_1_2_SpecificConfig),
                                         WriteFlag);
      break;
#endif
#ifdef I2R_LOSSLESS
    case SLS:
    case SLS_NCORE:
      initSLSSpecConf( &(audioSpecificConfig->slsSpecificConfig) );
      bitCount += advanceSLSspecConf (bs, &(audioSpecificConfig->slsSpecificConfig), WriteFlag, audioSpecificConfig->channelConfiguration.value, bitCount);
      break;
#endif
    case USAC:
    {
      size_t i,tmp = 0xf;
      static const unsigned long samplingRate [] = {
          96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0
      };

      UsacConfig_Init( &(audioSpecificConfig->specConf.usacConfig), WriteFlag );
      bitCount += UsacConfig_Advance(bs, &(audioSpecificConfig->specConf.usacConfig), WriteFlag);

      if(audioSpecificConfig->audioDecoderType.value == USAC){
        audioSpecificConfig->sbrPresentFlag.value = 1; 
        if ( WriteFlag == 0 ) {
          audioSpecificConfig->extensionAudioDecoderType.value = SBR;
          audioSpecificConfig->extensionSamplingFrequency.value = audioSpecificConfig->samplingFrequency.value;
          audioSpecificConfig->extensionSamplingFrequencyIndex.value = audioSpecificConfig->samplingFreqencyIndex.value;
          if (audioSpecificConfig->specConf.usacConfig.coreSbrFrameLengthIndex.value == 4) {
            audioSpecificConfig->samplingFrequency.value = audioSpecificConfig->extensionSamplingFrequency.value/4;
          } else if ( (audioSpecificConfig->specConf.usacConfig.coreSbrFrameLengthIndex.value == 2) || (audioSpecificConfig->specConf.usacConfig.coreSbrFrameLengthIndex.value == 3) ) {
            audioSpecificConfig->samplingFrequency.value = audioSpecificConfig->extensionSamplingFrequency.value/2;
          }
        }
        else {
          audioSpecificConfig->extensionAudioDecoderType.value = 0;
        }
        for ( i = 0 ; i < sizeof(samplingRate)/sizeof(samplingRate[0]) ; i++ ) {
          if ( samplingRate[i] == audioSpecificConfig->samplingFrequency.value ) {
            tmp = i;
            break;
          }
        }
        audioSpecificConfig->samplingFreqencyIndex.value = (UINT32) tmp;
      } else {
        audioSpecificConfig->sbrPresentFlag.value = 0;
      }
    } 
    break;
    default :
      CommonWarning("audioObjectType %ld not implemented\n",audioSpecificConfig->audioDecoderType.value);
      return -1;
      break;
  }

#ifndef EP_CONFIG_MISSING
  /* parse epConfig */
  if((audioSpecificConfig->audioDecoderType.value == ER_AAC_LC) || (audioSpecificConfig->audioDecoderType.value == ER_AAC_LTP) ||
      (audioSpecificConfig->audioDecoderType.value == ER_AAC_LD) || (audioSpecificConfig->audioDecoderType.value == ER_AAC_SCAL) ||
#ifdef AAC_ELD
      (audioSpecificConfig->audioDecoderType.value == ER_AAC_ELD) ||
#endif
      (audioSpecificConfig->audioDecoderType.value == ER_TWIN_VQ)|| (audioSpecificConfig->audioDecoderType.value == ER_BSAC) ||
      (audioSpecificConfig->audioDecoderType.value == ER_CELP)   || (audioSpecificConfig->audioDecoderType.value == ER_HVXC) ||
      (audioSpecificConfig->audioDecoderType.value == ER_HILN)   || (audioSpecificConfig->audioDecoderType.value == ER_PARA)) {
    bitCount+=BsRWBitWrapper(bs, &(audioSpecificConfig->epConfig.value), audioSpecificConfig->epConfig.length, WriteFlag);
    ObjDescPrintf(WriteFlag, "   audioSpC->epConfig             : %ld",audioSpecificConfig->epConfig.value);

    if (audioSpecificConfig->epConfig.value ==2 || audioSpecificConfig->epConfig.value == 3){
      initEpSpecConf(&(audioSpecificConfig->epSpecificConfig));
      bitCount+=advanceEpSpecConf(bs, &(audioSpecificConfig->epSpecificConfig), WriteFlag);
    }
    if (audioSpecificConfig->epConfig.value == 3) {
      bitCount+=BsRWBitWrapper(bs, &(audioSpecificConfig->directMapping.value), audioSpecificConfig->directMapping.length, WriteFlag);
      ObjDescPrintf(WriteFlag, "   audioSpC->directMapping        : %ld",audioSpecificConfig->directMapping.value );
    }

  }
#endif

  if (SystemsFlag) {  /* skip remaining data in class */
    if ( WriteFlag == 0 ) {
      len = (sizeOfClass<<3) - (BsCurrentBit(bs) - startOfData);
      if ( len > 0 ) {

#ifdef CT_SBR

        /*This is the backwards compatible explicit signalling of SBR,
          where the SBR data is appended at the end of the AudioSpecificConfig. */

        if (audioSpecificConfig->extensionAudioDecoderType.value != SBR && len >= 16 ) {

          /*Read the sync-word.*/
          bitCount += BsRWBitWrapper(bs,
                                     &(audioSpecificConfig->syncExtensionType.value),
                                     audioSpecificConfig->syncExtensionType.length,
                                     WriteFlag);

          if(audioSpecificConfig->syncExtensionType.value == 0x2b7){ /*Check that we got the right sync word.*/

            /* Read the extensionAudioObjectType.*/
            bitCount += BsRWBitWrapper(bs,
                                       &(audioSpecificConfig->extensionAudioDecoderType.value),
                                       audioSpecificConfig->extensionAudioDecoderType.length,
                                       WriteFlag);

            if (audioSpecificConfig->extensionAudioDecoderType.value < (int)(sizeof (codecNames) / sizeof (codecNames [0])))
              ObjDescPrintf(WriteFlag,"   audioSpC->extensionAudioObjType         : %s", codecNames [audioSpecificConfig->extensionAudioDecoderType.value] );
            else
              ObjDescPrintf(WriteFlag,"   audioSpC->extensionAudioObjType         : %d", audioSpecificConfig->extensionAudioDecoderType.value );

            if(audioSpecificConfig->extensionAudioDecoderType.value == SBR){
              /* Read the SBR present flag.*/
              bitCount += BsRWBitWrapper(bs,
                                         &(audioSpecificConfig->sbrPresentFlag.value),
                                         audioSpecificConfig->sbrPresentFlag.length,
                                         WriteFlag);

              ObjDescPrintf(WriteFlag,"   audioSpC->sbrPresentFlag                : %d", audioSpecificConfig->sbrPresentFlag.value );

              if(audioSpecificConfig->sbrPresentFlag.value == 1){

                /* Read extensionSamplingFrequencyIndex*/
                bitCount+=BsRWBitWrapper(bs,
                                         &(audioSpecificConfig->extensionSamplingFrequencyIndex.value),
                                         audioSpecificConfig->extensionSamplingFrequencyIndex.length,
                                         WriteFlag);

                if (audioSpecificConfig->extensionSamplingFrequencyIndex.value == 0xf)

                  /* Read extensionSamplingFrequencyValue*/
                  bitCount+=BsRWBitWrapper(bs,
                                           &(audioSpecificConfig->extensionSamplingFrequency.value),
                                           audioSpecificConfig->extensionSamplingFrequency.length,
                                           WriteFlag);

                else {
                  static const unsigned long samplingRate [] = { 96000, 88200, 64000, 48000, 44100,
                      32000, 24000, 22050, 16000, 12000,
                      11025, 8000, 7350, 0, 0, 0 };

                  audioSpecificConfig->extensionSamplingFrequency.value =
                    samplingRate [audioSpecificConfig->extensionSamplingFrequencyIndex.value];
                }

                ObjDescPrintf(WriteFlag, "   audioSpC->extensionSamplingFrequencyIdx : %ld",audioSpecificConfig->extensionSamplingFrequencyIndex.value);
                ObjDescPrintf(WriteFlag, "   audioSpC->extensionSamplingFrequency    : %ld",audioSpecificConfig->extensionSamplingFrequency.value);
#ifdef PARAMETRICSTEREO
                len = (sizeOfClass<<3) - (BsCurrentBit(bs) - startOfData);
                if (len >= 12 ) {
                  /*Read the sync-word.*/
                  bitCount += BsRWBitWrapper(bs,
                                             &(audioSpecificConfig->syncExtensionType.value),
                                             audioSpecificConfig->syncExtensionType.length,
                                             WriteFlag);

                  ObjDescPrintf(WriteFlag, "   syncExtensionType.value               : %03x",audioSpecificConfig->syncExtensionType.value);
                  if(audioSpecificConfig->syncExtensionType.value == 0x548){ /*Check that we got the right sync word.*/
                    /* Read the PS present flag.*/
                    bitCount += BsRWBitWrapper(bs,
                                               &(audioSpecificConfig->psPresentFlag.value),
                                               audioSpecificConfig->psPresentFlag.length,
                                               WriteFlag);

                    ObjDescPrintf(WriteFlag,"   audioSpC->psPresentFlag                 : %d", audioSpecificConfig->psPresentFlag.value );
                  }
                }
#endif
              }
            }
            /* 20070326 BSAC Ext.*/
            if(audioSpecificConfig->extensionAudioDecoderType.value == ER_BSAC){
              /* Read the SBR present flag.*/
              bitCount += BsRWBitWrapper(bs,
                                         &(audioSpecificConfig->sbrPresentFlag.value),
                                         audioSpecificConfig->sbrPresentFlag.length,
                                         WriteFlag);

              ObjDescPrintf(WriteFlag,"   audioSpC->sbrPresentFlag                : %d", audioSpecificConfig->sbrPresentFlag.value );

              if(audioSpecificConfig->sbrPresentFlag.value == 1){

                /* Read extensionSamplingFrequencyIndex*/
                bitCount+=BsRWBitWrapper(bs,
                                         &(audioSpecificConfig->extensionSamplingFrequencyIndex.value),
                                         audioSpecificConfig->extensionSamplingFrequencyIndex.length,
                                         WriteFlag);

                if (audioSpecificConfig->extensionSamplingFrequencyIndex.value == 0xf)

                  /* Read extensionSamplingFrequencyValue*/
                  bitCount+=BsRWBitWrapper(bs,
                                           &(audioSpecificConfig->extensionSamplingFrequency.value),
                                           audioSpecificConfig->extensionSamplingFrequency.length,
                                           WriteFlag);

                else {
                  static const unsigned long samplingRate [] = { 96000, 88200, 64000, 48000, 44100,
                      32000, 24000, 22050, 16000, 12000,
                      11025, 8000, 7350, 0, 0, 0 };

                  audioSpecificConfig->extensionSamplingFrequency.value =
                    samplingRate [audioSpecificConfig->extensionSamplingFrequencyIndex.value];
                }

                ObjDescPrintf(WriteFlag, "   audioSpC->extensionSamplingFrequencyIdx : %ld",audioSpecificConfig->extensionSamplingFrequencyIndex.value);
                ObjDescPrintf(WriteFlag, "   audioSpC->extensionSamplingFrequency    : %ld",audioSpecificConfig->extensionSamplingFrequency.value);
              }

              /* Read the extensionChannelConfiguration.*/
              bitCount += BsRWBitWrapper(bs,
                                         &(audioSpecificConfig->extensionChannelConfiguration.value),
                                         audioSpecificConfig->extensionChannelConfiguration.length,
                                         WriteFlag);

              ObjDescPrintf(WriteFlag,"   audioSpC->extensionChannelConfiguration                : %d", audioSpecificConfig->extensionChannelConfiguration.value );
            }
            /* 20070326 BSAC Ext.*/
          }
        }

        len = (sizeOfClass<<3) - (BsCurrentBit(bs) - startOfData);
        if ( len > 0 ) {
          ObjDescPrintf(WriteFlag, "   skipping remaining %ld bits of AudioSpecificConfig", len);
          BsGetSkip (bs, len);
        }
#else
        ObjDescPrintf(WriteFlag, "   skipping remaining %ld bits of AudioSpecificConfig", len);
        BsGetSkip (bs, len);
#endif
      }
    }
  }

#ifdef CT_SBR
  if ( WriteFlag != 0 ) {

    /*This is the backwards compatible explicit signalling of SBR,
      where the SBR data is appended at the end of the AudioSpecificConfig. */

    if (audioSpecificConfig->extensionAudioDecoderType.value == SBR) {

      /* Write the sync-word.*/
      audioSpecificConfig->syncExtensionType.value = 0x2b7;
      bitCount += BsRWBitWrapper(bs,
                                 &(audioSpecificConfig->syncExtensionType.value),
                                 audioSpecificConfig->syncExtensionType.length,
                                 WriteFlag);

      /* Write the extensionAudioObjectType.*/
      bitCount += BsRWBitWrapper(bs,
                                 &(audioSpecificConfig->extensionAudioDecoderType.value),
                                 audioSpecificConfig->extensionAudioDecoderType.length,
                                 WriteFlag);

      if (audioSpecificConfig->extensionAudioDecoderType.value < (int)(sizeof (codecNames) / sizeof (codecNames [0])))
        ObjDescPrintf(WriteFlag,"   audioSpC->extensionAudioObjType         : %s", codecNames [audioSpecificConfig->extensionAudioDecoderType.value] );
      else
        ObjDescPrintf(WriteFlag,"   audioSpC->extensionAudioObjType         : %d", audioSpecificConfig->extensionAudioDecoderType.value );

      /* Write the SBR present flag.*/
      bitCount += BsRWBitWrapper(bs,
                                 &(audioSpecificConfig->sbrPresentFlag.value),
                                 audioSpecificConfig->sbrPresentFlag.length,
                                 WriteFlag);

      ObjDescPrintf(WriteFlag,"   audioSpC->sbrPresentFlag                : %d", audioSpecificConfig->sbrPresentFlag.value );

      if(audioSpecificConfig->sbrPresentFlag.value == 1){

        /* Write extensionSamplingFrequencyIndex*/
        bitCount+=BsRWBitWrapper(bs,
                                 &(audioSpecificConfig->extensionSamplingFrequencyIndex.value),
                                 audioSpecificConfig->extensionSamplingFrequencyIndex.length,
                                 WriteFlag);

        if (audioSpecificConfig->extensionSamplingFrequencyIndex.value == 0xf)

          /* Write extensionSamplingFrequencyValue*/
          bitCount+=BsRWBitWrapper(bs,
                                   &(audioSpecificConfig->extensionSamplingFrequency.value),
                                   audioSpecificConfig->extensionSamplingFrequency.length,
                                   WriteFlag);


        ObjDescPrintf(WriteFlag, "   audioSpC->extensionSamplingFrequencyIdx : %ld",audioSpecificConfig->extensionSamplingFrequencyIndex.value);
        ObjDescPrintf(WriteFlag, "   audioSpC->extensionSamplingFrequency    : %ld",audioSpecificConfig->extensionSamplingFrequency.value);
#ifdef PARAMETRICSTEREO
        if(audioSpecificConfig->psPresentFlag.value == 1){
          /* Write the sync-word.*/
          audioSpecificConfig->syncExtensionType.value = 0x548;
          bitCount += BsRWBitWrapper(bs,
                                     &(audioSpecificConfig->syncExtensionType.value),
                                     audioSpecificConfig->syncExtensionType.length,
                                     WriteFlag);

          ObjDescPrintf(WriteFlag, "   syncExtensionType.value               : %03x",audioSpecificConfig->syncExtensionType.value);
          /* Write the PS present flag.*/
          bitCount += BsRWBitWrapper(bs,
                                     &(audioSpecificConfig->psPresentFlag.value),
                                     audioSpecificConfig->psPresentFlag.length,
                                     WriteFlag);

          ObjDescPrintf(WriteFlag,"   audioSpC->psPresentFlag                 : %d", audioSpecificConfig->psPresentFlag.value );
        }
#endif
      }
    }
  }
#endif


  return bitCount;
}


/*public*/
int advanceDecoderConfigDescriptor(HANDLE_BSBITSTREAM  bs,
                                   DEC_CONF_DESCRIPTOR *decConfig,
                                   int WriteFlag,
                                   int SystemsFlag
#ifndef CORRIGENDUM1
                                   ,int isEnhLayer
#endif
)
{
  unsigned long tag = decoderConfigDescrTag, tmp ;
  signed long len;
  unsigned long sizeOfClass = 0;
  unsigned long startOfData = 0;
  int i;

  if (SystemsFlag) {   /* read class tag and length */
    if (WriteFlag == 0) {
      BsGetBitAhead (bs, &tmp, 8);
      if (tmp != tag) {
        return 1;
      }
    }

    BsRWBitWrapper (bs, &tag, 8, WriteFlag);

    if (WriteFlag == 0) {
      for (sizeOfClass = 0, tmp = 0x80 ; tmp &= 0x80 ;
      sizeOfClass = (sizeOfClass << 7) | (tmp & 0x7F))
        BsRWBitWrapper (bs, &tmp, 8, WriteFlag);
      startOfData = BsCurrentBit( bs );
    }
  }

  BsRWBitWrapper(bs, &(decConfig->profileAndLevelIndication.value), decConfig->profileAndLevelIndication.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "   decConfig->objectTypeIndication: 0x%lx",decConfig->profileAndLevelIndication.value);
  if (decConfig->profileAndLevelIndication.value != 0x40) {
    CommonWarning("Failure: objectTypeIndication not Audio\n");
    return -1;
  }

  BsRWBitWrapper(bs, &(decConfig->streamType.value),  decConfig->streamType.length, WriteFlag) ;
  ObjDescPrintf(WriteFlag, "   decConfig->streamType          : 0x%lx",decConfig->streamType.value);
  if (decConfig->streamType.value != 0x5) {
    CommonWarning("Failure: streamType not Audio\n");
    return -1;
  }
  BsRWBitWrapper(bs, &(decConfig->upsteam.value),  decConfig->upsteam.length,WriteFlag) ;
  BsRWBitWrapper(bs, &(decConfig->specificInfoFlag.value) ,  decConfig->specificInfoFlag.length,WriteFlag) ;
  ObjDescPrintf(WriteFlag, "   decConfig->upstream            : %ld",decConfig->upsteam.value);

  BsRWBitWrapper(bs, &(decConfig->bufferSizeDB.value) ,  decConfig->bufferSizeDB.length,WriteFlag) ;
  BsRWBitWrapper(bs, &(decConfig->maxBitrate.value) ,  decConfig->maxBitrate.length,WriteFlag) ;
  BsRWBitWrapper(bs, &(decConfig->avgBitrate.value) ,  decConfig->avgBitrate.length,WriteFlag) ;
  ObjDescPrintf(WriteFlag, "   decConfig->decoderBufferSize   : %ld",decConfig->bufferSizeDB.value);
  ObjDescPrintf(WriteFlag, "   decConfig->maxBitrate          : %ld",decConfig->maxBitrate.value);
  ObjDescPrintf(WriteFlag, "   decConfig->avgBitrate          : %ld",decConfig->avgBitrate.value);

  i = advanceAudioSpecificConfig(bs, &(decConfig->audioSpecificConfig), WriteFlag, SystemsFlag
#ifndef CORRIGENDUM1
                                 ,isEnhLayer
#endif
  );
  if (i<0) return i;

  if (SystemsFlag) {   /* skip remaining data in class */
    if ( WriteFlag == 0 ) {
      len = (sizeOfClass<<3) - (BsCurrentBit(bs) - startOfData);
      if ( len > 0 ) {
        ObjDescPrintf(WriteFlag, "   skipping remaining %ld bits of DecoderConfigDescriptor", len);
        BsGetSkip (bs, len);
      }
    }
  }

  return 0;
}


static int  advanceALconfigDescriptor (HANDLE_BSBITSTREAM  bs,
                                       AL_CONF_DESCRIPTOR *al,
                                       int WriteFlag,
                                       int SystemsFlag)
{
  unsigned long tag = slConfigTag, tmp ;
  unsigned long sizeOfClass = 0;
  unsigned long startOfData = 0;

  if (SystemsFlag) {
    if (WriteFlag == 0) {
      BsGetBitAhead (bs, &tmp, 8);
      if (tmp != tag) {
        return 1;
      }
    }

    BsRWBitWrapper (bs, &tag, 8, WriteFlag);

    if (WriteFlag == 0) {
      for (sizeOfClass = 0, tmp = 0x80 ; tmp &= 0x80 ;
      sizeOfClass = (sizeOfClass << 7) | (tmp & 0x7F))
        BsRWBitWrapper (bs, &tmp, 8, WriteFlag);
      startOfData = BsCurrentBit( bs );

      ObjDescPrintf(WriteFlag, " Reading AL Config descriptor (size: %ld):", sizeOfClass);
    }
  }

  BsRWBitWrapper(bs, &(al->useAccessUnitStartFlag.value),  al->useAccessUnitStartFlag.length, WriteFlag);
  BsRWBitWrapper(bs, &(al->useAccessUnitEndFlag.value),    al->useAccessUnitEndFlag.length, WriteFlag);
  BsRWBitWrapper(bs, &(al->useRandomAccessPointFlag.value),al->useRandomAccessPointFlag.length, WriteFlag);
  BsRWBitWrapper(bs, &(al->usePaddingFlag.value),          al->usePaddingFlag.length, WriteFlag);
  BsRWBitWrapper(bs, &(al->seqNumLength.value),            al->seqNumLength.length, WriteFlag);

  if (SystemsFlag) {
    if ( WriteFlag == 0 ) {
      tmp = (sizeOfClass<<3) - (BsCurrentBit(bs) - startOfData);
      if ( tmp > 0 ) {
        ObjDescPrintf(WriteFlag, "   skipping remaining %ld bits of AL Config descriptor", tmp);
        BsGetSkip (bs, tmp);
      }
    }
  }

  return 0;
}


/*public*/
int  advanceESDescr ( HANDLE_BSBITSTREAM  bs,
                      ES_DESCRIPTOR *es,
                      int WriteFlag,
                      int SystemsFlag)
{
  unsigned long tag = elementStreamDescrTag, tmp ;
  unsigned long sizeOfClass = 0;
  unsigned long startOfData = 0;

  if (SystemsFlag) {
    if (WriteFlag == 0) {
      BsGetBitAhead (bs, &tmp, 8);
      if (tmp != tag) {
        return 1;
      }
    }

    BsRWBitWrapper (bs, &tag, 8, WriteFlag);

    if (WriteFlag == 0) {
      for (sizeOfClass = 0, tmp = 0x80 ; tmp &= 0x80 ;
      sizeOfClass = (sizeOfClass << 7) | (tmp & 0x7F))
        BsRWBitWrapper (bs, &tmp, 8, WriteFlag);
      startOfData = BsCurrentBit( bs );

      ObjDescPrintf(WriteFlag, "Reading ESD (size: %ld):", sizeOfClass);
    }
  }

  BsRWBitWrapper(bs, &(es->ESNumber.value), es->ESNumber.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "   ESD id    : #%ld",es->ESNumber.value);
  BsRWBitWrapper(bs, &(es->streamDependence.value),es->streamDependence.length,WriteFlag);
  BsRWBitWrapper(bs, &(es->URLFlag.value),  es->URLFlag.length,WriteFlag);

  BsRWBitWrapper(bs, &(es->OCRFlag.value),  es->OCRFlag.length,WriteFlag);
  BsRWBitWrapper(bs, &(es->streamPriority.value),  es->streamPriority.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "   priority  : %ld",es->streamPriority.value);

  if (es->streamDependence.value != 0) {
    BsRWBitWrapper(bs, &(es->dependsOn_Es_number.value),  es->dependsOn_Es_number.length,WriteFlag);
    ObjDescPrintf(WriteFlag, "   depends on: ESD #%ld",es->dependsOn_Es_number.value);
  } else {
    ObjDescPrintf(WriteFlag, "   depends on: no other ESD");
  }

  if (es->URLFlag.value != 0) {  /* URL is not properly stored yet */
    unsigned long tmp;
    BsRWBitWrapper(bs, &(es->URLlength.value),  es->URLlength.length,WriteFlag);
    for (tmp=0; tmp<es->URLlength.value; tmp++) {
      BsRWBitWrapper(bs, &(es->URLstring.value),  es->URLstring.length,WriteFlag);
    }
  }
  if (es->OCRFlag.value != 0) {
    BsRWBitWrapper(bs, &(es->OCR_ES_id.value),  es->OCR_ES_id.length,WriteFlag);
  }

  advanceDecoderConfigDescriptor(bs, &(es->DecConfigDescr), WriteFlag, SystemsFlag
#ifndef CORRIGENDUM1
                                 ,(es->streamDependence.value != 0)/*isEnhLayer*/
#endif
  );
  advanceALconfigDescriptor(bs, &(es->ALConfigDescriptor), WriteFlag, SystemsFlag);


  if (SystemsFlag) {
    if ( WriteFlag == 0 ) {
      tmp = (sizeOfClass<<3) - (BsCurrentBit(bs) - startOfData);
      if ( tmp > 0 ) {
        ObjDescPrintf(WriteFlag, "   skipping remaining %ld bits of ES descriptor", tmp);
        BsGetSkip (bs, tmp);
      }
    }
  }

  return 0;
}


void  advanceODescr (   HANDLE_BSBITSTREAM       bitStream, OBJECT_DESCRIPTOR *od,int WriteFlag)
{
  od->ODLength.length=32;
  od->ODescrId.length=10;
  od->streamCount.length=5;
  od->extensionFlag.length=1;

  /*   &(od->ODLength.value); is written later as soon as  length is known*/

  BsRWBitWrapper(bitStream, &(od->ODescrId.value),  od->ODescrId.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "od->id             : %ld",od->ODescrId.value);
  BsRWBitWrapper(bitStream, &(od->streamCount.value),  od->streamCount.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "od->streamCount    : %ld",od->streamCount.value);
  BsRWBitWrapper(bitStream, &(od->extensionFlag.value),  od->extensionFlag.length,WriteFlag);
  ObjDescPrintf(WriteFlag, "od->extensionFlag  : %ld",od->extensionFlag.value);
}



static int getNormalTrackCount(AUDIO_SPECIFIC_CONFIG *asc)
{
  int ret = -1;
  int aacScalefactorDataResilienceFlag;
  switch (asc->audioDecoderType.value) {
    case ER_AAC_LTP:
    case ER_AAC_LD:
#ifdef AAC_ELD
    case ER_AAC_ELD:
#endif
    case ER_AAC_LC:
    case ER_AAC_SCAL:
    {
      int singleChannelElements, channelPairElements;
      switch (asc->channelConfiguration.value) {
        case 1:
          singleChannelElements = 1;
          channelPairElements = 0;
          break;
        case 2:
          channelPairElements = 1;
          singleChannelElements = 0;
          break;
        case 3:
          singleChannelElements = 1;
          channelPairElements = 1;
          break;
        case 4:
          singleChannelElements = 2;
          channelPairElements = 1;
          break;
        case 5:
          singleChannelElements = 1;
          channelPairElements = 2;
          break;
        case 6:
          singleChannelElements = 2;
          channelPairElements = 2;
          break;
        case 7:
          singleChannelElements = 2;
          channelPairElements = 3;
          break;
        default:
          /* reserved */
          CommonWarning("channel config not supported in getNormalTrackCount()");
          return -1;
      }

      switch (asc->audioDecoderType.value) {
#ifdef AAC_ELD
        case ER_AAC_ELD:
          aacScalefactorDataResilienceFlag = asc->specConf.eldSpecificConfig.aacScalefactorDataResilienceFlag.value;
          break;
#endif
        default:
          aacScalefactorDataResilienceFlag = asc->specConf.TFSpecificConfig.aacScalefactorDataResilienceFlag.value;
          break;
      }
      if (aacScalefactorDataResilienceFlag == 1){
        ret = (4*singleChannelElements + 9*channelPairElements);
      } else {
        ret = (3*singleChannelElements + 7*channelPairElements);
      }
    }
    break;
    default:
      break;
  }
  return ret;
}



void tracksPerLayer(DEC_CONF_DESCRIPTOR *decConf, int *maxLayer, int *streamCount, int *tracksInLayer, int *extInLayer)
{
  int stream;
  int numLayer;
  int numStreams;
  int layer=0, aacLayer=0;

  if (streamCount==NULL) numStreams=0;
  else numStreams=*streamCount;
  if (maxLayer==NULL) numLayer = numStreams;
  else numLayer=*maxLayer;
  if (numLayer < 0) numLayer = numStreams;

  for (stream=0; (layer<=numLayer)&&(stream<numStreams);) {
    AUDIO_SPECIFIC_CONFIG *asc = &decConf[stream].audioSpecificConfig;
    int tracksThisLayer = tracksInLayer[layer];
    int dontIncreaseLayer = 0;
    switch (asc->audioDecoderType.value) {
      case ER_BSAC:
        /* always need all tracks, BSAC handles layer internally */
        if (asc->epConfig.value>1) {tracksThisLayer=1;break;}
      case ER_AAC_LTP:
      case ER_AAC_LD:
#ifdef AAC_ELD
      case ER_AAC_ELD:
#endif
      case ER_AAC_LC:
        tracksThisLayer = numStreams;
        break;
      case AAC_SCAL:
      case ER_AAC_SCAL:
        /* rely on layerNr */
        stream -= tracksThisLayer;
        if (aacLayer==(signed)asc->specConf.TFSpecificConfig.layerNr.value) {
          tracksThisLayer++;
          dontIncreaseLayer=1;
        } else {
          aacLayer++;
        }
        if (stream + tracksThisLayer == numStreams) {
          aacLayer++;
          dontIncreaseLayer=0;
        }
        break;
      case ER_TWIN_VQ:
        /* 2 tracks per layer with ep1 */
        if (asc->epConfig.value==1) tracksThisLayer = 2;
        else tracksThisLayer = 1;
        break;
      case ER_CELP:
      case ER_HILN:
        /* 5 tracks in base layer with ep1 */
        if ((asc->epConfig.value==1)&&(layer==0)) tracksThisLayer = 5;
        else tracksThisLayer = 1;
        break;
      case ER_HVXC:
        if (asc->epConfig.value==1) {
          if (layer==0) {
            if (asc->specConf.hvxcSpecificConfig.HVXCrateMode.value == 0) tracksThisLayer = 4;
            else tracksThisLayer = 5;
          } else tracksThisLayer = 3;
        } else tracksThisLayer = 1;
        break;
      case ER_PARA:
        if ((asc->epConfig.value==1)&&(layer==0)) {
          if ((asc->specConf.paraSpecificConfig.PARAmode.value == 0) ||
              (asc->specConf.paraSpecificConfig.PARAmode.value == 1)) {
            tracksThisLayer = 5;
          }
          if((asc->specConf.paraSpecificConfig.PARAmode.value == 2) ||
              (asc->specConf.paraSpecificConfig.PARAmode.value == 3)) {
            tracksThisLayer = 15;
          }
        } else tracksThisLayer = 1;
        break;
#ifdef I2R_LOSSLESS
      case SLS:
      case SLS_NCORE:
        /******************* TODO *************/
        /*  case AAC_SCAL:  */
        /* rely on layerNr */
        stream -= tracksThisLayer;
        if (aacLayer==(signed)asc->specConf.TFSpecificConfig.layerNr.value) {
          tracksThisLayer++;
          dontIncreaseLayer=1;
        } else {
          aacLayer++;
        }
        if (stream + tracksThisLayer == numStreams) {
          aacLayer++;
          dontIncreaseLayer=0;
        }
        break;
#endif
      default:
        tracksThisLayer = 1;
    }

    tracksInLayer[layer] = tracksThisLayer;
    stream += tracksThisLayer;
    if (!dontIncreaseLayer) {
      if (extInLayer) {
        int normalTracks = getNormalTrackCount(asc);
        if (normalTracks==-1) normalTracks = tracksThisLayer;
        extInLayer[layer] = tracksThisLayer - normalTracks;
      }
      layer++;
      if (layer<=numLayer) tracksInLayer[layer]=0;
    }
  }

  if (maxLayer) *maxLayer = (layer-1);
  if (streamCount) *streamCount = stream;
}



void removeAU(HANDLE_BSBITSTREAM  stream, int decBits, FRAME_DATA*        fd,int layer)
{
  unsigned int i;
  int AUlen = 0;
  if (fd->layer[layer].NoAUInBuffer<1)
    CommonExit(-1,"no more  AUs !!!!!");

  AUlen = fd->layer[layer].AULength[0];

  if (AUlen-decBits>7)
    DebugPrintf(2," %d Fill bits \n",AUlen-decBits);
  if (AUlen-decBits<0)
    CommonExit(-1,"more bits decoded than in AU!!!!!");

  DebugPrintf(7,"in removeAU, track %i: %i of %i bits used\n",layer,decBits,AUlen);

  BsGetSkip(stream,AUlen-decBits);
  for (i=0;i<fd->layer[layer].NoAUInBuffer-1;i++) {
    fd->layer[layer].AULength[i]=fd->layer[layer].AULength[i+1];
    fd->layer[layer].AUPaddingBits[i]=fd->layer[layer].AUPaddingBits[i+1];
  }
  fd->layer[layer].NoAUInBuffer--;

}

