/************************* MPEG-4 Audio Coder **************************
 *                                                                           *
 "This software module was originally developed by 
 Fraunhofer IIS
 and edited by
 -

 in the course of 
 development of the MPEG-4 Audio standard ISO/IEC 14496-3.
 This software module is an implementation of a part of one or more 
 MPEG-4 Audio tools as specified by the MPEG-4 Audio standard.
 ISO/IEC  gives users of the
 standards free license to this software module or modifications thereof for use in 
 hardware or software products claiming conformance to the MPEG-4
 Audio  standards. Those intending to use this software module in hardware or 
 software products are advised that this use may infringe existing patents. 
 The original developer of this software module and his/her company, the subsequent 
 editors and their companies, and ISO/IEC have no liability for use of this software 
 module or modifications thereof in an implementation. Copyright is not released for 
 non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
 retains full right to use the code for his/her  own purpose, assign or donate the 
 code to a third party and to inhibit third party from using the code for non 
 MPEG-4 Audio conforming products. This copyright notice must
 be included in all copies or derivative works." 
 Copyright(c)2006.
 *                                                                           *
 ****************************************************************************/

#ifndef __VMTYPES_H__
#define __VMTYPES_H__

#ifndef HAS_Float
#define HAS_Float
typedef double   Float;
#endif /*HAS_FLOAT*/

#ifndef HAS_ULONG
#define HAS_ULONG
typedef unsigned long   ulong;
#endif /*HAS_ULONG*/

#ifndef HAS_BYTE
#define HAS_BYTE
typedef unsigned char   byte;
#endif

#endif
