/**********************************************************************
MPEG-4 Audio VM Module
parameter based codec - FFT and ODFT module by UHD



This software module was originally developed by

Bernd Edler (University of Hannover / Deutsche Telekom Berkom)
Heiko Purnhagen (University of Hannover / Deutsche Telekom Berkom)

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



Header file: uhd_fft.h

 

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BE    Bernd Edler, Uni Hannover <edler@tnt.uni-hannover.de>

Changes:
11-sep-96   HP    adapted existing code
11-feb-97   BE    optimised odft/fft
12-feb-97   BE    optimised ifft
26-jan-01   BE    float -> double
**********************************************************************/

#ifndef _uhd_fft_h_
#define _uhd_fft_h_


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif

void UHD_fft(double xr[], double xi[], int n);
void UHD_ifft(double xr[], double xi[], int n);

void UHD_odft(double xr[], double xi[], int n);
void UHD_iodft(double xr[], double xi[], int n);

#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _uhd_fft_h_ */

/* end of uhd_fft.h */
