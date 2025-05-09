/*****************************************************************************
 *                                                                           *
"This software module was originally developed by 

Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)

and edited by

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 
Audio standard. ISO/IEC  gives uers of the MPEG-2 AAC/MPEG-4 Audio 
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
Copyright(c)1998.
 *                                                                           *
 ****************************************************************************/
/******************** MPEG AAC Error Resilient Decoder ************************
 
    
   contents/description: handling of "Escape" structure
 
******************************************************************************/

#include <stdlib.h>
#include "escapeScfResil.h"

typedef struct tag_escape{
  int * pEscValues;
  int   lastValue;
  int   numOfValues;
}T_ESCAPE;



/******************************************

 CreateEscape

 reserve memory for the structure

******************************************/
Escape CreateEscape(
                    void
                    )
{
  Escape blue;

  blue = (Escape) malloc( sizeof(T_ESCAPE) );
  return blue;
}

/******************************************

 InitEscape

 initialize struct and assign an array

******************************************/
void
InitEscape(
           Escape escStruct,
           int *  escValArray
           )
{
  escStruct->pEscValues     = escValArray;
  escStruct->lastValue      = 0;
  escStruct->numOfValues    = 0;
}

/******************************************

 AddEscValue

 add a value to assigned array

******************************************/
void
AddEscValue(
            Escape escStruct,
            int    value
            )
{
  *escStruct->pEscValues = value;
  escStruct->pEscValues++;

  escStruct->lastValue = value;

  escStruct->numOfValues++;
}

/******************************************

 GetNumOfValues

******************************************/
int
GetNumOfValues(
               Escape escStruct
               )
{
  return escStruct->numOfValues;
}

/******************************************

 GetLastValue

******************************************/
int
GetLastValue(
             Escape escStruct
             )
{
  int value;

  value = escStruct->lastValue;
  escStruct->lastValue = 0;

  return value;
}

