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

Source file: Log.c

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
 *   Module              : Log
 *
 *   Description         : This module provides logging functionality.
 *                         The logging functions are to be accessed via
 *                         the LOG_Xxxx macros defined in the header file
 *                         The logging functionality can be disable by defining
 *                         the LOG_DISABLE symbol.
 *
 *   Tools               : Microsoft Visual C++ 6.0
 *
 *   Target Platform     : ANSI-C
 *
 *   Naming Conventions  : All public functions (macros) are prefixed by LOG_
 *
 *
 ******************************************************************************/



/*============================================================================*/
/*       INCLUDES                                                             */
/*============================================================================*/
#include <stdarg.h> 
#include <assert.h>

#include "Log.h"

#ifndef LOG_DISABLE

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/


/*============================================================================*/
/*       TYPE DEFINITIONS                                                     */
/*============================================================================*/


/*============================================================================*/
/*       STATIC VARIABLES                                                     */
/*============================================================================*/
static FILE* pLogFile = NULL;
static FILE* pLogParmsFile = NULL ;

/*============================================================================*/
/*       PROTOTYPES STATIC FUNCTIONS                                          */
/*============================================================================*/


/*============================================================================*/
/*       GLOBAL FUNCTION IMPLEMENTATIONS                                      */
/*============================================================================*/


/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : LOG_OpenFile
 *
 * Description : Create logging file. 
 *
 * Parameters  : FileName - Name of the the file to create.
 *
 * Returns:    : Pointer to file opened.
 * 
 *****************************************************************************/
FILE* LogOpenFile(const char* pFileName)
{
    assert(pLogFile == NULL);

    pLogFile = fopen(pFileName, "w");

    return pLogFile;
}

FILE* LogParmsOpenFile(const char* pFileName)
{
    assert(pLogParmsFile == NULL);

    pLogParmsFile = fopen(pFileName, "w");

    return pLogParmsFile;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : LOG_SetFile
 *
 * Description : Specifiy the file in which the log output should be stored.
 *
 * Parameters  : pFile - File pointer of file with write permission.
 *
 * Returns:    : 
 * 
 *****************************************************************************/
void LogSetFile(FILE* pFile)
{
    assert(pLogFile == NULL);

    pLogFile = pFile;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : LOG_Printf
 *
 * Description : Write to log file using printf style arguments
 *
 * Parameters  : format - Pointer to zero terminated string containing the
 *                        format specification..
 *
 *               ...    - Optional arguments.
 *
 * Returns     : The number of characters written, not including the
 *               terminating null character, or a negative value if an output
 *               error occurs. 
 * 
 *****************************************************************************/
int LogPrintf(const char* format,...)
{
    va_list argp;
    int     ReturnValue = -1;

    if(pLogFile != NULL)
    {
        va_start(argp, format);
        ReturnValue = vfprintf(pLogFile, format, argp);
        fflush( pLogFile );
        va_end(argp);
    }
    return ReturnValue;
}

int LogParmsPrintf(const char* format,...)
{
    va_list argp;
    int     ReturnValue = -1;

    if(pLogParmsFile != NULL)
    {
        va_start(argp, format);
        ReturnValue = vfprintf(pLogParmsFile, format, argp);
        fflush( pLogParmsFile );
        va_end(argp);
    }
    return ReturnValue;
}

/*************************GLOBAL FUNCTION**************************************
 * 
 * Name        : LOG_CloseFile
 *
 * Description : Close logging file.
 *
 * Parameters  : -
 *
 * Returns     : -
 * 
 *****************************************************************************/
void LogCloseFile(void)
{
    if(pLogFile != NULL)
    {
        fclose(pLogFile);
        pLogFile = NULL;
    }
}

void LogParmsCloseFile(void)
{
    if(pLogParmsFile != NULL)
    {
        fclose(pLogParmsFile);
        pLogParmsFile = NULL;
    }
}

#endif /* LOG_DISABLE */
