/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by
  Y.B. Thomas Kim and S.H. Park (Samsung AIT)
and edited by
  Y.B. Thomas Kim (Samsung AIT) on 1997-08-22

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1997.


	SAMSUNG_2005-09-30 : modified buffer structure to support multi-channel output

**********************************************************************/
#include <stdio.h>
/*  RF 20000106    #include <sys/fcntl.h>   */
#include <sys/types.h>

#include "allHandles.h"

#include "tns3.h"                /* structs */

#include "bitstream.h"
#include "sam_encode.h"

#define  MAX_LENGTH  32
#define  MINIMUM    4

/* SAMSUNG_2005-09-30 */
typedef struct {
unsigned char bs_buf[2048];      /* bit stream buffer */
int      bs_buf_size;            /* size of buffer */
int      bs_buf_byte_idx;        /* pointer to top byte in buffer */
int      bs_buf_bit_idx;         /* pointer to top bit of top byte in buffer */
int      bs_eob;                 /* end of buffer index */
int      bs_eobs;                /* end of bit stream flag */
int      bs_buf_byte_idx_r;        /* pointer to top byte in buffer */
int      bs_buf_bit_idx_r;         /* pointer to top bit of top byte in buffer */
long			bs_totbit;              /* bit counter of bit stream */
}BS_ENC;

BS_ENC bs[MAX_CHANNEL_ELEMENT];
static int g_Elm;
/* ~SAMSUNG_2005-09-30 */

void sam_init_bs(void)
{
  int  i;

  /* 20060705 */
  bs[g_Elm].bs_buf_size = 2048;
  bs[g_Elm].bs_buf_byte_idx = 0;
  bs[g_Elm].bs_buf_bit_idx=8;
  bs[g_Elm].bs_totbit=0;
  bs[g_Elm].bs_eob = 0;
  bs[g_Elm].bs_eobs = 0;
  for(i = 0; i < 2048; i++)
    bs[g_Elm].bs_buf[i] = 0;
}

int putmask[9]={0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff};

/*write N bits into the bit stream */
int sam_putbits2bs(int val, int N)
{
 register int j = N;
 register int k, tmp;

 if (N < 1)
  return 0;
 if (N > MAX_LENGTH) {
    printf("Cannot read or write more than %d bits[%d] at a time.\n", MAX_LENGTH, val);
    return 0;
 }

/*
 bs_totbit += N;
 while (j > 0) {
   k = (j < bs_buf_bit_idx) ? j : bs_buf_bit_idx;
   tmp = val >> (j-k);
   bs_buf[bs_buf_byte_idx] |= (tmp&putmask[k]) << (bs_buf_bit_idx-k);
   bs_buf_bit_idx -= k;
   if (!bs_buf_bit_idx) {
       bs_buf_bit_idx = 8;
       bs_buf_byte_idx++;
       if (bs_buf_byte_idx > bs_buf_size-1) {
    fprintf(stderr, "SAM: bitstream buffer overflow!\n");
     }
   }
   j -= k;
 }
*/
/* SAMSUNG_2005-09-30 */
 bs[g_Elm].bs_totbit += N;
 while (j > 0) {
   k = (j < bs[g_Elm].bs_buf_bit_idx) ? j : bs[g_Elm].bs_buf_bit_idx;
   tmp = val >> (j-k);
   bs[g_Elm].bs_buf[bs[g_Elm].bs_buf_byte_idx] |= (tmp&putmask[k]) << (bs[g_Elm].bs_buf_bit_idx-k);
   bs[g_Elm].bs_buf_bit_idx -= k;
   if (!bs[g_Elm].bs_buf_bit_idx) {
       bs[g_Elm].bs_buf_bit_idx = 8;
       bs[g_Elm].bs_buf_byte_idx++;
       if (bs[g_Elm].bs_buf_byte_idx > bs[g_Elm].bs_buf_size-1) {
    fprintf(stderr, "SAM: bitstream buffer overflow!\n");
     }
   }
   j -= k;
 }

 return N;
}

