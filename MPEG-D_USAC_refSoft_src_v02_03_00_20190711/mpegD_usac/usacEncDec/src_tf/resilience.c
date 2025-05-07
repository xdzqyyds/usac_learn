/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by

Heiko Purnhagen     (University of Hannover / ACTS-MoMuSys)

and edited by

Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
Ali Nowbakht-Irani  (Fraunhofer Gesellschaft IIS)
Markus Werner       (SEED / Software Development Karlsruhe) 

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

Copyright (c) 2000.

$Id: resilience.c,v 1.1.1.1 2009-05-29 14:10:17 mul Exp $
Error resilience
**********************************************************************/


#include <stdlib.h>
#include <string.h>

#include "allHandles.h"

#include "resilience.h"
#include "common_m4a.h"

typedef struct tag_resilience
{
  char epFlag;
  char aacLenOfLongestCwFlag;
  char aacReorderSpecPreSortFlag;
  char aacReorderSpecFlag;
  char aacConsecutiveReorderSpecFlag;
  char aacVcb11Flag; 
  char aacLenOfScfDataFlag;
  char aacRvlcScfFlag;
  char aacScfConcealment;
  char aacScfBitFlag;
} T_RESILIENCE;


HANDLE_RESILIENCE CreateErrorResilience (char* decPara, 
                                         int epFlag, 
                                         int aacSectionDataResilienceFlag,
                                         int aacScalefactorDataResilienceFlag,
                                         int aacSpectralDataResilienceFlag)
{
  
  HANDLE_RESILIENCE handle;


  handle = (HANDLE_RESILIENCE) calloc(1,sizeof(T_RESILIENCE));
  handle->epFlag                        = epFlag;
  handle->aacLenOfLongestCwFlag         = aacSpectralDataResilienceFlag;
  handle->aacReorderSpecFlag            = aacSpectralDataResilienceFlag;
  handle->aacReorderSpecPreSortFlag     = aacSpectralDataResilienceFlag;
  handle->aacConsecutiveReorderSpecFlag = aacSpectralDataResilienceFlag;
  handle->aacVcb11Flag                  = aacSectionDataResilienceFlag;
  handle->aacLenOfScfDataFlag           = 0;
  handle->aacRvlcScfFlag                = aacScalefactorDataResilienceFlag;
  handle->aacScfConcealment             = aacScalefactorDataResilienceFlag;
  handle->aacScfBitFlag                 = aacScalefactorDataResilienceFlag;

  return ( handle );
}

void DeleteErrorResilience(const HANDLE_RESILIENCE handle)
{
  free(handle);
}

char GetEPFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->epFlag );
}

/* aac only */

char GetLenOfLongestCwFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacLenOfLongestCwFlag );
}

char GetLenOfScfDataFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacLenOfScfDataFlag );
}

char GetReorderSpecPreSortFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacReorderSpecPreSortFlag );
}
char GetReorderSpecFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacReorderSpecFlag );
}

char GetConsecutiveReorderSpecFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacConsecutiveReorderSpecFlag );
}

char GetRvlcScfFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacRvlcScfFlag );
}

char GetScfBitFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacScfBitFlag );
}

char GetScfConcealmentFlag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacScfConcealment );
}

char GetVcb11Flag ( const HANDLE_RESILIENCE handle )
{
  return ( handle->aacVcb11Flag );
}


