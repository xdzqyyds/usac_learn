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



#include "sac_resdefs.h"


#include "sac_reshuffdec_intern.h"
#include "sac_reshuffinit.h"
#include "sac_resintrins.h"
#include "sac_bitinput.h"
#include <assert.h>

typedef int DINT_DATATYPE ;


static void
deinterleave(DINT_DATATYPE inptr[], DINT_DATATYPE outptr[], short ngroups,
             short nsubgroups[], int ncells[], short cellsize[])
{
  int i, j, k, l;
  DINT_DATATYPE *start_inptr, *start_subgroup_ptr, *subgroup_ptr;
  short cell_inc, subgroup_inc;

  start_subgroup_ptr = outptr;

  for (i = 0; i < ngroups; i++)
    {
      cell_inc = 0;
      start_inptr = inptr;

      

      subgroup_inc = 0;
      for (j = 0; j < ncells[i]; j++) {
        subgroup_inc += cellsize[j];
      }

      

      for (j = 0; j < ncells[i]; j++) {
        subgroup_ptr = start_subgroup_ptr;

        for (k = 0; k < nsubgroups[i]; k++) {
          outptr = subgroup_ptr + cell_inc;
          for (l = 0; l < cellsize[j]; l++) {
            *outptr++ = *inptr++;
          }
          subgroup_ptr += subgroup_inc;
        }
        cell_inc += cellsize[j];
      }
      start_subgroup_ptr += (inptr - start_inptr);
    }
}

static void 
calc_gsfb_table(Info *info, byte *group)
{
  int group_offset;
  int group_idx;
  int offset;
  short * group_offset_p;
  int sfb,len;
  
  if (info->islong){
    return;
  } else {
    group_offset = 0;
    group_idx =0;
    do  {
      info->group_len[group_idx]=group[group_idx]-group_offset;
      group_offset=group[group_idx];
      group_idx++;
    } while (group_offset<8);
    info->num_groups=group_idx;
    group_offset_p = info->bk_sfb_top;
    offset=0;
    for (group_idx=0;group_idx<info->num_groups;group_idx++){
      len = info->group_len[group_idx];
      for (sfb=0;sfb<info->sfb_per_sbk[group_idx];sfb++){
        offset += info->sfb_width_128[sfb] * len;
        *group_offset_p++ = offset;
      }
    }
  }
}


static int
huffcb(byte *sect, int *sectbits, int tot_sfb, int sfb_per_sbk, byte max_sfb)
{
  int nsect, n, base, bits, len;

  if (debug['s']) {
    PRINT(SE,"total sfb %d\n", tot_sfb);
    PRINT(SE,"sect, top, cb\n");
  }
  bits = sectbits[0];
  len = (1 << bits) - 1;
  nsect = 0;
  for(base = 0; base < tot_sfb && nsect < tot_sfb; ){
    *sect++ = (unsigned char) s_getbits(LEN_CB);

    n = s_getbits(bits);
    while(n == len && base < tot_sfb){
      base += len;
      n = s_getbits(bits);
    }
    base += n;
    *sect++ = base;
    nsect++;
    if (debug['s'])
      PRINT(SE," %6d %6d %6d \n", nsect, sect[-1], sect[-2]);

    
    if ((sect[-1] % sfb_per_sbk) == max_sfb) {
      base += (sfb_per_sbk - max_sfb);
      *sect++ = 0;
      *sect++ = base;
      nsect++;
      if (debug['s'])
	PRINT(SE,"(%6d %6d %6d)\n", nsect, sect[-1], sect[-2]);
    }
  }

  if(base != tot_sfb || nsect > tot_sfb)
    return 0;
  return nsect;
}