/* SAMSUNG_2005-09-30 : write N bits into the bit stream , overwrite and not advance the bits */
int sam_putbits2bs_ow(int val, int N)
{
 register int j = N;
 register int k, tmp;

 if (N < 1)
  return 0;
 if (N > MAX_LENGTH) {
    printf("Cannot read or write more than %d bits[%d] at a time.\n", MAX_LENGTH, val);
    return 0;
 }
 
 while (j > 0) {
   k = (j < bs[g_Elm].bs_buf_bit_idx) ? j : bs[g_Elm].bs_buf_bit_idx;
   tmp = val >> (j-k);
   /* 20060107 */
   bs[g_Elm].bs_buf[bs[g_Elm].bs_buf_byte_idx] &= (putmask[8-k]);
   bs[g_Elm].bs_buf[bs[g_Elm].bs_buf_byte_idx] |= (tmp&putmask[k]) << (bs[g_Elm].bs_buf_bit_idx-k);
   bs[g_Elm].bs_buf_bit_idx -= k;
   if (!bs[g_Elm].bs_buf_bit_idx) {
       bs[g_Elm].bs_buf_bit_idx = 8;
       bs[g_Elm].bs_buf_byte_idx++;
       if (bs[g_Elm].bs_buf_byte_idx > bs[g_Elm].bs_buf_size-1) {
    fprintf(stderr, "SAM: bitstream buffer overflow!\n");
     }
   }
   j -= k;
 }
 return N;
}

int sam_getWBitBufPos()
{
  /*return ((bs_buf_byte_idx * 8) + (8 - bs_buf_bit_idx));*/	
  return ((bs[g_Elm].bs_buf_byte_idx * 8) + (8 - bs[g_Elm].bs_buf_bit_idx));
}

void sam_setWBitBufPos(int pos)
{
  /*bs_buf_byte_idx = pos/8;
  bs_buf_bit_idx = 8-(pos%8);*/
	bs[g_Elm].bs_buf_byte_idx = pos/8;
  bs[g_Elm].bs_buf_bit_idx = 8-(pos%8);

}

int sam_sstell()
{
  /*return bs_totbit;*/
  return bs[g_Elm].bs_totbit;
}

void sam_frame_length_rewrite2bs(int framesize, int pos, int len)
{
/*
  int      bs_buf_byte_idx_save = bs_buf_byte_idx; 
  int      bs_buf_bit_idx_save =  bs_buf_bit_idx; 

  framesize /= 8;
  bs_buf_byte_idx = pos/8;
  bs_buf_bit_idx = 8-(pos%8);
  sam_putbits2bs(framesize, len);

  bs_buf_byte_idx = bs_buf_byte_idx_save;
  bs_buf_bit_idx =  bs_buf_bit_idx_save;*/
  int      bs_buf_byte_idx_save = bs[g_Elm].bs_buf_byte_idx; 
  int      bs_buf_bit_idx_save =  bs[g_Elm].bs_buf_bit_idx; 

  framesize /= 8;
  bs[g_Elm].bs_buf_byte_idx = pos/8;
  bs[g_Elm].bs_buf_bit_idx = 8-(pos%8);
  sam_putbits2bs_ow(framesize, len);

  bs[g_Elm].bs_buf_byte_idx = bs_buf_byte_idx_save;
  bs[g_Elm].bs_buf_bit_idx =  bs_buf_bit_idx_save;
	if(pos==0) 
		g_Elm++;

}

void sam_bsflush(HANDLE_BSBITSTREAM fixed_stream, int len)
{
  int  i;

  len /= 8;

  for(i = 0; i < len; i++)
    /*BsPutBit(fixed_stream, bs_buf[i], 8);*/
    BsPutBit(fixed_stream, bs[0].bs_buf[i], 8);
	g_Elm = 0; 
}


/* 20070530 SBR */
void sam_bsflushAll( HANDLE_BSBITSTREAM fixed_stream, int frameSize )
{
  int  i, j, len, acum;
  int rbit;

	acum = 0;
 	for(i=g_Elm-1; i>=0; i--)	
	{
		len = ( (bs[i].bs_totbit) / 8 ) * 8;

		len /= 8;

		rbit = bs[i].bs_totbit - len*8;
		for(j = 0; j < len; j++)
		{
			BsPutBit(fixed_stream, bs[i].bs_buf[j], 8);			
		}	
		if(rbit)
			BsPutBit(fixed_stream, (bs[i].bs_buf[len] >>(8-rbit)), rbit);

		acum += bs[i].bs_totbit;
	}

	if( acum % 8 ) {
		BsPutBit(fixed_stream, 0, 8-(acum%8) );
	}

	g_Elm = 0;
}

