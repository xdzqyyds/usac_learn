/*******************************************************************
This software module was originally developed by

Yoshiaki Oikawa (Sony Corporation) and
Mitsuyuki Hatanaka (Sony Corporation)

and edited by


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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "allHandles.h"
#include "tf_mainHandle.h"

#include "common_m4a.h"
#include "sony_local.h"

/*******************************************************************************
	Return Log2(Gain) of ID
*******************************************************************************/
static int son_lngainof_id(int id)
{
	static int	a_lngain[npow2(IDGAINBITS)] = {
		-4, -3, -2, -1,  0,  1,  2,  3, 
		 4,  5,  6,  7,  8,  9, 10, 11
	};

	if (id < 0 || id >= npow2(IDGAINBITS)) {
          CommonExit ( 1, "lngainof_id(): Wrong ID. id=%d\n", id);
	}

	return(a_lngain[id]);
}

/*******************************************************************************
	Return ID of Log2(Gain)
*******************************************************************************/
int	son_idof_lngain(int lngain)
{
  int	id;

  for (id = 0; id < npow2(IDGAINBITS); id++){
    if (lngain == son_lngainof_id(id)) {
      return(id);
    }
  }
  CommonExit ( 1, "idof_lngain(): No ID. lngain=%d\n", lngain);
  return ( 0 ); 
}


/*******************************************************************************
	Interpolation Function for Gain Control 

	I: alev0	: Level at the Preceding Point
	I: alev1	: Level at the Following Point
	I: iloc		: Distance from the Preceding Point

	Interpolated Value is returned.
*******************************************************************************/
static double son_gainc_interpolate(double alev0, double alev1, int iloc)
{
	double	val;
	double	a0, a1;

	a0 = mylog2(alev0);
	a1 = mylog2(alev1);
	val = pow(2.0, (double) (((8-iloc)*a0 + iloc*a1)/8));

	return(val);
}

