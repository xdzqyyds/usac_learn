/**********************************************************************
MPEG-4 Audio VM
Common module



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

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



Source file: common.c

 

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
06-jun-96   HP    new module based on ErrorMsg() from cmdline module
25-jun-96   HP    added CommonFreeAlloc()
26-aug-96   HP    CVS
25-jun-97   HP    added random()
16-oct-98   HP    fflush(stderr) in warning/exit
15-feb-00   HP    added random1init()
**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "allHandles.h"

#include "common_m4a.h"		/* common module */
#include "statistics.h"

/* ---------- variables ---------- */

static char *CMprogName = "<progname>";		/* program name */

static int debugLevel ;

/* ---------- functions ---------- */

/* CommonFreeAlloc() */
/* Free previously allocated memory if present, then allocate new memory. */

void *CommonFreeAlloc (
  void **ptr,			/* in/out: addr of memory pointer */
  size_t size)			/* in: memory size */
				/* returns: memory pointer */
				/*          or NULL if error*/
{
  if (*ptr != NULL)
    free(*ptr);
  return *ptr = malloc(size);
}


/* CommonProgName() */
/* Set program name for error/warning message. */

void CommonProgName (
  char *progName)		/* in: program name */
{
  CMprogName = progName;
}


/* CommonWarning() */
/* Print program name and warning message to stderr. */

void CommonWarning (
  char *message,		/* in: warning message */
  ...)				/* in: args as for printf */
{
  va_list args;

  va_start(args,message);
  fflush(stdout);
  fprintf(stderr,"%s: WARNING: ",CMprogName);
  vfprintf(stderr,message,args);
  fprintf(stderr,"\n");
  fflush(stderr);
  va_end(args);
}

void SetDebugLevel (int verbosityLevel)
{
  debugLevel = verbosityLevel;
}


void DebugVPrintf (
  int verbosityLevel, /* in: debug trace verbosity level */
  char *message,		/* in: warning message */
  va_list args)
{
  if (verbosityLevel <= debugLevel) {
    fflush(stderr);
    vfprintf(stderr,message,args);
    fprintf(stderr,"\n");
    fflush(stderr);
  }
}

void DebugPrintf (
  int verbosityLevel, /* in: debug trace verbosity level */
  char *message,		/* in: warning message */
  ...)				/* in: args as for printf */
{
  va_list args;

  va_start(args,message);
  DebugVPrintf(verbosityLevel, message, args);
  va_end(args);
}

/* CommonExit() */
/* Print program name and error message to stderr and exit program. */

void CommonExit (
  int errorCode,		/* in: error code for exit() */
  char *message,		/* in: error message */
  ...)				/* in: args as for printf */
{
  va_list args;

  va_start(args,message);
  fflush(stdout);
  fprintf(stderr,"%s: ERROR[%d]: ",CMprogName,errorCode);
  vfprintf(stderr,message,args);
  fprintf(stderr,"\n");
  fflush(stderr);
  va_end(args);
  exit (errorCode);
}


/* random1() */
/* Generate random long (uniform distribution 0..RND_MAX). */

static long common_m4a_randx = 1;
static long common_m4a_randx_p = 0;
static long common_m4a_randx_s = 0;
static long common_m4a_randx_i = 0;

long random1 (void)
{
  if (common_m4a_randx_p) {
    common_m4a_randx_i++;
    if (common_m4a_randx_i >= common_m4a_randx_p) {
      common_m4a_randx = common_m4a_randx_s;
      common_m4a_randx_i = 0;
    }
  }

  common_m4a_randx = (common_m4a_randx * 1103515245) + 12345;
  return (common_m4a_randx & RND_MAX);
}


/* random1init() */
/* Init seed / period of random1(). */

void random1init (
  long seed,		/* in: random seed */
  long period)		/* in: period of rand. sequence (0=+inf) */
{
  common_m4a_randx_s = common_m4a_randx = 1+seed;
  common_m4a_randx_p = period;
}


/* end of common.c */

