/*****************************************************************************
 *                                                                           *
"This software module was originally developed by 

Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
  
and edited by

Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)

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
Copyright(c)1996, 1997, 1998, 1999.
 *                                                                           *
 ****************************************************************************/
#ifndef _resilience_h_
#define _resilience_h_


HANDLE_RESILIENCE CreateErrorResilience ( char* decPara, 
                                          int epFlag, 
                                          int aacSectionDataResilienceFlag,
                                          int aacScalefactorDataResilienceFlag,
                                          int aacSpectralDataResilienceFlag);

void DeleteErrorResilience( const HANDLE_RESILIENCE ptr);

char GetEPFlag ( const HANDLE_RESILIENCE ptr );

char GetLenOfLongestCwFlag ( const HANDLE_RESILIENCE ptr );

char GetLenOfScfDataFlag ( const HANDLE_RESILIENCE ptr );

char GetReorderSpecPreSortFlag ( const HANDLE_RESILIENCE handle );

char GetReorderSpecFlag ( const HANDLE_RESILIENCE ptr );

char GetConsecutiveReorderSpecFlag ( const HANDLE_RESILIENCE handle );

char GetRvlcScfFlag ( const HANDLE_RESILIENCE ptr );

char GetScfBitFlag ( const HANDLE_RESILIENCE ptr );

char GetScfConcealmentFlag ( const HANDLE_RESILIENCE ptr );

char GetVcb11Flag ( const HANDLE_RESILIENCE handle );

#endif /* _resilience_h_ */
