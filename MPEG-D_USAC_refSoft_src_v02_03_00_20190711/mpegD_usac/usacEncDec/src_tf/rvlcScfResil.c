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
#include <limits.h>
#include <math.h>
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
#include "buffers.h"
#include "common_m4a.h"
#include "port.h"
#include "escapeScfResil.h"
#include "rvlcScfResil.h"
#include "rvlcScfConceal.h"
#include "resilience.h"

#define ABS_ESC_VAL 7
#ifndef ABS
#define ABS(x) ( (x)>0 ? (x): -(x) ) /* should be replaced by _ABS from math.h */
#endif

/* #define WRITE_ESCAPES */
/* #define WRITE_FACTORS */
/* #define WRITE_OUT_SF */

/*****************************************************************************************/
static int DecodeVLC( Huffman *h, BIT_BUF *data );

static int EscapeDecode( unsigned long len_esc, int * pEscValues, BIT_BUF *readBuff );

#ifdef WRITE_ESCAPES
static void  WriteEscapes( int * escapes, int numberOfEscapes );
#endif /* WRITE_ESCAPES */

#ifdef WRITE_FACTORS
static void  WriteFactors( short * factors, int numberOfFactors );
#endif /* WRITE_FACTORS */

static int   ForwardDecoding( Info *info, 
                              byte *group, 
                              int nsect, 
                              byte *sect,
                              short global_gain, 
                              short *factors, 
                              BIT_BUF *data, 
                              int * escValues, 
                              int * numFac, 
                              short rev_glob_gain, 
                              int   dpcm_noise_nrg,
                              char * noError );

static int   ReverseDecoding( Info *info, 
                              byte *group, 
                              int nsect, 
                              byte *sect,
                              short global_gain, 
                              short *factors, 
                              BIT_BUF *data, 
                              int * revEscVal );

static void ReadFromBitstream( CODE_TABLE code, 
                               unsigned long numOfBitsToRead, 
                               BIT_BUF * writeBuff, 
                               HANDLE_BUFFER hVm, 
                               HANDLE_RESILIENCE hResilience, 
                               HANDLE_ESC_INSTANCE_DATA hEPInfo );

static void  InvertBitOrder( BIT_BUF * to, unsigned char * from, unsigned long SizeInBits );
static void  InvertIntArray( int * to, int * from, int size );
static void  InvertShortArray( short * to, short * from, int size );
static void  ReversGrouping( byte * revGroup, byte * group, Info *info );
static void  shBlockDegroup( Info * info, byte * group, short * grp_fac, short * de_grp_fac );

/*****************************************************************************************/


/*****************

  RVLCScfDecoding

******************/


