/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by
  Y.B. Thomas Kim and S.H. Park (Samsung AIT)
and edited by
  Y.B. Thomas Kim (Samsung AIT) on 1997-08-25

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

**********************************************************************/
#ifndef _sam_facoding_h_
#define _sam_facoding_h_

/* DECLARATIONS USED FOR ARITHMETIC ENCODING AND DECODING */

/* SIZE OF ARITHMETIC CODE VALUES */

#define	Code_value_bits 	16	/* Number of bits in a code value */
typedef unsigned long code_value;       /* Type of an arithmetic code value */
 
#define Top_value 0xffff

/* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */

#define First_qtr 0x4000		/* Point after first quarter */
#define Half	  0x8000		/* Point after frist half    */
#define Third_qtr 0xc000		/* Point after third quarter */
#endif /* #ifndef _sam_facoding_h_ */