static int
hufffac(Info *info, byte *group, int nsect, byte *sect,
        short global_gain, short *factors)
{
  Hcb *hcb;
  Huffman *hcw;
  int i, b, bb, t, n, sfb, top, fac, is_pos;
  int factor_transmitted[MAXBANDS], *fac_trans;
#ifdef MPEG4V1
  int noise_pcm_flag = 1;
  int noise_nrg;
#endif

  
  s_intclr(factor_transmitted, MAXBANDS);
  s_shortclr(factors, MAXBANDS);

  sfb = 0;
  fac_trans = factor_transmitted;
  for(i = 0; i < nsect; i++){
    top = sect[1];		
    t = sect[0];		
    sect += 2;
    for(; sfb < top; sfb++) {
      fac_trans[sfb] = t;
    }
  }

  
  fac = global_gain;
  is_pos = 0;
#ifdef MPEG4V1
  noise_nrg = global_gain - NOISE_OFFSET;
#endif

  
  hcb = &s_book[BOOKSCL];
  hcw = hcb->hcw;
  bb = 0;
  if (debug['f'])
    PRINT(SE,"scale factors\n");
  for(b = 0; b < info->nsbk; ){
    n = info->sfb_per_sbk[b];
    b = *group++;
    for(i = 0; i < n; i++){
      switch (fac_trans[i]) {
      case ZERO_HCB:	    
        break;
      default:		    
        
        t = s_decode_huff_cw(hcw);
        fac += t - MIDFAC;    
        if(fac >= 2*s_maxfac || fac < 0)
          return 0;
        factors[i] = fac;
        break;
      case BOOKSCL:	    
        return 0;
#ifndef MPEG4V1
      case RESERVED_HCB:
        CommonExit(1,"The MPEG-4 tool PNS is not supported!\n");
        return 0;
#endif
      case INTENSITY_HCB:	    
      case INTENSITY_HCB2:
        
        t = s_decode_huff_cw(hcw);
        is_pos += t - MIDFAC;
        factors[i] = is_pos;
        break;
#ifdef MPEG4V1
      case NOISE_HCB:	    
        
        noise_present=1;
        if (noise_pcm_flag) {
          noise_pcm_flag = 0;
          t = getbits( NOISE_PCM_BITS ) - NOISE_PCM_OFFSET;
        }
        else
          t = decode_huff_cw(hcw) - MIDFAC;
        noise_nrg += t;
        if (debug['f'])
          PRINT(SE,"\n%3d %3d (noise, code %d)", i, noise_nrg, t);
        factors[i] = noise_nrg;
        break;
#endif

      }
      if (debug['f'])
        PRINT(SE,"%3d: %3d %3d\n", i, fac_trans[i], factors[i]);
    }
    if (debug['f'])
      PRINT(SE,"\n");

    
    if (!(info->islong)) {
      for(bb++; bb < b; bb++) {
        for (i=0; i<n; i++) {
          factors[i+n] = factors[i];
        }
        factors += n;
      }
    }
    fac_trans += n;
    factors += n;
  }
  return 1;
}



struct Pulse_Info
{
  int pulse_data_present;
  int number_pulse;
  int pulse_start_sfb;
  int pulse_position[NUM_PULSE_LINES];
  int pulse_offset[NUM_PULSE_LINES];
  int pulse_amp[NUM_PULSE_LINES];
};
static struct Pulse_Info pulse_info;

static void
get_pulse_nc(struct Pulse_Info *pulse_info)
{
  int i;
  pulse_info->number_pulse = s_getbits(LEN_PULSE_NPULSE);
  pulse_info->pulse_start_sfb = s_getbits(LEN_PULSE_ST_SFB);
  for(i=0; i<pulse_info->number_pulse+1; i++) {
    pulse_info->pulse_offset[i] = s_getbits(LEN_PULSE_POFF);
    pulse_info->pulse_amp[i] = s_getbits(LEN_PULSE_PAMP);
    if (debug['P'])
      PRINT(SE,"Pulse: %d %d %d %d\n", pulse_info->pulse_start_sfb,
            i, pulse_info->pulse_offset[i], pulse_info->pulse_amp[i]);
  }
}


