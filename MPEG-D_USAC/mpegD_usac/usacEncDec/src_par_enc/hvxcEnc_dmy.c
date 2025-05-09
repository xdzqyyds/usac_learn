
/*



This software module was originally developed by

Masayuki Nishiguchi, Kazuyuki Iijima, Jun Matsumoto (Sony Corporation)
Heiko Purnhagen (University of Hannover)

and edited by

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


                                                                  
*/


/**********************************************************************
Dummy version of hvxcEnc.c

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
06-feb-97   HP    born (based on hvxcEnc.c)
09-may-97   HP    updated
11-nov-97   HP    updated judge
12-nov-97   HP    updated IPC_HVXCFree()
**********************************************************************/

#include <math.h>
/* #include <values.h> */
#include <stdio.h>
/* #include <sys/file.h> */
#include <signal.h>

#include "hvxc.h"
#include "common_m4a.h"
#include "hvxcCommon.h"
#include "hvxcEnc.h"

int judge = 0;

void IPC_HVXCInit(void)
{
  CommonExit(1,"IPC_HVXCInit: dummy");
}

void IPC_HVXCEncParFrm(
short int	*frmBuf,
IdLsp		*idLsp,
int		*idVUV,
IdCelp		*idCelp,
float		*mfdpch,
IdAm		*idAm)
{
  CommonExit(1,"IPC_HVXCEncParFrm: dummy");
}

void IPC_PackPrm2Bit(
IdLsp		*idLsp,
int		idVUV,
IdCelp		*idCelp,
float		pitch,
IdAm		*idAm,
unsigned char	*encBit)
{
  CommonExit(1,"IPC_PackPrm2Bit: dummy");
}

void IPC_HVXCFree(void)
{
  CommonExit(1,"IPC_HVXCFree: dummy");
}


/* end of hvxcEnc_dmy.c */
