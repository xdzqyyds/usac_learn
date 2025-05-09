/*****************************************************************************
 *                                                                           *
"This software module was originally developed by 

Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)

and edited by

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 
Audio standard. ISO/IEC  gives uers of the MPEG-2 AAC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 AAC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1998.

 
 *                                                                           *
 ****************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "allHandles.h"
#include "buffer.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "allVariables.h"        /* variables */

#include "bitfct.h"
#include "bitstream.h"          /* bit stream module */
#include "buffers.h"
#include "common_m4a.h"
#include "huffScfResil.h"
#include "port.h"
#include "resilience.h"
/* #define WRITE_FACTORS */

static int DecodeHuffCodeword(Huffman *h, BIT_BUF *data);

static int BufHuff( 
                   Info*              info, 
                   byte*              group, 
                   int                nsect, 
                   byte*              sect,
                   short              global_gain, 
                   short*             factors,
                   BIT_BUF            *data,
                   int *              numFac );

static void ReadSFFromBitstream(
                                unsigned long            numOfBitsToRead,
                                BIT_BUF*                 writeBuff, 
                                HANDLE_BUFFER            hVm, 
                                HANDLE_RESILIENCE        hResilience, 
                                HANDLE_ESC_INSTANCE_DATA hEPInfo );
static void shBlockDegroup(
                           Info * info, 
                           byte * group,
                           short * grp_fac,
                           short * de_grp_fac );

#ifdef WRITE_FACTORS
static void WriteFactors(
                         short * factors,
                         int numberOfFactors );
#endif /* WRITE_FACTORS */

/******************************************************************************************************/

/*****************

  HuffScfResil

******************/
int
HuffScfResil(Info*                    info, 
             byte*                    group, 
             int                      nsect, 
             byte*                    sect,
             short                    global_gain, 
             short*                   factors, 
             HANDLE_BUFFER            hVm, 
             HANDLE_RESILIENCE        hResilience, 
             HANDLE_ESC_INSTANCE_DATA hEPInfo )
{
  /* static variables */
  static int onlyOnce = 0;
  static short lastShortFactors[MAXBANDS];
  static short lastLongFactors[MAXBANDS];
  static unsigned int blockNumber = 0;

  int i;
  int numFac;
  int returnValue;
  BIT_BUF bitBuff; 
  unsigned char buffer[512];
  unsigned long bitCount;
  unsigned long len_sf_huff_code;

  unsigned short elemenLen;
  short tempFacs[MAXBANDS];

  /* initialization of static variables */
  if ( !onlyOnce ) {
    onlyOnce = 1;
    /* initialize vectors */
    shortclr(lastShortFactors, MAXBANDS);
    shortclr(lastLongFactors, MAXBANDS);
  }

  /* initialize buffers */
  for(i=0; i<100; i++) {
    buffer[i] = 0;
  }
  shortclr(tempFacs, MAXBANDS);

  /* read LENGTH of sf data from bitstream */
  elemenLen = ( info->islong ? LEN_LONG_LENGTH_OF_SF_DATA : LEN_SHORT_LENGTH_OF_SF_DATA );
  len_sf_huff_code = GetBits(elemenLen, 
                             MAX_ELEMENTS, /* not supported, no error resilient bitstream payload syntax exists */
                             hResilience, 
                             hEPInfo, 
                             hVm );

  /* read sf data from bitstream */
  if ( len_sf_huff_code > 0 ) {
    InitWriteBitBuf( &bitBuff, buffer );
    ReadSFFromBitstream(len_sf_huff_code, &bitBuff, hVm, hResilience, hEPInfo);
    bitCount = GetBitCount( &bitBuff );
    assert(len_sf_huff_code  == bitCount);
  }
  
  InitReadBitBuf( &bitBuff, buffer );
  returnValue = BufHuff(info, group, nsect, sect, global_gain, tempFacs, &bitBuff, &numFac);
  bitCount = GetBitCount( &bitBuff );

  /* no CRC used */
/*   assert( !BsGetCrcResultForDataElementFlagEP(HCOD_SF, hEPInfo) ); */

  /* errors during transmission ? */
  if ( (bitCount != len_sf_huff_code) || BsGetCrcResultForDataElementFlagEP(HCOD_SF, hEPInfo) ) {

    /* writes out scalefactors */
    /*  fprintf(stdout, "blockNumber : %u \n", blockNumber);  */
    /*     WriteFactors( tempFacs, numFac ); */

    /* concealment ? */
    if ( GetScfConcealmentFlag( hResilience ) ) {

#ifdef WRITE_FACTORS
      fprintf(stdout, "Concealing scalefactors!\n");
#endif

      /* prediction by last available frame of same type */
      if (info->islong) {
        /* overwrite scf's with those of last Long-Frame */
        memcpy( tempFacs, lastLongFactors, (MAXBANDS * sizeof(short)) );
      } else {
        /* overwrite scf's with those of last Short-Frame */
        /*         memcpy( tempFacs, lastShortFactors, (MAXBANDS * sizeof(short)) ); */
        /* it is more sensible to mute them than to use last Short-Frame */
        shortclr(tempFacs, MAXBANDS);    
      }

    } else { /* no concealment */
      /* mute factors */
#ifdef WRITE_FACTORS
      fprintf(stdout, "Muting scalefactors!\n");
#endif

      shortclr(tempFacs, MAXBANDS);    
    }
    
    /* writes out scalefactors */
    /*     WriteFactors( tempFacs, numFac ); */

  } else { /* no errors */

    /* writes out scalefactors */
    /*  fprintf(stdout, "blockNumber : %u \n", blockNumber);  */
    /*     WriteFactors( tempFacs, numFac ); */

    if (info->islong) {
      /* save used scf's for next Long frame */
      shortclr(lastLongFactors, MAXBANDS);
      memcpy( lastLongFactors, tempFacs, (MAXBANDS * sizeof(short)) );
    } else {
      /* save used scf's for next Short frame */
      shortclr(lastShortFactors, MAXBANDS);
      memcpy( lastShortFactors, tempFacs, (MAXBANDS * sizeof(short)) );
    }
  }

  shBlockDegroup( info, group, tempFacs, factors );

  blockNumber++;

  return returnValue;
}
/********************************************************************************************************/

