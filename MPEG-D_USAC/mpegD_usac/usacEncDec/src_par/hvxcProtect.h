
/*

This software module was originally developed by

    Yuuji Maeda (Sony Corporation)

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

#ifndef _hvxcProtect_h
#define _hvxcProtect_h

/* HP 980211 */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************/
/* definition for EP tool                      */
/***********************************************/

#define CLS1_2K_LEN	22
#define CLS2_2K_LEN	 0
#define CLS3_2K_LEN	 4
#define CLS4_2K_LEN	 4
#define CLS5_2K_LEN	10

#define CLS1_3K_LEN	32
#define CLS2_3K_LEN	17 
#define CLS3_3K_LEN	 4
#define CLS4_3K_LEN	 4
#define CLS5_3K_LEN	17

#define CLS1_4K_LEN	33
#define CLS2_4K_LEN	22 
#define CLS3_4K_LEN	 4
#define CLS4_4K_LEN	 4
#define CLS5_4K_LEN	17

#define EP_ENC	0
#define EP_DEC	1


void IPC_BitOrdering(
unsigned char enc[10],
unsigned char chout[],
int rate,
int vrMode,
int scalFlag);

void IPC_BitReordering(
unsigned char chin[],
unsigned char dec[10],
int rate,
int vrMode,
int scalFlag);

#ifdef __cplusplus
}
#endif

#endif		/* _hvxcProtect_h */

