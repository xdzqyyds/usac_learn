/**
 * @file MP4OSMacros.h
 * @brief Mac OS X Macros for the ISO file format library
 * @version 0.1
 *
 * @copyright This software module was originally developed by Apple Computer, Inc. in the course of
 * development of MPEG-4. This software module is an implementation of a part of one or more MPEG-4
 * tools as specified by MPEG-4. ISO/IEC gives users of MPEG-4 free license to this software module
 * or modifications thereof for use in hardware or software products claiming conformance to MPEG-4.
 * Those intending to use this software module in hardware or software products are advised that its
 * use may infringe existing patents. The original developer of this software module and his/her
 * company, the subsequent editors and their companies, and ISO/IEC have no liability for use of
 * this software module or modifications thereof in an implementation. Copyright is not released for
 * non MPEG-4 conforming products. Apple Computer, Inc. retains full right to use the code for its
 * own purpose, assign or donate the code to a third party and to inhibit third parties from using
 * the code for non MPEG-4 conforming products. This copyright notice must be included in all copies
 * or derivative works. Copyright (c) 1999.
 *
 */

#ifndef INCLUDED_MP4OSMACROS_H
#define INCLUDED_MP4OSMACROS_H

/* includes not needed by the library, but harmless, and useful to programs using library - dws */
#include <stdio.h>
#include <stdlib.h>

#ifndef TEST_RETURN
#define TEST_RETURN(err)
/* #define TEST_RETURN(err) assert((err)==0)*/
#endif

#define MP4_EXTERN(v) extern v

typedef long long u64;
typedef long long s64;

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef int s32;
typedef short s16;
typedef char s8;

/**
 * @brief Makes a four-character code when needed
 *
 * e.g. for a group type, sample-entry type etc.). The arguments are the four characters.
 * @ingroup Types
 */
#define MP4_FOUR_CHAR_CODE(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

#endif
