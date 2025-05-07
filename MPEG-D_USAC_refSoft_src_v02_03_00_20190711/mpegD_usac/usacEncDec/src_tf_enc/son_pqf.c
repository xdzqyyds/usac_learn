/*******************************************************************
This software module was originally developed by

Yoshiaki Oikawa (Sony Corporation) and
Mitsuyuki Hatanaka (Sony Corporation)

and edited by

Takashi Koike (Sony Corporation)

in the course of development of the MPEG-2 AAC/MPEG-4 System/MPEG-4
Video/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3. This
software module is an implementation of a part of one or more MPEG-2
AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio tools as specified by the
MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio standard. ISO/IEC
gives users of the MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio
standards free license to this software module or modifications
thereof for use in hardware or software products claiming conformance
to the MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio conforming products.The
original developer retains full right to use the code for his/her own
purpose, assign or donate the code to a third party and to inhibit
third party from using the code for non MPEG-2 AAC/MPEG-4
System/MPEG-4 Video/MPEG-4 Audio conforming products. This copyright
notice must be included in all copies or derivative works.

Copyright (C) 1996.
*******************************************************************/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "allHandles.h"
#include "sony_local.h"

#define		NPQFTAPS	96

#ifndef PI
#define	PI	(3.14159265358979)
#endif

/*******************************************************************************
	Set Coefficients for Efficient (2*kk*mm)-Tap PQF analysis

	mm:	Number of Bands
	kk:	(2*mm*kk) is the Tap Number
	p_proto[2*kk*mm]:	Prototype Filter Coef.
	(*ppp_h)[2*mm][kk]:	1st Coefficients
	(*ppp_s)[mm][mm]:	2nd Coefficients
*******************************************************************************/
static void son_setcoef_eff_pqfana(
	int	mm,
	int	kk,
	SCALAR	*p_proto,
	SCALAR	***ppp_h,
	SCALAR	***ppp_s
	)
{
	int	i, j, k, jj;
	SCALAR	w;

/* Set 1st Mul&Acc Coef's */
	*ppp_h = (SCALAR **) calloc(2*mm, sizeof(SCALAR *));
	for (j = 0; j < 2*mm; ++j) {
		(*ppp_h)[j] = (SCALAR *) calloc(kk, sizeof(SCALAR));
	}
	for (j = 0; j < 2*mm; ++j) {
		for (k = 0; k < kk; ++k) {
			if ((k%2) == 0) {
				(*ppp_h)[j][k] = p_proto[2*mm*k+j];
			}
			else {
				(*ppp_h)[j][k] = -p_proto[2*mm*k+j];
			}
		}
	}

/* Set 2nd Mul&Acc Coef's */
	*ppp_s = (SCALAR **) calloc(mm, sizeof(SCALAR *));
	for (i = 0; i < mm; ++i) {
		(*ppp_s)[i] = (SCALAR *) calloc(mm, sizeof(SCALAR));
	}
	for (i = 0; i < mm; ++i) {
		for (j = 0; j < mm; ++j) {
			jj = (j < (mm/2)) ? j: j+mm/2;
			w = ((2*i+1)*(mm-1-2*jj)*PI)/(4*mm);
			(*ppp_s)[i][j] = 2.0 * cos((double) w);
			if ((kk  %2) != 0) {
				(*ppp_s)[i][j] = -(*ppp_s)[i][j];
			}
		}
	}
}

/*******************************************************************************
	Implement (2*kk*mm)-Tap Efficient PQF Analysis

	mm:	Number of Bands
	kk:	(2*mm*kk) is the Tap Number
	p_proto[2*kk*mm]:	Prototype Filter Coef.
	pp_h[2*mm][kk]:		1st Coefficients
	pp_s[mm][mm]:		2nd Coefficients

	p_xnew[mm]:		Newly Input Data
	p_vout[mm]:		Output Data
*******************************************************************************/
static void son_eff_pqfana(
	int	mm,
	int	kk,
	SCALAR	**pp_h,
	SCALAR	**pp_s,
	SCALAR	*p_xnew,
	SCALAR	*p_vout,
	SCALAR	*p_buf
	)
{
	int	i, j, k, jj, ntaps;
	SCALAR	acc;
	SCALAR	*p_bb;

/* Work Area Buffer */
	p_bb = (SCALAR *) calloc(mm, sizeof(SCALAR));

/* Shift Buffer Data and Append New Data */
	ntaps = 2*kk*mm;
	for (j = 0; j < ntaps-mm; ++j) {
		p_buf[j] = p_buf[j+mm];
	}
	for (j = 0; j < mm; ++j) {
		p_buf[ntaps-mm+j] = p_xnew[j];
	}

/* 1st Mul&Acc Operations */
	for (j = 0; j < mm/2; ++j) {
		acc = 0.0;
		for (k = 0; k < kk; ++k) {
			jj = j;
			acc += pp_h[jj][k]*p_buf[2*mm*k+jj];
		}
		for (k = 0; k < kk; ++k) {
			jj = mm-1-j;
			acc += pp_h[jj][k]*p_buf[2*mm*k+jj];
		}
		p_bb[j] = acc;
	}
	for (j = mm/2; j < mm; ++j) {
		acc = 0.0;
		for (k = 0; k < kk; ++k) {
			jj = j+mm/2;
			acc += pp_h[jj][k]*p_buf[2*mm*k+jj];
		}
		for (k = 0; k < kk; ++k) {
			jj = 5*mm/2-1-j;
			acc -= pp_h[jj][k]*p_buf[2*mm*k+jj];
		}
		p_bb[j] = acc;
	}

/* 2nd Mul & Acc Operation */
	for (i = 0; i < mm; ++i) {
		acc = 0.0;
		for (j = 0; j < mm; ++j) {
			acc += pp_s[i][j]*p_bb[j];
		}
		p_vout[i] = acc;
	}

	free((char *)p_bb);
}

/*******************************************************************************
	PQF Analysis
*******************************************************************************/
void	son_pqf_main(
	double	timeSigBufCh[],
	int	block_size_samples,
	int	ch,
	double	*pqfOutBufCh[NBANDS]
	)
{
	/* This parameter will be removed outside */
	static	double	a_pqfbuf[MAX_TIME_CHANNELS][NPQFTAPS];

	static	int	initFlag = 0;
	static	double	a_pqfproto[NPQFTAPS];
	static	double	**pp_h, **pp_s;

	int	i, j;
	double	a_pqfout[NBANDS];

	if (initFlag == 0) {
		son_set_protopqf(a_pqfproto);
		son_setcoef_eff_pqfana(NBANDS, NPQFTAPS/(2*NBANDS), a_pqfproto,
				&pp_h, &pp_s);
		initFlag = 1;
	}

	for (i = 0; i < block_size_samples / NBANDS; i++) {
		son_eff_pqfana(NBANDS, NPQFTAPS/(2*NBANDS), pp_h, pp_s,
				timeSigBufCh+i*NBANDS, a_pqfout,
				a_pqfbuf[ch]);
		for (j = 0; j < NBANDS; j++) {
			pqfOutBufCh[j][i] = a_pqfout[j];
		}
	}
}

