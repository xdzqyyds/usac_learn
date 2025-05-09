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

#include <math.h>
#include <stdio.h>

#include "hvxc.h"
#include "hvxcProtect.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

/***************************************************/
/* definition for EP tool                          */
/***************************************************/

static unsigned char v2k[40] = {
     8,  9, 10, 11, 12, 19, 21, 30, 31, 32,
    33, 34, 20, 35, 36, 37, 38,  2,  0,  1,
    13, 14, 15, 16, 17, 18, 39, 22, 23, 24,
    25, 26, 27, 28, 29,  3,  4,  5,  6,  7
};

static unsigned char uv2k[40] = {
    11, 12, 13, 14, 15, 16, 17, 18, 19, 22,
    23, 24, 20, 21, 25, 26, 27,  2,  0,  1,
    28, 29, 30, 31, 32, 33,  3,  4,  5,  6,
    34, 35, 36, 37, 38, 39,  7,  8,  9, 10
};

static unsigned char v4k[80] = {
     8,  9, 10, 11, 12, 19, 20, 21, 22, 63,
    64, 65, 28, 66, 67, 68, 69,  2,  0,  1,
    13, 14, 15, 16, 17, 18, 77, 55, 56, 57,
    58, 59, 60, 61, 62,  3,  4,  5,  6,  7,
    29, 70, 71, 72, 73, 74, 75, 76, 23, 24,
    25, 26, 27, 78, 79, 30, 33, 34, 35, 36,
    37, 38, 39, 40, 41, 31, 42, 43, 44, 45,
    46, 47, 48, 49, 32, 50, 51, 52, 53, 54
};

static unsigned char uv4k[80] = {
    11, 12, 13, 14, 15, 16, 17, 18, 19, 34,
    35, 36, 20, 37, 38, 39, 40,  2,  0,  1,
    48, 49, 50, 51, 52, 53,  3,  4,  5,  6,
    54, 55, 56, 57, 58, 59,  7,  8,  9, 10,
    21, 41, 42, 43, 44, 45, 46, 47, 60, 61,
    62, 63, 64, 22, 23, 24, 65, 66, 67, 68,
    69, 25, 26, 27, 70, 71, 72, 73, 74, 28,
    29, 30, 75, 76, 77, 78, 79, 31, 32, 33
};

/* Appended by YM on 020110 */
static unsigned char v3k[80] = {
     8,  9, 10, 11, 12, 19, 20, 21, 22, 57,
    58, 59, 28, 60, 61, 62, 63,  2,  0,  1,
    13, 14, 15, 16, 17, 18, 71, 49, 50, 51,
    52, 53, 54, 55, 56,  3,  4,  5,  6,  7,
    29, 64, 65, 66, 67, 68, 69, 70, 23, 24,
    25, 26, 27, 72, 73, 30, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 31, 41, 42, 43, 44,
    45, 46, 47, 48, 74, 75, 76, 77, 78, 79
};

static unsigned char uv3k[80] = {
    11, 12, 13, 14, 15, 16, 17, 18, 19, 31,
    32, 33, 20, 34, 35, 36, 37,  2,  0,  1,
    45, 46, 47, 48, 49, 50,  3,  4,  5,  6,
    51, 52, 53, 54, 55, 56,  7,  8,  9, 10,
    21, 38, 39, 40, 41, 42, 43, 44, 57, 58,
    59, 60, 61, 22, 23, 24, 62, 63, 64, 65,
    66, 25, 26, 27, 67, 68, 69, 70, 71, 28,
    29, 30, 72, 73, 74, 75, 76, 77, 78, 79
};

/* Appended by Y.M on 991116 */
static unsigned char scV2k[80] = {
     8,  9, 10, 11, 12, 19, 21, 30, 31, 32,
    33, 34, 20, 35, 36, 37, 38,  2,  0,  1,
    13, 14, 15, 16, 17, 18, 39, 22, 23, 24,
    25, 26, 27, 28, 29,  3,  4,  5,  6,  7,
    45, 71, 72, 73, 74, 75, 76, 77, 40, 41,
    42, 43, 44, 78, 79, 46, 49, 50, 51, 52,
    53, 54, 55, 56, 57, 47, 58, 59, 60, 61,
    62, 63, 64, 65, 48, 66, 67, 68, 69, 70
};

