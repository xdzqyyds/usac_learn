/**********************************************************************
MPEG-4 Audio VM Module
parameter based codec - individual lines: common module



This software module was originally developed by

Heiko Purnhagen (University of Hannover / Deutsche Telekom Berkom)
Bernd Edler (University of Hannover / Deutsche Telekom Berkom)

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



Source file: indilinecom.c


Required libraries:
(none)

Required modules:
common.o

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BE    Bernd Edler, Uni Hannover <edler@tnt.uni-hannover.de>

Changes:
01-sep-96   HP    first version based on indiline.c
10-sep-96   BE
11-sep-96   HP
01-Mar-2002 HP    changing constants to double
**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "common_m4a.h"		/* common module */

#include "indilinecom.h"	/* indiline common module */

Tlar_enc_par lar_enc_par[3] = 
{
	{  2.00f,  0.75f, 0.30f, 0.500f, 0 },
	{ -0.75f,  0.75f, 0.30f, 0.500f, 0 },
	{  0.00f,  0.75f, 0.40f, 0.375f, 0 }
};

/*
Tlar_enc_par lar_enc_par[13] = 
{
	{  2.0,  0.80, 0.30, 0.45, 0 },
	{ -0.8,  0.80, 0.30, 0.43, 0 },
	{  0.0,  0.78, 0.40, 0.38, 0 },
	{  0.0,  0.70, 0.40, 0.33, 0 },
	{  0.0,  0.70, 0.40, 0.32, 0 },
	{  0.0,  0.65, 0.40, 0.30, 0 },
	{  0.0,  0.64, 0.40, 0.30, 0 },
	{  0.0,  0.63, 0.40, 0.30, 0 },
	{  0.0,  0.63, 0.40, 0.30, 0 },
	{  0.0,  0.63, 0.40, 0.30, 0 },
	{  0.0,  0.62, 0.40, 0.30, 0 },
	{  0.0,  0.61, 0.40, 0.30, 0 },	
	{  0.0,  0.60, 0.40, 0.30, 0 }
};
*/


Tlar_enc_par hlar_enc_par[8] = 
{
	{  5.00f, 0.75f, 0.4f,          0.50f, 2 },
	{ -1.50f, 0.75f, 0.4f,          0.50f, 2 },
	{  0.00f, 0.50f, 0.3f,          0.50f, 1 },
	{  0.00f, 0.50f, 0.3f,          0.50f, 1 },
	{  0.00f, 0.50f, 0.3f,          0.50f, 1 },
	{  0.00f, 0.50f, 0.3f,          0.50f, 1 },
	{  0.00f, 0.50f, 0.3f,          0.50f, 1 },
	{  0.00f, 0.50f, (float)HQSTEP, 0.50f, 0 }
};

/*
Tlar_enc_par hlar_enc_par[8] = 
{
	{  5.00, 0.75, 0.4,    0.50, 6 },
	{ -1.50, 0.75, 0.4,    0.50, 6 },
	{  0.00, 0.50, 0.3,    0.50, 5 },
	{  0.00, 0.50, 0.3,    0.50, 5 },
	{  0.00, 0.50, 0.3,    0.50, 5 },
	{  0.00, 0.50, 0.3,    0.50, 5 },
	{  0.00, 0.50, 0.3,    0.50, 5 },
	{  0.00, 0.50, HQSTEP, 0.50, 4 }
};
*/

/*

Tlar_enc_par hlar_enc_par[25] = 
{
	{  5.00, 0.80, 0.4,    0.50, 2 },
	{ -1.50, 0.80, 0.4,    0.50, 2 },
	{  0.00, 0.70, 0.3,    0.50, 1 },
	{  0.00, 0.57, 0.3,    0.50, 1 },
	{  0.00, 0.51, 0.3,    0.50, 1 },
	{  0.00, 0.45, 0.3,    0.50, 1 },
	{  0.00, 0.43, HQSTEP, 0.50, 0 },
	{  0.00, 0.39, HQSTEP, 0.50, 0 },
	{  0.00, 0.38, HQSTEP, 0.50, 0 },
	{  0.00, 0.37, HQSTEP, 0.50, 0 },
	{  0.00, 0.36, HQSTEP, 0.50, 0 },
	{  0.00, 0.35, HQSTEP, 0.50, 0 },	
	{  0.00, 0.34, HQSTEP, 0.50, 0 },
	{  0.00, 0.33, HQSTEP, 0.50, 0 },
	{  0.00, 0.32, HQSTEP, 0.50, 0 },
	{  0.00, 0.31, HQSTEP, 0.50, 0 },
	{  0.00, 0.30, HQSTEP, 0.50, 0 },
	{  0.00, 0.29, HQSTEP, 0.50, 0 },
	{  0.00, 0.28, HQSTEP, 0.50, 0 },
	{  0.00, 0.27, HQSTEP, 0.50, 0 },
	{  0.00, 0.26, HQSTEP, 0.50, 0 },
	{  0.00, 0.25, HQSTEP, 0.50, 0 },
	{  0.00, 0.24, HQSTEP, 0.50, 0 },
	{  0.00, 0.23, HQSTEP, 0.50, 0 },
	{  0.00, 0.23, HQSTEP, 0.50, 0 }
};


Tlar_enc_par hlar_enc_par[25] = 
{
	{  4.93, 0.80, 0.25, 0.47, 0 },
	{ -1.56, 0.80, 0.27, 0.47, 0 },
	{  0.00, 0.70, 0.29, 0.46, 0 },
	{  0.00, 0.57, 0.30, 0.46, 0 },
	{  0.00, 0.51, 0.30, 0.45, 0 },
	{  0.00, 0.45, 0.30, 0.45, 0 },
	{  0.00, 0.43, 0.35, 0.42, 0 },
	{  0.00, 0.39, 0.35, 0.42, 0 },
	{  0.00, 0.38, 0.35, 0.41, 0 },
	{  0.00, 0.37, 0.35, 0.41, 0 },
	{  0.00, 0.36, 0.35, 0.40, 0 },
	{  0.00, 0.35, 0.35, 0.40, 0 },	
	{  0.00, 0.34, 0.35, 0.40, 0 },
	{  0.00, 0.33, 0.35, 0.40, 0 },
	{  0.00, 0.32, 0.35, 0.40, 0 },
	{  0.00, 0.31, 0.35, 0.40, 0 },
	{  0.00, 0.30, 0.35, 0.40, 0 },
	{  0.00, 0.29, 0.35, 0.40, 0 },
	{  0.00, 0.28, 0.35, 0.40, 0 },
	{  0.00, 0.27, 0.35, 0.40, 0 },
	{  0.00, 0.26, 0.35, 0.40, 0 },
	{  0.00, 0.25, 0.35, 0.40, 0 },
	{  0.00, 0.24, 0.35, 0.40, 0 },
	{  0.00, 0.23, 0.35, 0.40, 0 },
	{  0.00, 0.23, 0.35, 0.40, 0 }
};
*/

