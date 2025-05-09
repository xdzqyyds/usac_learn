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
#ifndef	ERR_STATE_H
#define	ERR_STATE_H

#define	EA_RMS_PREVIOUS		0x0001
#define	EA_RMS_REDUCE		0x0002
#define	EA_RMS_SMOOTH		0x0004
#define	EA_LSP_PREVIOUS		0x0008
#define	EA_LAG_PREVIOUS		0x0010
#define	EA_GAIN_PREVIOUS	0x0020
#define	EA_MODE_PREVIOUS	0x0040
#define	EA_MPULSE_RANDOM	0x0080
#define	EA_BLSP0_RECALC		0x0100

extern void changeErrState( long crc );
extern int  getErrState( void );
extern int  getErrAction( void );
extern void setErrSubState( int stat );
extern int  getErrSubState( void );

#endif	/* ERR_STATE_H */
