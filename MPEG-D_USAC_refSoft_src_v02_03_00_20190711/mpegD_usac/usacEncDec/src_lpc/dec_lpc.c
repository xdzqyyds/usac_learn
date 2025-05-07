/**********************************************************************
MPEG-4 Audio VM
Decoder core (LPC-based)



This software module was originally developed by

Heiko Purnhagen   (University of Hannover)
Naoya Tanaka      (Matsushita Communication Industrial Co., Ltd.)
Rakesh Taori, Andy Gerrits (Philips Research Laboratories, Eindhoven, The Netherlands),
Toshiyuki Nomura           (NEC Corporation)

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

$Id: dec_lpc.c,v 1.1.1.1 2009-05-29 14:10:11 mul Exp $
LPC Decoder module
**********************************************************************/


/* =====================================================================*/
/* Standard Includes                                                    */
/* =====================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =====================================================================*/
/* MPEG-4 VM Includes                                                   */
/* =====================================================================*/
#include "all.h"
#include "allHandles.h"
#include "block.h"

#include "lpc_common.h"          /* structs                 */
#include "nok_ltp_common.h"      /* structs                 */
#include "obj_descr.h"           /* structs                 */
#include "tf_mainStruct.h"       /* structs                 */

#include "common_m4a.h"          /* common module           */
#include "cmdline.h"             /* command line module     */
#include "bitstream.h"           /* bit stream module       */
#include "lpc_common.h"
#include "dec_lpc.h"             /* decoder cores           */
#include "mp4_lpc.h"             /* common lpc defs         */


/* =====================================================================*/
/* PHILIPS Includes                                                     */
/* =====================================================================*/
#include "phi_cons.h"
#include "celp_decoder.h"

/* =====================================================================*/
/* Panasonic Includes                                                   */
/* =====================================================================*/
#include "celp_proto_dec.h"
#include "pan_celp_const.h"

/* =====================================================================*/
/* NEC Includes                                                         */
/* =====================================================================*/
#include "nec_abs_const.h"

#include "flex_mux.h"

/* =====================================================================*/
/* L O C A L     S Y M B O L     D E C L A R A T I O N S                */
/* =====================================================================*/
#define PROGVER "CELP decoder core FDAM1 16-Feb-2001"
#define SEPACHAR " ,="

/* =====================================================================*/
/* L O C A L     D A T A      D E C L A R A T I O N S                   */
/* =====================================================================*/
/* configuration inputs                                                 */
/* =====================================================================*/
extern int samplFreqIndex[];    /* undesired, but ... */

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b)) 
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif



/*
Table 5.2   CELP coder (Mode II) configuration for 8 kHz sampling rate 
MPE_Configuration | Frame_size (#samples) | nrof_subframes | sbfrm_size (#samples) 
0,1,2        320          4   80 
3,4,5        240          3   80 
6 ... 12     160          2   80  
13 ... 21    160          4   40 
22 ... 26    80           2   40 
27           240          4   60 
28 ... 31 Reserved
*/



static long PostFilterSW;
extern int CELPdecDebugLevel;   /* HP 971120 */

static CmdLineSwitch switchList[] =
{
  {"h",NULL,NULL,NULL,NULL,"print help"},
  {"lpc_p",&PostFilterSW,"%d","0",NULL,NULL},
  {"lpc_d",&CELPdecDebugLevel,"%d","0",NULL,"debug level"},
  {NULL,NULL,NULL,NULL,NULL,NULL}
};

/* =====================================================================*/
/* instance context                                                     */
/* =====================================================================*/

static  void    *InstanceContext;   /*handle to one instance context */


/* =====================================================================*/
/* L O C A L    F U N C T I O N      D E F I N I T I O N S              */
/* =====================================================================*/
/* ---------------------------------------------------------------------*/
/* DecLpcInfo()                                                         */
/* Get info about LPC-based decoder core.                               */
/* ---------------------------------------------------------------------*/
char *DecLpcInfo (FILE *helpStream)       
{
    if (helpStream != NULL)
    {
      fprintf(helpStream,
        PROGVER "\n"
        "decoder parameter string format:\n"
        "  list of tokens (tokens separated by characters in \"%s\")\n",
        SEPACHAR);
      CmdLineHelp(NULL,NULL,switchList,helpStream);
    }
    return PROGVER;
}

