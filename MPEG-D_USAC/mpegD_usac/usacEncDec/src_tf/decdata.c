/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS and edited by
Yoshiaki Oikawa (Sony Corporation)
,Mitsuyuki Hatanaka (Sony Corporation),
in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio 
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
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

#include "allHandles.h"

#include "all.h"                 /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "port.h"

Float		iq_exp_tbl[MAX_IQ_TBL];
Float		exptable[TEXP];
Hcb		book[NSPECBOOKS+2+32]; /*TM 990527 */
int		current_program; /* used by config.c */
MC_Info mc_info ; /* used by config.c */
long		bno;
ProgConfig	prog_config;
Info		*win_seq_info[NUM_WIN_SEQ];
Info		only_long_info;
Info		eight_short_info;
Info*		winmap[NUM_WIN_SEQ];
short		sfbwidthShort[(1<<LEN_MAX_SFBS)];


/* scalefactor band tables also used by psych.c */

const short sfb_96_1024[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  32, 36, 40, 44, 48, 52, 56, 
  64, 72, 80, 88, 96, 108, 120, 
  132, 144, 156, 172, 188, 212, 240, 
  276, 320, 384, 448, 512, 576, 640, 
  704, 768, 832, 896, 960, 1024
};   /* 41 scfbands */

const short sfb_96_128[] =
{
  4, 8, 12, 16, 20, 24, 32, 
  40, 48, 64, 92, 128
};   /* 12 scfbands */

const short sfb_96_960[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  32, 36, 40, 44, 48, 52, 56, 
  64, 72, 80, 88, 96, 108, 120, 
  132, 144, 156, 172, 188, 212, 240, 
  276, 320, 384, 448, 512, 576, 640, 
  704, 768, 832, 896, 960
};   /* 41 scfbands */

const short sfb_96_120[] =
{
  4, 8, 12, 16, 20, 24, 32, 
  40, 48, 64, 92, 120
};   /* 12 scfbands */

const short sfb_96_768[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  32, 36, 40, 44, 48, 52, 56, 
  64, 72, 80, 88, 96, 108, 120, 
  132, 144, 156, 172, 188, 212, 240, 
  276, 320, 384, 448, 512, 576, 640, 
  704, 768
};   /* 37 scfbands */

const short sfb_96_96[] =
{
  4, 8, 12, 16, 20, 24, 32, 
  40, 48, 64, 92, 96
};   /* 12 scfbands */


const short sfb_64_1024[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  32, 36, 40, 44, 48, 52, 56, 
  64, 72, 80, 88, 100, 112, 124, 
  140, 156, 172, 192, 216, 240, 268, 
  304, 344, 384, 424, 464, 504, 544, 
  584, 624, 664, 704, 744, 784, 824, 
  864, 904, 944, 984, 1024
};   /* 41 scfbands 47 */ 

const short sfb_64_128[] =
{
  4, 8, 12, 16, 20, 24, 32, 
  40, 48, 64, 92, 128
};   /* 12 scfbands */

const short sfb_64_960[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  32, 36, 40, 44, 48, 52, 56, 
  64, 72, 80, 88, 100, 112, 124, 
  140, 156, 172, 192, 216, 240, 268, 
  304, 344, 384, 424, 464, 504, 544, 
  584, 624, 664, 704, 744, 784, 824, 
  864, 904, 944, 960
};   /* 41 scfbands 47 */ 

const short sfb_64_120[] =
{
  4, 8, 12, 16, 20, 24, 32, 
  40, 48, 64, 92, 120
};   /* 12 scfbands */

const short sfb_64_768[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  32, 36, 40, 44, 48, 52, 56, 
  64, 72, 80, 88, 100, 112, 124, 
  140, 156, 172, 192, 216, 240, 268, 
  304, 344, 384, 424, 464, 504, 544, 
  584, 624, 664, 704, 744, 768
};   /* 41 scfbands 41 */ 

const short sfb_64_96[] =
{
  4, 8, 12, 16, 20, 24, 32, 
  40, 48, 64, 92, 96
};   /* 12 scfbands */

const short sfb_48_1024[] = {
  4,	8,	12,	16,	20,	24,	28,	
  32,	36,	40,	48,	56,	64,	72,	
  80,	88,	96,	108,	120,	132,	144,	
  160,	176,	196,	216,	240,	264,	292,	
  320,	352,	384,	416,	448,	480,	512,	
  544,	576,	608,	640,	672,	704,	736,	
  768,	800,	832,	864,	896,	928,	1024
};

