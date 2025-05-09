/*******************************************************************
This software module was originally developed by

Yoshiaki Oikawa (Sony Corporation) and
Mitsuyuki Hatanaka (Sony Corporation)

and edited by


in the course of development of the MPEG-2 AAC/MPEG-4 System/MPEG-4
Video/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3. This
software module is an implementation of a part of one or more MPEG-2
AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio tools as specified by the
MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio standard. ISO/IEC
gives users of the MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio
standards free license to this software module or modifications
thereof for use in hardware or software products claiming conformance
to the MPEG-2 AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 System/MPEG-4 Video/MPEG-4 Audio conforming products.The
original developer retains full right to use the code for his/her own
purpose, assign or donate the code to a third party and to inhibit
third party from using the code for non MPEG-2 AAC/MPEG-4
System/MPEG-4 Video/MPEG-4 Audio conforming products. This copyright
notice must be included in all copies or derivative works.

Copyright (C) 1996.
*******************************************************************/

#include <stdio.h>
#include <math.h>

#include "allHandles.h"

#include "sony_local.h"

#ifndef PI
#define	PI	(3.14159265358979)
#endif


/*******************************************************************************
	Set Prototype Filter of PQF

	p_proto[96] should have been secured.
*******************************************************************************/
void	son_set_protopqf(
	SCALAR	*p_proto
	)
{
		int	j;
	static	SCALAR	a_half[48] =
{
   1.2206911375946939E-05,  1.7261986723798209E-05,  1.2300093657077942E-05,
  -1.0833943097791965E-05, -5.7772498639901686E-05, -1.2764767618947719E-04,
  -2.0965186675013334E-04, -2.8166673689263850E-04, -3.1234860429017460E-04,
  -2.6738519958452353E-04, -1.1949424681824722E-04,  1.3965139412648678E-04,
   4.8864136409185725E-04,  8.7044629275148344E-04,  1.1949430269934793E-03,
   1.3519708175026700E-03,  1.2346314373964412E-03,  7.6953209114159191E-04,
  -5.2242432579537141E-05, -1.1516092887213454E-03, -2.3538469841711277E-03,
  -3.4033123072127277E-03, -4.0028551071986133E-03, -3.8745415659693259E-03,
  -2.8321073426874310E-03, -8.5038892323704195E-04,  1.8856751185350931E-03,
   4.9688741735340923E-03,  7.8056704536795926E-03,  9.7027909685901654E-03,
   9.9960423120166159E-03,  8.2019366335594487E-03,  4.1642072876103365E-03,
  -1.8364453822737758E-03, -9.0384863094167686E-03, -1.6241528177129844E-02,
  -2.1939551286300665E-02, -2.4533179947088161E-02, -2.2591663337768787E-02,
  -1.5122066420044672E-02, -1.7971713448186293E-03,  1.6903413428575379E-02,
   3.9672315874127042E-02,  6.4487527248102796E-02,  8.8850025474701726E-02,
   0.1101132906105560    ,  0.1258540205143761    ,  0.1342239368467012    
};

	for (j = 0; j < 48; ++j) {
		p_proto[j] = p_proto[95-j] = a_half[j];
	}
}