/* ------------------------------------------------------------------- */
/* DecLpcInit()                                                        */
/* Init LPC-based decoder core.                                        */
/* ------------------------------------------------------------------- */
void DecLpcInit(char*           decPara,
                FRAME_DATA*     fD,
                LPC_DATA*       lpcData
)             
{
  int parac;
  char **parav;
  int result;
  int numChannel =0;
  double frameLengthTime;
  HANDLE_BSBITSTREAM hdrStream=NULL;
  AUDIO_SPECIFIC_CONFIG* audSpC ;
  int i;
  int numLayer, numTrack;
  int coreDecType, enhDecType;
  int layer=0; 
  CELP_SPECIFIC_CONFIG *celpConf;
  CELP_SPECIFIC_CONFIG *erCelpConf;
  int  DecEnhStage;
  int  DecBwsMode;
  int celpAacMode; 
  
  
  coreDecType = fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value;
  enhDecType = fD->od->ESDescriptor[fD->od->streamCount.value-1]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value;
  
  lpcData->SampleRateMode     = 1;  /* Default: 16 kHz */
  lpcData->QuantizationMode   = 1;  /* Default: Vector Quantizer */
  lpcData->FineRateControl    = 0;  /* Default: FineRateControl is OFF */
  lpcData->LosslessCodingMode = 0;  /* Default: Lossless coding is OFF */
  lpcData->Wideband_VQ = Optimized_VQ;
  
  if ( coreDecType == enhDecType ) {
    celpAacMode = 0; /* CELP only */
  }
  else {
    celpAacMode = 1; /* CELP core in AAC Scalable */
  }
  
  numTrack = fD->od->streamCount.value;
  lpcData->outputLayerNumber = fD->scalOutSelect;
  if ( celpAacMode ) { /* To be fixed */
    numLayer = numTrack;
    lpcData->outputTrackNumber = lpcData->outputLayerNumber;
  } else {
    audSpC = &fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig;
    if ( (audSpC->audioDecoderType.value == CELP) ||
     (audSpC->audioDecoderType.value == ER_CELP && audSpC->epConfig.value == 0) ) {
      numLayer = numTrack;
      lpcData->outputTrackNumber = lpcData->outputLayerNumber;
    } else {
      numLayer = numTrack - 4;
      if ( numLayer < 1 ) {
        CommonExit(1,"wrong fD->od->streamCount");
      }
      lpcData->outputTrackNumber = lpcData->outputLayerNumber + 4;
      if ( lpcData->outputTrackNumber >= numTrack ) {
        lpcData->outputTrackNumber = numTrack - 1;
        lpcData->outputLayerNumber = lpcData->outputTrackNumber - 4;
      }
    }
    if ( numLayer > 5 || numTrack > 9 ) {
      CommonExit(1,"wrong fD->od->streamCount");
    }
  }
  
  if ( lpcData->outputLayerNumber >= numLayer ){
    CommonExit(1,"wrong fD->scalOutSelect");
  }
  
  if ( lpcData->outputTrackNumber >= numTrack ){
    CommonExit(1,"wrong fD->scalOutSelect");
  }
  
  if ( coreDecType == CELP ) {
    lpcData->dec_objectVerType = CelpObjectV1;
  }
  else if ( coreDecType == ER_CELP ) {
    lpcData->dec_objectVerType = CelpObjectV2;
  }
  
  audSpC = &fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig;
  
  switch (audSpC->channelConfiguration.value) {
  case 1   :  numChannel=1;       
    break;
  case 2   :  numChannel=2; /* two channel configuration does not yet work properly! */
    break;
  default  :  CommonExit(1,"wrong channel config");
  }
  
  
  lpcData->coreBitBuf = BsAllocBuffer( 4000 );      /* just some large number. For G729 required: 160 */
  
  /* allocate the correct amount of memory even for tho channel core */
  /* Rem.: two-channel core configuration does not yet work properly */
  if((lpcData->sampleBuf=(float**)(malloc(numChannel * sizeof (float*) )))==NULL) {
    CommonExit(1, "Memory allocation error in enc_lpc");
  }
  
  for(i=0; i<numChannel; i++){
    if((lpcData->sampleBuf[i] =(float*)malloc(sizeof(float)*1024))==NULL){
      CommonExit(1, "Memory allocation error in enc_lpc");
    }
  }    
  
  
  if (!celpAacMode ) {
    for ( i = 1; i < numTrack; i++ ) {
      if ( fD->od->ESDescriptor[i]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value != 1 ){
        CommonExit(1,"wrong channel config");
      }
    }
  }
    
  /* evalute decoder parameter string */
  parav = CmdLineParseString(decPara,SEPACHAR,&parac);
  result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);
  
  
  if (result==1) {
    DecLpcInfo(stdout);
    exit (1);
  }
  
  
  
  if ( lpcData->dec_objectVerType == CelpObjectV1 ) {
    
    celpConf= &fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig;
    
    lpcData->ExcitationMode           =   celpConf->excitationMode.value ;
    lpcData->SampleRateMode           =   celpConf->sampleRateMode.value ;
    lpcData->FineRateControl          =   celpConf->fineRateControl.value;
    lpcData->RPE_configuration        =   celpConf->RPE_Configuration.value;
    lpcData->MPE_Configuration        =   celpConf->MPE_Configuration.value ;
    lpcData->NumEnhLayers             =   celpConf->numEnhLayers.value;
    lpcData->BandwidthScalabilityMode =   celpConf->bandwidthScalabilityMode.value;
    
    lpcData->SilenceCompressionSW     = OFF;
  } 
  else  {
    erCelpConf= &fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig;
    
    lpcData->ExcitationMode           =   erCelpConf->excitationMode.value ;
    lpcData->SampleRateMode           =   erCelpConf->sampleRateMode.value ;
    lpcData->FineRateControl          =   erCelpConf->fineRateControl.value;
    lpcData->RPE_configuration        =   erCelpConf->RPE_Configuration.value;
    lpcData->MPE_Configuration        =   erCelpConf->MPE_Configuration.value ;
    lpcData->NumEnhLayers             =   erCelpConf->numEnhLayers.value;
    lpcData->BandwidthScalabilityMode =   erCelpConf->bandwidthScalabilityMode.value;
    lpcData->SilenceCompressionSW     =   erCelpConf->silenceCompressionSW.value;
  }
  
  if ((lpcData->ExcitationMode == MultiPulseExc) && ( lpcData->SampleRateMode==fs16kHz) ) {
    lpcData->Wideband_VQ = Optimized_VQ;
  }
  
  
  /* BandwidthScalabilityMode shouldn't be left uninitialised   HP20001025 */
  if (fD->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.excitationMode.value != MultiPulseExc)
    lpcData->BandwidthScalabilityMode = OFF;
  
  if ( !celpAacMode && (lpcData->BandwidthScalabilityMode == ON) )  {
#ifdef CORRIGENDUM1
    CELP_SPECIFIC_CONFIG *celpEnhConf = &fD->od->ESDescriptor[numTrack-1]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig;
#else
    CELP_ENH_SPECIFIC_CONFIG *celpEnhConf = &fD->od->ESDescriptor[numTrack-1]->DecConfigDescr.audioSpecificConfig.specConf.celpEnhSpecificConfig;
#endif
    lpcData->BWS_configuration = celpEnhConf->BWS_Configuration.value;
  }
  
  if (!celpAacMode) {
    if (lpcData->outputLayerNumber > (lpcData->NumEnhLayers+lpcData->BandwidthScalabilityMode)) {
      CommonExit(1,"wrong fD->scalOutSelect");
    }
    
      if (lpcData->BandwidthScalabilityMode == ON)   {
        if (lpcData->outputLayerNumber <= lpcData->NumEnhLayers) {
          DecEnhStage = lpcData->outputLayerNumber;
          DecBwsMode = 0;
        }
        else {
          DecEnhStage = lpcData->NumEnhLayers;
          DecBwsMode = 1;
        }
      } 
      else {
        DecEnhStage = lpcData->outputLayerNumber;
        DecBwsMode = 0;
      }
  } 
  else{
    lpcData->outputLayerNumber = min(lpcData->outputLayerNumber,lpcData->NumEnhLayers);
    lpcData->outputTrackNumber = lpcData->outputLayerNumber;
    if ( audSpC->audioDecoderType.value == ER_CELP && audSpC->epConfig.value >= 1 ) {
      lpcData->outputTrackNumber += 4;
    }
    DecEnhStage = 0;
      DecBwsMode = 0;
  }
  
  if (lpcData->NumEnhLayers>0){ 
    DecEnhStage = min(lpcData->outputLayerNumber,lpcData->NumEnhLayers);
  }
  
  lpcData->NumEnhLayers = DecEnhStage;
  lpcData->BandwidthScalabilityMode = DecBwsMode;
  
  
  /* ---------------------------------------------------------------- */
  /* Decoder Initialisation                                           */
  /* ---------------------------------------------------------------- */
  celp_initialisation_decoder (hdrStream, 
                               DecEnhStage, 
                               DecBwsMode, 
                               PostFilterSW, 
                               &lpcData->frame_size, 
                               &lpcData->n_subframes, 
                               &lpcData->sbfrm_size,      
                               &lpcData->lpc_order,
                               &lpcData->num_lpc_indices,
                               &lpcData->num_shape_cbks,
                               &lpcData->num_gain_cbks,
                               &lpcData->org_frame_bit_allocation,
                               &lpcData->ExcitationMode,
                               &lpcData->SampleRateMode, 
                               &lpcData->QuantizationMode, 
                               &lpcData->FineRateControl, 
                               &lpcData->LosslessCodingMode, 
                               &lpcData->RPE_configuration, 
                               &lpcData->Wideband_VQ, 
                               &lpcData->MPE_Configuration, 
                               &lpcData->NumEnhLayers, 
                               &lpcData->BandwidthScalabilityMode, 
                               &lpcData->BWS_configuration,
                               &lpcData->SilenceCompressionSW,
                               lpcData->dec_objectVerType,
                               &InstanceContext);
  
  lpcData->frameNumSample = lpcData->frame_size;
  lpcData->delayNumSample = 0;
  
  lpcData-> bit_rate = (long)(fD->od->ESDescriptor[0]->DecConfigDescr.avgBitrate.value +.5);

  /* Calculate total core bitrate */
  
  for (i = 1; i <= lpcData->outputTrackNumber; i++){
    lpcData->bit_rate += (long)(fD->od->ESDescriptor[i]->DecConfigDescr.avgBitrate.value +.5);
  }
  
  
  lpcData->sampling_frequency = (long)(samplFreqIndex[fD->od->ESDescriptor[lpcData->outputTrackNumber]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value]+.5);
  /* HP20001012 samplingFreqencyIndex==0xf not handled */
  
  if (lpcData->sampling_frequency==7350) {
    lpcData->sampling_frequency=8000;
  }
  
  frameLengthTime = (double)lpcData->frameNumSample/(double)lpcData->sampling_frequency;
  lpcData->bitsPerFrame = (int)(lpcData->bit_rate*frameLengthTime);     
  
  if (!celpAacMode) {
    fD->od->streamCount.value = lpcData->outputTrackNumber + 1;
    fD->scalOutSelect = lpcData->outputLayerNumber;
  } 
  /* streamCount and scalOutSelect should be correctly set in
     somewhere (scal_dec_frame.c ?) */
}




