/**************************************************************************

This software module was originally developed by
Nokia in the course of development of the MPEG-2 AAC/MPEG-4 
Audio standard ISO/IEC13818-7, 14496-1, 2 and 3.
This software module is an implementation of a part
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
must be included in all copies or derivative works.
Copyright (c)1997.  

***************************************************************************/

#ifndef _nok_prediction_h_
#define _nok_prediction_h_

#define         NOK_LPC                    2
#define         NOK_BLN5                   10
#define         NOK_BLEN                   168
#define         NOK_CLEN                   672

/* Predictor status information for mono predictor */
typedef struct {
  short x_buffer[NOK_BLN5][NOK_BLEN];
  Float	*prev_quant;
  unsigned char pred[NOK_LPC][NOK_CLEN];
  int indicate;
} NOK_PRED_STATUS;

#endif






