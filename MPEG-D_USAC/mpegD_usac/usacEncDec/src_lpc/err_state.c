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
 * Error State Manager
 */
#include	<stdio.h>
#include	"err_state.h"

#define	NUM_STATES	6


static int curState;
static int curSubState;
static int transTable[NUM_STATES][2] = {
	/* good  bad */
	{     0,   1  },	/* state 0 */
	{     5,   2  },	/* state 1 */
	{     5,   3  },	/* state 2 */
	{     5,   4  },	/* state 3 */
	{     5,   4  },	/* state 4 */
	{     0,   1  }		/* state 5 */
};

static int actionTable[NUM_STATES] = {
	0,
	EA_RMS_PREVIOUS | EA_RMS_REDUCE | EA_LSP_PREVIOUS | EA_LAG_PREVIOUS
							  | EA_GAIN_PREVIOUS
							  | EA_MODE_PREVIOUS
							  | EA_MPULSE_RANDOM
							  | EA_BLSP0_RECALC,
	EA_RMS_PREVIOUS | EA_RMS_REDUCE | EA_LSP_PREVIOUS | EA_LAG_PREVIOUS
							  | EA_GAIN_PREVIOUS
							  | EA_MODE_PREVIOUS
							  | EA_MPULSE_RANDOM
							  | EA_BLSP0_RECALC,
	EA_RMS_PREVIOUS | EA_RMS_REDUCE | EA_LSP_PREVIOUS | EA_LAG_PREVIOUS
							  | EA_GAIN_PREVIOUS
							  | EA_MODE_PREVIOUS
							  | EA_MPULSE_RANDOM
							  | EA_BLSP0_RECALC,
	EA_RMS_PREVIOUS | EA_RMS_REDUCE | EA_LSP_PREVIOUS | EA_LAG_PREVIOUS
							  | EA_GAIN_PREVIOUS
							  | EA_MODE_PREVIOUS
							  | EA_MPULSE_RANDOM
							  | EA_BLSP0_RECALC,
	EA_RMS_SMOOTH
};

void
changeErrState( long crc )
{
#ifdef	DEBUG_TRACE
	++nframe;
#endif

	if( crc != 0 )
		crc = 1;

	if( crc || curState != transTable[curState][crc] ){
#ifdef	DEBUG_TRACE
		fprintf( stderr, "[%d] State: %d --> %d\n", nframe, curState,
			 transTable[curState][crc] );
#endif
		if( curState != transTable[curState][crc] ){
			curSubState = 0;
			curState = transTable[curState][crc];
		}
	}
}

int
getErrState( void )
{
	return curState;
}

int
getErrAction( void )
{
	return actionTable[curState];
}

void
setErrSubState( int stat )
{
	curSubState = stat;
}

int
getErrSubState( void )
{
	return curSubState;
}