/* ------------------------------------------------------------------- */
/* DecLpcFrameNew()                                                    */
/* Decode one bit stream frame into one audio frame with               */
/* LPC-based decoder core.                                             */
/* ------------------------------------------------------------------- */

void DecLpcFrame(FRAME_DATA*    fD,
                 LPC_DATA*       lpcData,
                 int            *usedNumBit)        /* out: num bits used for this frame         */
{
  int i, tempNumBit;
  HANDLE_BSBITSTREAM bitStream[5+4];  
  

  for (i = 0; i <= lpcData->outputTrackNumber; i++)    {
    bitStream[i] = BsOpenBufferRead(fD->layer[i].bitBuf) ; 
  }
  
  /* ----------------------------------------------------------------- */
  /*Call Decoder                                                       */
  /* ----------------------------------------------------------------- */
  celp_decoder (bitStream, lpcData->sampleBuf, lpcData->ExcitationMode, lpcData->SampleRateMode, 
                lpcData->QuantizationMode, lpcData->FineRateControl,
                lpcData->RPE_configuration, lpcData->Wideband_VQ, 
                lpcData->BandwidthScalabilityMode,
                lpcData->MPE_Configuration,
                lpcData->SilenceCompressionSW,
                lpcData->dec_objectVerType,
                fD->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.epConfig.value,
                lpcData->frame_size, lpcData->n_subframes, lpcData->sbfrm_size, lpcData->lpc_order,           
                lpcData->num_lpc_indices, lpcData->num_shape_cbks, lpcData->num_gain_cbks,           
                lpcData->org_frame_bit_allocation,
                InstanceContext);
  
  *usedNumBit = 0;
  
  for ( i = 0; i <= lpcData->outputTrackNumber; i++ ){
    tempNumBit = BsCurrentBit(bitStream[i]);
    removeAU(bitStream[i],tempNumBit,fD,i);
    *usedNumBit += tempNumBit;
    BsCloseRemove(bitStream[i],1);
  }
  /*  BsClose(bitStream); */
  /* #pragma message ("BsCloseRemove must be used instead of BsClose")*/
}


