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
software module in 
#endifhardware or software products are advised that this
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
/* modification by Markus Multrus, fhg
   2000 Sept. 12
*/

#ifndef __INCLUDED_EPTOOL_H
#define __INCLUDED_EPTOOL_H

#define RS_FEC_CAPABILITY 0
/* only here for debugging reasons ... this stuff was thrown out of the standard ... */

/* Data Structure for UEP Encoder */
#include "bch.h"

#define CLASSMAX 10		/* Used in PayloadEncode */
#define PINTDEPTH 28
#define MAXPREDwCA 32
#define MAXHEADER 2000
#define MAXDATA (4096*7)
#define MAXBITS 6144*2
#define MAXCODE (MAXDATA*4)
#define MAXCONCAT 10

/* UEP Class Attribution */
typedef struct {
  unsigned int ClassBitCount;	/* class_bit_count */
  int ClassCodeRate;		/* class_code_rate */
  int ClassCRCCount;		/* class_crc_count */
  int Concatenate;		/* Concatenate(1) or not(0) */
  int FECType;			/* SRCPC(0) RS-Noconj(1) RS-conj(2) */
  int TermSW;                   /* termination_switch */
  int ClassInterleave;		/* Interleaved(1) or Non-Interleaved(0) */
} ClassAttribContent;

/* Ep Frame Attribution */
typedef struct EPFrameAttrib {		
  int ClassCount;		/* Class Count */
  int TotalIninf;		/* Number of inf bits in EP header */
  int UntilTheEnd;
  int TotalData;
  int TotalCRC;
  int *lenbit;
  ClassAttribContent *cattresc;
  ClassAttribContent *cattr;
  int ClassReorderedOutput;
  int ClassOutputOrder[CLASSMAX];
  int SortedClassOutputOrder[CLASSMAX];
  int OutputIndex[CLASSMAX];
} EPFrameAttrib;

/* EP Encoder-Decoder Configuration */
typedef struct EPConfig {
  int NPred;
  int Interleave;
  int BitStuffing;
  int NConcat;			/* Number of Concatenated Frames */
  EPFrameAttrib *fattr;
  int HeaderProtectExtend;	/* Normal (0) or SRCPC(1) */
  int HeaderRate;		/* Header SRCPC Rate */
  int HeaderCRC;		/* Header CRC Rate */
#if RS_FEC_CAPABILITY
  int RSFecCapability;		/* RS FEC Capability */
#endif
} EPConfig;


typedef struct headerLength {
  int headerLen; /* length of coded header part*/
  int interleavingWidth; /* interleaving width to coded header part */
} headerLength;


/* Prototypes */
#ifndef EPTOOL_BODY

int FECEncodeHeader(int data[], int len, int coded[], BCHTable *bchtbl, int Extend, int rate, int crc);
int FECDecodeHeader(int data[], int len, int coded[], BCHTable *bchtbl, int Extend, int rate, int crc);
int GenerateHeader(const EPConfig *epcfg, EPFrameAttrib *fattr, int pred, int data1[], headerLength *h1len, int data2[], headerLength *h2len, int bitstuffing, int nstuff);
int propdepth(int codedlen);
int RetrieveHeader(EPConfig *epcfg, int *pred, int data[], int *nstuff);
int RetrieveHeaderDeinter(EPConfig *epcfg, int *pred, int data[], int datalen, int remdata[], int *nstuff);
int PayloadEncode(int src[], int srclen, EPFrameAttrib *fattr, int coded[], int codedlen[]);
int PayloadDecode(int payload[], int coded[], int clen, EPFrameAttrib *fattr, int *check);

#endif

/* bintoint.c */
unsigned int bintoint( int bin[], int nbit );
void inttobin( int bin[], int nbit, int val);

/* interleave.c */
int interleaver(int pdata[], int plen, int rdata[], int rlen, int out[]);
int deinterleaver(int pdata[], int plen, int rdata[], int indata[], int inlen);

