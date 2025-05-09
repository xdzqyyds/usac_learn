/*****************************************************************************
 *                                                                           *
"This software module was originally developed by 

Ralph Sperschneider (Fraunhofer Gesellschaft IIS)

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
#ifndef _HUFFDEC2_H_
#define _HUFFDEC2_H_

#include "allHandles.h"
#include "all.h"
#include "block.h"
#include "tns.h"

#include "tf_mainStruct.h"       /* typedefs */

void deinterleave ( void                    *inptr,  /* formerly pointer to type int */
                    void                    *outptr, /* formerly pointer to type int */
                    short                   length,  /* sizeof base type of inptr and outptr in chars */
                    const short             nsubgroups[], 
                    const int               ncells[], 
                    const short             cellsize[],
                    int                     nsbk,
                    int                     blockSize,
                    const HANDLE_RESILIENCE hResilience,
                    short                   ngroups );

void          clr_tns ( Info*           info, 
                        TNS_frame_info* tns_frame_info );

void          getgroup ( Info*             info, 
                         unsigned char*    group,
                         HANDLE_RESILIENCE hResilience, 
                         HANDLE_ESC_INSTANCE_DATA    hEPInfo,
                         HANDLE_BUFFER     hVm  );

int getics ( HANDLE_AACDECODER        hDec,
             Info*                    info, 
             int                      common_window, 
             WINDOW_SEQUENCE*         win, 
             WINDOW_SHAPE* 	      wshape, 
             unsigned char*           group, 
             unsigned char*           max_sfb, 
             PRED_TYPE                pred_type, 
             int*                     lpflag, 
             int*                     prstflag, 
             byte*                    cb_map, 
             double*                   coef, 
             int                      max_spec_coefficients,
             short*                   global_gain, 
             short*                   factors,
             NOK_LT_PRED_STATUS*      nok_ltp, 
             TNS_frame_info*          tns,
             HANDLE_BSBITSTREAM       gc_streamCh, 
             enum AAC_BIT_STREAM_TYPE bitStreamType,
             HANDLE_RESILIENCE        hResilience,
             HANDLE_BUFFER            hHcrSpecData,
             HANDLE_HCR               hHcrInfo,
             HANDLE_ESC_INSTANCE_DATA           hEPInfo,
             HANDLE_CONCEALMENT       hConcealment,
             QC_MOD_SELECT            qc_select,
             HANDLE_BUFFER            hVm,
             /* needful for ER_AAC_Scalable */
             int                      extensionLayerFlag,   /* signaled an extension layer */
             int                      er_channel            /* 0:left 1:right channel      */
#ifdef I2R_LOSSLESS
             , int*                   sls_quant_mono_temp
#endif
             );


int           getmask ( Info*             info, 
                        unsigned char*    group, 
                        unsigned char     max_sfb, 
                        unsigned char*    mask,
                        HANDLE_RESILIENCE hResilience, 
                        HANDLE_ESC_INSTANCE_DATA    hEPInfo,
                        HANDLE_BUFFER     hVm  );

unsigned short HuffSpecKernelPure ( int*                     qp, 
                                    Hcb*                     hcb, 
                                    Huffman*                 hcw, 
                                    int                      step,
                                    HANDLE_HCR               hHcrInfo,
                                    HANDLE_RESILIENCE        hResilience,
                                    HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                    HANDLE_CONCEALMENT       hConcealment,
                                    HANDLE_BUFFER            hSpecData );

unsigned char Vcb11Used ( unsigned short ); 

#endif  /* #ifndef _HUFFDEC2_H_ */