const short sfb_48_960[] = { 
  4,      8,      12,     16,     20,     24,     28,
  32,     36,     40,     48,     56,     64,     72,
  80,     88,     96,     108,    120,    132,    144,
  160,    176,    196,    216,    240,    264,    292,
  320,    352,    384,    416,    448,    480,    512,
  544,    576,    608,    640,    672,    704,    736,
  768,    800,    832,    864,    896,    928,    960, 0 };

const short sfb_48_768[] = {
  4,	8,	12,	16,	20,	24,	28,	
  32,	36,	40,	48,	56,	64,	72,	
  80,	88,	96,	108,	120,	132,	144,	
  160,	176,	196,	216,	240,	264,	292,	
  320,	352,	384,	416,	448,	480,	512,	
  544,	576,	608,	640,	672,	704,	736,	
  768
};

const short sfb_48_512[] = {
  4,      8,      12,     16,     20,     24,     28, 
  32,     36,     40,     44,     48,     52,     56, 
  60,     68,     76,     84,     92,     100,    112,
  124,    136,    148,    164,    184,    208,    236,
  268,    300,    332,    364,    396,    428,    460,
  512
};   /* 36 scfbands */

const short sfb_48_480[] = { 
  4,      8,      12,     16,     20,     24,     28, 
  32,     36,     40,     44,     48,     52,     56, 
  64,     72,     80,     88,     96,     108,    120,
  132,    144,    156,    172,    188,    212,    240,
  272,    304,    336,    368,    400,    432,    480
};   /* 35 scfbands */

const short sfb_48_128[] =
{
  4,	8,	12,	16,	20,	28,	36,	
  44,	56,	68,	80,	96,	112,	128
};

const short sfb_48_120[] =
{         
  4,    8,      12,     16,     20,     28,     36,
  44,   56,     68,     80,     96,     112,    120
};

const short sfb_48_96[] =
{
  4,	8,	12,	16,	20,	28,	36,	
  44,	56,	68,	80,	96
};

const short sfb_32_1024[] =
{
  4,	8,	12,	16,	20,	24,	28,	
  32,	36,	40,	48,	56,	64,	72,	
  80,	88,	96,	108,	120,	132,	144,	
  160,	176,	196,	216,	240,	264,	292,	
  320,	352,	384,	416,	448,	480,	512,	
  544,	576,	608,	640,	672,	704,	736,	
  768,	800,	832,	864,	896,	928,	960,
  992,	1024
};

const short sfb_32_960[] =
{ 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72, 80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292, 320, 
  352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736, 768, 800, 832, 864, 896, 928, 960,0 
};

const short sfb_32_768[] =
{
  4,	8,	12,	16,	20,	24,	28,	
  32,	36,	40,	48,	56,	64,	72,	
  80,	88,	96,	108,	120,	132,	144,	
  160,	176,	196,	216,	240,	264,	292,	
  320,	352,	384,	416,	448,	480,	512,	
  544,	576,	608,	640,	672,	704,	736,	
  768
};

const short sfb_32_512[] =
{
  4,     8,       12,     16,     20,     24,     28, 
  32,    36,      40,     44,     48,     52,     56, 
  64,    72,      80,     88,     96,     108,    120, 
  132,   144,     160,    176,    192,    212,    236, 
  260,   288,     320,    352,    384,    416,    448, 
  480,   512
};   /* 37 scfbands */

const short sfb_32_480[] =
{ 
  4,     8,       12,     16,     20,      24,     28, 
  32,    36,      40,     44,     48,      52,     56, 
  60,    64,      72,     80,     88,      96,     104, 
  112,   124,     136,    148,    164,     180,    200, 
  224,   256,     288,    320,    352,     384,    416, 
  448,   480
};   /* 37 scfbands */

const short sfb_32_120[] =
{ 4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96, 112, 120, 0 };

const short sfb_24_1024[] =
{
  4,   8,   12,  16,  20,  24,  28, 
  32,  36,  40,  44,  52,  60,  68, 
  76,  84,  92,  100, 108, 116, 124, 
  136, 148, 160, 172, 188, 204, 220, 
  240, 260, 284, 308, 336, 364, 396, 
  432, 468, 508, 552, 600, 652, 704, 
  768, 832, 896, 960, 1024
};   /* 47 scfbands */

const short sfb_24_960[] =
{
  4,   8,   12,  16,  20,  24,  28,
  32,  36,  40,  44,  52,  60,  68,
  76,  84,  92,  100, 108, 116, 124,
  136, 148, 160, 172, 188, 204, 220,
  240, 260, 284, 308, 336, 364, 396,
  432, 468, 508, 552, 600, 652, 704,
  768, 832, 896, 960,0
};   /* 47 scfbands */

