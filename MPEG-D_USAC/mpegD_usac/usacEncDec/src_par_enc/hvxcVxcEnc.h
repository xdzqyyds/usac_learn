
/*


This software module was originally developed by

    Kazuyuki Iijima (Sony Corporation)

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

#define		SUBFRMSIZE	(FRM / 2)
#define		Np		10
#define		Nfir		11
#define		FLTBUF		100

#define		N_SHAPE_L0	64
#define		N_GAIN_L0	16
#define		N_SHAPE_L1	32
#define		N_GAIN_L1	8

#define		Np		10
#define		Nfir		11
#define		FLTBUF		100

#define		N_SFRM_L0	2
#define		N_SFRM_L1	4


typedef struct
{
    float	g[N_GAIN_L0];
    float	s[N_SHAPE_L0][FRM / 2];
}
CbCELPL0;

typedef struct
{
    float	g[N_GAIN_L1];
    float	s[N_SHAPE_L1][FRM / 4];
}
CbCELPL1;

