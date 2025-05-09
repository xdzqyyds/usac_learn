/*
This software module was originally developed by
Koji Yoshida (Matsushita Communication Industrial Co.,Ltd.)
and edited by Souich Ohtsuka (NEC Corporation)
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
	Frame Erasure Concealment related sub-modules
	'98.11.20 by K.Yoshida (Matsushita Communication Industrial Co.,Ltd.)
*/

#include <stdio.h>
#include <stdlib.h>

#include "lpc_common.h"
#include "fe_sub.h"
#include "bitstream.h"

void bitstream_random( HANDLE_BSBITSTREAM bitstream )
{
	int	i, n, n1, n2;
	static short	seed = 0;
	short	rnd;

	n = BsBufferGetSize(BsGetBitBufferHandle(0,bitstream));
	n1 = n / 8;
	n2 = n - n1 * 8;	/* 0 <= n2 < 8 */

	for( i = 0; i < n1; i++ ){
		rnd = rndgen( &seed );
                BsBufferManipulateSetDataElement((unsigned char)(rnd & 0xff),i,BsGetBitBufferHandle(0,bitstream));
	}
	if( n2 != 0 ){
		rnd = rndgen( &seed );
                BsBufferManipulateSetDataElement((unsigned char)(rnd & ((0xff << (8-n2)) & 0xff)),n1,BsGetBitBufferHandle(0,bitstream));
	}
}

/****************************************/
/*	random sequence generate	*/
/****************************************/
short rndgen(short *SEED)
{
	long	d ;

	d = (long)(*SEED) ;
	d   = ( 521 * d + 259 ) & 0x0000ffff ;
	if( (d & 0x00008000) != 0 )
	{
		d = d - 65536 ;
	}
	*SEED = (short)d ;
	return(*SEED);
}