const short sfb_24_768[] =
{
  4,   8,   12,  16,  20,  24,  28, 
  32,  36,  40,  44,  52,  60,  68, 
  76,  84,  92,  100, 108, 116, 124, 
  136, 148, 160, 172, 188, 204, 220, 
  240, 260, 284, 308, 336, 364, 396, 
  432, 468, 508, 552, 600, 652, 704, 
  768
};   /* 47 scfbands */

const short sfb_24_128[] =
{
  4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 128
};   /* 15 scfbands */

const short sfb_24_120[] =
{
  4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 120, 0
};   /* 15 scfbands */

const short sfb_24_96[] =
{
  4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 96
};   /* 14 scfbands */

const short sfb_24_512[] =
{
  4,   8,   12,  16,  20,  24,  28,  
  32,  36,  40,  44,  52,  60,  68, 
  80,  92,  104, 120, 140, 164, 192, 
  224, 256, 288, 320, 352, 384, 416, 
  448, 480, 512
};   /* 31 scfbands */

const short sfb_24_480[] =
{
  4,   8,   12,  16,  20,  24,  28,  
  32,  36,  40,  44,  52,  60,  68, 
  80,  92,  104, 120, 140, 164, 192, 
  224, 256, 288, 320, 352, 384, 416, 
  448, 480
};   /* 30 scfbands */

const short sfb_16_1024[] =
{
  8,   16,  24,  32,  40,  48,  56, 
  64,  72,  80,  88,  100, 112, 124, 
  136, 148, 160, 172, 184, 196, 212, 
  228, 244, 260, 280, 300, 320, 344, 
  368, 396, 424, 456, 492, 532, 572, 
  616, 664, 716, 772, 832, 896, 960, 
  1024
};   /* 43 scfbands */

const short sfb_16_960[] =
{
  8,   16,  24,  32,  40,  48,  56, 
  64,  72,  80,  88,  100, 112, 124, 
  136, 148, 160, 172, 184, 196, 212, 
  228, 244, 260, 280, 300, 320, 344, 
  368, 396, 424, 456, 492, 532, 572, 
  616, 664, 716, 772, 832, 896, 960
};   /* 42 scfbands */

const short sfb_16_768[] =
{
  8,   16,  24,  32,  40,  48,  56, 
  64,  72,  80,  88,  100, 112, 124, 
  136, 148, 160, 172, 184, 196, 212, 
  228, 244, 260, 280, 300, 320, 344, 
  368, 396, 424, 456, 492, 532, 572, 
  616, 664, 716, 768
};   /* 39 scfbands */

const short sfb_16_128[] =
{
  4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108, 128
};   /* 15 scfbands */

const short sfb_16_120[] =
{
  4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108, 120
};   /* 15 scfbands */

const short sfb_16_96[] =
{
  4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 96
};   /* 14 scfbands */

const short sfb_8_1024[] =
{
  12, 24, 36, 48, 60, 72, 84, 
  96, 108, 120, 132, 144, 156, 172, 
  188, 204, 220, 236, 252, 268, 288, 
  308, 328, 348, 372, 396, 420, 448, 
  476, 508, 544, 580, 620, 664, 712, 
  764, 820, 880, 944, 1024
};   /* 40 scfbands */

const short sfb_8_768[] =
{
   12,  24,  36,  48,  60,  72,  84, 
   96, 108, 120, 132, 144, 156, 172, 
  188, 204, 220, 236, 252, 268, 288, 
  308, 328, 348, 372, 396, 420, 448, 
  476, 508, 544, 580, 620, 664, 712, 
  764, 768
};   /* 37 scfbands */

const short sfb_8_128[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  36, 44, 52, 60, 72, 88, 108, 
  128
};   /* 15 scfbands */

const short sfb_8_96[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  36, 44, 52, 60, 72, 88, 96
};   /* 14 scfbands */

const short sfb_8_960[] =
{
  12, 24, 36, 48, 60, 72, 84, 
  96, 108, 120, 132, 144, 156, 172, 
  188, 204, 220, 236, 252, 268, 288, 
  308, 328, 348, 372, 396, 420, 448, 
  476, 508, 544, 580, 620, 664, 712, 
  764, 820, 880, 944, 960
};   /* 40 scfbands */

const short sfb_8_120[] =
{
  4, 8, 12, 16, 20, 24, 28, 
  36, 44, 52, 60, 72, 88, 108, 
  120
};   /* 15 scfbands */

