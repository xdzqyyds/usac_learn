/**************************************************************************

This software module was originally developed by

Dolby Laboratories

and edited by

Takashi Koike (Sony Corporation)
Olaf Kaehler (Fraunhofer IIS-A)
Guillaume Fuchs (Fraunhofer IIS)

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

**************************************************************************/

#ifndef _INCLUDED_USAC_FD_QC_H_
#define _INCLUDED_USAC_FD_QC_H_

#include "block.h"
#include "nok_ltp_enc.h"
#include "tns3.h"
#include "sony_local.h"
#include "aac_tools.h"
#include "usac_arith_dec.h"
#include "usac_bitmux.h"


#ifdef __cplusplus
extern "C" {
#endif

/* assumptions for the first run of this quantizer */
#define DEBUG 1
#define CHANNEL  1
#define NUM_COEFF  1152
#define MAGIC_NUMBER  0.4054
#define MAX_QUANT 8192
#define SF_OFFSET 100
#define abs(A) ((A) < 0 ? (-A) : (A))
#define sgn(A) ((A) > 0 ? (1) : (-1))

struct USAC_QUANT_INFO{
  int    scale_factor[SFB_NUM_MAX];
  int    quant[NUM_COEFF];
  int    quantDegroup[NUM_COEFF];
  int    arithPreviousSize;
  int    reset;
  Tqi2   arithQ[(NUM_COEFF/2)+4];
  int    max_spec_coeffs;
  int    huffTable[13][1090][4];
};

void usac_init_quantize_spectrum(UsacQuantInfo *qInfo, int debugLevel);

int usac_quantize_spectrum(
  UsacQuantInfo   *qInfo,
  double      *spectrum,
  double      *p_reconstructed_spectrum,
  const double energy[MAX_SCFAC_BANDS],
  const double allowed_dist[MAX_SCFAC_BANDS],
  WINDOW_SEQUENCE  windowSequence,
  WINDOW_SHAPE windowShape,
  const int   sfb_width_table[MAX_SCFAC_BANDS],
  int         sfb_offset[MAX_SCFAC_BANDS+1],   /* out: grouped sfb offsets */
  int         max_sfb,
  int         nr_of_sfb,
  int         available_bits,
  int         blockSizeSamples,
  int         num_window_groups,
  const int   window_group_length[],
  int         aacAllowScalefacs, 
  UsacToolsInfo *tool_data,
  TNS_INFO      *tnsInfo,
  int common_window,
  int common_tw,
  int flag_twMdct,
  int flag_noiseFilling,
  int bUsacIndependencyFlag
  );



#ifdef __cplusplus
}
#endif


#endif