void
RVLCScfDecodingESC1( Info *info, 
                     byte *group, 
                     int nsect, 
                     byte *sect,
                     HANDLE_BUFFER hVm, 
                     HANDLE_RESILIENCE hResilience, 
                     HANDLE_ESC_INSTANCE_DATA hEPInfo,
                     RVLC_ESC1_DATA* pRvlcESC1data)
{
  unsigned short elemenLen;
  int i, t, sfb, top;
  int countFac = 0;
  int count_loop = 0;
  int noise_used = 0;

  /* see if we have PNS */

  sfb = 0;
  for(i = 0; i < nsect; i++) {
    top = sect[1];            /* top of section in sfb */
    t = sect[0];              /* codebook for this section */
    sect += 2;
    if(t == NOISE_HCB){
      noise_used = 1;
      break;
    }
  }

  pRvlcESC1data->sf_concealment  = GetBits(LEN_SF_CONCEALMENT, 
                                           SF_CONCEALMENT, 
                                           hResilience, 
                                           hEPInfo, 
                                           hVm);  

  /* read in rev_global_gain */
  pRvlcESC1data->rev_global_gain = GetBits(LEN_REV_GLOBAL_GAIN, 
                                           REV_GLOBAL_GAIN, 
                                           hResilience, 
                                           hEPInfo, 
                                           hVm);  

  /* determine the length of the length_of_rvlc_sf element */
  elemenLen = ( info->islong ? LEN_LONG_LENGTH_OF_RVLC_SF : LEN_SHORT_LENGTH_OF_RVLC_SF );

  /* read length_of_rvlc_sf element */
  pRvlcESC1data->length_of_rvlc_sf = GetBits(elemenLen, 
                                             LENGTH_OF_RVLC_SF, 
                                             hResilience, 
                                             hEPInfo, 
                                             hVm);

  if(noise_used) {
    pRvlcESC1data->dpcm_noise_nrg=GetBits(LEN_DPCM_NOISE_NRG, 
                                          DPCM_NOISE_NRG, 
                                          hResilience, 
                                          hEPInfo, 
                                          hVm);

    pRvlcESC1data->length_of_rvlc_sf-=LEN_DPCM_NOISE_NRG;
  }

  /* read in sf_escapes_present flag */
  pRvlcESC1data->sf_escapes_present = GetBits(LEN_SF_ESCAPES_PRESENT,
                                              SF_ESCAPES_PRESENT, 
                                              hResilience, 
                                              hEPInfo, 
                                              hVm);

  if ( pRvlcESC1data->sf_escapes_present ) {
    /* read in length_of_rvlc_escapes */
    pRvlcESC1data->length_of_rvlc_escapes = GetBits(LEN_LENGTH_OF_RVLC_ESCAPES, 
                                                    LENGTH_OF_RVLC_ESCAPES, 
                                                    hResilience, 
                                                    hEPInfo, 
                                                    hVm);

  }
  if(noise_used) {
    pRvlcESC1data->dpcm_noise_last_position=GetBits(LEN_DPCM_NOISE_LAST_POSITION, 
                                                    DPCM_NOISE_LAST_POSITION, 
                                                    hResilience, 
                                                    hEPInfo, 
                                                    hVm);
  }
}

