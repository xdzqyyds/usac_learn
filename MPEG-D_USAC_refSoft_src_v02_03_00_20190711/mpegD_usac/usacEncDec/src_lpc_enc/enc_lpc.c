/*====================================================================*/
/*         MPEG-4 Audio (ISO/IEC 14496-3) Copyright Header            */
/*====================================================================*/
/*
This software module was originally developed by HP, NT, AG and TN in
the course of development of the MPEG-4 Audio (ISO/IEC 14496-3). This
software module is an implementation of a part of one or more MPEG-4
Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
(ISO/IEC 14496-3). ISO/IEC gives users of the MPEG-4 Audio (ISO/IEC
14496-3) free license to this software module or modifications thereof
for use in hardware or software products claiming conformance to the
MPEG-4 Audio (ISO/IEC 14496-3). Those intending to use this software
module in hardware or software products are advised that its use may
infringe existing patents. The original developer of this software
module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not
released for non MPEG-4 Audio (ISO/IEC 14496-3) conforming products.
CN1 retains full right to use the code for his/her own purpose, assign
or donate the code to a third party and to inhibit third parties from
using the code for non MPEG-4 Audio (ISO/IEC 14496-3) conforming
products.  This copyright notice must be included in all copies or
derivative works. Copyright 1996.
*/
/**********************************************************************
MPEG-4 Audio VM
Encoder core (LPC-based)

This software module was originally developed by

Heiko Purnhagen (University of Hannover)
Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.)
Rakesh Taori, Andy Gerrits (Philips Research Laboratories, Eindhoven, The Netherlands),
Toshiyuki Nomura (NEC Corporation)
Ralf Funken (Philips Consumer Electronics)

and edited by

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

Copyright (c) 1996.

Source file: enc_lpc.c


Required modules:
common.o         common module
cmdline.o        command line module
bitstream.o      bits stream module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
NT    Naoya Tanaka, Panasonic <natanaka@telecom.mci.mei.co.jp>
AG    Andy Gerrits, Philips <gerritsa@natlab.research.philips.com>
TN    Toshiyuki Nomura, NEC <t-nomura@dsp.cl.nec.co.jp>
RF    Ralf Funken, Philips <Ralf.Funken@philips.com>

Changes:
20-jun-96   HP    dummy core
14-aug-96   HP    added EncLpcInfo(), EncLpcFree()
15-aug-96   HP    adapted to new enc.h
26-aug-96   HP    CVS
03-Sep-96   NT    LPC-abs Core Ver. 1.11
26-Sep-96   AG    adapted for PHILIPS
25-oct-96   HP    joined Panasonic and Philips LPC frameworks
                  made function names unique
                  e.g.: abs_lpc_decode() -> PAN_abs_lpc_decode()
                  NOTE: pan_xxx() and PAN_xxx() are different functions!
                  I changed the PAN (and not the PHI) function names
                  just because thus less files had to be modified ...
28-oct-96   HP    added auto-select for pan/phi lpc modes
08-Nov-96   NT    Narrowband LPC Ver. 2.00
                    - replased preprocessing, LP analysis and weighting modules
                      with Philips modules
                    - I/Fs revision for common I/Fs with Philips modules
15-nov-96   HP    adapted to new bitstream module
26-nov-96   AG    abs_xxx() changed to celp_xxx()
27-Feb-97   TN    adapted to Rate Control functionality
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
#include "common_m4a.h"     /* common module                            */
#include "cmdline.h"        /* command line module                      */
#include "bitstream.h"      /* bit stream module                        */
#include "enc_lpc.h"        /* encoder cores                            */
#include "mp4_lpc.h"        /* common lpc defs                          */
#include "flex_mux.h"       
                            
#include "lpc_common.h"      /* HP 961025 */

/* =====================================================================*/
/* PHILIPS Includes                                                     */
/* =====================================================================*/
#include "phi_cons.h"
#include "celp_encoder.h"