/* ------------------------------------------------------------------- */
/* DeLpcrFree()                                                        */
/* Free memory allocated by LPC-based decoder core.                    */
/* ------------------------------------------------------------------- */

void DecLpcFree ( LPC_DATA*       lpcData)
{
  celp_close_decoder (lpcData->ExcitationMode, 
                      lpcData->BandwidthScalabilityMode, 
                      lpcData->org_frame_bit_allocation, 
                      &InstanceContext);
}


/* ------------------------------------------------------------------- */
/* lpcframelength()                                                    */
/*                                                                     */
/* ------------------------------------------------------------------- */

int lpcframelength (CELP_SPECIFIC_CONFIG    *celpConf, 
                    int                     layer )
{
  int frame_size = 0;
  
  if (celpConf->excitationMode.value == RegularPulseExc)    {
    if (celpConf->RPE_Configuration.value == 0)  {
      frame_size = FIFTEEN_MS;
    }
    else if (celpConf->RPE_Configuration.value == 1)  {
      frame_size  = TEN_MS;
    }
    else if (celpConf->RPE_Configuration.value == 2)   {
      frame_size  = FIFTEEN_MS;
    }
    else if (celpConf->RPE_Configuration.value == 3)   {
      frame_size  = FIFTEEN_MS;
    }
    else   {
      fprintf(stderr, "ERROR: Illegal RPE Configuration\n");
      CommonExit(1,"celp error"); 
    }
  }
  
  if (celpConf->excitationMode.value == MultiPulseExc){
    if (celpConf->sampleRateMode.value == fs8kHz) {
      if (celpConf->MPE_Configuration.value < 3){
        frame_size = NEC_FRAME40MS;
      }
      
      if (celpConf->MPE_Configuration.value >= 3 && celpConf->MPE_Configuration.value < 6){
        frame_size = NEC_FRAME30MS;
      }
      
      if (celpConf->MPE_Configuration.value >= 6 && celpConf->MPE_Configuration.value < 22){
        frame_size = NEC_FRAME20MS;
      }
      
      if (celpConf->MPE_Configuration.value >= 22 && celpConf->MPE_Configuration.value < 27){
        frame_size = NEC_FRAME10MS;
      }
      
      if (celpConf->MPE_Configuration.value == 27){
        frame_size = NEC_FRAME30MS;
      }
      
      if (celpConf->MPE_Configuration.value > 27){
        fprintf(stderr,"Error: Illegal MPE Configuration.\n");
        CommonExit(1,"celp error"); 
      }
      
      if (celpConf->bandwidthScalabilityMode.value == ON){
        if (layer == (int) (celpConf->numEnhLayers.value + 1)) {
          frame_size *= 2;
        }
      }
    }
    
    if (celpConf->sampleRateMode.value == fs16kHz) {
      if (celpConf->MPE_Configuration.value < 16){
        frame_size = NEC_FRAME20MS_FRQ16;
      }
      
      if (celpConf->MPE_Configuration.value >= 16 && celpConf->MPE_Configuration.value < 32) {
        frame_size = NEC_FRAME10MS_FRQ16;
      }
      
      if (celpConf->MPE_Configuration.value == 7 || celpConf->MPE_Configuration.value == 23){
        fprintf(stderr,"Error: Illegal BitRate configuration.\n");
        CommonExit(1,"celp error"); 
      }
    }
  }
  
  return (frame_size);
}



