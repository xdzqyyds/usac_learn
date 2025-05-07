/*
This software module was originally developed by
Souich Ohtsuka and Masahiro Serizawa (NEC Corporation)
and edited by

in the course of development of the
MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.
This software module is an implementation of a part of one or more
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio
standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards
free license to this software module or modifications thereof for use in
hardware or software products claiming conformance to the MPEG-2 AAC/
MPEG-4 Audio  standards. Those intending to use this software module in
hardware or software products are advised that this use may infringe
existing patents. The original developer of this software module and
his/her company, the subsequent editors and their companies, and ISO/IEC
have no liability for use of this software module or modifications
thereof in an implementation. Copyright is not released for non
MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer
retains full right to use the code for his/her  own purpose, assign or
donate the code to a third party and to inhibit third party from using
the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.
This copyright notice must be included in all copies or derivative works.
Copyright (c)1996.
*/
/*
 * Error Concealment
 */
#include	<stdio.h>

#include	"err_concealment.h"

#define	ENH_MAX_NUM	3

static unsigned long	gainBuff[ENH_MAX_NUM+1];
static unsigned long	gainBwsBuff;
static unsigned long	lagBuff;
static unsigned long	lagBwsBuff;
static float	rmsBuff;
static float	rmsBwsBuff;
static unsigned long	modeBuff;
static unsigned long	modeBuff_received;
static unsigned long	scModeBuff;
static unsigned long	scModePreBuff;
static long		fsModeBuff = (long)-1;

void
errConInit( long fsmod )
{
	int n;

	lagBuff = (unsigned long)0;
	lagBwsBuff = (unsigned long)0;
	rmsBuff = (float)0.0;
	rmsBwsBuff = (float)0.0;
	modeBuff = (unsigned long)0;
	for( n = 0; n <= ENH_MAX_NUM; n++ )
		gainBuff[n] = (unsigned long)0;
	gainBwsBuff = (unsigned long)0;
	modeBuff_received = (unsigned long)0;
	scModeBuff = (unsigned long)1;
	scModePreBuff = (unsigned long)1;
	fsModeBuff = fsmod;
}

/*
 * Save functions
 */
void
errConSaveGain( unsigned long gain, long enh )
{
	gainBuff[enh] = gain;
}

void
errConSaveGainBws( unsigned long gain )
{
	gainBwsBuff = gain;
}

void
errConSaveLag( unsigned long lag )
{
	lagBuff = lag;
}

void
errConSaveLagBws( unsigned long lag )
{
	lagBwsBuff = lag;
}

void
errConSaveRms( float rms )
{
	rmsBuff = rms;
}

void
errConSaveRmsBws( float rms )
{
	rmsBwsBuff = rms;
}

void
errConSaveMode( unsigned long mode )
{
	modeBuff = mode;
}

void
errConSaveMode_received( unsigned long mode )
{
	modeBuff_received = mode;
}

void
errConSaveScMode( unsigned long mode )
{
	scModePreBuff = scModeBuff;
	scModeBuff = mode;
}

/*
 * Load functions
 */
void
errConLoadGain( unsigned long *d, long enh )
{
	*d = gainBuff[enh];
}

void
errConLoadGainBws( unsigned long *d )
{
	*d = gainBwsBuff;
}

void
errConLoadLag( unsigned long *d )
{
	*d = lagBuff;
}

void
errConLoadLagBws( unsigned long *d )
{
	*d = lagBwsBuff;
}

void
errConLoadRms( float *d )
{
	*d = rmsBuff;
}

void
errConLoadRmsBws( float *d )
{
	*d = rmsBwsBuff;
}

void
errConLoadMode( unsigned long *d )
{
	*d = modeBuff;
}

void
errConLoadMode_received( unsigned long *d )
{
	*d = modeBuff_received;
}

unsigned long
errConGetScMode()
{
	return scModeBuff;
}

unsigned long
errConGetScModePre()
{
	return scModePreBuff;
}

long
errConGetFsMode()
{
	return fsModeBuff;
}
