/*******************************************************************************
This software module was originally developed by

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips

and edited by

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
*******************************************************************************/



#ifndef _adifdec_h_
#define _adifdec_h_

#include		<stdio.h>
#include		<stdlib.h>
#include		<sys/types.h>
#include 		<math.h>
#include		<string.h>

#include "sac_interface.h"
#include "common_m4a.h" 

#define	PRINT		fprintf
#define	SO		stdout
#define	SE		stderr

typedef struct
{
    int	    islong;			
    int	    nsbk;			
    int	    bins_per_bk;		
    int	    sfb_per_bk;			
    int	    bins_per_sbk[MAX_SBK];	
    int	    sfb_per_sbk[MAX_SBK];	
    int	    sectbits[MAX_SBK];
    short   *sbk_sfb_top[MAX_SBK];	
    short   *sfb_width_128;		
    short   bk_sfb_top[200];		
    int	    num_groups;
    short   group_len[8];
    short   group_offs[8];
} Info;

extern long	bno;
extern int	debug[256];
#ifdef CT_SBR
extern int	sbrPresentInFillElem;
#endif

typedef double		Float;
typedef	unsigned char	byte;

typedef struct {
  byte    this_bk;
  byte    prev_bk;
} Wnd_Shape;


void		decoder_init(void);
void            decoder_free(void);



void		decoder_selectProgram(int selectedProgram);


void		decoder_setupParameters(int sampling_rate_idx);
#ifdef DRC
void		decoder_setDrc(Float drc_high, Float drc_low, int drc_ref_level);
#endif



void		dec_parse(void);


void		dec_decode(Float ***output_data, int *block_length);


void		dec(Float ***output_data, int *block_length);


#endif
