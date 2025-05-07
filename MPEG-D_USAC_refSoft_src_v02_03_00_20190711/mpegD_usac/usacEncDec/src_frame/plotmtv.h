/**********************************************************************
MPEG-4 Audio VM


This software module was originally developed by

Bodo Teichmann Fraunhofer Institute of Erlangen tmn@iis.fhg.de

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



Header file: plotmtv.h

 

Authors:
tmn  Bodo Teichmann  tmn@iis.fhg.de
HP   Heiko Purnhagen  purnhage@tnt.uni-hannover.de

Changes:
xx-apr-97  BT   contributed to VM
22-may-97  HP   added abs(real,imag) MTV_CPLXFLOAT
                plotDirect("",MTV_CPLXFLOAT,npts,re1,im1,re2,im2)

**********************************************************************/


#ifndef _plotmtv_h
#define _plotmtv_h
/* plotmtv_interf.c is just a C interface to the freeware program plotmtv which  */
/* is a graphical data-display program for X-windows with a nice user interface */
/* with this C interface it can be use to display 1-dim arrays (eg. the mdct spectrum)  */
/* directly from the debugger while debugging with 'call plotDirect("",MTV_DOUBLE,npts,array1,array2,array3,array4) */
/* is is verry verry usefull for debugging; please do not remove the files from the VM frame work  */
/* i will sent an executable (sgi,linux,solaris,sunos) to everybody who wants to use it */

enum DATA_TYPE {MTV_DOUBLE,MTV_FLOAT,MTV_ABSFLOAT,MTV_LONG,MTV_INT,MTV_SHORT,MTV_CPLXFLOAT,MTV_DOUBLE_SQA,MTV_INT_SQA};
int plotInit(int,char*,int);
int plotSend( char *legend, char *plotSet  , enum DATA_TYPE dtype,long npts             ,const void *dataVector, const void *dataVector2);
/*                          legend, subwindow name , data type           , number of data values, start adress of vector of data */
/* it is possible to plot different vectors with different legendnames into to the same graph(=subwindow)       */

void plotDisplay(int);

/* just for use in the debugger: type call plotDirect(...) in the GNUdebugger command window, 
   vector2,vector3 and vector4 might be the  NULL vector 
   and are ignored then .

   TO USE THIS FEATURE YOU NEED A PLOTMTV.RC FILE IN THE COURRENT WORK DIRECTORY,
   WHICH ENABLES THE PLOTSETS CALLED:   "direct1" and "direct2"

   in other words: plotmtv.rc should include  at least these lines :
   :direct1 
   %xlabel=""
   %ylabel=""
   %xlog=On
   %ylog=Off  
   %boundary=True
   EOF;
   :direct2 
   %xlabel=""
   %ylabel=""
   %xlog=On
   %ylog=Off
   %boundary=True
   EOF;
   # end of mplot.rc
   
   you can replace the words "True"(or "On")  with "False"(or "Off") and vice versa 
*/
extern void plotDirect(char *label,enum DATA_TYPE dtype,long npts,void *vector1,void *vector2,void *vector3,void *vector4  );

extern int plotChannel;

#endif
