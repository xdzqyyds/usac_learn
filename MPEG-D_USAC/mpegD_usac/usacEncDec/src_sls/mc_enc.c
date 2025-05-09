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
 * Multichannel allocation module.
 *
 * Authors:
 * CL    Chuck Lueck, TI
 * RG    Ralf Geiger, FhG/IIS
 * MCF   Matt Fellers, Dolby Laboratories
 *
 * Changes:
 * 22-jan-98   CL   Initial revision.
 * 07-apr-98   RG   Added determination of LFE channel.
 * 26-may-98   MCF  Added support for independent coupling channel
 ***************************************************************************************/

#include "mc_enc.h"
#include "common_m4a.h"

void DetermineChInfo(Ch_Info* chInfo, int numChannels, int cplenabl, int lfePresent) {

  /* This shows number of channels and channel configurations for different			*/
  /* combinations of coupling and LFE channels.  This chart assumes only 1 LFE and 	*/
  /* coupling channel is supported in this encoder.									*/

  /* LFE, no coupling present														*/
  /*  Total num channels # of SCE's       # of CPE's    #of LFE's	#of CCE's	*/ 
  /*  ================== ==========       ==========    =========	 =======	*/
  /*      1                  1                0             0			0		*/
  /*      2                  0                1             0			0		*/
  /*      3                  1                1             0			0		*/
  /*      4                  1                1             1			0		*/
  /*      5                  1                2             0			0		*/
  /* For more than 5 channels, use the following elements:						*/
  /*      2*N                1                2*(N-1)       1			0		*/
  /*      2*N+1              1                2*N           0			0		*/
  /*																			*/
  /* No LFE, no coupling present												*/
  /*																			*/  
  /*  Total num channels # of SCE's       # of CPE's    #of LFE's	 #of CCE's  */
  /*  ================== ==========       ==========	========= 	 =======	*/
  /*      1                  1                0				0			0		*/
  /*      2                  0                1				0			0		*/
  /*      3                  1                1				0			0		*/
  /*      4                  2                1				0			0		*/
  /*      5                  1                2				0			0		*/
  /* For more than 5 channels, use the following elements:						*/
  /*      2*N                2                2*(N-1)		0			0		*/
  /*      2*N+1              1                2*N			0			0		*/

  /* LFE, coupling present														*/
  /*  Total num channels # of SCE's       # of CPE's    #of LFE's	 #of CCE's	*/ 
  /*  ================== ==========       ==========    =========    =======	*/
  /*      1                  1                0             0			 0		*/
  /*	  2					 1				  0				0			 1 (ind)*/
  /*      3                  1                0             1			 1 (ind)*/
  /*      4                  0                1             1			 1		*/
  /*      5                  1                1             1            1		*/
  /*      6                  0                2				1			 1		*/
  /* For more than 6 channels, use the following elements:						*/
  /*      2*N+1 (N>=2)		 1                N				 1			 1		*/
  /*      2*N+2 (N>=3)		 0                N				 1			 1		*/

  /* no LFE, coupling present													*/
  /*																			*/  
  /*  Total num channels # of SCE's       # of CPE's     #of LFE	  #of CCE	*/
  /*  ================== ==========       ==========     =========    =======	*/
  /*      1                  1                0				 0			 0		*/
  /*	  2					 1				  0				 0			 1 (ind)*/
  /*      3                  0                1				 0			 1		*/
  /*      4                  1                1				 0			 1		*/
  /*      5                  0                2				 0			 1		*/
  /*      6                  1                2				 0			 1		*/
  /* For more than 6 channels, use the following elements:					 	*/
  /*      2*N+1 (N>=3)		 0                N				 0			 1		*/
  /*      2*N+2 (N>=3)		 1                N				 0			 1		*/

  int sceTag=0;
  int cpeTag=0;
  int lfeTag=0;
  int numChannelsLeft;
  int cceTag = 0;

  numChannelsLeft=numChannels;

  if (cplenabl)
  {

  /* Last element is coupling channel if coupling enabled */
    chInfo[numChannelsLeft-1].present = 1;
    chInfo[numChannelsLeft-1].tag=cceTag++;
    chInfo[numChannelsLeft-1].cpe=0;
    chInfo[numChannelsLeft-1].cch[0]=0;	 /* coupling channel index */
    chInfo[numChannelsLeft-1].cc_dom[0]=1; /* coupling decoding is performed after TNS decoding*/
    chInfo[numChannelsLeft-1].cc_ind[0]=1;/* coupling channel is independently switched */
    /* chInfo[numChannels-numChannelsLeft].lfe=0;     */
    numChannelsLeft--;
    numChannels--;
  }

/* First element is sce, except for 2 channel case */
  if (numChannelsLeft!=2) 
  {
    chInfo[numChannels-numChannelsLeft].present = 1;
    chInfo[numChannels-numChannelsLeft].ncch = cplenabl;
    chInfo[numChannels-numChannelsLeft].tag=sceTag++;
    chInfo[numChannels-numChannelsLeft].cpe=0;
    /* chInfo[numChannels-numChannelsLeft].lfe=0; */
    numChannelsLeft--;
  }

  /* Next elements are cpe's */
  while (numChannelsLeft>1) {
    /* Left channel info */
    chInfo[numChannels-numChannelsLeft].present = 1;
    chInfo[numChannels-numChannelsLeft].ncch = cplenabl;
    chInfo[numChannels-numChannelsLeft].tag=cpeTag++;
    chInfo[numChannels-numChannelsLeft].cpe=1;
    chInfo[numChannels-numChannelsLeft].common_window=0;
    chInfo[numChannels-numChannelsLeft].ch_is_left=1;
    chInfo[numChannels-numChannelsLeft].paired_ch=numChannels-numChannelsLeft+1;
    /* chInfo[numChannels-numChannelsLeft].lfe=0;     */
    numChannelsLeft--;
     
    /* Right channel info */
    chInfo[numChannels-numChannelsLeft].present = 1;
    chInfo[numChannels-numChannelsLeft].ncch = cplenabl;
    chInfo[numChannels-numChannelsLeft].cpe=1;
    chInfo[numChannels-numChannelsLeft].common_window=0;
    chInfo[numChannels-numChannelsLeft].ch_is_left=0;
    chInfo[numChannels-numChannelsLeft].paired_ch=numChannels-numChannelsLeft-1;
    /* chInfo[numChannels-numChannelsLeft].lfe=0;     */
    numChannelsLeft--;
  }
  
  /* Is there another channel left ?  One more sce. */
  if (numChannelsLeft) {
    if (lfePresent) { 
      chInfo[numChannels-numChannelsLeft].present = 1;
      chInfo[numChannels-numChannelsLeft].tag=lfeTag++;
      chInfo[numChannels-numChannelsLeft].cpe=0;
      /* chInfo[numChannels-numChannelsLeft].lfe=1;  */
    } else {
      chInfo[numChannels-numChannelsLeft].present = 1;
      chInfo[numChannels-numChannelsLeft].tag=sceTag++;
      chInfo[numChannels-numChannelsLeft].cpe=0;
      /* chInfo[numChannels-numChannelsLeft].lfe=0; */
    }    
    numChannelsLeft--;
  } else {
    if (lfePresent) CommonExit(1,"no LFE channel detected");
  }

}



