/*******************************************************************************
This software module was originally developed by

AT&T, Dolby Laboratories, Fraunhofer IIS, and VoiceAge Corp.


Initial author:

and edited by:

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Assurance that the originally developed software module can be used (1) in
ISO/IEC 23003 once ISO/IEC 23003 has been adopted; and (2) to develop ISO/IEC
23003:s
Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, VoiceAge Corp.
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, VoiceAge Corp. 
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, VoiceAge Corp. retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
$Id: mod_buf.c,v 1.1.1.1 2009-05-29 14:10:17 mul Exp $
*******************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "common_m4a.h"

#include "allHandles.h"
#include "mod_buf.h"

#define INVALID_HANDLE 0
#define TRUE 1
#define FALSE 0

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b)) 
#endif
#ifndef NULL 
#define NULL 0
#endif

struct tag_modulo_buffer {
    unsigned int   size;
    unsigned int   current_position;
    unsigned int   readPosition;
    float*         paBuffer;
    int            numVal;
} T_MODULO_BUFFER;

/*****************************************************************************

    functionname: CreateFloatModuloBuffer  
    description:  creates a circular buffer
    returns:      Status info
    input:        number of float values inside circular buffer
    output:       Handle

*****************************************************************************/
HANDLE_MODULO_BUF_VM CreateFloatModuloBuffer(unsigned int size)
{
    HANDLE_MODULO_BUF_VM pModuloBuffer;

    assert(size > 0);

    pModuloBuffer = (HANDLE_MODULO_BUF_VM)calloc(1,sizeof(T_MODULO_BUFFER));

    if(pModuloBuffer == NULL)
        return(INVALID_HANDLE);

    pModuloBuffer->size = size;
    pModuloBuffer->paBuffer = (float*)malloc(sizeof(float)*size);

    if(pModuloBuffer->paBuffer == NULL) {
      DeleteFloatModuloBuffer(pModuloBuffer);
      return INVALID_HANDLE;
    }

    pModuloBuffer->current_position = 0;
    pModuloBuffer->readPosition     = 0;
    pModuloBuffer->numVal     = 0;

    return(pModuloBuffer);
}



/*****************************************************************************

    functionname: DeleteFloatModuloBuffer  
    description:  frees memeory 
    returns:      
    input:        Handle
    output:       

*****************************************************************************/
void DeleteFloatModuloBuffer(HANDLE_MODULO_BUF_VM hModuloBuffer)
{
  if (hModuloBuffer) {
    if (hModuloBuffer->paBuffer) free(hModuloBuffer->paBuffer);
    free(hModuloBuffer);
  }
}



/*****************************************************************************

    functionname: AddFloatModuloBufferValues  
    description:  adds values to circular buffer
    returns:      TRUE (size match is verified by assertion)
    input:        Handle, ptr to values to enter, number of values to enter
    output:       

*****************************************************************************/
static void copyFLOAT( const float src[], float dest[], int inc_src, int inc_dest, int vlen )
{
  int i;

  for( i=0; i<vlen-1; i++ ) {
    *dest = *src;
    dest += inc_dest;
    src  += inc_src;
  }
  if (vlen) /* just for bounds-checkers sake */
    *dest = *src;

}

unsigned char AddFloatModuloBufferValues(HANDLE_MODULO_BUF_VM hModuloBuffer, const float *values, unsigned int n){
    unsigned int lim1, lim2;

    assert(n <= hModuloBuffer->size);

    lim1 = min(hModuloBuffer->current_position+n, hModuloBuffer->size) - hModuloBuffer->current_position;
    copyFLOAT(values,
              hModuloBuffer->paBuffer+hModuloBuffer->current_position,
              1,
              1,
              lim1);

    hModuloBuffer->current_position = (hModuloBuffer->current_position+lim1)%hModuloBuffer->size;
    hModuloBuffer->numVal += n;
    
    if ( (unsigned int) hModuloBuffer->numVal > hModuloBuffer->size){
      CommonExit(1,"modulo buffer overflow");
    }

    lim2 = n - lim1;

    if (lim2 > 0) {
      copyFLOAT(values+lim1,
                hModuloBuffer->paBuffer+hModuloBuffer->current_position,
                1,
                1,
                lim2);
      hModuloBuffer->current_position=(hModuloBuffer->current_position+lim2)%hModuloBuffer->size;
    }
    return(TRUE);
}



