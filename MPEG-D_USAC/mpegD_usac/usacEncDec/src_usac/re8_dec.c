/*
This material is strictly confidential and shall remain as such.

Copyright © 1999 VoiceAge Corporation.  All Rights Reserved.  Any unauthorized
use, distribution, reproduction or storage of this material or any part thereof
is strictly prohibited.

ACELP is registered trademark of VoiceAge Corporation in Canada and / or other
countries.
*/

#include <stdio.h>
#include <stdlib.h>

#include <math.h>
#include <assert.h>
#include "re8.h"

extern void re8_k2y(int *k, int m, int *y);
static void re8_decode_base_index(int *n, long *I, int *y);
static void re8_decode_rank_of_permutation(int t, int *xs, int *x);
static int table_lookup(const unsigned int *table, unsigned int index, int range);

/*--------------------------------------------------------------------------
  RE8_dec(n, I, k, y)
  MULTI-RATE INDEXING OF A POINT y in THE LATTICE RE8 (INDEX DECODING)
  (i) n: codebook number (*n is an integer defined in {0,2,3,4,..,n_max})
  (i) I: index of c (pointer to unsigned 16-bit word)
  (i) k: index of v (8-dimensional vector of binary indices) = Voronoi index
  (o) y: point in RE8 (8-dimensional integer vector)
  note: the index I is defined as a 32-bit word, but only
  16 bits are required (long can be replaced by unsigned integer)
  --------------------------------------------------------------------------
 */
void RE8_dec(int n, long I, int k[], int y[])
{
  int i, m, v[8];

  /* decode the sub-indices I and kv[] according to the codebook number n:
     if n=0,2,3,4, decode I (no Voronoi extension)
     if n>4, Voronoi extension is used, decode I and kv[] */
  if (n <= 4)
  {
    re8_decode_base_index(&n, &I, y);
  }
  else {
    /* compute the Voronoi modulo m = 2^r where r is extension order */
    m = 1;
    while (n > 4)
    {                                     
      m *= 2;
      n -= 2;
    }
    /* decode base codebook index I into c (c is an element of Q3 or Q4)
       [here c is stored in y to save memory] */
    re8_decode_base_index(&n, &I, y);
    /* decode Voronoi index k[] into v */
    re8_k2y(k, m, v);
    /* reconstruct y as y = m c + v (with m=2^r, r integer >=1) */
    for (i=0;i<8;i++)
    {
      y[i] = m*y[i] + v[i];
	}
  }
  return;
}

/*--------------------------------------------------------------------------
  re8_decode_base_index(n, I, y)
  DECODING OF AN INDEX IN Qn (n=0,2,3 or 4)
  (i) n: codebook number (*n is an integer defined in {0,2,3,4})
  (i) I: index of c (pointer to unsigned 16-bit word)
  (o) y: point in RE8 (8-dimensional integer vector)
  note: the index I is defined as a 32-bit word, but only
  16 bits are required (long can be replaced by unsigned integer)
  --------------------------------------------------------------------------
 */
static void re8_decode_base_index(int *n, long *I, int *y)
{
  int i,im,t,sign_code,ka,ks,rank,leader[8];
  unsigned int index;

  if (*n < 2)
  {
    for (i=0;i<8;i++) y[i]=0;
  }
  else
  {
    index = (unsigned int)*I;
    /* search for the identifier ka of the absolute leader (table-lookup)
       Q2 is a subset of Q3 - the two cases are considered in the same branch
     */
    switch (*n)
    {
      case 2:
      case 3:
        i = table_lookup(I3, index, NB_LDQ3);
        ka = A3[i];
        break;
      case 4:
        i = table_lookup(I4, index, NB_LDQ4);
        ka = A4[i];
        break;
    }
    /* reconstruct the absolute leader */
    for (i=0;i<8;i++)
    {
	  leader[i]=Da[ka][i];
    }
    /* search for the identifier ks of the signed leader (table look-up)
       (this search is focused based on the identifier ka of the absolute
        leader)*/
    t=Ia[ka];
    im=Ns[ka];
    ks = table_lookup(Is+t, index, im);

    /* reconstruct the signed leader from its sign code */
    sign_code = 2*Ds[t+ks];
    for (i=7; i>=0; i--)
    {
      leader[i] *= (1-(sign_code&2));
      sign_code >>= 1;
    }

    /* compute and decode the rank of the permutation */
    rank = index - Is[t+ks];    /* rank = index - cardinality offset */

    re8_decode_rank_of_permutation(rank, leader, y);
  }
  return;
}

