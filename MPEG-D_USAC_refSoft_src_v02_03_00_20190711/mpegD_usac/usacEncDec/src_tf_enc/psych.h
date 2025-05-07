/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
 "This software module was originally developed by 
 Fraunhofer Gesellschaft IIS / University of Erlangen (UER) in the course of 
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

/* CREATED BY :  Bernhard Grill -- August-96  */

typedef struct { 
  long   sampling_rate;                   /* the following entries are for this sampling rate */
  int    num_cb_long;
  int    num_cb_short;
  const short* cb_offset_long;
  const short* cb_offset_short;
  double fixed_ratio_long[NSFB_LONG];
  double fixed_ratio_short[NSFB_SHORT];
  int    cb_width_long[NSFB_LONG];
  int    cb_width_short[NSFB_LONG];

} SR_INFO;

 
typedef struct {
  double *p_ratio;
  int    *cb_width;
  int    no_of_cb;
} CH_PSYCH_OUTPUT;

#ifdef __cplusplus
extern "C" {
#endif

void EncTf_psycho_acoustic_init( void );
int EncTf_psycho_acoustic ( double          sampling_rate,
                            int             no_of_chan,         
                            int             frameLength,
                            int             windowLength,
                            CH_PSYCH_OUTPUT p_chpo_long[],
                            CH_PSYCH_OUTPUT p_chpo_short[][MAX_SHORT_WINDOWS] );

#ifdef __cplusplus
}
#endif