static unsigned char scUv2k[80] = {
    11, 12, 13, 14, 15, 16, 17, 18, 19, 22,
    23, 24, 20, 21, 25, 26, 27,  2,  0,  1,
    28, 29, 30, 31, 32, 33,  3,  4,  5,  6,
    34, 35, 36, 37, 38, 39,  7,  8,  9, 10,
    40, 53, 54, 55, 56, 57, 58, 59, 60, 61,
    62, 63, 64, 41, 42, 43, 65, 66, 67, 68,
    69, 44, 45, 46, 70, 71, 72, 73, 74, 47,
    48, 49, 75, 76, 77, 78, 79, 50, 51, 52
};
/* Appended by Y.M on 991116 : end */

static unsigned char varUv2k[28] = {
     0,  1, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 22, 23, 24, 20, 21, 25, 26, 27,  2,
     3,  4,  5,  6,  7,  8,  9, 10
};

static unsigned char varV2k[40] = {
     0,  1,  8,  9, 10, 11, 12, 19, 21, 30,
    31, 32, 33, 34, 20, 35, 36, 37, 38,  2,
    13, 14, 15, 16, 17, 18, 39, 22, 23, 24,
    25, 26, 27, 28, 29,  3,  4,  5,  6,  7
};

static unsigned char varBGN4k[25] = {
     0,  1,  2,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 19, 20, 21, 17, 18, 22, 23, 24,
     3,  4,  5,  6,  7
};

static unsigned char varUv4k[40] = {
     0,  1, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 22, 23, 24, 20, 21, 25, 26, 27,  2,
    28, 29, 30, 31, 32, 33,  3,  4,  5,  6, 
    34, 35, 36, 37, 38, 39,  7,  8,  9, 10
};

static unsigned char varV4k[80] = {
     0,  1,  8,  9, 10, 11, 12, 19, 20, 21, 
    22, 63, 64, 65, 28, 66, 67, 68, 69,  2, 
    13, 14, 15, 16, 17, 18, 77, 55, 56, 57,
    58, 59, 60, 61, 62,  3,  4,  5,  6,  7,
    29, 70, 71, 72, 73, 74, 75, 76, 23, 24,
    25, 26, 27, 78, 79, 30, 33, 34, 35, 36,
    37, 38, 39, 40, 41, 31, 42, 43, 44, 45,
    46, 47, 48, 49, 32, 50, 51, 52, 53, 54
};

static unsigned char varVsc2k[80] = {
     0,  1,  8,  9, 10, 11, 12, 19, 21, 30,
    31, 32, 33, 34, 20, 35, 36, 37, 38,  2,
    13, 14, 15, 16, 17, 18, 39, 22, 23, 24,
    25, 26, 27, 28, 29,  3,  4,  5,  6,  7,
    45, 71, 72, 73, 74, 75, 76, 77, 40, 41,
    42, 43, 44, 78, 79, 46, 49, 50, 51, 52,
    53, 54, 55, 56, 57, 47, 58, 59, 60, 61,
    62, 63, 64, 65, 48, 66, 67, 68, 69, 70
};