/* *************************************** *

 * BufHuff : get scale factors from buffer *

 * *************************************** */
static int BufHuff ( 
                    Info*              info, 
                    byte*              group, 
                    int                nsect, 
                    byte*              sect,
                    short              global_gain, 
                    short*             factors,
                    BIT_BUF            *data, 
                    int *              numFac )
     
{
  Hcb*     hcb;
  Huffman* hcw;
  int      i;
  int      b;
  int      bb;
  int      t;
  int      n;
  int      sfb;
  int      top;
  int      fac;
  int      is_pos;
  int      factor_transmitted[MAXBANDS];
  int*     fac_trans;
#ifdef PNS
  int      noise_pcm_flag = 1;
#endif
  int      noise_nrg;
  int      countFac = 0;

  /* clear array for the case of max_sfb == 0 */
  intclr(factor_transmitted, MAXBANDS);
  shortclr(factors, MAXBANDS);

  sfb = 0;
  fac_trans = factor_transmitted;
  for ( i = 0; i < nsect; i++ ) {
    top = sect[1];              /* top of section in sfb */
    t   = sect[0];              /* codebook for this section */
    sect += 2;
    for ( ; sfb < top; sfb++ ) {
      fac_trans[sfb] = t;
    }
  }

  /* scale factors are dpcm relative to global gain
   * intensity positions are dpcm relative to zero
   */
  fac = global_gain;
  is_pos = 0;
  noise_nrg = global_gain - NOISE_OFFSET;

  /* get scale factors */
  hcb = &book[BOOKSCL];
  hcw = hcb->hcw;
  bb = 0;
  if (debug['f'])
    fprintf(stderr,"scale factors\n");
  for(b = 0; b < info->nsbk; ){
    n = info->sfb_per_sbk[b];
    countFac += n;
    b = *group++;
    for(i = 0; i < n; i++){
      switch (fac_trans[i]) {
      case ZERO_HCB:        /* zero book */
        break;
      default:              /* spectral books */
        /* decode scale factor */
        t = DecodeHuffCodeword(hcw, data);
        fac += t - MIDFAC;    /* 1.5 dB */

        if (debug['f'])
          fprintf(stderr,"%3d:%3d", i, fac);
        if( ( fac >= ( 2 * TEXP ) ) || fac < 0 ) {
          CommonWarning("Scale factor extremes");
        }
        factors[i] = fac;
        break;
      case BOOKSCL:         /* invalid books */
        printf( "book %d: invalid book (huffScfResil.c, BufHuff())", BOOKSCL );
        fac_trans[i] = ZERO_HCB;
        printf ( " --> book set to %d\n", ZERO_HCB );
        break;
      case INTENSITY_HCB:           /* intensity books */
      case INTENSITY_HCB2:
        fprintf(stdout, "Warning! Intensity book !\n");
        /* decode intensity position */
        t = DecodeHuffCodeword(hcw, data);
        is_pos += t - MIDFAC;
        factors[i] = is_pos;
        break;
#ifdef PNS
      case NOISE_HCB:       /* noise books */
        fprintf(stdout, "Warning! PNS book !\n");
        /* decode noise energy */
        if (noise_pcm_flag) {
          noise_pcm_flag = 0;
          t = ReadBitBuf(data, NOISE_PCM_BITS);
        }
        else
          t = DecodeHuffCodeword(hcw, data) - MIDFAC;
        noise_nrg += t;
        if (debug['f'])
          fprintf(stderr,"\n%3d %3d (noise, code %d)", i, noise_nrg, t);
        factors[i] = noise_nrg;
        break;
#endif /*PNS*/
      }
      if (debug['f'])
        fprintf(stderr,"%3d: %3d %3d\n", i, fac_trans[i], factors[i]);
    }
    if (debug['f'])
      fprintf(stderr,"\n");

    fac_trans += n;
    factors += n;
  }

  *numFac = countFac;
  return 1;
}

