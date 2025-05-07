/**********************************************************************
 MPEG-4 Audio VM

 This software module was originally developed by
 
 Manuela Schinn (Fraunhofer IIS)
  
 and edited by
 
 Michael Matejko (Fraunhofer IIS)

 
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
 
 Copyright (c) 2006.
 
 $Id: dec_mpeg12.c,v 1.1.1.1 2009-05-29 14:10:13 mul Exp $
 MPEG12 Decoder module
**********************************************************************/

#include <stdlib.h>
#include <stdio.h>     

#include "common_m4a.h"   /* common module */
#include "decLayer123.h"  /* MPEG12 decoder core */
#include "dec_mpeg12.h"
#include "obj_descr.h"
#include "bitstream.h"
#include "flex_mux.h"


#define MPEG12_MAX_PCM_SAMPLES 13824



/* DecMPEG12Info() */
/* Get info about MPEG12-based decoder core. */
char *DecMPEG12Info (FILE *helpStream) {
    return decLayer123Info (helpStream); 
}



/* DecMPEG12Init() */
/* Init MPEG12-based decoder core. */
void DecMPEG12Init (char *decPara, MPEG12_DATA *mpeg12Data, int stream, FRAME_DATA *fD) {
  
    int channelElement;
    int i_ch;

    mpeg12Data->debug = debugInit(D_WARNING, stderr);
    mpeg12Data->numChannels = fD->od->ESDescriptor[stream]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value;
    mpeg12Data->pcmbuf = pcmBufferInit (MPEG12_MAX_PCM_SAMPLES);      
  
    switch (mpeg12Data->numChannels) 
    {
        case 1:
        case 2:
            mpeg12Data->numChannelElements = 1;
        break;
        case 3:
            mpeg12Data->numChannelElements = 2;
        break;
        case 4:
        case 5:
            mpeg12Data->numChannelElements = 3;
        break;
        case 6:
            mpeg12Data->numChannelElements = 4;
        break;
        case 7:
            mpeg12Data->numChannelElements = 5;
        break;
        default:
            CommonWarning("unsupported channelConfiguration %d" ,mpeg12Data->numChannels);
        break;
    }
     
    /* Allocate memory for each channel */  
    for (i_ch = 0; i_ch < mpeg12Data->numChannels; i_ch++) 
    {       
        mpeg12Data->sampleBuf[i_ch] = (float*)malloc(MPEG12_MAX_PCM_SAMPLES * sizeof(float)); 
    }

       
    /* Create a decoder instance for each mp3_channel_element */   
    for (channelElement = 0; channelElement < mpeg12Data->numChannelElements; channelElement++) 
    {    
        mpeg12Data->dec[channelElement] = decLayer123Init(decPara, stderr);
        if ( mpeg12Data->dec[channelElement] == NULL ) {
          CommonExit(1,"can't open MPEG12 decoder\n") ;
        }
    }
}



/* DecMPEG12Frame() */
/* Decode one bit stream frame into one audio frame with */
/* MPEG12-based decoder core. */
void DecMPEG12Frame (FRAME_DATA *fD, MPEG12_DATA *mpeg12Data) {

    HANDLE_BSBITSTREAM bs;        /* bitstream */
    int sampleRate;               /* sampleRate */
    int NoChannelElements;        /* number of channel elements */
    int channelsDone = 0;
    int i = 0;

    sampleRate = fD->layer[0].sampleRate;
    bs = BsOpenBufferRead (fD->layer[0].bitBuf);
    NoChannelElements = mpeg12Data->numChannelElements;
  
    /** for each mp3_channel_element in every access unit do: */
    for (i = 0 ; i < NoChannelElements ; i++) 
    {
    
        /** restore syncword and IDex */
        long pcmSample;
        unsigned long data;
        unsigned long dataLength;           /* length of one channel_elemenet */
        HANDLE_bitBuffer auBuffer = NULL;   /* bitBuffer */
          
        int samp, channel, noOfChannels;
        STATUS_pcmBuffer pcmStatus;
    
        switch (fD->scalOutObjectType)
        {
            case LAYER_1:
            case LAYER_2:
                dataLength = *fD->layer[0].AULength;
                auBuffer = bitBufferInit (dataLength, "auBuffer", mpeg12Data->debug);
            break;
            
            case LAYER_3:
                
                BsGetBit(bs, &dataLength, 12);
                dataLength *= 8;
                
                auBuffer = bitBufferInit (dataLength, "auBuffer", mpeg12Data->debug);
                
                /* write syncword to buffer */
                bitBufferPut (auBuffer, 0xffff, 11);
                
                /* write IDex to buffer */
                if (sampleRate > 12000) {
                    /* IDex == 1 */  
                    bitBufferPut (auBuffer, 1, 1);
                } else {
                    /* IDex == 0 */
                    bitBufferPut (auBuffer, 0, 1);
                }
                
                /* 12 bits are written */
                dataLength -= 12;
            break;
        }  
    
        /* write data to buffer */
        while (dataLength > 32)
        {
            dataLength -= 32;
            BsGetBit (bs, &data, 32);
            bitBufferPut (auBuffer, data, 32); 
        }
    
        BsGetBit (bs, &data, dataLength);
        bitBufferPut (auBuffer, data, dataLength);
        
        /* Call mp3_decoder core */
        decLayer123Frame (mpeg12Data->dec[i], auBuffer, mpeg12Data->pcmbuf);
        bitBufferFree (auBuffer);
        
        /* Convert PCM output into refSoft sample_buffer format */
        noOfChannels = (int) pcmBufferGetChannels(mpeg12Data->pcmbuf);
        
        if (channelsDone+noOfChannels > mpeg12Data->numChannels)
        {
            CommonWarning("dec_mpeg12: Number of channels in mp3 bitstream exceeded expected channel configuration: %d",mpeg12Data->numChannels);
            mpeg12Data->frameNumSample = 0;
            break;
            
        }  

        for (samp=0, pcmStatus=PCM_OK; pcmStatus != PCM_EMPTY; samp++)
        {
            for (channel=0; channel < noOfChannels; channel++)
            {
                pcmStatus = pcmBufferGetCasted(mpeg12Data->pcmbuf, &pcmSample, 24);
                mpeg12Data->sampleBuf[channelsDone+channel][samp] = (float)pcmSample/256;
            }
        }
        
        mpeg12Data->frameNumSample = samp - 1;
        channelsDone += noOfChannels;
                                       
    } /* for */
    
    if (channelsDone < mpeg12Data->numChannels  &&  mpeg12Data->frameNumSample > 0)
    {
       CommonWarning("dec_mpeg12: Number of channels in mp3 bitstream didn't reach expected channel configuration: %d",mpeg12Data->numChannels);
    }
    
    /* close bitstream and remove bits read from buffer */  
    BsCloseRemove (bs,  1);
  
    fD->layer[0].NoAUInBuffer--;
}



/* DecL3Free() */
/* Free memory allocated by L3-based decoder core. */
void DecMPEG12Free (MPEG12_DATA *mpeg12Data) {
    int i;
   
    for (i = 0 ; i < mpeg12Data->numChannelElements ; i++ )
    {
        decLayer123Free (mpeg12Data->dec[i]);
    }
  
    pcmBufferFree (mpeg12Data->pcmbuf);
}