void IPC_BitOrdering(
unsigned char enc[10],
unsigned char chout[],
int rate,
int vrMode,
int scalFlag)
{
    int i, k, num;
    unsigned char ch2[160], cch;
    unsigned char *vuvNk;

    if (vrMode == BM_CONSTANT) {
	if (scalFlag == SF_ON) {	/* Appended by Y.M on 991117 */
	    num = 80;
	} else if (rate == ENC2K) {	/* Appended by Y.M on 991117 : end */
	    num = 40;
        } else {
	    num = 80;
        }

	cch = enc[2] & 0x0030;	/* extract V/UV bits */

        if (cch != 0) {
	    if (scalFlag == SF_ON)	/* Appended by Y.M on 991117 */
	        vuvNk = scV2k;
	    else if (rate == ENC2K) 	/* Appended by Y.M on 991117 : end */
	        vuvNk = v2k;
	    else if (rate == ENC4K)
	        vuvNk = v4k;
            else
                vuvNk = v3k;
        } else {
            if (scalFlag == SF_ON)	/* Appended by Y.M on 991117 */
	        vuvNk = scUv2k;
	    else if (rate == ENC2K) 	/* Appended by Y.M on 991117 : end */
		vuvNk = uv2k;
	    else if (rate == ENC4K)
		vuvNk = uv4k;
            else
                vuvNk = uv3k;
	}

	for (i = 0; i < num; i++) {
	    k = vuvNk[i];
	    cch = (enc[i/8] & (0x1<<(8-(i%8)-1))) ? 1 : 0;
            ch2[k] = cch;
        }

	for (i = 0; i < num; i++, k++) {
	    if ((i%8) == 0) chout[i/8] = 0;
	    chout[i/8] |= ch2[i] << (8-(i%8)-1);
        }
    } else {	/* BM_VARIABLE */
	cch = (enc[0] & 0xc0) >> 6;    /* extract V/UV bits */

	if (rate == ENC2K && scalFlag == SF_OFF) {
	    if (cch == 0) { vuvNk = varUv2k; num = 28; }
	    else if (cch == 1) { vuvNk = varUv2k; num = 2; }
	    else { vuvNk = varV2k; num = 40; }
        } else {
	    if (cch == 0) { vuvNk = varUv4k; num = 40; }
	    else if (cch == 1) { vuvNk = varBGN4k; num = 25; }
	    else {
		if (scalFlag == SF_ON) vuvNk = varVsc2k;
	        else  vuvNk = varV4k; 
		num = 80; 
	    }
	}

	for (i = 0; i < num; i++) {
	    k = vuvNk[i];
	    cch = (enc[i/8] & (0x1<<(8-(i%8)-1))) ? 1 : 0;
            ch2[k] = cch;
        }
	for (i = 0; i < num; i++) {
	    if ((i%8) == 0) chout[i/8] = 0;
	    chout[i/8] |= ch2[i] << (8-(i%8)-1);
        }
    }
}

void IPC_BitReordering(
unsigned char chin[],
unsigned char dec[10],
int rate,
int vrMode,
int scalFlag)
{
    int i, k, num;
    unsigned char ch[160], ch2[80], cch;
    unsigned char *vuvNk;

    if (vrMode == BM_CONSTANT) {
        if (scalFlag == SF_ON) {	/* Appended by Y.M on 991117 */
	    num = 80;
        } else if (rate == DEC2K) {	/* Appended by Y.M on 991117 : end */
	    num = 40;
        } else {
	    num = 80;
        }

        for (i = 0; i < num; i++) 
            ch2[i] = (chin[i/8] & (0x1<<(8-(i%8)-1))) ? 1 : 0;

	if ((ch2[0]|ch2[1]) == 1) {
            if (scalFlag == SF_ON)	/* Appended by Y.M on 991117 */
		    vuvNk = scV2k;
	    else if (rate == ENC2K) 	/* Appended by Y.M on 991117 : end */
		    vuvNk = v2k;
	    else if (rate == ENC4K)     /* Appended by Y.M on 020110 */
		    vuvNk = v4k;
            else
                    vuvNk = v3k;
        } else {
            if (scalFlag == SF_ON)	/* Appended by Y.M on 991117 */
		    vuvNk = scUv2k;
	    else if (rate == ENC2K) 	/* Appended by Y.M on 991117 : end */
		    vuvNk = uv2k;
	    else if (rate == ENC4K)     /* Appended by Y.M on 020110 */
		    vuvNk = uv4k;
            else
                    vuvNk = uv3k;
	}

	for (i = 0; i < num; i++) {
	    if ((i%8) == 0) dec[i/8] = 0;
	    k = vuvNk[i];
	    dec[i/8] |= ch2[k] << (8-(i%8)-1);
        }
    } else {	/* BM_VARIABLE */
	cch = (chin[0] & 0xc0) >> 6;    /* extract V/UV bits */

        if (rate == DEC2K && scalFlag == SF_OFF) {
	    if (cch == 0) { vuvNk = varUv2k; num = 28; }
	    else if (cch == 1) { vuvNk = varUv2k; num = 2; }
	    else { vuvNk = varV2k; num = 40; }
        } else {
	    if (cch == 0) { vuvNk = varUv4k; num = 40; }
	    else if (cch == 1) { vuvNk = varBGN4k; num = 25; }
	    else {
	        if (scalFlag == SF_ON) vuvNk = varVsc2k;
	        else  vuvNk = varV4k; 
	        num = 80; 
	    }
	}

	for (i = 0; i < num; i++) 
	    ch[i] = (chin[i/8] & (0x1<<(8-(i%8)-1))) ? 1 : 0;
            
	for (i = 0; i < num; i++) {
	    if ((i%8) == 0) dec[i/8] = 0;
	    k = vuvNk[i];
	    dec[i/8] |= ch[k] << (8-(i%8)-1);
        }
    }
}