/* 20070530 SBR */
extern const int v_Huff_envelopeLevelC11F[63];
extern const int v_Huff_envelopeLevelL11F[63];

static void writeSbrGridinBSAC()
{
  sam_putbits2bs(/*FIXFIX*/0, /*SBR_CLA_BITS*/2);
  sam_putbits2bs(1, 2); /* 2 envelopes per frame */
  sam_putbits2bs(1, /*SBR_RES_BITS*/1);
}

static void writeSbrDtdfinBSAC()
{
  int env, noise;

  for (env=0; env<2; env++)
    sam_putbits2bs(0, 1); /* delta f */

  for (noise=0; noise<2; noise++)
    sam_putbits2bs(0, 1); /* delta f */
}

static void writeSbrEnvelopeinBSAC(SBR_CHAN *sbrChan)
{
  int env, band;
  int delta;

  for (env=0; env<2; env++) {
    sam_putbits2bs(sbrChan->envData[env][0], 
             /*SI_SBR_START_ENV_BITS_AMP_RES_3_0*/6);
    for (band=1; band<sbrChan->numBands; band++) {
      delta = sbrChan->envData[env][band] - sbrChan->envData[env][band-1] + 31;
      sam_putbits2bs(v_Huff_envelopeLevelC11F[delta],
               v_Huff_envelopeLevelL11F[delta]);
    }
  }
}

static void writeSbrNoiseinBSAC(SBR_CHAN *sbrChan)
{
  int noise, band;
  int delta;

  for (noise=0; noise<2; noise++) {
    sam_putbits2bs(sbrChan->noiseData[noise][0], 
             /*SI_SBR_START_NOISE_BITS_AMP_RES_3_0*/5);
    for (band=1; band<sbrChan->numNoiseBands; band++) {
      delta = sbrChan->noiseData[noise][band] - sbrChan->noiseData[noise][band]
              + 31;
      sam_putbits2bs(v_Huff_envelopeLevelC11F[delta],
               v_Huff_envelopeLevelL11F[delta]);
    }
  }
}

static void writeSbrSCEdatainBSAC(SBR_CHAN *sbrChan)
{
  int band;

  sam_putbits2bs(0, 1); /* no reserved bits */

  writeSbrGridinBSAC();
  writeSbrDtdfinBSAC();

  for (band=0; band<sbrChan->numNoiseBands; band++)
    sam_putbits2bs(0, /*SI_SBR_INVF_MODE_BITS*/2); 

  writeSbrEnvelopeinBSAC(sbrChan);

  writeSbrNoiseinBSAC(sbrChan);

  sam_putbits2bs(0, /*SI_SBR_ADD_HARMONIC_ENABLE_BITS*/1);

  sam_putbits2bs(0, /*SI_SBR_EXTENDED_DATA_BITS*/1); /* no extended data */
}

static void writeSbrCPEDatainBSAC(SBR_CHAN *sbrChanL, 
                            SBR_CHAN *sbrChanR)
{
  int band;

  sam_putbits2bs(0, 1); /* no reserved bits */

  sam_putbits2bs(0, /*SI_SBR_COUPLING_BITS*/1);

  writeSbrGridinBSAC();
  writeSbrGridinBSAC();
  writeSbrDtdfinBSAC();
  writeSbrDtdfinBSAC();

  for (band=0; band<sbrChanL->numNoiseBands; band++)
    sam_putbits2bs(0, /*SI_SBR_INVF_MODE_BITS*/2); 
  for (band=0; band<sbrChanR->numNoiseBands; band++)
    sam_putbits2bs(0, /*SI_SBR_INVF_MODE_BITS*/2);

  writeSbrEnvelopeinBSAC(sbrChanL);
  writeSbrEnvelopeinBSAC(sbrChanR);

  writeSbrNoiseinBSAC(sbrChanL);
  writeSbrNoiseinBSAC(sbrChanR);

  sam_putbits2bs(0, /*SI_SBR_ADD_HARMONIC_ENABLE_BITS*/1);
  sam_putbits2bs(0, /*SI_SBR_ADD_HARMONIC_ENABLE_BITS*/1);

  sam_putbits2bs(0, /*SI_SBR_EXTENDED_DATA_BITS*/1); /* no extended data */
}