/* =====================================================================*/
/* Panasonic Includes                                                   */
/* =====================================================================*/
#include "celp_proto_enc.h"
#include "pan_celp_const.h"

/* =====================================================================*/
/* NEC Includes                                                         */
/* =====================================================================*/
#include "nec_abs_const.h"

/* =====================================================================*/
/* L O C A L     S Y M B O L     D E C L A R A T I O N S                */
/* =====================================================================*/
#define PROGVER "CELP encoder core FDAM1 16-Feb-2001"
#define SEPACHAR " ,="

/* =====================================================================*/
/* L O C A L     D A T A      D E C L A R A T I O N S                   */
/* =====================================================================*/
/* configuration inputs                                                 */
/* =====================================================================*/
static long bit_rate;
static long sampling_frequency;
static long frame_size;
static long n_subframes;
static long sbfrm_size;
static long lpc_order;
static long num_lpc_indices;

static long num_shape_cbks;
static long num_gain_cbks;

static long n_lpc_analysis;
static long *window_offsets;
static long *window_sizes;
static long n_lag_candidates; 
static float min_pitch_frequency;    
static float max_pitch_frequency;    
static long *org_frame_bit_allocation;    

static long QuantizationMode; 
static long FineRateControl;  
static long LosslessCodingMode;

static long SampleRateMode;
static long RPE_configuration;
static long Wideband_VQ;
static long MPE_Configuration;
static long NumEnhLayers;
static long BandwidthScalabilityMode;
static long BWS_configuration;
static long BWS_nb_bitrate;
static long PreProcessingSW;

static long ExcitationMode;
static long SilenceCompressionSW;
static long InputConfiguration = -1;

extern int CELPencDebugLevel;   /* HP 971120 */

static int mp4ffFlag;

static CmdLineSwitch switchList[] = {
    {"h",NULL,NULL,NULL,NULL,"print help"},
    {"e",&ExcitationMode,"%d",NULL,NULL,"Multi-Pulse Excitation (0) or Regular-Pulse Excitation (1)"},
    {"n",&NumEnhLayers,"%d",NULL,NULL,"Number of enhancement layers (0,1,2,3)"},
    {"f",&FineRateControl,"%d",NULL,NULL,"Fine Rate Control OFF (0) or ON (1)"},
    {"b",&BWS_nb_bitrate,"%d",NULL,NULL,"Bitrate for narrowband core in the BandWidthScalable coder"},
    {"p",&PreProcessingSW,"%d",NULL,NULL,"Pre-processing OFF (0) or ON (1)"},
    {"s",&SilenceCompressionSW,"%d",NULL,NULL,"Silence Compression OFF (0) or ON (1)"},
    {"cf",&InputConfiguration,"%d",NULL,NULL,"RPE/MPE Configuration\n"
                                             "0,...,3 for RPE\n"
                                             "0,...,31 for MPE"},
    {"d",&CELPencDebugLevel,"%d","0",NULL,"debug level"},
    {"-mp4ff",&mp4ffFlag,NULL,NULL,NULL,"use system interface(flexmux)"},
    {NULL,NULL,NULL,NULL,NULL,NULL}
};

static int enc_objectVerType;

/* =====================================================================*/
/* instance context                                                     */
/* =====================================================================*/

static  void    *InstanceContext;   /*handle to one instance context */


/* =====================================================================*/
/* L O C A L    F U N C T I O N      D E F I N I T I O N S              */
/* =====================================================================*/
/* ---------------------------------------------------------------------*/
/* EncLpcInfo()                                                         */
/* Get info about LPC-based encoder core.                               */
/* ---------------------------------------------------------------------*/
char *EncLpcInfo (FILE *helpStream)    /* in: print encPara help text to helpStream    */
                                       /*     if helpStream not NULL                   */
                                       /* returns: core version string                 */
{
    if (helpStream != NULL)
    {
	fprintf(helpStream,
		PROGVER "\n"
		"encoder parameter string format:\n"
		"  list of tokens (tokens separated by characters in \"%s\")\n",
		SEPACHAR);
	CmdLineHelp(NULL,NULL,switchList,helpStream);
    }  
    
    return PROGVER;
}

