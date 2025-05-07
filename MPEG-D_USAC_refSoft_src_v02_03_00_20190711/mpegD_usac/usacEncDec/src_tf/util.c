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

#include <stdlib.h>
#include "all.h"
#include "util.h"
#include "common_m4a.h"

void
specFilter (
	    double p_in[],
	    double p_out[],
	    int  samp_rate,
	    float lowpass_freq,
	    int    slope,
	    int    specLen
	    )
{  
  float lowpass;
  int    xlowpass,i;

   /* calculate the last line which is not zero */
   lowpass = (float) ((double)lowpass_freq * (double)specLen);
   lowpass /= (samp_rate>>1);
   lowpass += 1.0;             /* round up to ensure that the desired upper frequency limit is correct */
   xlowpass = ((int)lowpass < specLen) ? (int)lowpass : specLen ;

   if( p_out != p_in ) { 
      for (i = 0; i < specLen; i++ ) {
	p_out[i] = p_in[i];
      } 
   } 
   for (i = xlowpass; i <specLen ; i++ ) {
      p_out[i]  = 0;
   } 

   if( slope != 0 ) 
     CommonWarning("\nunsupported slope type in specFilter\n" );

}