void
RVLCScfDecodingESC2( Info *info, 
                 byte *group, 
                 int nsect, 
                 byte *sect,
                 short global_gain, 
                 short *factors, 
                 HANDLE_BUFFER hVm, 
                 HANDLE_RESILIENCE hResilience, 
                 HANDLE_ESC_INSTANCE_DATA hEPInfo,
                 RVLC_ESC1_DATA* pRvlcESC1data)
{
  /* static variables */
  static short lastShortFactors[MAXBANDS];
  static short lastLongFactors[MAXBANDS];
  static unsigned int lastShortNumber = 0;
  static unsigned int lastLongNumber = 0;
  static unsigned int blockNumber = 0;

  unsigned int i;
  unsigned char buffer[100];
  unsigned char reversedBuffer[100];
  BIT_BUF reversedBitBuff; 
  unsigned long reversedBitCount;
  short reversedGlobalGain;

  unsigned char escapeBuffer[100];
  BIT_BUF escapeBitBuff; 
  int escapeValues[MAXBANDS];
  int reverEscVal[MAXBANDS];
  unsigned long escBitCount;
  int forwPos;
  int backwPos;
  short forwFacs[MAXBANDS];
  short backwFacs[MAXBANDS];
  short recoveredFacs[MAXBANDS];
  short lastFrame[MAXBANDS];

  short tempFacs[MAXBANDS];
  int numFac;
  BIT_BUF txEpBitBuff; 
  int encEpVal[MAXBANDS];
  unsigned char txEpBuffer[100];
  unsigned long txEpbitCount;

  unsigned long len_rvlc_code;
  unsigned long num_escapes;
  unsigned long length_escapes;
  char noError;
  char escError;
  unsigned long escPresent;

  char enhc_con_bit;

  /****!!!!! Now comes the dirty, dirty hack !!!!!****/
  static int onlyOnce = 0;
 
  if ( !onlyOnce ) {
    onlyOnce = 1;
    /* initialize vectors */
    shortclr(lastShortFactors, MAXBANDS);
    shortclr(lastLongFactors, MAXBANDS);
  }
  /*****!!!!       It's over by now       !!!!*******/

  /* initialize buffers */
  intclr(escapeValues, MAXBANDS);
  intclr(reverEscVal, MAXBANDS);
  intclr(encEpVal, MAXBANDS);
  shortclr(factors, MAXBANDS);
  shortclr(recoveredFacs, MAXBANDS);

  for(i=0; i<100; i++) {
    buffer[i] = 0;
    reversedBuffer[i] = 0;
    escapeBuffer[i] = 0;
    txEpBuffer[i] = 0;
  }

  /*******************************************

         RVLC Codebook + ESCAPE Values

  *******************************************/

  /* read sf_concealment bit, if enabled */
  enhc_con_bit = pRvlcESC1data->sf_concealment;  

  /* read in rev_global_gain */
  reversedGlobalGain =  pRvlcESC1data->rev_global_gain;
  
  /* superflous checking */
  assert( reversedGlobalGain >= 0 );
  assert( reversedGlobalGain < (short) pow(2, LEN_REV_GLOBAL_GAIN) );

  /* read length_of_rvlc_sf element */
  len_rvlc_code = pRvlcESC1data->length_of_rvlc_sf;

  if (len_rvlc_code>0) {
    InitWriteBitBuf( &escapeBitBuff, escapeBuffer );
    ReadFromBitstream(RVLC_CODE_SF, len_rvlc_code, &escapeBitBuff, hVm, hResilience, hEPInfo);
    escBitCount = GetBitCount( &escapeBitBuff );
    assert( len_rvlc_code == escBitCount );
  }

  /* read in sf_escapes_present flag */
  escPresent = pRvlcESC1data->sf_escapes_present;

  /* no error supposed */
  noError = 1;
  escError = 0;

  num_escapes = 0;
  if ( escPresent ) {
    /* read in length_of_rvlc_escapes */
    length_escapes = pRvlcESC1data->length_of_rvlc_escapes;

    if (length_escapes <= 0) {
#ifdef WRITE_OUT_SF
      fprintf(stdout, "length_escapes has impossible value!\n");
      fprintf(stdout, "length_escapes : %lu \n", length_escapes);
#endif
      length_escapes = 2; /* most probable value ! */
    }

    /* read in escapes into a buffer */
    InitWriteBitBuf( &txEpBitBuff, txEpBuffer );
    ReadFromBitstream(RVLC_ESC_SF, length_escapes, &txEpBitBuff, hVm, hResilience, hEPInfo);
    txEpbitCount = GetBitCount( &txEpBitBuff );
    assert( length_escapes == txEpbitCount);
    
    /* decode escapes */
    InitReadBitBuf( &txEpBitBuff, txEpBuffer );    
    num_escapes = EscapeDecode(length_escapes, escapeValues, &txEpBitBuff );
    txEpbitCount = GetBitCount( &txEpBitBuff );
    
    if (txEpbitCount != length_escapes) {
#ifdef WRITE_OUT_SF
      fprintf(stdout, "\nerror in escapes detected\n");
#endif
      /*   CommonWarning("\n DELETING ESCAPES !!"); */
      /*       intclr(escapeValues, MAXBANDS); */

      /* concealment : reduce the absolute value of escapes */
      /*       DivideIntVector( escapeValues, num_escapes, 2 ); */

      noError = 0;
      escError = 1;
      /* if no errors */
      /*       assert(0); */
    }
  }

  /* Forward Direction : decode corrupted rvlc+escape bits */
  InitReadBitBuf( &escapeBitBuff, escapeBuffer);
  shortclr(forwFacs, MAXBANDS);
  forwPos = ForwardDecoding(info, group, nsect, sect, global_gain, forwFacs, &escapeBitBuff, escapeValues, 
                            &numFac, reversedGlobalGain, pRvlcESC1data->dpcm_noise_nrg, &noError);


  /* true if there were no errors : written and read bits have to be equal */
  /*   assert( len_rvlc_code == GetBitCount( &escapeBitBuff ) ); */
  /*   assert(noError); */

  if ( !noError || (len_rvlc_code != GetBitCount(&escapeBitBuff)) ) {

    CommonWarning("Error parsing RVLC data");

#ifdef WRITE_OUT_SF
    fprintf(stdout, "blockNumber : %u \n", blockNumber); 
    WriteFactors( forwFacs, numFac );
#endif    
    /* fprintf(stdout, "forward Pos : %d \n", forwPos); */

    /* Reverse bits */
    /* One extra 0 inserted at the begining of the reversedBuffer
       (see:  escBitCount+1). This is done because the reversed_global_gain
       is the last factor. This works because buffer is initialized to 0 ! */
    InitWriteBitBuf( &reversedBitBuff, reversedBuffer );
    InvertBitOrder( &reversedBitBuff, escapeBuffer, len_rvlc_code+1 );
    reversedBitCount = GetBitCount( &reversedBitBuff );
    assert( reversedBitCount == (len_rvlc_code+1) );

    /* Reverse escape values */
    InvertIntArray( reverEscVal, escapeValues, num_escapes);

    /* Backward Direction : decoding rvlc+escape */
    InitReadBitBuf( &reversedBitBuff, reversedBuffer );
    backwPos = ReverseDecoding(info, group, nsect, sect, reversedGlobalGain, backwFacs, &reversedBitBuff, reverEscVal);
    reversedBitCount = GetBitCount( &reversedBitBuff );

#ifdef WRITE_OUT_SF
    /* fprintf(stdout, "backward Pos: %d \n", backwPos); */
    WriteFactors( backwFacs, numFac );
#endif

    /* prediction by last available frame of same type */
    if (info->islong) {
      /* overwrite scf's with those of last Long-Frame */
      memcpy( lastFrame, lastLongFactors, (MAXBANDS * sizeof(short)) );
#ifdef WRITE_OUT_SF
      fprintf(stdout, "lastLongNumber : %u \n", lastLongNumber); 
#endif
    } else {
      /* overwrite scf's with those of last Short-Frame */
      memcpy( lastFrame, lastShortFactors, (MAXBANDS * sizeof(short)) );
#ifdef WRITE_OUT_SF
      fprintf(stdout, "lastShortNumber : %u \n", lastShortNumber); 
#endif
    }

    /* reversedBitCount == escBitCount if global_gain == first factor*/
    shortclr(recoveredFacs, MAXBANDS);
    shortclr(tempFacs, MAXBANDS);

    /* if escapes are incorrect ensure concealment of whole array ??*/
    if (escError) {
      ;
    }

    ScfRecovery( hResilience, numFac, forwFacs, forwPos, backwFacs, backwPos, global_gain, tempFacs, lastFrame, enhc_con_bit, info );
    
    memcpy( recoveredFacs, tempFacs, (MAXBANDS * sizeof(short)) );

#ifdef WRITE_OUT_SF
    WriteFactors( lastFrame, numFac );
    WriteFactors( tempFacs, numFac );
#endif

  } else { /* noError */

    /*     fprintf(stdout, "blockNumber : %u \n", blockNumber);  */
    /*     WriteFactors( forwFacs, numFac ); */

    /* is it possible that an error in escapes was not detected ? */
    assert( !escError);
    
    memcpy( recoveredFacs, forwFacs, (MAXBANDS * sizeof(short)) );
  }
  
  /* if short blocks -> degrouping */
  shBlockDegroup( info, group, recoveredFacs, factors );

  if ( GetScfBitFlag(hResilience) || !( !noError || (len_rvlc_code != GetBitCount(&escapeBitBuff)) ) ) {  
    if (info->islong) {
      /* save used scf's for next Long frame */
      shortclr(lastLongFactors, MAXBANDS);
      memcpy( lastLongFactors, recoveredFacs, (MAXBANDS * sizeof(short)) );
      lastLongNumber = blockNumber;
    } else {
      /* save used scf's for next Short frame */
      shortclr(lastShortFactors, MAXBANDS);
      memcpy( lastShortFactors, recoveredFacs, (MAXBANDS * sizeof(short)) );
      lastShortNumber = blockNumber;
    }
  }
  blockNumber++;
}