#ifdef BSACCONF
extern int sbrheaderInBSAC;
#endif
#ifdef READ_SBR
extern int sbr_bits_bsac;
extern int readEnable_bsac;
extern FILE* sbrPayloadFile_bsac;
#endif

int WriteSbrSCEinBSAC(SBR_CHAN *sbrChan)
{
#ifndef READ_SBR
  sam_putbits2bs(sbrChan->count, /*LEN_F_CNT*/4); /* count */
  if (sbrChan->esc_count) {
    sam_putbits2bs(sbrChan->esc_count, /*LEN_BYTE*/8); /* esc_count */
  }

  /* extension type */

  /*  no CRC bits */
#ifndef BSACCONF
  if (sbrChan->sendHdrCnt==0)
    sam_putbits2bs(1, /*SI_SBR_HDR_BIT*/1); /* header will follow */
  else
    sam_putbits2bs(0, /*SI_SBR_HDR_BIT*/1); /* no header */

  /* header */
  if (sbrChan->sendHdrCnt==0) {
	sam_putbits2bs(1, /*SI_SBR_AMP_RES_BITS*/1);
	sam_putbits2bs(sbrChan->startFreq, /*SI_SBR_START_FREQ_BITS*/4);
	sam_putbits2bs(sbrChan->stopFreq, /*SI_SBR_STOP_FREQ_BITS*/4);
	sam_putbits2bs(0,  /*SI_SBR_XOVER_BAND_BITS*/3);
	sam_putbits2bs(0,  /*SI_SBR_RESERVED_BITS_HDR*/2);
	sam_putbits2bs(0,  /*SI_SBR_HEADER_EXTRA_1_BITS*/1);
	sam_putbits2bs(0,  /*SI_SBR_HEADER_EXTRA_2_BITS*/1);
  }
#else
  if(!sbrheaderInBSAC) {
    sam_putbits2bs(0, /*SI_SBR_HDR_BIT*/1); /* no header */
	sbrChan->sendHdrCnt = -1;
  }
  else {
	if (sbrChan->sendHdrCnt==0)
      sam_putbits2bs(1, /*SI_SBR_HDR_BIT*/1); /* header will follow */
    else
      sam_putbits2bs(0, /*SI_SBR_HDR_BIT*/1); /* no header */

    /* header */
    if (sbrChan->sendHdrCnt==0) {
	  sam_putbits2bs(1, /*SI_SBR_AMP_RES_BITS*/1);
	  sam_putbits2bs(sbrChan->startFreq, /*SI_SBR_START_FREQ_BITS*/4);
	  sam_putbits2bs(sbrChan->stopFreq, /*SI_SBR_STOP_FREQ_BITS*/4);
	  sam_putbits2bs(0,  /*SI_SBR_XOVER_BAND_BITS*/3);
	  sam_putbits2bs(0,  /*SI_SBR_RESERVED_BITS_HDR*/2);
	  sam_putbits2bs(0,  /*SI_SBR_HEADER_EXTRA_1_BITS*/1);
	  sam_putbits2bs(0,  /*SI_SBR_HEADER_EXTRA_2_BITS*/1);
	}
  }
#endif

  /* data */
  writeSbrSCEdatainBSAC(sbrChan);

  /* fill bits */
  sam_putbits2bs(0, sbrChan->fillBits);

#ifdef BSACCONF
  if(!sbrheaderInBSAC || sbrChan->sendHdrCnt!=0)
	  sam_putbits2bs(0, 16);
#endif

  return sbrChan->totalBits;
#else
  /* read SBR data */
  static int count, esc_count, loop;
  static unsigned char Data[128];
  int i, k;

  if(readEnable_bsac) {
	 count = sbr_bits_bsac >> 3;
	 esc_count = 0;
	 if(count >= 16) {
	  loop = count - 1;
	  esc_count = count-15+1;
	  count = 15;
	}
	else
	  loop = count;
	for(i = 0; i < loop; i ++) {
		k = fread(&Data[i], sizeof(unsigned char), 1, sbrPayloadFile_bsac);
		if(k != 1)
		  printf("error in reading SBR file\n");
	}    
	readEnable_bsac = 0;
  }
  sam_putbits2bs(count, /*LEN_F_CNT*/4); /* count */
  if (esc_count) {
    sam_putbits2bs(esc_count, /*LEN_BYTE*/8); /* esc_count */
  }  
  sam_putbits2bs(Data[0]&0xF, 4);
  for(i = 1; i < loop; i ++) {
	sam_putbits2bs(Data[i], 8);
  }

  return sbr_bits_bsac;
#endif
}

