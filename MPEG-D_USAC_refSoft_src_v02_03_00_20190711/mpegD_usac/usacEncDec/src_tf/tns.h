/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of 
development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
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
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

#ifndef	_tns_h_
#define	_tns_h_

#include "tf_mainHandle.h"

#define TNS_MAX_BANDS	49
#define TNS_MAX_ORDER	31
#define	TNS_MAX_WIN	8
#define TNS_MAX_FILT	3

typedef struct
{
    int start_band;
    int stop_band;
    int order;
    int direction;
    int coef_compress;
    short coef[TNS_MAX_ORDER];
} TNSfilt;

typedef struct 
{
    int n_filt;
    int coef_res;
    TNSfilt filt[TNS_MAX_FILT];
} TNSinfo;

typedef struct 
{
    int n_subblocks;
    TNSinfo info[TNS_MAX_WIN];
  int tnsChanMonoLayFromRight;
} TNS_frame_info;

#ifndef HAS_Float
typedef    double   Float;
#define HAS_Float
#endif

TNS_COMPLEXITY get_tns_complexity(QC_MOD_SELECT qc_select);

void tns_encode_subblock ( double*        spec, 
                           int           nbands, 
                           const short*  sfb_top, 
                           int           islong,
                           TNSinfo*      tns_info, 
                           QC_MOD_SELECT qc_select ,
                           int           samplRateIdx);



#endif	/* _tns_h_ */