/*********************************************************************************************************/

/*******************************************************

  DecodeVLC

  *****************************************************/
static int
DecodeVLC(Huffman *h, BIT_BUF *data)
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


/***********************************************************************

  ReverseDecoding

 ***********************************************************************/
static int
ReverseDecoding(Info *info, byte *group, int nsect, byte *sect,
                short global_gain, short *factors, BIT_BUF *data, int * revEscVal)
{
  Hcb *hcb;
  Huffman *hcw;
  int i, b, bb, t, n, sfb, top, fac, is_pos;
  int factor_transmitted[MAXBANDS], *fac_trans;
#ifdef PNS
  int noise_pcm_flag = 1;
  int noise_nrg;
#endif

  int revFactorTrx[MAXBANDS];
  short revFactors[MAXBANDS];
  short * pRevFactor;
  byte reversedGroup[8];
  byte * pRevGroup;
  int countFactors;
  int error_detect = INT_MAX;
  int count_loop = 0;

  /* clear array for the case of max_sfb == 0 */
  intclr(factor_transmitted, MAXBANDS);
  shortclr(factors, MAXBANDS);
  intclr(revFactorTrx, MAXBANDS);
  shortclr(revFactors, MAXBANDS);
  byteclr(reversedGroup, 8);
  countFactors = 0;

  sfb = 0;
  fac_trans = factor_transmitted;
  for(i = 0; i < nsect; i++){
    top = sect[1];		/* top of section in sfb */
    t = sect[0];		/* codebook for this section */
    sect += 2;
    for(; sfb < top; sfb++) {
      fac_trans[sfb] = t;
    }
  }

  /* reverse factor_transmitted */
  InvertIntArray( revFactorTrx, factor_transmitted, sfb);
  fac_trans = revFactorTrx;

  /* set pointer on reversed factors */
  pRevFactor = revFactors;

  /* reverse grouping data */
  ReversGrouping(reversedGroup, group, info);
  pRevGroup = reversedGroup;

  /* scale factors are dpcm relative to global gain
   * intensity positions are dpcm relative to zero
   */
  fac = global_gain;
  is_pos = 0;
#ifdef PNS
  noise_nrg = global_gain - NOISE_OFFSET;
#endif

  /* get scale factors */
  hcb = &book[BOOKSCL];
  hcw = hcb->hcw;
  bb = 0;
  if (debug['f'])
    fprintf(stderr,"scale factors\n");
  for(b = 0; b < info->nsbk; ){
    /* more correctly :  if (!info->islong) n = info->sfb_per_sbk[info->nsbk-1-b];  */
    n = info->sfb_per_sbk[b];
    countFactors += n;        /* (n * (*pRevGroup -b)); */
    count_loop++;
    b = *pRevGroup++;
    for(i = 0; i < n; i++){
      switch (fac_trans[i]) {
      case ZERO_HCB:	    /* zero book */
        break;
      default:		    /* spectral books */
        /* decode scale factor */
        t = DecodeVLC(bookRvlc, data);

        if (t != INT_MAX) {

          /* everything O.K. */
          fac -= t ;    /* 1.5 dB */
          if(fac >= 2*TEXP || fac < 0) { 
            CommonWarning("Scale factor extremes");
            /* this indicates an error in rev_global_gain */
            goto j_err;
          }
          assert( ABS(t) <= ABS_ESC_VAL );

          if ( t == +ABS_ESC_VAL ) {
            fac -=  *revEscVal++;
          }
          if ( t == -ABS_ESC_VAL ) {
            fac +=  *revEscVal++;
          }
          
          pRevFactor[i] = fac;

        } else {
        j_err:          /* there was an error -> make all remaining SCF = 0 */
          error_detect = (count_loop-1)*n + i;     /* ATTENTION : n is supposed to be fixed. If not -> doesn't work */
          pRevFactor[i] = 0;
          intclr(revFactorTrx, MAXBANDS);
        }
        break;
      case BOOKSCL:	    /* invalid books */
#ifndef PNS
      case RESERVED_HCB:
#endif
        fprintf(stderr,"Invalid Books\n"); 
        /*  assert(0); */
        /*  return 0; */
        break;
      case INTENSITY_HCB:	    /* intensity books */
      case INTENSITY_HCB2:
        fprintf(stderr,"Invalid Books : INTENSITY_HCB\n"); 
        /* assert(0); */
        /* decode intensity position */
        /* t = ReversibleDecodeHuff(hcw, data); */
        /*         is_pos += t - MIDFAC; */
        /*         pRevFactor[i] = is_pos; */
        break;
#ifdef PNS
      case NOISE_HCB:	    /* noise books */
        noise_pcm_flag = noise_pcm_flag;  /* dummy to avoid compiler warning */
        fprintf(stderr,"Invalid Books :NOISE_HCB .\n"); 
        /*   assert(0); */
        /* decode noise energy */
        /*   if (noise_pcm_flag) { */
        /*           noise_pcm_flag = 0; */
        /*           t = GetBits( DPCM_NOISE_NRG, NOISE_PCM_BITS, hResilience, hVm, hEPInfo ) - NOISE_PCM_OFFSET; */
        /*          assert(0); */
        /*         } */
        /*         else */
        /*           t = ReversibleDecodeHuff(hcw, data) - MIDFAC; */
        /*         noise_nrg += t; */
        /*         if (debug['f']) */
        /*           fprintf(stderr,"\n%3d %3d (noise, code %d)", i, noise_nrg, t); */
        /*         pRevFactor[i] = noise_nrg; */
        break;
#endif

      }
      if (debug['f'])
        fprintf(stderr,"%3d: %3d %3d\n", i, fac_trans[i], pRevFactor[i]);
    }
    if (debug['f'])
      fprintf(stderr,"\n");
    fac_trans += n;
    pRevFactor += n;
  }

  /* Invert factor ordering */
  InvertShortArray( factors, revFactors, countFactors); 
  if ( error_detect!=INT_MAX ) {
    error_detect = countFactors-1-error_detect;
  }
  return error_detect;
}

