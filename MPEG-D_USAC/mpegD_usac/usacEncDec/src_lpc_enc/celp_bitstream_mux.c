/*====================================================================*/
/*         MPEG-4 Audio (ISO/IEC 14496-3) Copyright Header            */
/*====================================================================*/
/*
This software module was originally developed by Rakesh Taori and Andy
Gerrits (Philips Research Laboratories, Eindhoven, The Netherlands)
and edited by Naoya Tanaka (Matsushita Communication Ind. Co., Ltd.)
in the course of development of the MPEG-4 Audio (ISO/IEC 14496-3). This
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
/*====================================================================*/

/*====================================================================*/
/*                                                                    */
/*      SOURCE_FILE:    CELP_BITSTREAM_MUX.C                          */
/*                                                                    */
/*====================================================================*/
    
/*====================================================================*/
/*      I N C L U D E S                                               */
/*====================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "lpc_common.h"  /* Common LPC core Defined Constants           */
#include "bitstream.h"     /* bit stream module                         */
#include "phi_cons.h"
#include "phi_priv.h"
#include "celp_bitstream_mux.h"    /* Parameter to bitstream conversion routines  */
/* #include "phi_freq.h"   */
#include "pan_celp_const.h"

/*======================================================================*/
/*      G L O B A L    D A T A   D E F I N I T I O N S                  */
/*======================================================================*/

/*======================================================================*/
/* Function Definition: write_celp_bitstream_header                     */
/*======================================================================*/
void write_celp_bitstream_header (HANDLE_BSBITSTREAM hdrStream,               /* Out: Bitstream                   */
                                  const long ExcitationMode,            /* In: Excitation Mode              */
                                  const long SampleRateMode,            /* In: SampleRate Mode              */
                                  const long FineRateControl,           /* In: Fine Rate Control switch     */
                                  const long RPE_configuration,         /* In: Wideband configuration       */
                                  const long MPE_Configuration,         /* In: Narrowband configuration     */
                                  const long NumEnhLayers,              /* In: Number of Enhancement Layers */
                                  const long BandwidthScalabilityMode,  /* In: bandwidth switch             */
                                  const long SilenceCompressionSW,      /* In: Silence Compression          */
                                  int        enc_objectVerType,
                                  const long BWS_Configuration          /* In: BWS_configuration            */
)
{
    
    BsPutBit(hdrStream, ExcitationMode, 1);
    BsPutBit(hdrStream, SampleRateMode, 1);
    BsPutBit(hdrStream, FineRateControl, 1);
    
    if (enc_objectVerType == CelpObjectV2)
    {
        BsPutBit(hdrStream, SilenceCompressionSW, 1);
    }
    
    if (ExcitationMode == RegularPulseExc )
    {
        BsPutBit(hdrStream, RPE_configuration, 3);
    }
    
    if (ExcitationMode == MultiPulseExc )
    {
        BsPutBit(hdrStream, MPE_Configuration, 5);
        BsPutBit(hdrStream, NumEnhLayers, 2);
        BsPutBit(hdrStream, BandwidthScalabilityMode, 1);
        
        if (BandwidthScalabilityMode == 1)
        {
            BsPutBit(hdrStream, BWS_Configuration, 2);
        }
    }
}

/*======================================================================*/
/* Function Definition: Write_Narrowband_LSP                            */
/*======================================================================*/
void Write_NarrowBand_LSP
(
    HANDLE_BSBITSTREAM bitStream,            /* Out: Bitstream                */
    const long indices[]               /* In: indices                   */
)
{
    /*==================================================================*/
    /*     Read the Parameters from the Bitstream                       */
    /*           These represent the packed LPC Indices                 */           
    /*==================================================================*/
    BsPutBit(bitStream, indices[0], PAN_BIT_LSP22_0);
    BsPutBit(bitStream, indices[1], PAN_BIT_LSP22_1);
    BsPutBit(bitStream, indices[2], PAN_BIT_LSP22_2);
    BsPutBit(bitStream, indices[3], PAN_BIT_LSP22_3);
    BsPutBit(bitStream, indices[4], PAN_BIT_LSP22_4);
}