int WriteSbrCPEinBSAC(SBR_CHAN *sbrChan)
{
#ifndef READ_SBR
  sam_putbits2bs(sbrChan->count, /*LEN_F_CNT*/4); /* count */
  if (sbrChan->esc_count) {
    sam_putbits2bs(sbrChan->esc_count, /*LEN_BYTE*/8); /* esc_count */
  }

#ifdef BSACCONF
  if(!sbrheaderInBSAC) {
    sam_putbits2bs(0, /*SI_SBR_HDR_BIT*/1); /* no header */
    sbrChan->sendHdrCnt = -1;
  }
  else {
#endif
  if (sbrChan->sendHdrCnt==0)
    sam_putbits2bs(1, /*SI_SBR_HDR_BIT*/1); /* header will follow */
  else
    sam_putbits2bs(0, /*SI_SBR_HDR_BIT*/1); /* no header */

  /* header */
  if (sbrChan->sendHdrCnt==0) {
	sam_putbits2bs(1, /*SI_SBR_AMP_RES_BITS*/1);
	sam_putbits2bs(sbrChan->startFreq, /*SI_SBR_START_FREQ_BITS*/4);
	sam_putbits2bs(sbrChan->stopFreq, /*SI_SBR_STOP_FREQ_BITS*/4);
	sam_putbits2bs(0,  /*SI_SBR_XOVER_BAND_BITS*/3);
	sam_putbits2bs(0,  /*SI_SBR_RESERVED_BITS_HDR*/2);
	sam_putbits2bs(0,  /*SI_SBR_HEADER_EXTRA_1_BITS*/1);
	sam_putbits2bs(0,  /*SI_SBR_HEADER_EXTRA_2_BITS*/1);
  }
#ifdef BSACCONF
  }
#endif

  /* data */
  writeSbrCPEDatainBSAC(sbrChan, sbrChan+1);

  /* fill bits */
  sam_putbits2bs(0, sbrChan->fillBits);

#ifdef BSACCONF
  if(!sbrheaderInBSAC || sbrChan->sendHdrCnt!=0)
	  sam_putbits2bs(0, 16);
#endif

  return sbrChan->totalBits;
#else
    /* read SBR data */
  static int count, esc_count, loop;
  static unsigned char Data[128];
  int i, k;

  if(readEnable_bsac) {
	 count = sbr_bits_bsac >> 3;
	 esc_count = 0;
	 if(count >= 16) {
	  loop = count - 1;
	  esc_count = count-15+1;
	  count = 15;
	}
	else
	  loop = count;
	for(i = 0; i < loop; i ++) {
		k = fread(&Data[i], sizeof(unsigned char), 1, sbrPayloadFile_bsac);
		if(k != 1)
		  printf("error in reading SBR file\n");
	}    
	readEnable_bsac = 0;
  }
  sam_putbits2bs(count, /*LEN_F_CNT*/4); /* count */
  if (esc_count) {
    sam_putbits2bs(esc_count, /*LEN_BYTE*/8); /* esc_count */
  }  
  sam_putbits2bs(Data[0]&0xF, 4);
  for(i = 1; i < loop; i ++) {
	sam_putbits2bs(Data[i], 8);
  }

  return sbr_bits_bsac;
#endif
}

int WriteSBRinBSAC(SBR_CHAN *sbrChan, int nch)
{
	int bitused = 0;

	if(nch==1)
		bitused = WriteSbrSCEinBSAC(sbrChan);
	else if(nch==2)
		bitused = WriteSbrCPEinBSAC(sbrChan);

	return bitused;
}
