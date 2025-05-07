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
 * Error Concealment
 */
#ifndef	ERR_CONCEALMENT_H
#define	ERR_CONCEALMENT_H

#include	"err_state.h"

/*
 * BER/FER condition
 *    0 --> BER
 *    1 --> FER
 */
#define	FLG_BER_or_FER	0

/* subframe dependent mode */
#define	EC_SF_DEPEND
#define EC_SF_DEPEND_SZ1	80
#define EC_SF_DEPEND_SZ2	40
#define	EC_SF_DEPEND_FAC1	((float)2048/8192)
#define	EC_SF_DEPEND_FAC2	((float)1024/8192)

/* random irregular pulse */
#define	_EC_RAND_IRR_PULSE

/*
 * crc
 */
extern long	crc_dmy( void );

/*
 * error concealment
 */
extern void errConInit( long );
extern void errConSaveGain( unsigned long, long );
extern void errConSaveGainBws( unsigned long );
extern void errConSaveLag( unsigned long );
extern void errConSaveLagBws( unsigned long );
extern void errConSaveRms( float );
extern void errConSaveRmsBws( float );
extern void errConSaveMode( unsigned long );
extern void errConLoadGain( unsigned long *, long );
extern void errConLoadGainBws( unsigned long * );
extern void errConLoadLag( unsigned long * );
extern void errConLoadLagBws( unsigned long * );
extern void errConLoadRms( float * );
extern void errConLoadRmsBws( float * );
extern void errConLoadMode( unsigned long * );

extern void errConSaveMode_received( unsigned long );
extern void errConLoadMode_received( unsigned long * );

extern void errConSaveScMode( unsigned long );
extern unsigned long errConGetScMode( void );
extern unsigned long errConGetScModePre( void );

extern long errConGetFsMode( void );

#endif	/* ERR_CONCEALMENT_H */
