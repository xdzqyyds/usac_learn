/***********

This software module was originally developed by

Dolby Laboratories

and edited by

Takashi Koike (Sony Corporation)
Olaf Kaehler (Fraunhofer IIS-A)

in the course of development of the MPEG-2 AAC/MPEG-4 
Audio standard ISO/IEC13818-7, 14496-1, 2 and 3. This software module is an implementation of a part
of one or more MPEG-2 AAC/MPEG-4 Audio tools as specified by the
MPEG-2 aac/MPEG-4 Audio standard. ISO/IEC  gives users of the
MPEG-2aac/MPEG-4 Audio standards free license to this software module
or modifications thereof for use in hardware or software products
claiming conformance to the MPEG-2 aac/MPEG-4 Audio  standards. Those
intending to use this software module in hardware or software products
are advised that this use may infringe existing patents. The original
developer of this software module, the subsequent
editors and their companies, and ISO/IEC have no liability for use of
this software module or modifications thereof in an
implementation. Copyright is not released for non MPEG-2 aac/MPEG-4
Audio conforming products. The original developer retains full right to
use the code for the developer's own purpose, assign or donate the code to a
third party and to inhibit third party from using the code for non
MPEG-2 aac/MPEG-4 Audio conforming products. This copyright notice
must be included in all copies or derivative works. Copyright 1996.  

***********/

/******************************************************************** 
/ 
/ filename : aac_qc.h
/ project : MPEG-2 aac
/ author : SNL, Dolby Laboratories, Inc
/ date : November 11, 1996
/ contents/description : header file for aac_qc.c
/          
/
*******************************************************************/

#ifndef _INCLUDED_AAC_QC_H_
#define _INCLUDED_AAC_QC_H_

#include "block.h"
#include "nok_ltp_enc.h"
#include "tns3.h"
#include "sony_local.h"
#include "aac_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

/* assumptions for the first run of this quantizer */
#define DEBUG 1
#define CHANNEL  1
#define NUM_COEFF  1024
#define MAGIC_NUMBER  0.4054
#define MAX_QUANT 8192
#define SF_OFFSET 100
#define abs(A) ((A) < 0 ? (-A) : (A))
#define sgn(A) ((A) > 0 ? (1) : (-1))



#define PNS_HCB 13                               /* reserved codebook for flagging PNS */
#define PNS_PCM_BITS 9                           /* size of first (PCM) PNS energy */
#define PNS_PCM_OFFSET (1 << (PNS_PCM_BITS-1))   /* corresponding PCM transmission offset */
#define PNS_SF_OFFSET 90                         /* transmission offset for PNS energies */


typedef struct {
  int scale_factor[SFB_NUM_MAX];
  int book_vector[SFB_NUM_MAX];
  int quant[NUM_COEFF];
} QuantInfo;

#ifdef I2R_LOSSLESS
typedef struct {
  int scale_factor[SFB_NUM_MAX];
  int book_vector[SFB_NUM_MAX];
  int quant_aac[MAX_OSF*NUM_COEFF];

  int ll_present;

  int max_sfb;                          /* max_sfb, should = nr_of_sfb/num_window_groups */
  int sfb_offset[250];                  /* Scalefactor spectral offset, interleaved */

  WINDOW_SEQUENCE windowSequence;
  int num_window_groups;                /* Number of window groups */
  int window_group_length[MAX_SHORT_IN_LONG_BLOCK];          /* Length (in windows) of each window group */

  int total_sfb;		        /*  total sfb number */

  WINDOW_TYPE block_type;	        /* Block type */
  
} AACQuantInfo;
#endif

void tf_init_encode_spectrum_aac( int debug );

int tf_quantize_spectrum(
  QuantInfo   *qInfo,
  double      *spectrum,
  double      *p_reconstructed_spectrum,
  const double energy[MAX_SCFAC_BANDS],
  const double allowed_dist[MAX_SCFAC_BANDS],
  WINDOW_SEQUENCE  windowSequence,
  const int   sfb_width_table[MAX_SCFAC_BANDS],
  int         grouped_sfb_offsets[MAX_SCFAC_BANDS+1],
  int         nr_of_sfb,
  const PnsInfo *pnsInfo,
  int         available_bits,
  int         blockSizeSamples,
  int         num_window_groups,
  const int   window_group_length[],
  int         aacAllowScalefacs);

int bitpack_ind_channel_stream(
  HANDLE_AACBITMUX bitmux,
  ICSinfo     *ics_info,      /* buffer for ics_info */
  ToolsInfo   *tools_info,    /* buffer for gc_info and tns_info */
  QuantInfo   *qInfo,
  int         commonWindow,
  WINDOW_SEQUENCE windowSequence,
  int         num_window_groups,
  int         sfb_offset[MAX_SCFAC_BANDS+1],
  int         nr_of_sfb,
  const PnsInfo *pnsInfo,
  int         scale_flag,
  int         qdebug
);


#ifdef __cplusplus
}
#endif


#endif



