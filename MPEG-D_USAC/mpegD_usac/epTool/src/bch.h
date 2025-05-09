/*****************************************************************************
 *                                                                           *
"This software module was originally developed by NTT Mobile
Communications Network, Inc. in the course of development of the
MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and
3. This software module is an implementation of a part of one or more
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4
Audio standard. ISO/IEC gives users of the MPEG-2 AAC/MPEG-4 Audio
standards free license to this software module or modifications
thereof for use in hardware or software products claiming conformance
to the MPEG-2 AAC/MPEG-4 Audio standards. Those intending to use this
software module in hardware or software products are advised that this
use may infringe existing patents. The original developer of this
software module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not
released for non MPEG-2 AAC/MPEG-4 Audio conforming products.The
original developer retains full right to use the code for his/her own
purpose, assign or donate the code to a third party and to inhibit
third party from using the code for non MPEG-2 AAC/MPEG-4 Audio
conforming products. This copyright notice must be included in all
copies or derivative works." Copyright(c)1998.
 *                                                                           *
 ****************************************************************************/
/*
  BCH Code encoder/decoder with table lookup
  Data structure/parameters definitions
  1998 Feb. 20  Toshiro Kawahara
*/

#ifndef __INCLUDED_BCH_H
#define __INCLUDED_BCH_H

typedef enum {
  BCH_3116, BCH_3111, BCH_1511, BCH_155, GOLAY_2312, BCH_74, BCH_157
} BCHType;


#define GPMAX      21
#define CODEMAX    31
#define INFMAX     16
#define PARITYMAX  20

typedef struct {
  int initialized;
  int codelen;
  int inflen;
  int gentbl[CODEMAX];
  int inftbl[(1<<INFMAX)];			
  int paritytbl[(1<<PARITYMAX)];
} BCHTable;


void 
maketables(BCHTable *BCHtbl, BCHType type, int ThreshBCH);
void
bchenc(BCHTable *BCHtbl, int inf[], int parity[]);
int
bchdec(BCHTable *BCHtbl, int code[], int dec[]);

#endif /* __INCLUDED_BCH_H */
