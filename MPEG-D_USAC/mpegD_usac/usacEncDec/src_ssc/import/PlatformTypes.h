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

Source file: PlatformTypes.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

Changes:
06-Sep-2001	JD	Initial version
************************************************************************/

/*===========================================================================================
 *
 *  Module              :   PLATFORMTYPES
 *
 *  File                :   PLATFORMTYPES.h
 *
 *  Description         :   This header file defines the following platform independant types:
 *
 *                           SInt (si) for signed integers
 *                           UInt (ui) for unsigned integers
 *                           SByte (sb) for 8 bit signed integers
 *                           UByte (ub) for 8 bit unsigned integers
 *                           SInt16 (ss) for 16 bit signed integers
 *                           UInt16 (us) for 16 bit unsigned integers
 *                           SInt32 (sl) for 32 bit signed integers
 *                           UInt32 (ul) for 32 bit unsigned integers
 *                           SInt64 (si64) for 64 bit signed integers
 *                           UInt64 (ui64) for 64 bit unsigned integers
 *                           Float (f) for 4-byte IEEE floats
 *                           Double (d) for 8-byte IEEE floats
 *                           Char (ch) for standard characters
 *                           Unicode (cl) for Unicode characters
 *                           Bool (b) for booleans, a Bool can only have value True or False
 *
 *                          Using these types instead of the build-in types should ease the
 *                          effort needed to port to a specific platform.
 *
 *                          To compile for specific platform define a PLATFORM_XXX_YYY macro,
 *                          where XXX is the abbreviation for the platform and YYY the
 *                          abbreviation for the compiler.
 *
 *  Platforms           :   PLATFORM_WIN32_MSVC - Win32 / Microsoft Visual C++ 4.2 and later.
 *
 *  Comments            :   To add typedefs for a new platform/compiler combination:
 *                          1. Make a decent PLATFORM_XXX_YYY abbreviation.
 *                          2. Update the 'Platforms :' comment above.
 *                          3. Insert before the #else line:
 *                                  #elif defined(PLATFORM_XXX_YYY)
 *                             followed by the lines containing the type definitions.
 *                          5. If a type is not available on a platform/compiler DO NOT TRY to
 *                             define one anyway; it is better to have a compiler error showing
 *                             where the problem is than to deal with undefined unbehaviour.
 *
 *
 ===========================================================================================*/
#ifndef _PLATFORMTYPES_H
#define _PLATFORMTYPES_H

#ifdef _MSC_VER
#define PLATFORM_WORDSIZE   (32)  /* Native word size of platform */
#define PLATFORM_LITTLE_ENDIAN    /* Least significant byte of word stored at lowest address */

#define PATH_SEP ('\\')
#pragma warning( disable : 4711 ) /* suppress function xxx selected for automatic inline expansion */

#ifdef _SSC_STANDALONE
#define SSC_INTF_EXPORT
#define _SSC_LIB_BUILD
#else
#ifdef _SSC_LIB_BUILD
#define SSC_INTF_EXPORT __declspec ( dllexport )
#else
#define SSC_INTF_EXPORT __declspec ( dllimport )
#endif
#endif

    typedef unsigned int     UInt;
    typedef signed int       SInt;

    typedef unsigned char    UByte;
    typedef signed char      SByte;

    typedef unsigned short   UInt16;
    typedef signed short     SInt16;

    typedef unsigned int     UInt32;
    typedef signed int       SInt32;

    typedef unsigned __int64 UInt64;
    typedef signed __int64   SInt64;

    typedef float            FloatSSC;
    typedef double           Double;

    typedef char             Char;

    typedef int              Bool;

#elif defined(PLATFORM_LINUX_I386)
#define PLATFORM_WORDSIZE   (32)  /* Native word size of platform                            */
#define PLATFORM_LITTLE_ENDIAN    /* Least significant byte of word stored at lowest address */

#define PATH_SEP ('/')

#ifdef _SSC_LIB_BUILD
#define SSC_INTF_EXPORT
#else
#define SSC_INTF_EXPORT
#endif

    typedef unsigned int     UInt;
    typedef signed int       SInt;

    typedef unsigned char    UByte;
    typedef signed char      SByte;

    typedef unsigned short   UInt16;
    typedef signed short     SInt16;

    typedef unsigned int     UInt32;
    typedef signed int       SInt32;

    typedef unsigned long long UInt64;
    typedef signed long long   SInt64;

    typedef float            FloatSSC;
    typedef double           Double;

    typedef char             Char;

    typedef int              Bool;
#elif defined(PLATFORM_GIANTS)
#define PLATFORM_WORDSIZE   (32)  /* Native word size of platform                            */
#define PLATFORM_LITTLE_ENDIAN    /* Least significant byte of word stored at lowest address */

#ifdef _SSC_LIB_BUILD
#define SSC_INTF_EXPORT
#else
#define SSC_INTF_EXPORT
#endif

    typedef unsigned int     UInt;
    typedef signed int       SInt;

    typedef unsigned char    UByte;
    typedef signed char      SByte;

    typedef unsigned short   UInt16;
    typedef signed short     SInt16;

    typedef unsigned int     UInt32;
    typedef signed int       SInt32;

    typedef unsigned long    UInt64;
    typedef signed long      SInt64;

    typedef float            FloatSSC;
    typedef double           Double;

    typedef char             Char;

    typedef int              Bool;

#else
#error PLATFORMTYPES.h: PLATFORM_XXX_YYY macro not defined or not supported.
#endif

#define True  (1)
#define False (0)

#endif
