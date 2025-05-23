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

This software was modified by Y.B. Thomas Kim on 1997-11-06
 *                                                                           *
 ****************************************************************************/

#ifndef	_sam_tns_h_
#define	_sam_tns_h_

#define TNS_MAX_BANDS	49
#define TNS_MAX_ORDER	31
#define	TNS_MAX_WIN	8
#define TNS_MAX_FILT	3
#define C_PI               3.14159265358979323846

typedef struct
{
    int start_band;
    int stop_band;
	int length;
    int order;
    int direction;
    int coef_compress;
    short coef[TNS_MAX_ORDER];
    short coef1[TNS_MAX_ORDER];
} sam_TNSfilt;

typedef struct 
{
    int n_filt;
    int coef_res;
    sam_TNSfilt filt[TNS_MAX_FILT];
} sam_TNSinfo;

typedef struct 
{
    int n_subblocks;
	int tns_data_present;
    sam_TNSinfo info[TNS_MAX_WIN];
} sam_TNS_frame_info;

#endif	/* _sam_tns_h_ */
