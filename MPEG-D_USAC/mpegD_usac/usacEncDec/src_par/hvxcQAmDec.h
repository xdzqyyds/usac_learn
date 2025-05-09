
/*

This software module was originally developed by

    Masayuki Nishiguchi and Kazuyuki Iijima (Sony Corporation)

    and edited by

    Akira Inoue (Sony Corporation)

    in the course of development of the MPEG-4 Audio standard (ISO/IEC 14496-3).
    This software module is an implementation of a part of one or more
    MPEG-4 Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
    standard (ISO/IEC 14496-3).
    ISO/IEC gives users of the MPEG-4 Audio standards (ISO/IEC 14496-3)
    free license to this software module or modifications thereof for use
    in hardware or software products claiming conformance to the MPEG-4
    Audio standards (ISO/IEC 14496-3).
    Those intending to use this software module in hardware or software
    products are advised that this use may infringe existing patents.
    The original developer of this software module and his/her company,
    the subsequent editors and their companies, and ISO/IEC have no
    liability for use of this software module or modifications thereof in
    an implementation.
    Copyright is not released for non MPEG-4 Audio (ISO/IEC 14496-3)
    conforming products. The original developer retains full right to use
    the code for his/her own purpose, assign or donate the code to a third
    party and to inhibit third party from using the code for non MPEG-4
    Audio (ISO/IEC 14496-3) conforming products.
    This copyright notice must be included in all copies or derivative works.

    Copyright (c)1996.

*/

#ifndef _hvxcQAmDec_h_
#define _hvxcQAmDec_h_

#define MAXNUMVQ	10
#define MAXSIZE0	1024
#define MAXSIZE1	1024
#define MAXSIZE2	16


#define MAXDIM0		8
#define MAXDIM1		8
#define MAXDIM2		8
#define MAXDIM3		44
#define GMAX		32
#define MAXITE1		20
#define EPSI		0.001
#define THRESHDIST	0.001
#define THRESHIP	25.
#define THRESHDISTA	14.
#define THRESHDISTAVQ	400.
#define THRESHDISTA1	50.

typedef struct
{
    int numpulse;
    int vqdim_lpc[MAXNUMVQ];
    int vqsize_lpc[MAXNUMVQ];
    int gsize_lpc[MAXNUMVQ];
}
vqscheme_lpc;
	
typedef struct
{
    float g0lpc[GMAX];
    float cb1lpc[MAXSIZE2][MAXDIM3];
    float cb2lpc[MAXSIZE2][MAXDIM3];
}
cbook_lpc;

typedef struct
{
    int num_vq;
    int dim_tot;
    int vqdim[MAXNUMVQ];
    int vqsize[MAXNUMVQ];
}
vqschm4k;

typedef struct
{
    float cb0[128][2];
    float cb1[1024][4];
    float cb2[512][4];
    float cb3[64][4];
}
cbook4k;

#endif		/* #ifndef _hvxcQAmDec_h_ */

