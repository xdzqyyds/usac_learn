/***********************************************************************
MPEG-4 Audio RM Module
Parametric based codec - SSC (SinuSoidal Coding) bit stream Encoder

This software was originally developed by:
* Arno Peters, Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
* Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
* Werner Oomen, Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

And edited by:
*

in the course of development of the MPEG-4 Audio standard ISO-14496-1, 2 and 3.
This software module is an implementation of a part of one or more MPEG-4 Audio
tools as specified by the MPEG-4 Audio standard. ISO/IEC gives users of the
MPEG-4 Audio standards free licence to this software module or modifications
thereof for use in hardware or software products claiming conformance to the
MPEG-4 Audio standards. Those intending to use this software module in hardware
or software products are advised that this use may infringe existing patents.
The original developers of this software of this module and their company,
the subsequent editors and their companies, and ISO/EIC have no liability for
use of this software module or modifications thereof in an implementation.
Copyright is not released for non MPEG-4 Audio conforming products. The
original developer retains full right to use this code for his/her own purpose,
assign or donate the code to a third party and to inhibit third party from
using the code for non MPEG-4 Audio conforming products. This copyright notice
must be included in all copies of derivative works.

Copyright © 2001.

Source file: SSC_Error.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
13-Aug-2001 AP/JD   Initial version
25 Jul 2002 TM      m8540 improved noise coding + 8 sf/frame
************************************************************************/




/*===========================================================================
 *
 *  Module          : SSC_Error
 *
 *  File            : SSC_Error.h
 *
 *  Description     : This header file defines the SSC constants and macros
 *                    for error handling.
 *
 *  Target Platform : ANSI C
 *
============================================================================*/

#ifndef _SSC_ERROR_H
#define _SSC_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif



/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/*
   SSC_ERROR constants:

    Bits 31 - 24: Unused.
    Bits 23 - 16: Error category
    Bits 15 -  0: Error description

   All error codes are prefixed by SSC_ERROR. Error codes
   are composed by ORing catagory and description codes.

*/

/*===========================================================================*/
/* Error Category                                                            */
/*===========================================================================*/
#define SSC_ERROR_VIOLATION               0x00400000  /* Syntax violation             */
#define SSC_ERROR_RESERVED                0x00300000  /* Reserved value in bitstream  */
#define SSC_ERROR_NOT_SUPPORTED           0x00200000  /* Unsupported function         */
#define SSC_ERROR_WARNING                 0x00100000  /* Warning - potential problem,
                                                         output unreliable.           */
#define SSC_ERROR_OK                      0x00000000  /* No problem                   */

/*===========================================================================*/
/* Error description                                                         */
/*===========================================================================*/
/* Error block offsets */
#define SSC_ERROR_STREAM                  0x00000000  /* Stream error block offset         */
#define SSC_ERROR_CODING                  0x00004000  /* Coding process error block offset */
#define SSC_ERROR_GENERAL                 0x00008000  /* General error block offset        */
#define SSC_ERROR_USER                    0x0000C000  /* User error block offset           */

/* Stream errors */
#define SSC_ERROR_NO_ERROR                0x00000000
#define SSC_ERROR_DECODER_LEVEL           0x00000001
#define SSC_ERROR_SAMPLING_RATE           0x00000002
#define SSC_ERROR_UPDATE_RATE             0x00000003
#define SSC_ERROR_SYNTHESIS_METHOD        0x00000004
#define SSC_ERROR_ENHANCEMENT             0x00000005
#define SSC_ERROR_MODE                    0x00000006
#define SSC_ERROR_MODE_EXT                0x00000007
#define SSC_ERROR_REFRESH_FRAME           0x00000008

#define SSC_ERROR_N_NROF_DEN              0x0000000A
#define SSC_ERROR_RESERVED_FIELD          0x0000000B
#define SSC_ERROR_PHASE_JITTER_PRESENT    0x0000000C
#define SSC_ERROR_PHASE_JITTER_PERCENTAGE 0x0000000D
#define SSC_ERROR_PHASE_JITTER_BAND       0x0000000E
#define SSC_ERROR_NROF_TRANS              0x00000010
#define SSC_ERROR_T_LOC                   0x00000011
#define SSC_ERROR_T_TYPE                  0x00000012
#define SSC_ERROR_T_B_PAR                 0x00000013
#define SSC_ERROR_T_CHI_PAR               0x00000014
#define SSC_ERROR_T_GAIN                  0x00000015
#define SSC_ERROR_T_NROF_SIN              0x00000016
#define SSC_ERROR_T_FREQ                  0x00000017
#define SSC_ERROR_T_AMP                   0x00000018
#define SSC_ERROR_T_PHI                   0x00000019
#define SSC_ERROR_S_NROF_CONTINUATIONS    0x0000001A
#define SSC_ERROR_S_NROF_BIRTHS           0x0000001B
#define SSC_ERROR_S_ISCONT                0x0000001C
#define SSC_ERROR_S_DELTA_FREQ            0x0000001D
#define SSC_ERROR_S_DELTA_AMP             0x0000001E
#define SSC_ERROR_S_PHI                   0x0000001F
#define SSC_ERROR_S_FREQ                  0x00000020
#define SSC_ERROR_S_AMP                   0x00000021

#define SSC_ERROR_N_LAR_DEN               0x00000023
#define SSC_ERROR_N_ENV_GAIN              0x00000024
#define SSC_ERROR_PADDING                 0x00000025
#define SSC_ERROR_INVALID_FILE            0x00000026
#define SSC_ERROR_OLD_FILE                0x00000027
#define SSC_ERROR_OLD_DECODER             0x00000028
#define SSC_ERROR_N_NROF_LSF              0x00000029
#define SSC_ERROR_N_LSF                   0x0000002A
#define SSC_ERROR_N_LAGUERRE              0x0000002B

#define SSC_ERROR_S_ADPCM_GRID            0x0000002C
#define SSC_ERROR_S_REFRESH               0x0000002D

/* Coding process errors */
#define SSC_ERROR_NROF_CHANNELS           0x00000001
#define SSC_ERROR_FREQUENCY               0x00000002
#define SSC_ERROR_FRAME_TOO_LONG          0x00000003
#define SSC_ERROR_TOO_MANY_SIN_TRACKS     0x00000004
#define SSC_ERROR_NOT_AVAILABLE           0x00000005
#define SSC_ERROR_ILLEGAL_HUFFMAN_CODE    0x00000006

/* General errors */
#define SSC_ERROR_OUT_OF_MEMORY           0x00000001
#define SSC_ERROR_STRUCT_FIELD            0x00000002
/* User errors */
typedef UInt32 SSC_ERROR;

/*===========================================================================*/
/*       MACRO DEFINITIONS                                                   */
/*===========================================================================*/
#define SSC_ERROR_CATAGORY(x)       (x & 0x00FF0000)
#define SSC_ERROR_DESCRIPTION(x)    (x & 0x0000FFFF)


/*===========================================================================*/
/*       FUNCTION PROTOTYPES                                                 */
/*===========================================================================*/


#ifdef __cplusplus
}
#endif

#endif /* _SSC_ERROR_H */