/**************************************

 InvertBitOrder

 Writes `SizeInBits' bits in backward order from 
 array `from' to bit-buffer `to'.
 
**************************************/
static void
InvertBitOrder(
               BIT_BUF * to,
               unsigned char * from,
               unsigned long SizeInBits
               )
{
  unsigned char charMask;
  unsigned char initial;
  unsigned long reversed;
  unsigned long RemainingBits;
  unsigned long numOfBits;
  unsigned long bitPosition;

  int numOfElement;
  int i;

  numOfBits = sizeof (unsigned char) * CHAR_BIT;
  
  /* number of full occupied chars */
  numOfElement = SizeInBits / numOfBits;

  /* first : write (eventually) partial occupied part of last char */
  RemainingBits = SizeInBits % numOfBits;

  if ( RemainingBits > 0 ) {
    initial = from[numOfElement];

    reversed = 0;
    charMask = 0x1;
    charMask <<= ( numOfBits -RemainingBits );
    for(bitPosition=0; bitPosition<RemainingBits; bitPosition++) {
      reversed <<= 1;
      if (charMask & initial) {
        reversed |= 1;
      }
      charMask <<= 1;
    }  

    WriteBitBuf( to, reversed, RemainingBits );
  }

  /* now : write full occupied chars */
  for(i=0; i<numOfElement; i++) {
    initial = from[(numOfElement-1) - i];

    charMask = 0x1;
    reversed = 0;
    for(bitPosition=0; bitPosition<numOfBits; bitPosition++) {
      reversed <<= 1;
      if ( charMask & initial) {
        reversed |= 1;
      }
      charMask <<= 1;
    }

    WriteBitBuf( to, reversed, numOfBits );
  }
 
  FlushWriteBitBuf( to );

}

