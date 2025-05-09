
/*

This software module was originally developed by

    Kazuyuki Iijima (Sony Corporation)

    and edited by

    Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.),
    Yuji Maeda (Sony Corporation) and
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

#ifndef _hvxc_h
#define _hvxc_h

#define LD_LEN 20

#define	SAMPLE		256
#define	OVERLAP		96
#define	FRM		160
#define	LPCORDER	10
#define	P	 	LPCORDER
#define IP              8
#define	NUMPITCH	256
#define	JISU		9
#define	R		8
#define	LEAKFAC		1.0
#define	OVER_R		8
#define	DM		10

#define N_SFRM_L0       2
#define N_SFRM_L1       4

#define L_VQ		6

/* correct rateMode assignment(AI 990205) */
#define ENC4K		1
#define ENC2K		0
#define ENC3K		2

#define DEC4K		1
#define DEC2K		0
#define DEC3K		2
#define DEC0K		3
/*
#define ENC4K           0
#define ENC2K           1

#define DEC4K           0
#define DEC2K           1
#define DEC3K           2
#define DEC0K           3
*/
#define DM_SHORT	0
#define DM_LONG		1

#define BM_CONSTANT	0
#define BM_VARIABLE	1

#define PM_SHORT	0
#define PM_LONG		1

#define SF_OFF		0
#define SF_ON		1

#define N_CLS_2K	4	
#define N_CLS_4K	5

#define	ACR		11

#define N_LSP_ENH	256

#define BGN_INTVL	8

#define BGN_CNT         6
#define BGN_INTVL_TX   16
#define BGN_INTVL_RX   12

/* test_mode definition */
#define TM_NORMAL 		0x0 /* normal operation */
#define TM_POSFIL_DISABLE 	0x1 /* postfilter and post processing are skipped */
#define TM_INIT_PHASE_ZERO	0x2 /* initial values of harmonic phase are reset to zeros */
#define TM_NOISE_ADD_DISABLE	0x4 /* noise component addition are disabled */
#define TM_VXC_DISABLE 		0x8 /* the output of Time Domain Decoder is disabled */

typedef struct
{
    float       enh[N_LSP_ENH][DM];
}
CbLsp;

typedef struct
{
    int         nVq[L_VQ];
}
IdLsp;

typedef struct
{
    int         idSL0[2], idGL0[2];
    int         idSL1[4], idGL1[4];
    int         upFlag;
}
IdCelp;

typedef struct
{
    int		idS0, idS1, idG, id4kS0, id4kS1, id4kS2, id4kS3;
}
IdAm;

typedef struct
{
    int         idSL0[2], idGL0[2];
    int         idSL1[4], idGL1[4];
}
IdUV;


typedef struct
{
    int		pchcode, idS0, idS1, idG, id4kS0, id4kS1, id4kS2, id4kS3;
}
IdV;

typedef struct
{
    IdLsp	idLsp;

    int		vuvFlag;
    int		bgnFlag;
		    
    union VUVPrm
    {
	IdUV	idUV;
	IdV	idV;
    }
    vuvPrm;
    int         upFlag;
}
EncPrm;

#endif /* _hvxc_h */