/*======================================================================*/
/* Function Definition: Write_Narrowband_LSP                            */
/*======================================================================*/
void Write_BandScalable_LSP
(
    HANDLE_BSBITSTREAM bitStream,            /* Out: Bitstream                */
    const long indices[]            /* In: indices                   */
)
{
    /*==================================================================*/
    /*     Read the Parameters from the Bitstream                       */
    /*           These represent the packed LPC Indices                 */           
    /*==================================================================*/
    BsPutBit(bitStream, indices[0], 4);
    BsPutBit(bitStream, indices[1], 7);
    BsPutBit(bitStream, indices[2], 4);
    BsPutBit(bitStream, indices[3], 6);
    BsPutBit(bitStream, indices[4], 7);
    BsPutBit(bitStream, indices[5], 4);
}

/*======================================================================*/
/* Function Definition: Write_Wideband_LSP                              */
/*======================================================================*/
void Write_WideBand_LSP
(
    HANDLE_BSBITSTREAM bitStream,            /* Out: Bitstream                */
    const long indices[]               /* In: indices                   */
)
{
    BsPutBit(bitStream, indices[0], PAN_BIT_LSP_WL_0);
    BsPutBit(bitStream, indices[1], PAN_BIT_LSP_WL_1);
    BsPutBit(bitStream, indices[2], PAN_BIT_LSP_WL_2);
    BsPutBit(bitStream, indices[3], PAN_BIT_LSP_WL_3);
    BsPutBit(bitStream, indices[4], PAN_BIT_LSP_WL_4);
    BsPutBit(bitStream, indices[5], PAN_BIT_LSP_WU_0);
    BsPutBit(bitStream, indices[6], PAN_BIT_LSP_WU_1);
    BsPutBit(bitStream, indices[7], PAN_BIT_LSP_WU_2);
    BsPutBit(bitStream, indices[8], PAN_BIT_LSP_WU_3);
    BsPutBit(bitStream, indices[9], PAN_BIT_LSP_WU_4);
}

