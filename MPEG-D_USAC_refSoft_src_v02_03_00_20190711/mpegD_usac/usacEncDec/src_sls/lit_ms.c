/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed in the course of 
development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 NBC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

/*******************************************************************************************
 *
 * MS stereo coding module
 *
 * Authors:
 * CL    Chuck Lueck, TI <lueck@ti.com>
 * RSY   Rongshan Yu, I2R
 *
 * Changes:
 * 22-jan-98   CL   Initial revision.  Lacks psychoacoustics for MS stereo.
 * 06-sep-98   CL   Disable MS stereo for bands which use PNS (MPEG4 only).
 * Oct-2003	   RSY  Modified for Scalable Lossless coding 
 ***************************************************************************************/

#ifdef I2R_LOSSLESS 
#include "lit_ms.h"
#include "aac_qc.h"


/*ms rotating*/
static double a1=-0.41421356,b1=0.70710678;

#define ROUND(x) ((x>=0)?(int)(x+0.5):(int)(x-0.5))

#define GIVEN(x1,x2,y1,y2,c,s) { y1 = x1 + ROUND(c*x2); y2 = x2;\
								y2 = ROUND(s*y1) + y2;\
								y1 = y1 + ROUND(c*y2); }


/* Perform MS encoding.  Spectrum is non-interleaved.  */
/* This would be a lot simpler on interleaved spectral data */
void lit_MSEncode(int *spectrum[MAX_TIME_CHANNELS],   /* array of pointers to spectral data */
                  Ch_Info *channelInfo,                  /* Pointer to Ch_Info */
		  MSInfo      *msInfo,
                  int sfb_offset_table[][MAX_SCFAC_BANDS+1],
                  WINDOW_TYPE block_type[MAX_TIME_CHANNELS], /* Block type */
                  AACQuantInfo* quantInfo,               /* Quant info */
                  int numberOfChannels,               /* Number of channels */
                  short msenable) {              

/***** heavily hacked to get long blocks working 1st ****/


  int chanNum;
  int sfbNum;
  int lineNum;
  int sum,diff;

  /* Look for channel_pair_elements */
  for (chanNum=0;chanNum<numberOfChannels;chanNum++) {
    if (channelInfo[chanNum].present) {
      if ((channelInfo[chanNum].cpe)&&(channelInfo[chanNum].ch_is_left)) {
	int leftChan=chanNum;
        int rightChan=channelInfo[chanNum].paired_ch;
	msInfo->ms_mask=0;

	/* Perform MS if block_types are the same */
	if ((block_type[leftChan]==block_type[rightChan])&&(msenable)) { 
	  int numGroups;
    int maxSfb;
    int totalSfb;
	  int g,b,w,line_offset;
	  int startWindow,stopWindow;

	  channelInfo[leftChan].common_window = 1;  /* Use common window */
	  /*channelInfo[leftChan].ms_info.is_present*/msInfo->ms_mask=2; /* all on*/

	  numGroups = 1;/* quantInfo[leftChan].num_window_groups; */

          maxSfb = 49;/* quantInfo[leftChan].max_sfb; */
		  totalSfb = 49;/* quantInfo[leftChan].total_sfb; */
	  w=0;

	  /* Determine which bands should be enabled */
	  /* Right now, simply enable bands which are below the pns start band */
	  /* and which do not use intensity stereo */
          /* isInfo = &(channelInfo[rightChan].is_info); */
          /* msInfo = &(channelInfo[leftChan].ms_info); */
	    for (g=0;g<numGroups;g++) {
	      for (sfbNum=0;sfbNum</*maxSfb*/totalSfb;sfbNum++) {
		b = g*maxSfb+sfbNum;
		msInfo->ms_used/***/[0][b] = 1; /*( (!isInfo->is_used[b])||(!isInfo->is_present) );*/ 
	      }
	    }
 
	  /* Perform sum and differencing on bands in which ms_used flag */
          /* has been set. */
	  line_offset=0;
	  startWindow = 0;
	  for (g=0;g<numGroups;g++) {
	    int numWindows = quantInfo[leftChan].window_group_length[g];
	    stopWindow = startWindow + numWindows;
	    for (sfbNum=0;sfbNum<totalSfb;sfbNum++) {
	      /* Enable MS mask */
			if (0 /* all on at this RM, done in stereo IntMDCT */) { 
		for (w=startWindow;w<stopWindow;w++) {
		  for (lineNum=sfb_offset_table[leftChan][sfbNum];
		       lineNum<sfb_offset_table[leftChan][sfbNum+1];
		       lineNum++) {
		    line_offset = w*BLOCK_LEN_SHORT;
			GIVEN(spectrum[leftChan][line_offset+lineNum],\
				spectrum[rightChan][line_offset+lineNum],diff,sum,\
				a1,b1);
			spectrum[leftChan][line_offset+lineNum] = sum;
			spectrum[rightChan][line_offset+lineNum] = diff;

		  }  /* for (lineNum... */
		}  /* for (w=... */
	      }
	    }  /* for (sfbNum... */
	    startWindow = stopWindow;
	  } /* for (g... */
	}  /* if (block_type... */
      }
    }  /* if (channelInfo[chanNum].present */
  }  /* for (chanNum... */
}


#endif