/**************************************************************
  DecodeHuffCodeword

 * Cword is working buffer to which shorts from
 *   bitstream are written. Bits are read from msb to lsb
 * Nbits is number of lsb bits not yet consumed
 * 
 * this uses a minimum-memory method of Huffman decoding
 */
static int
DecodeHuffCodeword(Huffman *h, BIT_BUF *data)
{
  int i, j;
  unsigned long cw;
    
  i = h->len;
  cw = ReadBitBuf(data, i);
  while (cw != h->cw) {
    h++;
    j = h->len-i;
    i += j;
    cw <<= j;
    cw |= ReadBitBuf(data, j);
  }

  return(h->index);

}
/***************************************************

 ReadSFFromBitstream : HCOD_SF

 ***************************************************/
static void
ReadSFFromBitstream(
                    unsigned long            numOfBitsToRead,
                    BIT_BUF*                 writeBuff, 
                    HANDLE_BUFFER            hVm, 
                    HANDLE_RESILIENCE        hResilience, 
                    HANDLE_ESC_INSTANCE_DATA hEPInfo
                    )
{
  unsigned long i;
  unsigned long numOfBytes;
  unsigned long remainingBits;
  unsigned long read;

  numOfBytes = numOfBitsToRead / 8;
  remainingBits = numOfBitsToRead % 8;
  assert( numOfBitsToRead == ( remainingBits+(numOfBytes*8) ) );

  for(i=0; i<numOfBytes; i++) {
    read = GetBits(8, 
                   HCOD_SF, 
                   hResilience,
                   hEPInfo, 
                   hVm);
    WriteBitBuf( writeBuff, read, 8);
  }
  
  read = GetBits(remainingBits, 
                 HCOD_SF, 
                 hResilience, 
                 hEPInfo, 
                 hVm);
  WriteBitBuf( writeBuff, read, remainingBits);

  FlushWriteBitBuf( writeBuff );
}

/***************************************************

 shBlockDegroup

 degrouping of short blocks
 
 ***************************************************/
static void
shBlockDegroup(
               Info * info, 
               byte * group,
               short * grp_fac,
               short * de_grp_fac
               )
{
  int b, bb, i, n;
  
  /* expand short block grouping */
  if (!(info->islong)) {
    bb = 0;
    for(b = 0; b < info->nsbk; ){
      n = info->sfb_per_sbk[b];
      b = *group++;
      for (i=0; i<n; i++) {
        de_grp_fac[i] = grp_fac[i];
      }
      de_grp_fac += n;

      for(bb++; bb < b; bb++) {
        for (i=0; i<n; i++) {
          de_grp_fac[i] = grp_fac[i];
        }
        de_grp_fac += n;
      }
      grp_fac += n;
    }
  } else {
    memcpy( de_grp_fac, grp_fac, (MAXBANDS * sizeof(short)) );
  }
}

#ifdef WRITE_FACTORS
/***************************************************

 WriteFactors                                                   

 ***************************************************/
static void
WriteFactors(
             short * factors,
             int numberOfFactors
             )
{
  int i;
  fprintf(stdout, "scalefactors : ");

  for(i=0; i<numberOfFactors; i++) {
    if (fprintf( stdout, "%hd \t", factors[i] ) < 0 ) {
      fprintf (stdout, "fprintf failed : %m\n");  
    }
  }
  fprintf(stdout, "\n");
}
#endif /* WRITE_FACTORS */