/* outinfo.c */
void OutInfoParse(FILE *inffp, EPConfig *epcfg, int reorderFlag, int CAtbl[MAXPREDwCA][CLASSMAX], int CAbits[MAXPREDwCA], int CAfunctionality);
void SideInfoParse(FILE *inffp, EPConfig *epcfg, int *pred, int *check, int CAfunctionality);
void SideInfoWrite(FILE *outfp, EPConfig *modepcfg, int modpred, int modcheck[], int reorderFlag, EPConfig *origepcfg, int CAtbl[MAXPREDwCA][CLASSMAX], int CAbits[MAXPREDwCA], int integrated);
void NullSideInfoWrite(FILE *outfp, EPConfig *modepcfg, int modpred, int modcheck[], EPConfig *origepcfg, int CAtbl[MAXPREDwCA][CLASSMAX], int CAbits[MAXPREDwCA], int integrated);
void PredSetConvert(EPConfig *origepcfg, EPConfig *modepcfg, int CAtbl[MAXPREDwCA][CLASSMAX], int CAbits[MAXPREDwCA], int origPredForCAPred[MAXPREDwCA], int reorderFlag );

/* srcpc2.c */
#define NOUT  4
int SRCPCEncode(int inseq[], int *outbuf[NOUT], int len, int TerminationSwitch);
int SRCPCPunc(int *inseq[], int from, int to, int ratefrom, int rateto, int outseq[]);
int SRCPCDecode(int *inseq[], int decseq[], int len, int TerminationSwitch);


/* reedsolomon.c */
int RSEncode(int *tbenc, int from, int to, int e_target, int *out);
int RSEncodeCount(int from, int to, int e_target);
int RSDecode(int *decseq, int from, int to, int e_target, int *inseq);

int IntCompare(const int *v1, const int *v2);
void ReorderOutputClasses (EPFrameAttrib *cfattr, int reorderFlag) ;

void enccrc(int in_buf[], int out_buf[], int n, int n_crc);
void deccrc(int in_buf[], int crc_buf[], int n, int n_crc, int *chk);

int SRCPCDepuncCount(int from, int to, int ratefrom, int rateto);
int SRCPCDepunc(int *outseq[], int from, int to, int ratefrom, int rateto, int inseq[]);
int SRCPCDepuncUTECount(int from, int ratefrom, int rateto, int inlen);

int recdeinter(int *coded, int codedlen, int depth, int *noncoded, int noncodedlen, int *buf);
int recinter(int *coded, int codedlen, int depth, int *noncoded, int noncodedlen, int *buf);
int recinterBytewise(int *coded, int codedlen, int rs_byte, int *noncoded, int noncodedlen, int *buf);
int recdeinterBytewise(int *coded, int codedlen, int rs_byte, int *noncoded, int noncodedlen, int *buf);

int def_GF(void);
int make_genpoly(int tt);
int RS_input(int *strm,int *seq, int len);
int RS_output(unsigned int *SEQ, int *strm, int len, int tt, int sw);
int RS_dec(int *seq, unsigned int *SEQ, int len, int tt);
int RS_enc(int *seq, int length, unsigned int *CODE,int tt);

/*void ConcatAttrib(EPConfig *epcfg, EPConfig *nepcfg);*/

#ifdef INTEGRATED
void SideInfoParseMod(FILE *ininfo, EPConfig *origepcfg, EPConfig *modepcfg, int origpred, int *modpred, int CAtbl[MAXPREDwCA][CLASSMAX], int PredCA[MAXPREDwCA][255]);   
/*void PredSetConvert(EPConfig *origepcfg, EPConfig *modepcfg, int CAtbl[MAXPREDwCA][CLASSMAX], int CAbits[MAXPREDwCA], int PredCA[MAXPREDwCA][255], int reorderFlag ); huh? */
void NullSideInfoWriteMod(FILE *outfp, EPConfig origepcfg, EPConfig modepcfg, int CAtbl[MAXPREDwCA][CLASSMAX], int CAbits[MAXPREDwCA], int modpred, int modcheck[CLASSMAX]);
void SideInfoWriteMod(FILE *outfp, EPConfig origepcfg, EPConfig modepcfg, int CAtbl[MAXPREDwCA][CLASSMAX], int CAbits[MAXPREDwCA], int modpred, int modcheck[CLASSMAX]);
#endif

#endif /* __INCLUDED_EPTOOL_H */
