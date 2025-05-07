#ifndef __CALCOVERHEAD_H__
#define __CALCOVERHEAD_H__

struct EPConfig;
struct EPFrameAttrib;

int CalculateHeaderOverhead (struct EPConfig *epcfg, int pred,int *d1len, int *d2len, int bitstuffing);
int CalculatePayloadOverhead (int srclen, struct EPFrameAttrib *fattr, int codedlen[]);

#endif /* __CALCOVERHEAD_H__ */
