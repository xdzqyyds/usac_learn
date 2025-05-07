/*******************************************************************************
This software module was originally developed by

Orange Labs.

Initial author:
Pierrick Philippe		(Orange Labs),

and edited by:

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Assurance that the originally developed software module can be used (1) in
ISO/IEC 23003 once ISO/IEC 23003 has been adopted; and (2) to develop ISO/IEC
23003:s
Orange Labs grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Orange Labs
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Orange Labs 
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Orange Labs retains full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2009.
$Id: usac_nf.c,v 1.2 2010-09-13 20:29:22 mul Exp $
*******************************************************************************/
#include "usac_nf.h"
#include <string.h>
 

/* correct energy estimate in order to avoid noise level over estimation due to strong tonal components */
static void	limiter(
                        double *E,				/* in/out */
                        double *spectrum, 
                        int *quant, int NN, 
                        int *sfb_offset, 
                        int sb, 
                        int cntr
                        )
{
  int	n,i;
	
  double *highest = 0 ;
  double	temp;
	
  if(!NN)	return;
  if( cntr <=  NN ) return;
	
  highest = (double*) malloc( NN * sizeof(*highest));

  memset(highest , 0 , NN * sizeof(*highest) );


  /* finds the NN strongest bins */
  for (i=sfb_offset[sb]; i<sfb_offset[sb+1]; i++)
    {
      if ( !quant[i] )
        {
          temp = spectrum[i] * spectrum[i];
			
          for (n=0; n<NN;n++)
            {
              if( temp > highest[n] )
                {
                  memmove(highest+1+n,highest+n,(NN-n-1)*sizeof(*highest));
                  highest[n] = temp;
                  break;
                }
            }
        }
    }
  /* remove the contribution of the highest components */
  for (n=0; n<NN;n++)
    *E -= highest[n] ;
	
  /* add the average component energy */
  *E += NN * (*E) /(cntr-NN);
	
  free(highest);
}


/* determine the level of noise filling based on the unquantized spectral lines power */
void	usac_noise_filling(
                           UsacToolsInfo *tool_data,		/* out */
                           double *spectrum, 
                           UsacQuantInfo   *qInfo, 
                           int *sfb_offset, 
                           int max_sfb
                           )
{
  double	E;
  double	nLevel;
  double	nOffset;

  double	sumOn, sumOff;
  double	eOn,eOff;
	
  int		cntr;
  int		sb , i;
	
  const double alpha = 0.5;
	
  eOn  = 0.;	
  eOff = 0.;
	
  sumOn  = 0;
  sumOff = 0;
	
  /* noise filling is only allowed above bin 160 */
  for (sb=0; sb<max_sfb; sb++)
    {
      if( sfb_offset[sb+1] > 160 )
        break;
    }
	
  /* estimation from this sb */
  for ( ; sb<max_sfb; sb++)
    {
      cntr = 0;
      E = 0;
      /* check whether that signal contains some quantized spectral lines */
      for (i=sfb_offset[sb]; i<sfb_offset[sb+1]; i++)
        {
          if ( !qInfo->quant[i] )
            {
              E += spectrum[i] * spectrum[i];
              cntr ++;
            }
        }
		
      /* optional step: remove highest (tonal) contributions */
      limiter(&E, spectrum, qInfo->quant, cntr/4, sfb_offset, sb, cntr);

      /* this subband contains some quantized signal */
      if ( cntr != (sfb_offset[sb+1] - sfb_offset[sb] )) /* quantized */
        {
          /* this subband is quantized */
          eOn += E;
          sumOn  += pow ( 2., 0.5 * qInfo->scale_factor[sb] -50 ) * cntr;
        }
      else
        /* this subband is silent */
        {
          eOff += E;
          sumOff += pow ( 2., 0.5 * qInfo->scale_factor[sb] -58 ) * (sfb_offset[sb+1] - sfb_offset[sb]);
        }
		
    }
	
  tool_data -> noiseOffset = 0;
  tool_data -> noiseLevel = 0;	/* default values: no noise is injected */
	
	
  if( sumOn )	/* noise level correction for allocated subbands */
    {
		
      nLevel =  1.5 * ( log ( alpha  * eOn ) - log(sumOn) ) / log(2.) + 14. ;
		
      tool_data -> noiseLevel = (int) ( nLevel + 0.5 );
		
      tool_data -> noiseLevel = max ( tool_data -> noiseLevel , 0 );
      tool_data -> noiseLevel = min ( tool_data -> noiseLevel , 7 );
		
      if ( tool_data -> noiseLevel * sumOff )  /* level correction for allocated subbands */
        {
          nOffset = 2. *  log ( eOff * sumOn / sumOff /eOn ) / log (2.);
			
          tool_data -> noiseOffset= (int) ( nOffset + 0.5 );
          tool_data -> noiseOffset = min (tool_data -> noiseOffset , 31); 
          tool_data -> noiseOffset = max (tool_data -> noiseOffset , 0);
        }
    }

}