/*
 * to get "closest" sampling frequency index for odd sampling rates
 * (ISO/IEC 14496-3 Table 4.55)
 */
int sampling_boundaries[(1<<LEN_SAMP_IDX)] = {
  92017, 75132, 55426, 46009, 37566, 27713, 23004, 18783,
  13856, 11502, 9391, 0, 0, 0, 0, 0
};

SR_Info samp_rate_info[(1<<LEN_SAMP_IDX)] =
{
  /* sampling_frequency, #long sfb, long sfb, #short sfb, short sfb */
  /* samp_rate, nsfb1024, SFbands1024, nsfb128, SFbands128 */
  {96000, 41, sfb_96_1024, 12, sfb_96_128, 40, sfb_96_960, 12, sfb_96_120, 37, sfb_96_768, 12, sfb_96_96, 8, 4
   ,  0,          0,  0,         0
  },	    /* 96000 */
  {88200, 41, sfb_96_1024, 12, sfb_96_128, 40, sfb_96_960, 12, sfb_96_120, 37, sfb_96_768, 12, sfb_96_96, 8, 4
   ,  0,          0,  0,         0
  },	    /* 88200 */
  {64000, 47, sfb_64_1024, 12, sfb_64_128, 46, sfb_64_960, 12, sfb_64_120, 41, sfb_64_768, 12, sfb_64_96, 13, 5
   ,  0,          0,  0,         0
  },	    /* 64000 */
  {48000, 49, sfb_48_1024, 14, sfb_48_128, 49, sfb_48_960, 14, sfb_48_120, 43, sfb_48_768, 12, sfb_48_96, 18, 5
   , 35, sfb_48_480, 36, sfb_48_512 
  },	    /* 48000 */
  {44100, 49, sfb_48_1024, 14, sfb_48_128, 49, sfb_48_960, 14, sfb_48_120, 43, sfb_48_768, 12, sfb_48_96, 18, 5
   , 35, sfb_48_480, 36, sfb_48_512
  },	    /* 44100 */
  {32000, 51, sfb_32_1024, 14, sfb_48_128, 49, sfb_32_960, 14, sfb_48_120, 43, sfb_32_768, 12, sfb_48_96, 26, 6
   , 37, sfb_32_480, 37, sfb_32_512
  },	    /* 32000 */
  {24000, 47, sfb_24_1024, 15, sfb_24_128, 46, sfb_24_960, 15, sfb_24_120, 43, sfb_24_768, 14, sfb_24_96, 36, 8
   , 30, sfb_24_480, 31, sfb_24_512
  },	    /* 24000 */
  {22050, 47, sfb_24_1024, 15, sfb_24_128, 46, sfb_24_960, 15, sfb_24_120, 43, sfb_24_768, 14, sfb_24_96, 36, 8
   , 30, sfb_24_480, 31, sfb_24_512
  },	    /* 22050 */
  {16000, 43, sfb_16_1024, 15, sfb_16_128, 42, sfb_16_960, 15, sfb_16_120, 39, sfb_16_768, 14, sfb_16_96, 54,  8
   ,  0,          0,  0,         0
  },	    /* 16000 */
  {12000, 43, sfb_16_1024, 15, sfb_16_128, 42, sfb_16_960, 15, sfb_16_120, 39, sfb_16_768, 14, sfb_16_96, 104, 10
   ,  0,          0,  0,         0
  },	    /* 12000 */
  {11025, 43, sfb_16_1024, 15, sfb_16_128, 42, sfb_16_960, 15, sfb_16_120, 39, sfb_16_768, 14, sfb_16_96, 104, 10
   ,  0,          0,  0,         0
  },	    /* 11025 */
  { 8000, 40,  sfb_8_1024, 15,  sfb_8_128, 40,  sfb_8_960, 15,  sfb_8_120, 37, sfb_8_768, 14,  sfb_8_96, 104, 9
   ,  0,          0,  0,         0
  },	    /*  8000 */
  { 7350, 40,  sfb_8_1024, 15,  sfb_8_128, 40,  sfb_8_960, 15,  sfb_8_120, 37, sfb_8_768, 14,  sfb_8_96, 104, 9
   ,  0,          0,  0,         0
  },	    /*  8000 */
  {    0,  0,           0,  0,          0,  0,          0,  0,          0, 0,   0
   ,  0,          0,  0,         0,0
  }
};





/* 
 * profile dependent parameters
 */