/*****************************************************************************

    functionname: ZeroFloatModuloBuffer
    description:  set n elements of circular buffer to zero
    returns:      TRUE
    input:        Handle, number of elements to zero out
    output:       

*****************************************************************************/
unsigned char ZeroFloatModuloBuffer(HANDLE_MODULO_BUF_VM hModuloBuffer, unsigned int n){
    unsigned int lim1, lim2;
    float fzero=0.0f;
    assert(n <= hModuloBuffer->size);

    lim1 = min(hModuloBuffer->current_position+n, hModuloBuffer->size) - hModuloBuffer->current_position;
    copyFLOAT(&fzero, &hModuloBuffer->paBuffer[hModuloBuffer->current_position],
              0,
              1,
              lim1);

    hModuloBuffer->current_position=(hModuloBuffer->current_position+lim1)%hModuloBuffer->size;

    lim2 = n - lim1;
    if (lim2 > 0) {
      copyFLOAT(&fzero, &hModuloBuffer->paBuffer[hModuloBuffer->current_position], 
                0,
                1,
                lim2);

      hModuloBuffer->current_position = (hModuloBuffer->current_position+lim2)%hModuloBuffer->size;
    }
    return(TRUE);
}



/*****************************************************************************

    functionname: GetFloatModuloBufferValues  
    description:  checks out circular buffer values
    returns:      TRUE
    input:        Handle, number of values to retrieve, logical position in 
                  circular buffer
    output:       values retrieved

*****************************************************************************/
unsigned char GetFloatModuloBufferValues(HANDLE_MODULO_BUF_VM hModuloBuffer, float *values, unsigned int n, unsigned int age){
    unsigned int lim1, lim2;
    unsigned int iReqPosition;

    assert(n <= hModuloBuffer->size);
    assert(age <= hModuloBuffer->size);
    assert(n <= age);

    iReqPosition = (hModuloBuffer->size + hModuloBuffer->current_position - age) % hModuloBuffer->size;

    lim1 = min(iReqPosition + n, hModuloBuffer->size) - iReqPosition;
    copyFLOAT(hModuloBuffer->paBuffer+iReqPosition, values, 1,1,lim1);

    iReqPosition = (iReqPosition + lim1) % hModuloBuffer->size;

    lim2 = n - lim1;

    if (lim2 > 0) 
      copyFLOAT(hModuloBuffer->paBuffer+iReqPosition, values+lim1, 1,1,lim2);
    
    return(TRUE);
}

/*****************************************************************************

    functionname: ReadFloatModuloBufferValues  
    description:  checks out circular buffer values from read pointer
    returns:      TRUE
    input:        Handle, number of values to retrieve
    output:       values retrieved

*****************************************************************************/
unsigned char ReadFloatModuloBufferValues(HANDLE_MODULO_BUF_VM hModuloBuffer, float *values, unsigned int n){
  unsigned int lim1, lim2;
  unsigned int iReqPosition;

  assert(n <= hModuloBuffer->size);
  
  iReqPosition = hModuloBuffer->readPosition;

  lim1 = min(iReqPosition + n, hModuloBuffer->size) - iReqPosition;
  copyFLOAT(hModuloBuffer->paBuffer+iReqPosition, values, 1,1,lim1);

  hModuloBuffer->numVal -= n;
  if (hModuloBuffer->numVal < 0 ){
    CommonExit(1,"modulo buffer underrun");
  }

  
  lim2 = n - lim1;
  
  if (lim2 > 0) {
    copyFLOAT(hModuloBuffer->paBuffer,
              values+lim1,
              1,
              1,
              lim2);

    hModuloBuffer->readPosition = lim2;

  } else {   
    hModuloBuffer->readPosition = iReqPosition+lim1;
  }

  return(TRUE);
}

int GetNumVal ( HANDLE_MODULO_BUF_VM hModuloBuffer )
{
  return ( hModuloBuffer->numVal );
}