/*******************************************************************************
	Set Gain Control Window

	I: nsamples	: Num of Samples
	I: gainc0	: Gain Control Information for 1st Half
	I: gainc1	: Gain Control Information for 2st Half
	O: *p_ad	: Gain Control Window Function
			(Inverse of Level Augmentaion)
	I: windowSequence : Each windowSequence ID

	p_gcwind[nsamples] is obtained.
*******************************************************************************/
void son_gainc_window(
	int	nsamples,
	GAINC	gainc0,	
	GAINC	gainc1,
	GAINC	gainc2,
	double	*p_ad,
	int	windowSequence
	)
{
	int	a_aloc[10], *p_m;
	double	a_alev[10];
	double	a_alev2nd;
	int	lngain;
	int	i, j;
	double	*p_fmd0, *p_fmd1, *p_fmd2, *p_mod;
	int	max_loc_gainc0 = 0;
        int     max_loc_gainc1 = 0; 
        int     max_loc_gainc2 = 0;
	int flat_length;

	p_m = (int *) calloc(nsamples/2, sizeof(double));
	p_mod = (double *) calloc(nsamples, sizeof(double));
	p_fmd0 = (double *) calloc(nsamples/2, sizeof(double));
	p_fmd1 = (double *) calloc(nsamples/2, sizeof(double));
	p_fmd2 = (double *) calloc(nsamples/2, sizeof(double));

	switch(windowSequence){
	  case ONLY_LONG_SEQUENCE: max_loc_gainc0 = nsamples/2;
		               max_loc_gainc1 = nsamples/2;
		               max_loc_gainc2 = 0;
                       break;
	  case EIGHT_SHORT_SEQUENCE: max_loc_gainc0 = nsamples/2;
		                max_loc_gainc1 = nsamples/2;
		                max_loc_gainc2 = 0;
                        break;
	  case LONG_START_SEQUENCE: max_loc_gainc0 = nsamples/2;
		                max_loc_gainc1 = nsamples*7/32;
		                max_loc_gainc2 = nsamples/16;
                        break;
	  case LONG_STOP_SEQUENCE: max_loc_gainc0 = nsamples/16;
		               max_loc_gainc1 = nsamples*7/32;
		               max_loc_gainc2 = nsamples/2;
                       break;
	  default:
            CommonExit ( 1, "Block Type Error :%d \n",windowSequence);
            exit(1);
            break;
	}

/* 1st Area */
	for (i = 0; i < gainc0.natks; i++) {
		a_aloc[i+1] = 8 * gainc0.a_loc[i];
		if ((lngain = son_lngainof_id(gainc0.a_idgain[i])) < 0) {
			a_alev[i+1] = (double)1/npow2(-lngain);
		}
		else {
			a_alev[i+1] = (double)npow2(lngain);
		}
	}

	a_aloc[0] = 0;
	if (gainc0.natks == 0) {
		a_alev[0] = 1.0;
	}
	else {
		a_alev[0] = a_alev[1];
	}

	a_aloc[gainc0.natks+1] = max_loc_gainc0;
	a_alev[gainc0.natks+1] = 1.0;

	for (i = 0; i < max_loc_gainc0; i++) {
		p_m[i] = 0;
		for (j = 0; j <= gainc0.natks+1; j++) {
			if (a_aloc[j] <= i) {
				p_m[i] = j;
			}
		}
	}

	for (i = 0; i < max_loc_gainc0; i++) {
		if ((i >= a_aloc[p_m[i]]) && (i <= a_aloc[p_m[i]] + 7)) {
			p_fmd0[i] = son_gainc_interpolate(a_alev[p_m[i]],
				a_alev[p_m[i]+1], i - a_aloc[p_m[i]]);
		}
		else {
			p_fmd0[i] = a_alev[p_m[i]+1];
		}
	}

/* 2st Area */

	for (i = 0; i < gainc1.natks; i++) {
		a_aloc[i+1] = 8 * gainc1.a_loc[i];
		if ((lngain = son_lngainof_id(gainc1.a_idgain[i])) < 0) {
			a_alev[i+1] = (double)1/npow2(-lngain);
		}
		else {
			a_alev[i+1] = (double)npow2(lngain);
		}
	}

	a_aloc[0] = 0;
	if (gainc1.natks == 0) {
		a_alev[0] = 1.0;
	}
	else {
		a_alev[0] = a_alev[1];
	}
	
	a_alev2nd = a_alev[0];

	a_aloc[gainc1.natks+1] = max_loc_gainc1;
	a_alev[gainc1.natks+1] = 1.0;

	for (i = 0; i < max_loc_gainc1; i++) {
		p_m[i] = 0;
		for (j = 0; j <= gainc1.natks+1; j++) {
			if (a_aloc[j] <= i) {
				p_m[i] = j;
			}
		}
	}

	for (i = 0; i < max_loc_gainc1; i++) {
		if ((i >= a_aloc[p_m[i]]) && (i <= a_aloc[p_m[i]] + 7)) {
			p_fmd1[i] = son_gainc_interpolate(a_alev[p_m[i]],
					a_alev[p_m[i]+1], i - a_aloc[p_m[i]]);
		}
		else {
			p_fmd1[i] = a_alev[p_m[i]+1];
		}
	}

/* 3rd Area */
	if((windowSequence == LONG_START_SEQUENCE)||(windowSequence == LONG_STOP_SEQUENCE)){
		for (i = 0; i < gainc2.natks; i++) {
			a_aloc[i+1] = 8 * gainc2.a_loc[i];
			if ((lngain = son_lngainof_id(gainc2.a_idgain[i])) < 0) {
				a_alev[i+1] = (double)1/npow2(-lngain);
			}
			else {
				a_alev[i+1] = (double)npow2(lngain);
			}
		}

		a_aloc[0] = 0;
		if (gainc2.natks == 0) {
			a_alev[0] = 1.0;
		}
		else {
			a_alev[0] = a_alev[1];
		}

		a_aloc[gainc2.natks+1] = max_loc_gainc2;
		a_alev[gainc2.natks+1] = 1.0;

		for (i = 0; i < max_loc_gainc2; i++) {
			p_m[i] = 0;
			for (j = 0; j <= gainc2.natks+1; j++) {
				if (a_aloc[j] <= i) {
					p_m[i] = j;
				}
			}
		}

		for (i = 0; i < max_loc_gainc2; i++) {
			if ((i >= a_aloc[p_m[i]]) && (i <= a_aloc[p_m[i]] + 7)) {
				p_fmd2[i] = son_gainc_interpolate(
						a_alev[p_m[i]],
						a_alev[p_m[i]+1],
						i - a_aloc[p_m[i]]);
			}
			else {
				p_fmd2[i] = a_alev[p_m[i]+1];
			}
		}
	}

/* Generate Complete Gain Control Function */

	flat_length = 0;
	if(windowSequence == LONG_STOP_SEQUENCE){
		flat_length = nsamples/2-max_loc_gainc0-max_loc_gainc1;
		for(i = 0; i < flat_length; i++){
			p_mod[i] = 1.0;
		}
	}
	if((windowSequence == ONLY_LONG_SEQUENCE)||(windowSequence == EIGHT_SHORT_SEQUENCE)){
		a_alev[0] = 1.0;
	}

	for (i = 0; i < max_loc_gainc0; i++) {
		p_mod[i+flat_length] = a_alev[0] * a_alev2nd * p_fmd0[i];
	}
	for (i = 0; i < max_loc_gainc1; i++) {
		p_mod[i+flat_length+max_loc_gainc0] = a_alev[0] * p_fmd1[i];
	}
	if(windowSequence == LONG_START_SEQUENCE){
		for (i = 0; i < max_loc_gainc2; i++) {
			p_mod[i+max_loc_gainc0+max_loc_gainc1] = p_fmd2[i];
		}
		flat_length = nsamples/2 - max_loc_gainc1 - max_loc_gainc2;
		for (i = 0; i < flat_length; i++) {
			p_mod[i+max_loc_gainc0+max_loc_gainc1+max_loc_gainc2] = 1.0;
		}
	}
	else if(windowSequence == LONG_STOP_SEQUENCE){
		for (i = 0; i < max_loc_gainc2; i++) {
			p_mod[i+flat_length+max_loc_gainc0+max_loc_gainc1] = p_fmd2[i];
		}
	}
	for (i = 0; i < nsamples; i++) {
		p_ad[i] = 1.0 / p_mod[i];
	}
	
	free((char *)p_m);
	free((char *)p_mod);
	free((char *)p_fmd0);
	free((char *)p_fmd1);
	free((char *)p_fmd2);
}