/* ---------------------------------------------------------------------*/
/* EncLpcInit()                                                         */
/* Init LPC-based encoder core.                                         */
/* ---------------------------------------------------------------------*/

void EncLpcInit (int                numChannel,               /* in: num audio channels           */
                 float              fSample,                  /* in: sampling frequency [Hz]      */
                 float              bitRate,                  /* in: total bit rate [bit/sec]     */
                 char*              encPara,                  /* in: encoder parameter string     */
                 int                varBitRate,               /* in: variable bit rate            */
                 int*               frameNumSample,           /* out: num samples per frame       */
                 int*               delayNumSample,           /* out: encoder delay (num samples) */
                 HANDLE_BSBITBUFFER        bitHeader,                /* out: header for bit stream       */
                 ENC_FRAME_DATA*    frameData,                /* out: system interface            */
                 int                mainDebugLevel 
                 )
{
    int parac;
    char **parav;
    int result;
    HANDLE_BSBITSTREAM hdrStream;
    
    int layer, numLayer, bitRateLay;
    
    /* default selection */
    Wideband_VQ = Optimized_VQ;
    QuantizationMode = VectorQuantizer;
    LosslessCodingMode = OFF;
    if (fSample < 12000.)
    {
        ExcitationMode = MultiPulseExc;
        SampleRateMode = fs8kHz;
        FineRateControl  = OFF;
        NumEnhLayers = 0;
        BandwidthScalabilityMode = OFF;
        BWS_nb_bitrate = 0;
    }
    else
    {
        ExcitationMode = RegularPulseExc;
        SampleRateMode = fs16kHz;
        FineRateControl = OFF;
        NumEnhLayers = 0;
        BandwidthScalabilityMode = OFF;
        BWS_nb_bitrate = 0;
    }
    PreProcessingSW = ON;
    SilenceCompressionSW = OFF;
    
    /* evalute encoder parameter string */
    parav = CmdLineParseString(encPara,SEPACHAR,&parac);
    result = CmdLineEval(parac,parav,NULL,switchList,1,NULL);
    free(*parav);
    free(parav);
    
    if (result) 
    {
        if (result == 1) 
        {
            EncLpcInfo(stdout);
            exit (1);
        }
        else
        {
            CommonExit(1,"encoder parameter string error");
        }
    }
    
    if (strstr(encPara, "-mp4ff") != NULL) 
    {
        mp4ffFlag = 1;
    }
    else 
    {
        mp4ffFlag = 0;
    }
    
    if (numChannel != 1 ) 
    {
        CommonExit(1,"EncLpcInit: Multi-channel audio input is not supported");
    }
    
    if (BWS_nb_bitrate != 0)
    {
        BandwidthScalabilityMode = ON;
        ExcitationMode = MultiPulseExc;
        SampleRateMode = fs8kHz;
        FineRateControl  = OFF;
    }
    
    if ((ExcitationMode == MultiPulseExc) && (SampleRateMode == fs16kHz)) 
    {
        BandwidthScalabilityMode = OFF;
        BWS_nb_bitrate = 0;
    }
    
    if (((BandwidthScalabilityMode==ON) && ((ExcitationMode==RegularPulseExc) || (FineRateControl==ON) || (SampleRateMode==fs16kHz))))
    {
        CommonExit(1,"EncLpcInit: Specified combination of options (2) is not supported in combination with BWS");
    }
    
    if (((NumEnhLayers > 0) && (ExcitationMode == RegularPulseExc))) 
    {
        CommonExit(1,"EncLpcInit: BRS in combination with RPE is not supported");
    }
    
    if (((SampleRateMode == fs8kHz) && (ExcitationMode == RegularPulseExc))) 
    {
        CommonExit(1,"EncLpcInit: RPE in combination with 8 kHz sampling rate is not supported");
    }
    
    if ((FineRateControl == ON) && (varBitRate == 0))
    {
        CommonExit(1,"EncLpcInit: FineRateControl must be used in combination with -vr and -b switches");
    }
    
    if ((SilenceCompressionSW == ON) && (varBitRate == 0))
    {
        CommonExit(1,"EncLpcInit: SilenceCompression must be used in combination with -vr and -b switches");
    }
    
    if ((ExcitationMode == MultiPulseExc) &&
        ((FineRateControl == ON) || (SilenceCompressionSW == ON)) &&
        ((InputConfiguration < 0) || (InputConfiguration > 31)))
    {
        CommonExit(1,"EncLpcInit: MPE_Configuration 0...31 must be specified when FRC is used");
    }
    
    /* -------------------------------------------------------------------*/
    /* Memory allocation                                                  */
    /* -------------------------------------------------------------------*/
    hdrStream = BsOpenBufferWrite(bitHeader);

    /* ---------------------------------------------------------------- */
    /* Conversion of parameters from float to longs                     */
    /* ---------------------------------------------------------------- */
    bit_rate           = (long)(bitRate+.5);
    sampling_frequency = (long)(fSample+.5);

    
    enc_objectVerType = CelpObjectV2;


    /* ---------------------------------------------------------------- */
    /* Encoder Initialisation                                           */
    /* ---------------------------------------------------------------- */
    celp_initialisation_encoder (hdrStream, bit_rate, sampling_frequency, ExcitationMode,
                                 SampleRateMode, QuantizationMode, FineRateControl, LosslessCodingMode,
                                 &RPE_configuration, &MPE_Configuration,
                                 NumEnhLayers, BandwidthScalabilityMode, &BWS_configuration,
                                 BWS_nb_bitrate, 
                                 SilenceCompressionSW,
                                 enc_objectVerType,
                                 InputConfiguration, &frame_size, &n_subframes, &sbfrm_size,
                                 &lpc_order, &num_lpc_indices, &num_shape_cbks, &num_gain_cbks,
                                 &n_lpc_analysis, &window_offsets, &window_sizes, 
                                 &n_lag_candidates, &min_pitch_frequency, &max_pitch_frequency, 
                                 &org_frame_bit_allocation, &InstanceContext,
                                 mp4ffFlag);

    /* system interface(flexmux) configuration */
    /* only single CELP layer(layer=0) */
    if (mp4ffFlag)
    {
        numLayer = 1+NumEnhLayers+BandwidthScalabilityMode;
        initObjDescr(frameData->od,mainDebugLevel  );
        presetObjDescr(frameData->od, numLayer);
        
        for (layer=0; layer<numLayer; layer++) 
        {
            initESDescr(&(frameData->od->ESDescriptor[layer]));
            presetESDescr(frameData->od->ESDescriptor[layer], layer);
            
            /* elementary stream configuration */
            if (layer>0)
            {
                frameData->od->ESDescriptor[layer]->dependsOn_Es_number.value = 
                  frameData->od->ESDescriptor[layer-1]->ESNumber.value;
            }
            
            /* DecConfigDescriptor configuration */
            if (ExcitationMode == RegularPulseExc) 
            {
                frameData->layer[layer].bitRate = (int)bitRate;
            }
            else 
            {
                if (BandwidthScalabilityMode == OFF) 
                {
                    if (SampleRateMode == fs8kHz) 
                    {
                        if (layer == 0)
                        {
                            bitRateLay = (int)bitRate - 2000*NumEnhLayers;
                        }
                        else 
                        {
                            bitRateLay = 2000;
                        }
                    } 
                    else 
                    {
                        if (layer == 0)
                        {
                            bitRateLay = (int)bitRate - 4000*NumEnhLayers;
                        }
                        else 
                        {
                            bitRateLay = 4000;
                        }
                    }
                } 
                else 
                {
                    if (layer == 0)
                    {
                        bitRateLay = BWS_nb_bitrate - 2000*NumEnhLayers;
                    }
                    else if (layer == numLayer - 1)
                    {
                        bitRateLay = (int)bitRate - BWS_nb_bitrate; 
                    } 
                    else 
                    {
                        bitRateLay = 2000;
                    }
                }
                
                frameData->layer[layer].bitRate = bitRateLay;
            }
            
            frameData->od->ESDescriptor[layer]->DecConfigDescr.avgBitrate.value = frameData->layer[layer].bitRate;
	    if (FineRateControl == ON && layer == 0) {
	      frameData->od->ESDescriptor[layer]->DecConfigDescr.maxBitrate.value = frameData->layer[layer].bitRate
		+ (unsigned long)(2.0*frame_size/sampling_frequency);
	    } else {
	      frameData->od->ESDescriptor[layer]->DecConfigDescr.maxBitrate.value = frameData->layer[layer].bitRate;
	    }
            frameData->od->ESDescriptor[layer]->DecConfigDescr.bufferSizeDB.value = 0;
            
            /* ALConfigDescriptor configuration */
            frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useAccessUnitStartFlag.value=1;
            frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useAccessUnitEndFlag.value=1;
            frameData->od->ESDescriptor[layer]->ALConfigDescriptor.useRandomAccessPointFlag.value=0;
            frameData->od->ESDescriptor[layer]->ALConfigDescriptor.usePaddingFlag.value=0;
            frameData->od->ESDescriptor[layer]->ALConfigDescriptor.seqNumLength.value=0;
            
            /* AudioSpecificConfig configuration */
            if (enc_objectVerType == CelpObjectV1)
            {
                frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value = CELP;
            }
            else
            {
                frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value = ER_CELP;
            }

            
            if (SampleRateMode == fs8kHz)
            {
                if ((BandwidthScalabilityMode == ON) && (layer == numLayer-1))
                {
                    frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value = 0x8; /* 16000Hz */
                }
                else 
                {
                    frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value = 0xb; /* 8000Hz */
                }
            }
            else 
            {
                frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value = 0x8;   /* 16000Hz */
            }
            
            frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value = 1;    /* mono */
#ifndef CORRIGENDUM1
            frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.BWS_on=0;
#endif
            
            /* celpSpecificConfig configuration */
            if (layer == 0) 
            {
#ifdef CORRIGENDUM1
  	        frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.isBaseLayer.value = 1;
#endif
                frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.excitationMode.value = ExcitationMode;
                frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.sampleRateMode.value = SampleRateMode;
                frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.fineRateControl.value = FineRateControl;
                frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.silenceCompressionSW.value = SilenceCompressionSW;
                frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.RPE_Configuration.value = RPE_configuration;
                frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.MPE_Configuration.value = MPE_Configuration;
                frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.numEnhLayers.value = NumEnhLayers;
                frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.bandwidthScalabilityMode.value = BandwidthScalabilityMode;
            } 
            else 
            {
	      if ((BandwidthScalabilityMode == ON) && (layer == numLayer-1)) {
#ifdef CORRIGENDUM1
  	        frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.isBaseLayer.value = 0;
  	        frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.isBWSLayer.value = 1;
                frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.BWS_Configuration.value = BWS_configuration;
#else
		frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.BWS_on=1;
		frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpEnhSpecificConfig.BWS_Configuration.value = BWS_configuration;
#endif
	      } else {
#ifdef CORRIGENDUM1
  	        frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.isBaseLayer.value = 0;
  	        frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.isBWSLayer.value = 0;
                frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.CELP_BRS_id.value = layer;
#else
		frameData->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.BWS_on=0;
#endif
	      }
            }
        }
    
        advanceODescr(hdrStream, frameData->od, 1);
        
        for (layer=0; layer<numLayer; layer++)
        { 
            advanceESDescr(hdrStream, frameData->od->ESDescriptor[layer], 1,0);
            frameData->layer[layer].bitBuf = BsAllocBuffer(512);
        }
    }
  
    if (ExcitationMode == RegularPulseExc)
    {
        *frameNumSample = frame_size;
        *delayNumSample = 2*frame_size;
    }
    
    if (ExcitationMode == MultiPulseExc)
    {
        *frameNumSample = frame_size;
        if (SampleRateMode == fs16kHz)
        {
            *delayNumSample = NEC_FRQ16_NLA;
        }
        else
        {
            if (BandwidthScalabilityMode == OFF)
            {
                *delayNumSample = NEC_PAN_NLA;
            }
            else
            {
                *delayNumSample = NEC_FRQ16_NLA+NEC_LPF_DELAY;
            }
        }
    }
    
    BsClose(hdrStream);
}

