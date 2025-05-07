/*
This material is strictly confidential and shall remain as such.

Copyright © 1999 VoiceAge Corporation.  All Rights Reserved.  Any unauthorized
use, distribution, reproduction or storage of this material or any part thereof
is strictly prohibited.

ACELP is registered trademark of VoiceAge Corporation in Canada and / or other
countries.
*/

#ifndef re8_h
#define re8_h

/* RE8 lattice quantiser constants */
#define NB_SPHERE 32
#define NB_LEADER 37
#define NB_LDSIGN 226
#define NB_LDQ3   9
#define NB_LDQ4   28

/* RE8 lattice quantiser functions in re8_*.c */
void RE8_PPV(float x[], int y[]);
void RE8_cod(int *y, int *n, long *I, int *k);
void RE8_dec(int nq, long I, int kv[], int y[]);
void RE8_vor(int y[], int *n, int k[], int c[], int *ka);
void re8_coord(int y[], int k[]);
void re8_k2y(int k[], int m, int y[]);

/* RE8 lattice quantiser tables */
extern const int tab_pow2[8];
extern const int tab_factorial[8];
extern const int Ia[NB_LEADER];
extern const unsigned char Ds[NB_LDSIGN];
extern const unsigned int Is[NB_LDSIGN];
extern const int Ns[], A3[], A4[];
extern const unsigned char Da[][8];
extern const unsigned int I3[], I4[];
extern const int Da_nq[], Da_pos[], Da_nb[];
extern const unsigned int Da_id[];

#endif
