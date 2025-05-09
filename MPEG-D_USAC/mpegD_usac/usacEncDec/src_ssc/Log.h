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

Copyright  2001.

Source file: Log.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
06-Sep-2001	JD	Initial version
************************************************************************/

/******************************************************************************
 *
 *   Module              : LOG
 *
 *   Description         : This module provides logging services.
 *
 *   Tools               : Microsoft Visual C++
 *
 *   Target Platform     : ANSI-C
 *
 *   Naming Conventions  : All function names are prefixed by LOG_
 *
 *   Functions           :
 *
 ******************************************************************************/
#ifndef  LOG_H
#define  LOG_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#include <stdio.h>


/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/
/*#ifdef _DEBUG */

/* #define LOG_BITS */

#define LOG_SINUSOIDS
#define LOG_GENERAL
#define LOG_TRANSIENT
#define LOG_NOISE
#define LOG_STEREO
/*
#define LOG_PARMS
*/
/*#endif */ /* _DEBUG */

/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/

/*============================================================================*/
/*       MACRO DEFINITIONS                                                    */
/*============================================================================*/

#ifdef LOG_DISABLE
    #define LOG_OpenFile(x)
    #define LOG_SetFile(x)
    #define LOG_Printf          1 ? 0 : LogPrintf
    #define LOG_CloseFile()

    #define LOGPARMS_OpenFile(x)
    #define LOGPARMS_Printf          1 ? 0 : LogParmsPrintf
    #define LOGPARMS_CloseFile()

#else
    #define LOG_OpenFile        LogOpenFile
    #define LOG_SetFile         LogSetFile
    #define LOG_Printf          LogPrintf
    #define LOG_CloseFile       LogCloseFile

    #define LOGPARMS_OpenFile        LogParmsOpenFile
    #define LOGPARMS_Printf          LogParmsPrintf
    #define LOGPARMS_CloseFile       LogParmsCloseFile

#endif

/*============================================================================*/
/*       FUNCTION PROTOTYPES                                                  */
/*============================================================================*/
int   LogPrintf(const char* format,...);
FILE* LogOpenFile(const char* pFileName);
void  LogSetFile(FILE* pFile);
void  LogCloseFile(void);

int   LogParmsPrintf(const char* format,...);
FILE* LogParmsOpenFile(const char* pFileName);
void  LogParmsCloseFile(void);

#ifdef __cplusplus
}
#endif

#endif /* LOG_H */
