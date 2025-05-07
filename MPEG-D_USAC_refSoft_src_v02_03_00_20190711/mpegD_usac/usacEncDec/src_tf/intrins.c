/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of
development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7,
14496-1,2 and 3. This software module is an implementation of a part of one or more
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4
Audio standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio
standards free license to this software module or modifications thereof for use in
hardware or software products claiming conformance to the MPEG-2 AAC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing patents.
The original developer of this software module and his/her company, the subsequent
editors and their companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not released for
non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the
code to a third party and to inhibit third party from using the code for non
MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works."
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "all.h"
#include "common_m4a.h"
#include "port.h"

static long	total1;
static long	total2;

void* mal1(long size)
{
    char *mem;

    mem = (char *)malloc(size);
    total1 += size;
    /*	printf( "total1 = %ld (%ld)\n", total1, size); */
    if(!mem){
      CommonExit(1, "out of memory 1\n");
    }
    return mem;
}

void* mal2(long size)
{
    char *mem;

    mem = (char *)malloc(size);
    total2 += size;
    /*	printf( "total2 = %ld (%ld)\n", total2, size); */
    if(!mem){
      CommonExit(1, "out of memory 2\n");
    }
    return mem;
}

long f2ir(float x)
{
    if(x >= 0)
	return (long) (x+.5);
    return -(long)(-x + .5);
}

void fltcpy(Float *dp1, Float *dp2, int cnt)
{
    memcpy(dp1, dp2, cnt*sizeof(*dp1));
}

void fltset(Float *dp1, Float dval, int cnt)
{
    Float *dp3 = dp1 + cnt;

    while(dp1 < dp3)
	*dp1++ = dval;
}

void fltclr(Float *dp1, int cnt)
{
    memset(dp1, 0, cnt*sizeof(*dp1));
}

void fltclrs(float *dp1, int cnt)
{
    memset(dp1, 0, cnt*sizeof(*dp1));
}

void intcpy(int *ip1, int *ip2, int cnt)
{
    memcpy(ip1, ip2, cnt*sizeof(*ip1));
}

void intclr(int *ip1, int cnt)
{
    memset(ip1, 0, cnt*sizeof(*ip1));
}

void shortclr(short *ip1, int cnt)
{
    memset(ip1, 0, cnt*sizeof(*ip1));
}


void byteclr(byte *ip1, int cnt)
{
    memset(ip1, 0, cnt);
}