TLPCEncPara LPCnoiseEncPar =
{
	2,		/* zstart	*/
	3,		/* nhcp		*/
	lar_enc_par,
	{1,2,3,4,5,6,7,9,11,13,15,17,19,21,23,LPCMaxNoisePara-1}
};

TLPCEncPara LPCharmEncPar =
{
	1000,		/* zstart	*/
	8,		/* nhcp		*/
	hlar_enc_par,
	{2,3,4,5,6,7,8,9,11,13,15,17,19,21,23,LPCMaxHarmPara-1}
};

int sdcILFTab[8][32] = {
{   0,  53,  87, 118, 150, 181, 212, 243, 275, 306, 337, 368, 399, 431, 462, 493,
  524, 555, 587, 618, 649, 680, 711, 743, 774, 805, 836, 867, 899, 930, 961, 992 },
{   0,  34,  53,  71,  89, 106, 123, 141, 159, 177, 195, 214, 234, 254, 274, 296,
  317, 340, 363, 387, 412, 438, 465, 494, 524, 556, 591, 629, 670, 718, 774, 847 },
{   0,  26,  41,  54,  66,  78,  91, 103, 116, 128, 142, 155, 169, 184, 199, 214,
  231, 247, 265, 284, 303, 324, 346, 369, 394, 422, 452, 485, 524, 570, 627, 709 },
{   0,  23,  35,  45,  55,  65,  75,  85,  96, 106, 117, 128, 139, 151, 164, 177,
  190, 204, 219, 235, 252, 270, 290, 311, 334, 360, 389, 422, 461, 508, 571, 665 },
{   0,  20,  30,  39,  48,  56,  64,  73,  81,  90,  99, 108, 118, 127, 138, 149,
  160, 172, 185, 198, 213, 228, 245, 263, 284, 306, 332, 362, 398, 444, 507, 608 },
{   0,  18,  27,  35,  43,  50,  57,  65,  72,  79,  87,  95, 104, 112, 121, 131,
  141, 151, 162, 174, 187, 201, 216, 233, 251, 272, 296, 324, 357, 401, 460, 558 },
{   0,  16,  24,  31,  38,  45,  51,  57,  64,  70,  77,  84,  91,  99, 107, 115,
  123, 132, 142, 152, 163, 175, 188, 203, 219, 237, 257, 282, 311, 349, 403, 493 },
{   0,  12,  19,  25,  30,  35,  41,  46,  51,  56,  62,  67,  73,  79,  85,  92,
   99, 106, 114, 122, 132, 142, 153, 165, 179, 195, 213, 236, 264, 301, 355, 452 }
};

int sdcILATab[32] = {
    0,  13,  27,  41,  54,  68,  82,  96, 110, 124, 138, 152, 166, 180, 195, 210,
  225, 240, 255, 271, 288, 305, 323, 342, 361, 383, 406, 431, 460, 494, 538, 602
};

int numHarmLineTab[32] = {
  3,  4,  5,  6,  7,  8,  9, 10, 12, 14, 16, 19, 22, 25, 29, 33,
 38, 43, 49, 56, 64, 73, 83, 94,107,121,137,155,175,197,222,250
};

int sampleRateTable[16] = {
 96000,88200,64000,48000,44100,32000,24000,22050,
 16000,12000,11025, 8000, 7350,    0,    0,    0
};

int maxFIndexTable[16] = {
 890,876,825,779,765,714,668,654,
 603,557,544,492,479,  0,  0,  0
};




/* ---------- functions ---------- */


/* IndiLineExit() */
/* Print "indiline" error message to stderr and exit program. */

void IndiLineExit(
  char *message,		/* in: error message */
  ...)				/* in: args as for printf */
{
  va_list args;

  va_start(args,message);
  fflush(stdout);
  fprintf(stderr,"indiline: ERROR: ");
  vfprintf(stderr,message,args);
  fprintf(stderr,"\n");
  va_end(args);
  CommonExit(99,"indiline module");
}


/* end of indilinecom.c */
