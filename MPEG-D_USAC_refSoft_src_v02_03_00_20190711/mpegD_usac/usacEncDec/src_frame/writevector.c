/**********************************************************************
MPEG-4 Audio VM

debugging - write vector(s) to matlab/octave/plotmtv file



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

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

Copyright (c) 1996.



Source file: writevector.c

 

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
22-may-97   HP    first version
		  (see also plotmtv_interf.c by Bodo Teichmann)
**********************************************************************/

/*
  In the debugger (gdb), type e.g.
    call WriteVector("filename","os",len,v1,v2)
  to create an octave file "filename" with a 2 x len matrix of the short
  vectors v1 (1st column) and v2 (2nd column).

  Parameters: WriteVector(fn,opt,len,v1,v2)

  fn:  file name
  opt: option string: 1st char: m=matlab, o=octave, p=plotmtv
                      2nd char: f=float, d=double, s=short, i=int, l=long
		                F=abs(float[,float]), D=abs(double[,double])
				F,D always create 1 x len matrix
  len: vector dimension
  v1:  1st vector
  v2:  2nd vector (or NULL for 1 x len matrix)
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#define SQR(a) ((a)*(a))


int WriteVector(char *fn, char *opt, int len, void *v1, void *v2)
{
  FILE *fp;
  int r,c,col;
  void *v[2];

  if (!(fp=fopen(fn,"w"))) {
    fprintf(stderr,"WriteVector error (open)\n");
    return 1;
  }
  if (!opt[0] || !opt[1]) {
    fprintf(stderr,"WriteVector error (opt)\n");
    return 1;
  }
  col = v2?2:1;
  if (opt[1]=='F' || opt[1]=='D')
    col = 1;
  switch (opt[0]) {
  case 'm':
    break;
  case 'o': 
    fprintf(fp,"# name: %s\n",fn);
    fprintf(fp,"# type: matrix\n");
    fprintf(fp,"# rows: %d\n",len);
    fprintf(fp,"# columns: %d\n",col);
    break;
  case 'p':
    fprintf(fp,"$ DATA=COLUMN\n");
    fprintf(fp,"%% toplabel = \"%s\"\n",fn);
    break;
  default:
    fprintf(stderr,"WriteVector error (1st)\n");
    return 1;
  }
  for (c=0; c<col; c++)
    v[c] = c?v2:v1;
  for (r=0; r<len; r++) {
    if (opt[0]=='p')
      fprintf(fp,"%d ",r);
    for (c=0; c<col; c++) {
      switch (opt[1]) {
      case 's':
	fprintf(fp,"%ld ",(long)((short*)v[c])[r]);
	break;
      case 'i':
	fprintf(fp,"%ld ",(long)((int*)v[c])[r]);
	break;
      case 'l':
	fprintf(fp,"%ld ",(long)((long*)v[c])[r]);
	break;
      case 'f':
	fprintf(fp,"%e ",((float*)v[c])[r]);
	break;
      case 'd':
	fprintf(fp,"%e ",((double*)v[c])[r]);
	break;
      case 'F':
	fprintf(fp,"%e ",(col==1)?fabs(((float*)v[c])[r]):
		sqrt(SQR(((float*)v[c])[r])+SQR(((float*)v[c+1])[r])));
	if (col>1)
	  c++;
	break;
      case 'D':
	fprintf(fp,"%e ",(col==1)?fabs(((double*)v[c])[r]):
		sqrt(SQR(((double*)v[c])[r])+SQR(((double*)v[c+1])[r])));
	if (col>1)
	  c++;
	break;
      default:
	fprintf(stderr,"WriteVector error (2nd)\n");
	return 1;
      }
    }
    fprintf(fp," \n");
  }
  fclose(fp);
  return 0;
}

/* end of writevector.c */