/* table look-up of unsigned value: find i where index >= table[i]
   Note: range must be >= 2, index must be >= table[0] */
static int table_lookup(const unsigned int *table, unsigned int index, int range)
{
  int i;

  for (i=4; i<range; i+=4)
  {
    if (index < table[i]) break;
  }
  if (i > range) i = range;

  if (index < table[i-2]) i -= 2;
  if (index < table[i-1]) i--;
  i--;

  return(i);    /* index >= table[i] */
}

/*--------------------------------------------------------------------------
  re8_decode_rank_of_permutation(rank, xs, x)
  DECODING OF THE RANK OF THE PERMUTATION OF xs
  (i) rank: index (rank) of a permutation
  (i) xs:   signed leader in RE8 (8-dimensional integer vector)
  (o) x:    point in RE8 (8-dimensional integer vector)
  --------------------------------------------------------------------------
 */
static void re8_decode_rank_of_permutation(int rank, int *xs, int *x)
{
  int i, j, a[8], w[8], B, fac, fac_B, target;

  /* --- pre-processing based on the signed leader xs ---
     - compute the alphabet a=[a[0] ... a[q-1]] of x (q elements)
       such that a[0]!=...!=a[q-1]
       it is assumed that xs is sorted in the form of a signed leader
       which can be summarized in 2 requirements:
          a) |xs[0]| >= |xs[1]| >= |xs[2]| >= ... >= |xs[7]|
          b) if |xs[i]|=|xs[i-1]|, xs[i]>=xs[i+1]
       where |.| indicates the absolute value operator
     - compute q (the number of symbols in the alphabet)
     - compute w[0..q-1] where w[j] counts the number of occurences of
       the symbol a[j] in xs
     - compute B = prod_j=0..q-1 (w[j]!) where .! is the factorial */
					    /* xs[i], xs[i-1] and ptr_w/a*/
  j = 0;
  w[j] = 1;
  a[j] = xs[0];
  B = 1;
  for (i=1; i<8; i++)
  {
    if (xs[i] != xs[i-1])
	{
	  j++;
	  w[j] = 1;
	  a[j] = xs[i];
	}
	else {
      w[j]++;
      B *= w[j];
	}
  }

  /* --- actual rank decoding ---
     the rank of x (where x is a permutation of xs) is based on
     Schalkwijk's formula
     it is given by rank=sum_{k=0..7} (A_k * fac_k/B_k)
     the decoding of this rank is sequential and reconstructs x[0..7]
     element by element from x[0] to x[7]
     [the tricky part is the inference of A_k for each k...]
   */

  if (w[0] == 8)
  {
    for (i=0; i<8; i++) x[i] = a[0];    /* avoid fac of 40320 */
  }
  else {
    target = rank*B;
    fac_B = 1;
    /* decode x element by element */
    for (i=0; i<8; i++)
      {
        fac = fac_B * tab_factorial[i];   /* fac = 1..5040 */
        j=-1;
        do
	  {
            target -= w[++j]*fac;           
	  }
        while (target >= 0);              /* max of 30 tests / SV */
        x[i] = a[j];
        /* update rank, denominator B (B_k) and counter w[j] */
        target += w[j]*fac;               /* target = fac_B*B*rank */
        fac_B *= w[j];
        w[j]--;
      }
  }
  
  return;
}