static const int 
tns_max_bands_tbl[(1<<LEN_SAMP_IDX)][6] = {
  /* entry for each sampling rate	
   * 1    Main/LC long window
   * 2    Main/LC short window
   * 3    USAC long window
   * 4    USAC short window
   * 5    SSR long window
   * 6    SSR short window
   */
  {31,  9, 31,  9, 28,  7},	    /* 96000 */
  {31,  9, 31,  9, 28,  7},	    /* 88200 */
  {34, 10, 34, 10, 27,  7},	    /* 64000 */
  {40, 14, 40, 14, 26,  6},	    /* 48000 */
  {42, 14, 42, 14, 26,  6},	    /* 44100 */
  {51, 14, 51, 14, 26,  6},	    /* 32000 */
  {46, 14, 47, 15, 29,  7},	    /* 24000 */
  {46, 14, 47, 15, 29,  7},	    /* 22050 */
  {42, 14, 43, 15, 23,  8},	    /* 16000 */
  {42, 14, 43, 15, 23,  8},	    /* 12000 */
  {42, 14, 43, 15, 23,  8},	    /* 11025 */
  {39, 14, 40, 15, 19,  7},	    /* 8000  */
  {39, 14, 40, 15, 19,  7},	    /* 7350  */
  {0,0,0,0,0,0},
  {0,0,0,0,0,0},
  {0,0,0,0,0,0}
};

static const int
tns_max_bands_tbl_low_delay[(1<<LEN_SAMP_IDX)][2] = {
  /* entry for each sampling rate	
   * 1    LowDelay long window 480
   * 2    LowDelay long window 512
   */
  {0, 0},	    /* 96000 */
  {0, 0},	    /* 88200 */
  {0, 0},	    /* 64000 */
  {31, 31},	    /* 48000 */
  {32, 32},	    /* 44100 */
  {37, 37},	    /* 32000 */
  {30, 31},	    /* 24000 */
  {30, 31},	    /* 22050 */
  {0, 0},	    /* 16000 */
  {0, 0},	    /* 12000 */
  {0, 0},	    /* 11025 */
  {0, 0},	    /* 8000  */
  {0, 0},           /* 7350  */
  {0,0},
  {0,0},
  {0,0}
};

int tns_max_bands ( int islong, TNS_COMPLEXITY profile, int samplFreqIdx)
{
  int i = 0;

  switch (profile) {
  case TNS_LC:
    i = islong ? 0 : 1;
    i+= (mc_info.profile == SSR_Profile) ? 2 : 0;
    return tns_max_bands_tbl[mc_info.sampling_rate_idx][i];
  case TNS_LD_480:
    return tns_max_bands_tbl_low_delay[samplFreqIdx][0];
  case TNS_LD_512:
    return tns_max_bands_tbl_low_delay[samplFreqIdx][1];
  case TNS_USAC:
    i = 2;
     /* construct second index */
    i += islong ? 0 : 1;
    return tns_max_bands_tbl[samplFreqIdx][i];
  case TNS_SSR:
    i = 4;
     /* construct second index */
    i += islong ? 0 : 1;
    return tns_max_bands_tbl[samplFreqIdx][i];
  default:
    /* construct second index */
    i += islong ? 0 : 1;
    return tns_max_bands_tbl[samplFreqIdx][i];
  }
}


int tns_max_order(int islong, TNS_COMPLEXITY profile, int samplFreqIdx)
{
  switch (profile) {
  case TNS_SCAL:
  case TNS_BSAC: /* BSAC previously always had 12 for long blocks, fixed according to Table 4.102 */
  default:
    if (islong) {
      if (samplFreqIdx>=5) /* SampleRate <= 32000 */ 
        return 12;
      else 
        return 20;
    } else {
      return 7;
    }
  case TNS_MAIN:
    if (islong) return 20;
    else return 7;
  case TNS_LC:
  case TNS_SSR:
    if (islong) return 12;
    else return 7;
  case TNS_USAC:
    if (islong) return 15;
    else return 7;
  }
  return 0;
}



static int
pred_max_bands_tbl[(1<<LEN_SAMP_IDX)] = {
  33,     /* 96000 */
  33,     /* 88200 */
  38,     /* 64000 */
  40,     /* 48000 */
  40,     /* 44100 */
  40,     /* 32000 */
  41,     /* 24000 */
  41,     /* 22050 */
  37,     /* 16000 */
  37,     /* 12000 */
  37,     /* 11025 */
  34,     /* 8000  */
  34,     /* 7350  */
  0,
  0,
  0
};

int pred_max_bands(int sf_index)
{
  return pred_max_bands_tbl[sf_index];
}