/*======================================================================*/
/* Function Definition: Write_WideBand_LSP_V2                           */
/*======================================================================*/
void Write_WideBand_LSP_V2
(
    HANDLE_BSBITSTREAM bitStream,            /* Out: Bitstream                */
    const long indices[],               /* In: indices                   */
    unsigned long ep_class
)
{
    
    switch (ep_class)
    {
        case 0:
            /* write class 0 */
            BsPutBit(bitStream, indices[0], 5);
            /* lpc1: 0-1 */
            BsPutBit(bitStream, (indices[1]&3), 2);
            /* lpc2: 0-4, 6 */
            BsPutBit(bitStream, ((indices[2]>>6)&1), 1);
            BsPutBit(bitStream, (indices[2]&0x1f), 5);
            /* lpc4 */
            BsPutBit(bitStream, indices[4], 1);
            /* lpc5: 0 */
            BsPutBit(bitStream, (indices[5]&1), 1);
            break;

        case 1:
            /* write class 1 */
            /* lpc1: 2-3 */
            BsPutBit(bitStream, ((indices[1]>>2)&3), 2);
            /* lpc2: 5 */
            BsPutBit(bitStream, ((indices[2]>>5)&1), 1);
            /* lpc5: 1 */
            BsPutBit(bitStream, ((indices[5]>>1)&1), 1);
            /* lpc6: 0-1 */
            BsPutBit(bitStream, (indices[6]&3), 2);
            break;
 
        case 2:
            /* write class 2 */
            /* lpc1: 4 */
            BsPutBit(bitStream, ((indices[1]>>4)&1), 1);
            /* lpc3: 1, 6 */
            BsPutBit(bitStream, ((indices[3]>>6)&1), 1);
            BsPutBit(bitStream, ((indices[3]>>1)&1), 1);
            /* lpc5: 2 */
            BsPutBit(bitStream, ((indices[5]>>2)&1), 1);
            /* lpc6: 3 */
            BsPutBit(bitStream, ((indices[6]>>3)&1), 1);
            /* lpc7: 0-1, 4, 6 */
            BsPutBit(bitStream, ((indices[7]>>6)&1), 1);
            BsPutBit(bitStream, ((indices[7]>>4)&1), 1);
            BsPutBit(bitStream, (indices[7]&0x3), 2);
            /* lpc9 */
            BsPutBit(bitStream, indices[9], 1);
            break;

        case 3:
            /* write class 3 */
            /* lpc3: 0, 2-4 */
            BsPutBit(bitStream, ((indices[3]>>2)&7), 3);
            BsPutBit(bitStream, (indices[3]&0x1), 1);
            /* lpc5: 3 */
            BsPutBit(bitStream, ((indices[5]>>3)&1), 1);
            /* lpc6: 2 */
            BsPutBit(bitStream, ((indices[6]>>2)&1), 1);
            /* lpc7: 2-3, 5 */
            BsPutBit(bitStream, ((indices[7]>>5)&1), 1);
            BsPutBit(bitStream, ((indices[7]>>2)&0x3), 2);
            /* lpc8: 1-4 */
            BsPutBit(bitStream, ((indices[8]>>1)&0xf), 4);
            break;

        case 4:
            /* write class 4 */
            /* lpc3: 5 */
            BsPutBit(bitStream, ((indices[3]>>5)&1), 1);
            /* lpc8: 0 */
            BsPutBit(bitStream, (indices[8]&1), 1);
            break; 

        default: 
            fprintf(stderr, "Wrong Class in Write-WideBand_LPC_V2");
            exit(1);
    }
}

/*======================================================================*/
/* Function Definition: Write_Narrowband_LSP_V2                         */
/*======================================================================*/
void Write_NarrowBand_LSP_V2
(
    HANDLE_BSBITSTREAM bitStream,            /* Out: Bitstream                */
    const long indices[],               /* In: indices                   */
    unsigned long ep_class
)
{
    
    switch (ep_class)
    {
        case 0:
            break;

        case 1:
            /* write class 1 */
            /* lpc0: 0-1 */
            BsPutBit(bitStream, ((indices[0])&3), 2);
            /* lpc1: 0 */
            BsPutBit(bitStream, ((indices[1])&1), 1);
            break;
 
        case 2:
            /* write class 2 */
            /* lpc2: 0, 6 */
            BsPutBit(bitStream, ((indices[2]>>6)&1), 1);
            BsPutBit(bitStream, ((indices[2])&1), 1);
            /* lpc4: 0 (all) */
            BsPutBit(bitStream, ((indices[4])), 1);
            break;

        case 3:
            /* write class 3 */
            /* lpc0: 2-3 */
            BsPutBit(bitStream, ((indices[0]>>2)&3), 2);
            /* lpc1: 1-2 */
            BsPutBit(bitStream, ((indices[1]>>1)&3), 2);
            /* lpc2: 1-5 */
            BsPutBit(bitStream, ((indices[2]>>1)&0x1f), 5);
            break;

        case 4:
            /* write class 4 */
            /* lpc1: 3 */
            BsPutBit(bitStream, ((indices[1]>>3)&1), 1);
            /* lpc3: all (6) */
            BsPutBit(bitStream, ((indices[3])), 6);
            break; 

        default: 
            fprintf(stderr, "Wrong Class in WriteNarrowBand_LPC_V2");
            exit(1);
    }
}

 
/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 07-06-96  R. Taori & A.Gerrits    Initial Version                    */
/* 18-09-96  R. Taori & A.Gerrits    MPEG bitstream used                */
/* 21-05-99  T. Mlasko               Version 2 stuff added              */
