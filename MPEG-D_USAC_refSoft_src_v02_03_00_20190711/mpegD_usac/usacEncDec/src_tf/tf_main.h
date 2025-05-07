/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
 "This software module was originally developed by 
 Fraunhofer Gesellschaft IIS / University of Erlangen (UER)
 and edited by
 Takashi Koike (Sony Corporation)
 Olaf Kaehler (Fraunhofer IIS-A)
 in the course of 
 development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
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

/* CREATED BY :  Bernhard Grill -- June-96  */

/* 28-Aug-1996  NI: added "NO_SYNCWORD" to enum TRANSPORT_STREAM */

#ifndef _TF_MAIN_H_INCLUDED
#define _TF_MAIN_H_INCLUDED

#include "ntt_conf.h"
#include "block.h"

void buffer2freq( double           p_in_data[],      /* Input: Time signal              */   
                  double           p_out_mdct[],     /* Output: MDCT cofficients        */
                  double           p_overlap[],
                  WINDOW_SEQUENCE  windowSequence,
                  WINDOW_SHAPE     wfun_select,      /* offers the possibility to select different window functions */
                  WINDOW_SHAPE     wfun_select_prev,
                  int              nlong,            /* shift length for long windows   */
                  int              nshort,           /* shift length for short windows  */
                  Mdct_in          overlap_select,   /* select mdct input *TK*          */
                  int              previousMode,
                  int              nextMode,
                  int              num_short_win);   /* number of short windows to      
                                                        transform                       */
void freq2buffer( double           p_in_data[],      /* Input: MDCT coefficients                */
                  double           p_out_data[],     /* Output:time domain reconstructed signal */
                  double           p_overlap[],  
                  WINDOW_SEQUENCE  windowSequence,
                  int              nlong,            /* shift length for long windows   */
                  int              nshort,           /* shift length for short windows  */
                  WINDOW_SHAPE     wfun_select,      /* offers the possibility to select different window functions */
                  WINDOW_SHAPE     wfun_select_prev, /* YB : 971113 */
                  Imdct_out        overlap_select,   /* select imdct output *TK*        */
                  int              num_short_win );  /* number of short windows to  transform */

void mdct( double in_data[], double out_data[], int len);

void imdct(double in_data[], double out_data[], int len);

void fft( double in[], double out[], int len );

/* functions from tvqAUDecode.e */

void tvqInitDecoder (char *decPara, int layer,
                     FRAME_DATA *frameData,
                     TF_DATA *tfData);
                     

void tvqFreeDecoder (HANDLE_TVQDECODER nttDecoder);


void tvqAUDecode ( 
                FRAME_DATA*  fd, /* config data , obj descr. etc. */
                TF_DATA*     tfData,
                HANDLE_DECODER_GENERAL hFault,
                QC_MOD_SELECT      qc_select,
                ntt_INDEX*   index,
                ntt_INDEX*   index_scl,
                int * nextLayer);

#endif  /* #ifndef _TF_MAIN_H_INCLUDED */