/* ------------------------------------------------------------------- */
/* erlpcframelength()                                                  */
/*                                                                     */
/* ------------------------------------------------------------------- */
int erlpcframelength (CELP_SPECIFIC_CONFIG   *erCelpConf, 
                      int                       layer )
{
  int frame_size = 0;
  
  if (erCelpConf->excitationMode.value == RegularPulseExc){
    if (erCelpConf->RPE_Configuration.value == 0)   {
      frame_size = FIFTEEN_MS;
    }
    else if (erCelpConf->RPE_Configuration.value == 1) {
      frame_size  = TEN_MS;
    }
    else if (erCelpConf->RPE_Configuration.value == 2) {
      frame_size  = FIFTEEN_MS;
    }
    else if (erCelpConf->RPE_Configuration.value == 3) {
      frame_size  = FIFTEEN_MS;
    }
    else{
      fprintf(stderr, "ERROR: Illegal RPE Configuration\n");
      CommonExit(1,"celp error"); 
    }
  }
  
  if ( erCelpConf->excitationMode.value == MultiPulseExc ) {
    if ( erCelpConf->sampleRateMode.value == fs8kHz) {
      if ( erCelpConf->MPE_Configuration.value < 3 ) {
        frame_size = NEC_FRAME40MS;
      }
      
      if ( erCelpConf->MPE_Configuration.value >= 3 && erCelpConf->MPE_Configuration.value < 6 ) {
        frame_size = NEC_FRAME30MS;
      }
      
      if ( erCelpConf->MPE_Configuration.value >= 6 && erCelpConf->MPE_Configuration.value < 22 ) {
        frame_size = NEC_FRAME20MS;
      }
      
      if ( erCelpConf->MPE_Configuration.value >= 22 && erCelpConf->MPE_Configuration.value < 27 ) {
        frame_size = NEC_FRAME10MS;
      }
      
      if ( erCelpConf->MPE_Configuration.value == 27 ) {
        frame_size = NEC_FRAME30MS;
      }
      
      if ( erCelpConf->MPE_Configuration.value > 27 ) {
        fprintf(stderr,"Error: Illegal MPE Configuration.\n");
        CommonExit(1,"celp error"); 
      }
      
      if ( erCelpConf->bandwidthScalabilityMode.value == ON ) {
        if ( layer == (int) (erCelpConf->numEnhLayers.value+1) ) frame_size *= 2;
      }
    }
    
    if ( erCelpConf->sampleRateMode.value == fs16kHz) {
      if ( erCelpConf->MPE_Configuration.value < 16 ) {
        frame_size = NEC_FRAME20MS_FRQ16;
      }
      
      if ( erCelpConf->MPE_Configuration.value >= 16 && erCelpConf->MPE_Configuration.value < 32 ) {
        frame_size = NEC_FRAME10MS_FRQ16;
      }
      
      if ( erCelpConf->MPE_Configuration.value == 7 || erCelpConf->MPE_Configuration.value == 23 ) {
        fprintf(stderr,"Error: Illegal BitRate configuration.\n");
        CommonExit(1,"celp error"); 
      }
    }
  }
  
  return( frame_size );
}

/* end of dec_lpc.c */