/******************************************

 InvertIntArray

 Copies array `from' of length `size' in backward
 direction in array `to'

 ******************************************/
static void
InvertIntArray(
               int * to,
               int * from,
               int size
               )
{
  int i;

  for(i=0; i<size; i++ ) {
    to[i] = from[size-1-i];
  }
}

/******************************************

 InvertShortArray

 Copies array `from' of length `size' in backward
 direction in array `to'

 ******************************************/
static void
InvertShortArray(
                 short * to,
                 short * from,
                 int size
                 )
{
  int i;

  for(i=0; i<size; i++ ) {
    to[i] = from[size-1-i];
  }
}

/************************************************

  ReversGrouping

 ************************************************/
static void
ReversGrouping(
               byte * revGroup, 
               byte * group,
               Info *info
               )
{
  int memory[8];
  int index;
  int i;

  if (info->islong) {
    
    *revGroup = *group;
  } else {

    for(index = 0; ; index++) {
      memory[index] = *group;
      if ( memory[index] == 8 ) {
        break;
      }
      group++;
    }

    revGroup[0] = memory[index] -memory[index-1];
    for(i=1; i < index; i++) {
      revGroup[i] = revGroup[i-1] +memory[index-i] -memory[index-i-1];
    }
    revGroup[index] = 8;
  }
}