static void
pulse_nc(int *coef, struct Pulse_Info *pulse_info)
{
  int i, k;
    
  if (pulse_info->pulse_start_sfb>0){
    k = s_win_seq_info[ONLYLONG_WINDOW]->sbk_sfb_top[0][pulse_info->pulse_start_sfb-1];
  }else{
    k = 0;
  }
    
  for(i=0; i<=pulse_info->number_pulse; i++) {
    k += pulse_info->pulse_offset[i];
    if (coef[k]>0) coef[k] += pulse_info->pulse_amp[i];
    else coef[k] -= pulse_info->pulse_amp[i];
  }
}


static int
huffspec(Info *info, int nsect, byte *sect, short *factors, Float *coef, int max_spectral_line)
{
  Hcb *hcb;
  Huffman *hcw;
  int i, j, k, table, step, temp, stop, bottom, top;
  short *bands, *bandp;
  int quant[LN2], *qp;	    

  int idx,kstart;
  int* quantDebug;
  int tmp_spec[LN2];
  int max_spectral_line_OK = 1;

  s_intclr(quant, LN2);

  bands = info->bk_sfb_top;
  bottom = 0;
  k = 0;
  bandp = bands;
  for(i = nsect; i; i--) {
    table = sect[0];
    top = sect[1];
    sect += 2;
    quantDebug=quant;
    if( (table == 0) ||
#ifdef MPEG4V1
        (table == NOISE_HCB) ||
#endif
        (table == INTENSITY_HCB) ||
        (table == INTENSITY_HCB2) ) {
      bandp = bands+top;
      k = bandp[-1];
      bottom = top;
      continue;
    }
    if(table < BY4BOOKS+1) {
      step = 4;
    }
    else {
      step = 2;
    }
    hcb = &s_book[table];
    hcw = hcb->hcw;
    qp = quant+k;
    kstart=k;
    for(j=bottom; j<top; j++) {
      stop = *bandp++;
      while(k < stop) {
        temp = s_decode_huff_cw(hcw);
        s_unpack_idx(qp, temp, hcb);

        if (!hcb->signed_cb)
          s_get_sign_bits(qp, step);
        if(table == ESCBOOK){
          qp[0] = s_getescape(qp[0]);
          qp[1] = s_getescape(qp[1]);
        }
        qp += step;
        k += step;
        if ((max_spectral_line>0)&&(k>=max_spectral_line)) { 
          for (idx=max_spectral_line;idx<k;idx++) if (quant[idx]!=0) max_spectral_line_OK=0;
        }
      }
      if(debug['q']){
        PRINT(SE,"\nsect %d %d\n", table, kstart);
        for (idx=kstart ;idx<k;idx++) {
          PRINT(SE,"%5i",quantDebug[idx]);
        }
        PRINT(SE,"\n");
      }
    }
    bottom = top;
  }
  if (max_spectral_line_OK==0) PRINT(SE,"WARNING: more non-zero spectral lines than expected (maximum of %i lines)!\n", max_spectral_line);

  
  if ( (info->islong) && (pulse_info.pulse_data_present) )
    pulse_nc(quant, &pulse_info);

  if (!info->islong) {
    deinterleave (quant,tmp_spec,
                  info->num_groups,   
                  info->group_len,
                  info->sfb_per_sbk,
                  info->sfb_width_128);
    memcpy(quant,tmp_spec,sizeof(tmp_spec));
  }

  
  for (i=0; i<info->bins_per_bk; i++) {
    coef[i] = s_esc_iquant(quant[i]);
  }

  
  {
    int sbk, nsbk, sfb, nsfb, fac, top;
    Float *fp, scale;

    i = 0;
    fp = coef;
    nsbk = info->nsbk;
    for (sbk=0; sbk<nsbk; sbk++) {
      nsfb = info->sfb_per_sbk[sbk];
      k=0;
      for (sfb=0; sfb<nsfb; sfb++) {
        top = info->sbk_sfb_top[sbk][sfb];
        fac = factors[i++]-SF_OFFSET;

        if (fac >= 0 && fac < TEXP) {
          scale = s_exptable[fac];
        }
        else {
          if (fac == -SF_OFFSET) {
            scale = 0;
          }
          else {
            scale = pow( 2.0,  0.25*fac );
          }
        }
        for ( ; k<top; k++) {
          if(debug['l'])
            PRINT(SE, "%6d %12.2f %12.2f\n", k, *fp, (*fp)*scale);
          *fp++ *= scale;
        }
      }
    }
  }
  return 1;
}


