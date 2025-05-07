/*
This software module was originally developed by
Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.)
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
Copyright (c)1996, 1997.
*/

/*	pan_par_const.h		*/
/*	Description of constant values used in the parametric core */
/*		06/18/97 Ver 3.01 */

#define CELP_LPC_TOOL

/* LSP quantizer */
#define PAN_N_DC_LSP_PAR 8
#define PAN_LSP_AR_R_PAR 0.7
#define PAN_MINGAP_PAR (4./256.)

/* Bit allocation */
#define	PAN_BIT_LSP18_0	5
#define	PAN_BIT_LSP18_1	0
#define	PAN_BIT_LSP18_2	7
#define	PAN_BIT_LSP18_3	5
#define	PAN_BIT_LSP18_4	1

/* LPC analysis with CELP tools */
#define PAN_LPC_ORDER_PAR 10
#define PAN_WIN_LEN_PAR 256
#define PAN_NUM_ANA_PAR 1
#define PAN_GAMMA_BE_PAR (.9902)
#define PAN_BITRATE_PAR 2000 /* dummy */
#define PAN_WIN_OFFSET_PAR 0

#define PAN_PI 3.14159265358979