/* ------------------------------------------------------------------- */
/* EncLpcFrame()                                                       */
/* Encode one audio frame into one bit stream frame with               */
/* LPC-based encoder core.                                             */
/* ------------------------------------------------------------------- */
void EncLpcFrame (float          **sampleBuf,       /* in: audio frame samples  sampleBuf[numChannel][frameNumSample]      */
                  HANDLE_BSBITBUFFER bitBuf,           /* out: bit stream frame or NULL during encoder start up               */
                  ENC_FRAME_DATA *frameData)
{
    int             layer, numLayer;
    HANDLE_BSBITSTREAM     bitStream[5];

    /* -------------------------------------------------------------------*/
    /* Memory allocation                                                  */
    /* -------------------------------------------------------------------*/
    if (mp4ffFlag)
    {  /* for flexmux bitstream, use writeFlexMuxPDU() */
        numLayer = frameData->od->streamCount.value;
        for ( layer = 0; layer < numLayer; layer++ ) 
        {
            if (bitBuf)
            {
                bitStream[layer] = BsOpenBufferWrite(frameData->layer[layer].bitBuf);
            }
            else
            {
                bitStream[layer] = NULL;
            }
        }
    } 
    else 
    {
        numLayer = 1;
        if (bitBuf)
        {
            bitStream[0] = BsOpenBufferWrite(bitBuf);
        }
        else
        {
            bitStream[0] = NULL;
        }
    }

    celp_coder (sampleBuf, bitStream,
                ExcitationMode, SampleRateMode, QuantizationMode, FineRateControl,
                RPE_configuration, Wideband_VQ,
                MPE_Configuration,
		BandwidthScalabilityMode,
                PreProcessingSW,
                SilenceCompressionSW,
                enc_objectVerType,
                frame_size, n_subframes, sbfrm_size, lpc_order,
                num_lpc_indices, num_shape_cbks, num_gain_cbks, 
                n_lpc_analysis, window_offsets, window_sizes, n_lag_candidates, 
                org_frame_bit_allocation, InstanceContext, mp4ffFlag);

    if (mp4ffFlag) 
    {
        if (bitBuf) 
        {
            for ( layer = 0; layer < numLayer; layer++ ) 
            {
                BsClose(bitStream[layer]);
            }
            
            bitStream[0] = BsOpenBufferWrite(bitBuf);
            for ( layer = 0; layer < numLayer; layer++ ) 
            {
                writeFlexMuxPDU(layer, bitStream[0], frameData->layer[layer].bitBuf);
            }
            BsClose(bitStream[0]);
        }
    } 
    else 
    {
        if (bitBuf != NULL)
        {
            BsClose(bitStream[0]);
        }
    }
    
}

/* ---------------------------------------------------------------------*/
/* EncLpcFree()                                                         */
/* Free memory allocated by LPC-based encoder core.                     */
/* ---------------------------------------------------------------------*/
void EncLpcFree ()
{
    /* -----------------------------------------------------------------*/
    /* Free all the arrays that were initialised                        */
    /* -----------------------------------------------------------------*/ 
    celp_close_encoder (ExcitationMode,
                        BandwidthScalabilityMode, sbfrm_size, 
                        org_frame_bit_allocation, window_offsets, window_sizes, 
                        n_lpc_analysis, &InstanceContext);
}

/* end of enc_lpc.c */

