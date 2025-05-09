/**********************************************************************
 
This software module was originally developed by
and edited by Texas Instruments in the course of 
development of the MPEG-2 AAC/MPEG-4 Audio standard
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
**********************************************************************/
/*********************************************************************
 *
 * tns.h  -  AAC temporal noise shaping
 *
 * Authors:
 * CL    Chuck Lueck, TI <lueck@ti.com>
 * OK    Olaf Kaehler, Fraunhofer IIS-A <kaehleof@iis.fhg.de>
 *
 * Changes:
 * 20-oct-97   CL   Initial revision.
 * 26-oct-03   OK   simplify complexity switch in TnsInit
 *
**********************************************************************/
 
#ifndef _TNS_H_INCLUDED
#define _TNS_H_INCLUDED

#include "interface.h"
#include "tf_mainHandle.h"
 
/*************************/
/* #defines              */
/*************************/
#define TNS_MAX_ORDER 31				   
#define DEF_TNS_GAIN_THRESH 1.4
#define DEF_TNS_COEFF_THRESH 0.1
#define DEF_TNS_COEFF_RES 4
#define DEF_TNS_RES_OFFSET 3

#ifndef PI                      /* as in stroustrup */
#define PI C_PI
#endif

/**************************/
/* Structure definitions  */
/**************************/
typedef struct {
  int order;                           /* Filter order */
  int direction;		       /* Filtering direction */
  int coefCompress;		       /* Are coeffs compressed? */
  int length;                          /* Length, in bands */                     
  double aCoeffs[TNS_MAX_ORDER+1];     /* AR Coefficients */
  double kCoeffs[TNS_MAX_ORDER+1];     /* Reflection Coefficients */
  int index[TNS_MAX_ORDER+1];	       /* Coefficient indices */
} TNS_FILTER_DATA;


typedef struct {
  int numFilters;				/* Number of filters */
  int coefResolution;				/* Coefficient resolution */
  TNS_FILTER_DATA tnsFilter[3];	                /* TNS filters */
} TNS_WINDOW_DATA;


typedef struct {
  int tnsDataPresent;
  int tnsMinBandNumberLong;
  int tnsMinBandNumberShort;
  int tnsMaxBandsLong;
  int tnsMaxBandsShort;
  int tnsMaxOrderLong;
  int tnsMaxOrderShort;
  TNS_WINDOW_DATA windowData[NSHORT];	/* TNS data per window */
} TNS_INFO;



/*************************/
/* Function prototypes   */
/*************************/
void Autocorrelation(int maxOrder,        /* Maximum autocorr order */
		     int dataSize,		  /* Size of the data array */
		     double* data,		  /* Data array */
		     double* rArray);	  /* Autocorrelation array */

double LevinsonDurbin(int maxOrder,       /* Maximum filter order */
		      int dataSize,		   /* Size of the data array */
		      double* data,		   /* Data array */
		      double* kArray);	   /* Reflection coeff array */

void StepUp(int fOrder,
	    double* kArray,
	    double* aArray);

void QuantizeReflectionCoeffs(int fOrder,int coeffRes,double* rArray,int* indexArray);
int TruncateCoeffs(int fOrder,double threshold,double* kArray);
void TnsFilter(int length,double* spec,TNS_FILTER_DATA* filter);
void TnsInvFilter(int length,double* spec,TNS_FILTER_DATA* filter);
int TnsInit(long samplingRate,TNS_COMPLEXITY profile,TNS_INFO* tnsInfo); 
void TnsEncode(int numberOfBands,int maxSfb,WINDOW_SEQUENCE blockType,int* sfbOffsetTable,double* spec,TNS_INFO* tnsInfo);
void TnsEncode2(int numberOfBands,int maxSfb,WINDOW_SEQUENCE blockType,int* sfbOffsetTable,double* spec,TNS_INFO* tnsInfo, int decodeFlag);
void TnsEncodeShortBlock(int numberOfBands, int maxSfb, int which_subblock, int* sfbOffsetTable, double* spec, TNS_INFO* tnsInfo);

#endif






