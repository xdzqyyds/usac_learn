/*
This software module was originally developed by 
Ralf Geiger (Fraunhofer IIS AEMT) 
in the course of development of the MPEG-4 extension 3 
audio scalable lossless (SLS) coding . This software module is an 
implementation of a part of one or more MPEG-4 Audio tools as 
specified by the MPEG-4 Audio standard. ISO/IEC  gives users of the
MPEG-4 Audio standards free license to this software module
or modifications thereof for use in hardware or software products
claiming conformance to the MPEG-4 Audio  standards. Those
intending to use this software module in hardware or software products
are advised that this use may infringe existing patents. The original
developer of this software module, the subsequent
editors and their companies, and ISO/IEC have no liability for use of
this software module or modifications thereof in an
implementation. Copyright is not released for non MPEG-4
Audio conforming products. The original developer retains full right to
use the code for the developer's own purpose, assign or donate the code to a
third party and to inhibit third party from using the code for non
MPEG-4 Audio conforming products. This copyright notice
must be included in all copies or derivative works. 

Copyright 2003.  
*/


#ifndef __INV_INT_MDCT_H
#define __INV_INT_MDCT_H


#include "all.h"
#include "int_mdct_defs.h"

void InvIntMDCT(int* p_in,
		int* p_overlap,
		int* p_out,	
		byte windowType,
		byte windowShape,
		int osf);

void InvStereoIntMDCT(int* p_in_0,
		      int* p_in_1,
		      int* p_overlap_0,
		      int* p_overlap_1,
		      int* p_out_0,	
		      int* p_out_1,	
		      byte windowType,
		      byte windowShape,
		      int osf);

#endif




