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
#ifndef _HUFFDEC3_H_
#define _HUFFDEC3_H_

#include "allHandles.h"

int           decode_huff_cw ( Huffman*          h,
                               unsigned short    dataFlag, 
                               HANDLE_RESILIENCE hResilience, 
                               HANDLE_ESC_INSTANCE_DATA    hEPInfo,
                               HANDLE_BUFFER     hVm );

void            get_sign_bits ( int*              q, 
                                int               n,
                                HANDLE_RESILIENCE hResilience, 
                                HANDLE_ESC_INSTANCE_DATA    hEPInfo,
                                HANDLE_BUFFER     hVm);

void            hufftab ( Hcb*          hcb, 
                          Huffman*      hcw, 
                          int           dim, 
                          int           lav, 
                          int           lavInclEsc,
                          int           signed_cb,
                          unsigned char maxCWLen );

void            unpack_idx ( int* qp, 
                             int  idx, 
                             Hcb* hcb);

#endif  /* #ifndef _HUFFDEC3_H_ */