void
s_getgroup(Info *info, byte *group)
{
  int i, j, first_short;

  if( debug['g'] ) PRINT(SE,"Grouping: 0");
  first_short=1;
  for (i = 0; i < info->nsbk; i++) {
    if (info->bins_per_sbk[i] > SN2) {
      
      *group++ = i+1;
    }
    else {
      
      if (first_short) {
        
        first_short=0;
      }
      else {
        if((j = s_getbits(1)) == 0) {
          *group++ = i;
        }
        if( debug['g'] ) PRINT(SE,"%1d", j);
      }
    }
  }
  *group = i;
  if( debug['g'] ) PRINT(SE,"\n");
}

int
s_getics(Info *info, int common_window, BLOCKTYPE *win, byte *wshape, 
         byte *group, byte *max_sfb, 
         byte *cb_map, Float *coef, short *global_gain, 
         short *factors,
         TNS_frame_info *tns, int ch, int max_spectral_line,
         Info* winmap[4]
         )
{
  int nsect, i, cb, top, bot, tot_sfb;
  byte sect[ 2*(MAXBANDS+1) ];

  
  *global_gain = (short) s_getbits(LEN_SCL_PCM);
  if (debug['f'])
    PRINT(SE,"global gain: %3d\n", *global_gain);

  if (!common_window)
    s_get_ics_info(win, 
                   wshape, 
                   group, 
                   max_sfb,
                   info,
                   winmap
                   );
  

  
  if (*max_sfb == 0) {
    tot_sfb = 0;
  }
  else {
    i=0;
    tot_sfb = info->sfb_per_sbk[0];
    if (debug['f'])PRINT(SE,"tot sfb %d %d\n", i, tot_sfb);
    while (group[i++] < info->nsbk) {
      tot_sfb += info->sfb_per_sbk[0];
      if (debug['f'])PRINT(SE,"tot sfb %d %d\n", i, tot_sfb);
    }
  }

  
  nsect = huffcb(sect, info->sectbits, tot_sfb, info->sfb_per_sbk[0], *max_sfb);
  if(nsect==0 && *max_sfb>0)
    return 0;

  
  if (nsect) {
    bot = 0;
    for (i=0; i<nsect; i++) {
      cb = sect[2*i];
      top = sect[2*i + 1];
      for (; bot<top; bot++)
        *cb_map++ = cb;
      bot = top;
    }
  }  else {
    for (i=0; i<MAXBANDS; i++)
      cb_map[i] = 0;
  }

  
  calc_gsfb_table(info, group);

  
  if(!hufffac(info, group, nsect, sect, *global_gain, factors))
    return 0;

  
  if ((pulse_info.pulse_data_present = s_getbits(LEN_PULSE_PRES))) {
    if (info->islong) {
      get_pulse_nc(&pulse_info);
    }
    else {
      PRINT(SE, "Pulse data not allowed for short blocks\n");
      CommonExit(1,"pulse_nc\n");
    }
  }

  
  if (s_getbits(LEN_TNS_PRES)) {
    s_get_tns(info, tns);
  }
  else {
    s_clr_tns(info, tns);
  }

  
  if (s_getbits(LEN_GAIN_PRES)) {
    assert(0);

  }

  return huffspec(info, nsect, sect, factors, coef, max_spectral_line);
}


