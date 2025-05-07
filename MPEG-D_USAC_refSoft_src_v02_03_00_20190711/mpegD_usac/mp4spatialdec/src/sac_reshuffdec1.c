/*******************************************************************************
This software module was originally developed by

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips

and edited by

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

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
*******************************************************************************/



#include "sac_resintrins.h"
#include "sac_config.h"
#include "sac_reshuffdec.h"
#include "sac_reshuffdec_intern.h"
#include "sac_bitinput.h"

#include <assert.h>

static int
getmask(Info *info, unsigned char* group, 
        unsigned char max_sfb, 
        unsigned char* mask)
{
  int b, i, mp;

  mp = s_getbits(LEN_MASK_PRES);
  if( debug['m'] )
    PRINT(SE,"\nExt. Mask Present: %d\n",mp);

  
  if(mp == 0) { 
    return 0;
  }
  if(mp == 2) {
    for(b = 0; b < info->nsbk; b = *group++)
      for(i = 0; i < info->sfb_per_sbk[b]; i ++)
        *mask++ = 1;
    return 2;
  }

  
  for(b = 0; b < info->nsbk; b = *group++){
    if( debug['m'] ) PRINT(SE," gr%1i:",b);
    for(i = 0; i < max_sfb; i ++) {
      *mask = (unsigned char) s_getbits(LEN_MASK);
      if( debug['m'] )PRINT(SE,"%1i",*mask);
      mask++;
    }
    for( ; i < info->sfb_per_sbk[b]; i++){
      *mask = 0;
      if( debug['m'] ) PRINT(SE,"%1i",*mask);
      mask++;
    }
  }
  if( debug['m'] ) PRINT(SE,"\n");
  return 1;
}



int
s_huffdecode(int id, 
             BLOCKTYPE *win, 
             Wnd_Shape *wshape, 
             byte *cb_map, 
             short* factors, 
             byte *group, byte *hasmask, byte* mask, 
             byte *max_sfb, 
             TNS_frame_info *tns, 
             Float *coef,
             Info* info,
             Info* winmap[4]
           )
{
  int i, common_window;
  
  int widx;
  int first=0, last=0;
  int ch=0;
  
  int max_spectral_line=0;
  short global_gain;  
  
  
  switch(id) {
  case ID_SCE:
	case ID_LFE:
	    common_window = 0;
	    break;
	case ID_CPE:
	    common_window = s_getbits(LEN_COM_WIN);
	    break;
	default:
	    PRINT(SE,"Unknown id %d\n", id);
	    return(-1);
    }

#ifdef CT_SBR
    *chIndx = ch;
#endif

    switch(id) {
    case ID_LFE:
      max_spectral_line=12;
    case ID_SCE:
      widx = 0;
      first = ch;
      last = ch;
      
      *hasmask = 0;
      memcpy(info, winmap[win[widx]], sizeof(Info));
      break;
    case ID_CPE:
          assert(0);
    }
    for (i=first; i<=last; i++) {
      s_fltclr(coef, LN2);

        if ((i==last) && (id==ID_CPE)) s_set_log_level(ID_2ND, 0, 0);  
	if(!s_getics(info, 
                     common_window, 
                     win,
                     &wshape->this_bk,
                     group,
                     max_sfb,
                     cb_map, 
                     coef, 
                     &global_gain, 
                     factors,
                     tns, 
                     i, 
                     max_spectral_line,
                     winmap
                     ))
          return -1;

        if (id==ID_LFE) {
          assert(0);
        }
    }

    s_set_log_level(LOG_FIN, 0, 0);  

    return 0;
}


void
s_get_ics_info(BLOCKTYPE *win, 
               byte *wshape, 
               byte *group,
               byte *max_sfb, 
               Info *wininfo,
               Info* winmap[4] 
               )
{
  int i;
  
  Info *info;
  
  i = s_getbits(LEN_ICS_RESERV);	    
  if (i) PRINT(SE,"WARNING: ICS - reserved bit is not set to 0\n");
  i=*win;
  *win = s_getbits(LEN_WIN_SEQ);
  if (debug['w']) PRINT(SE,"winseq: %i -> %i\n",i,*win);
    
  *wshape = (unsigned char) s_getbits(LEN_WIN_SH);
  if ((info = winmap[*win]) == NULL)
    CommonExit(1,"bad window code\n");
  
  memcpy(wininfo, info, sizeof(Info));
  
  
  if (info->islong) {
    *max_sfb = (unsigned char) s_getbits(LEN_MAX_SFBL);
    group[0] = 1;
    if (s_getbits(LEN_PRED_PRES))
      assert(0);
  }
  else {
    *max_sfb = (unsigned char) s_getbits(LEN_MAX_SFBS);
    s_getgroup(info, group);
  }
  
  if(debug['v']) {
    PRINT(SE,"win %d, wsh %d\n", *win, *wshape);
    PRINT(SE,"max_sf %d\n", *max_sfb);
  }
}