/***********************************************************************

  ForwardDecoding

 ***********************************************************************/
static int
ForwardDecoding(Info *info, byte *group, int nsect, byte *sect,
                short global_gain, short *factors, BIT_BUF *data, int * escValues, int * numFac, short rev_glob_gain, int  dpcm_noise_nrg, char * noError)
{
  Hcb *hcb;
  Huffman *hcw;
  int i, b, bb, t, n, sfb, top, fac, is_pos;
  int factor_transmitted[MAXBANDS], *fac_trans;
#ifdef PNS
  int noise_pcm_flag = 0;
  int noise_nrg;
#endif
  int is_flag = 0;
  int is_position = 0;
  int error_detect = INT_MAX;
  int count_loop = 0;
  int countFac = 0;

  /* clear array for the case of max_sfb == 0 */
  intclr(factor_transmitted, MAXBANDS);
  shortclr(factors, MAXBANDS);

  sfb = 0;
  fac_trans = factor_transmitted;
  for(i = 0; i < nsect; i++){
    top = sect[1];		/* top of section in sfb */
    t = sect[0];		/* codebook for this section */
    sect += 2;
    for(; sfb < top; sfb++) {
      fac_trans[sfb] = t;
    }
  }

  /* scale factors are dpcm relative to global gain
   * intensity positions are dpcm relative to zero
   */
  fac = global_gain;
  is_pos = 0;
#ifdef PNS
  noise_nrg = global_gain - NOISE_OFFSET - 256;
#endif

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
    count_loop++;
    for(i = 0; i < n; i++){
      switch (fac_trans[i]) {
      case ZERO_HCB:	    /* zero book */
        break;
      default:		    /* spectral books */
        /* decode scale factor */
        t = DecodeVLC(bookRvlc, data);

        if (t != INT_MAX) {

          /* everything O.K. */
          fac += t ;    /* 1.5 dB */
          if(fac >= 2*TEXP || fac < 0) {
            CommonWarning("Scale factor extremes");
            /*   assert(0); */
          }
          assert( ABS(t) <= ABS_ESC_VAL );
          if ( t == +ABS_ESC_VAL ) {
            fac +=  *escValues++;
          }
          if ( t == -ABS_ESC_VAL ) {
            fac -=  *escValues++;
          }
          factors[i] = fac;

        } else {
          /* there was an error -> make all remaining SCF = 0 */
          error_detect = (count_loop-1)*n+i;
          factors[i] = 0;
          intclr(factor_transmitted, MAXBANDS);
        }
        break;
      case BOOKSCL:	    /* invalid books */
#ifndef PNS
      case RESERVED_HCB:
#endif
        fprintf(stderr,"Invalid Books\n"); 
        /*  return 0; */
        break;
      case INTENSITY_HCB:	    /* intensity books */
      case INTENSITY_HCB2:

        if( !is_flag ) is_flag = 1;

        t = DecodeVLC(bookRvlc, data);
        is_position += t;

        assert( ABS(t) <= ABS_ESC_VAL );
        if ( t == +ABS_ESC_VAL ) {
          is_position +=  *escValues++;
        }
        if ( t == -ABS_ESC_VAL ) {
          is_position-=  *escValues++;
        }      

        factors[i] = is_position;

        break;
#ifdef PNS
      case NOISE_HCB:	    /* noise books */
        
        if (!noise_pcm_flag ) {
          noise_pcm_flag = 1;        
          noise_nrg += dpcm_noise_nrg;
        }
        else {
          t = DecodeVLC(bookRvlc, data);
          noise_nrg += t;

          assert( ABS(t) <= ABS_ESC_VAL );
          if ( t == +ABS_ESC_VAL ) {
            noise_nrg +=  *escValues++;
          }
          if ( t == -ABS_ESC_VAL ) {
            noise_nrg -=  *escValues++;
          }
        }

        factors[i] = noise_nrg;

        break;
#endif

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
  if (fac != rev_glob_gain){
    *noError = 0;
  }  

  if( is_flag ) {
    t = DecodeVLC(bookRvlc, data);    
  }

  return error_detect;
}



#ifdef WRITE_ESCAPES
/***************************************************

              WriteEscapes

***************************************************/
static void
WriteEscapes(
             int * escapes,
             int numberOfEscapes
             )
{
  int i;

  for(i=0; i<numberOfEscapes; i++) {
    if (fprintf( stdout, "%hd \t", escapes[i] ) < 0 ) {
      fprintf (stdout, "fprintf failed : %m\n");  
    }
  }
  if (numberOfEscapes == 0) {
    fprintf (stdout, "no escapes needed");  
  }
  fprintf(stdout, "\n");
}
#endif /* WRITE_ESCAPES */

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

/***************************************************

 ReadFromBitstream : RVLC_CODE

 ***************************************************/
static void
ReadFromBitstream(
                  CODE_TABLE        code,
                  unsigned long numOfBitsToRead,
                  BIT_BUF * writeBuff, 
                  HANDLE_BUFFER hVm, 
                  HANDLE_RESILIENCE hResilience, 
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
    read = GetBits( 8, 
                    code, 
                    hResilience, 
                    hEPInfo, 
                    hVm);
    WriteBitBuf( writeBuff, read, 8);
  }
  
  read = GetBits(remainingBits, 
                 code, 
                 hResilience, 
                 hEPInfo, 
                 hVm);
  WriteBitBuf( writeBuff, read, remainingBits);

  FlushWriteBitBuf( writeBuff );
}


/***************************************************

 EscapeDecode

 ***************************************************/
static int
EscapeDecode(
              unsigned long  len_esc,
              int * pEscValues, 
              BIT_BUF *readBuff )
{
  int value;
  unsigned long readCount;
  int num_escapes;

  /*   assert(len_esc>0); */
  num_escapes = 0;

  readCount = 0;
  while (len_esc > readCount) {
    value = DecodeVLC(bookEscape, readBuff);
    assert( value >= 0 );
    assert( value <= 53 );
    pEscValues[num_escapes] = value;
    readCount = GetBitCount( readBuff );
    num_escapes++;
  }
  return num_escapes;
}

