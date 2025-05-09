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



#include "sac_reshuffinit.h"
#include "sac_reshufftables.h"
#include "sac_config.h" 



static short	sfbwidth128[(1<<LEN_MAX_SFBS)];
static Info	eight_short_info;
static Info	only_long_info;



Hcb		s_book[NSPECBOOKS+2];
Float		s_iq_exp_tbl[MAX_IQ_TBL];
Float		s_exptable[TEXP];
int		s_maxfac;
Info		*s_win_seq_info[NUMWIN_SEQ];





static char *
bin_str(unsigned long v, int len)
{
    int i, j;
    static char s[32];

    for (i=len-1, j=0; i>=0; i--, j++) {
	s[j] = (v & (1<<i)) ? '1' : '0';
    }
    s[j] = 0;
    return s;
}

static void
print_cb(Hcb *hcb) {
    int i;
    Huffman *hcw = hcb->hcw;
   PRINT(SE,"Huffman Codebook\n");
   PRINT(SE,"size %d, dim %d, lav %d, mod %d, off %d, signed %d\n", 
	hcb->n, hcb->dim, hcb->lav, hcb->mod, hcb->off, hcb->signed_cb);
   PRINT(SE,"%6s %6s %8s %-32s\n", "index", "length", "cw_10", "cw_2");
    for (i=0; i<hcb->n; i++)
	PRINT(SE,"%6d %6d %8ld %-32s\n",
	    hcw[i].index, hcw[i].len, hcw[i].cw,
	    bin_str(hcw[i].cw, hcw[i].len));
}    

static int
huffcmp(const void *va, const void *vb)
{
    const Huffman *a, *b;

    a = (Huffman *)va;
    b = (Huffman *)vb;
    if (a->len < b->len)
	return -1;
    if ( (a->len == b->len) && (a->cw < b->cw) )
	return -1;
    return 1;
}


static void
hufftab(Hcb *hcb, Huffman *hcw, int dim, int lav, int signed_cb)
{
    int i, n;
    
    if (!signed_cb) {
	hcb->mod = lav + 1;
        hcb->off = 0;
    }
    else {
	hcb->mod = 2*lav + 1;
        hcb->off = lav;
    }
    n=1;	    
    for (i=0; i<dim; i++)
	n *= hcb->mod;
    hcb->n = n;
    hcb->dim = dim;
    hcb->lav = lav;
    hcb->signed_cb = signed_cb;
    hcb->hcw = hcw;
    
    if (debug['H'] && !debug['S']) print_cb(hcb);
    
    qsort(hcw, n, sizeof(Huffman), huffcmp);
    
    if (debug['H'] && debug['S']) print_cb(hcb);
}


static void
infoinit(int samp_rate_idx)
{ 
    int i, j, k, n, ws;
    short *sfbands;
    Info *ip;
    SR_Info *sip = &s_samp_rate_info[samp_rate_idx];
	extern MC_Info s_mc_info; 

	s_mc_info.sampling_rate_idx = samp_rate_idx;

    
    ip = &only_long_info;
    s_win_seq_info[ONLYLONG_WINDOW] = ip;
    ip->islong = 1;
    ip->nsbk = 1;
    ip->bins_per_bk = LN2;
    for (i=0; i<ip->nsbk; i++) {
	ip->sfb_per_sbk[i] = sip->nsfb1024;
	ip->sectbits[i] = LONG_SECT_BITS;
	ip->sbk_sfb_top[i] = sip->SFbands1024;
    }
    ip->sfb_width_128 = NULL;
    ip->num_groups = 1;
    ip->group_len[0] = 1;
    ip->group_offs[0] = 0;
    
    
    ip = &eight_short_info;
    s_win_seq_info[EIGHTSHORT_WINDOW] = ip;
    ip->islong = 0;
    ip->nsbk = NSHORT;
    ip->bins_per_bk = LN2;
    for (i=0; i<ip->nsbk; i++) {
	ip->sfb_per_sbk[i] = sip->nsfb128;
	ip->sectbits[i] = SHORT_SECT_BITS;
	ip->sbk_sfb_top[i] = sip->SFbands128;
    }
    
    ip->sfb_width_128 = sfbwidth128;
    for (i=0, j=0, n=sip->nsfb128; i<n; i++) {
	k = sip->SFbands128[i];
	sfbwidth128[i] = k - j;
	j = k;
    }
    
    
    for (ws=0; ws<NUMWIN_SEQ; ws++) {
      if ((ip = s_win_seq_info[ws]) == NULL)
            continue;
	ip->sfb_per_bk = 0;   
	k = 0;
	n = 0;
        for (i=0; i<ip->nsbk; i++) {
            
	    ip->bins_per_sbk[i] = ip->bins_per_bk / ip->nsbk;
	    
            
            ip->sfb_per_bk += ip->sfb_per_sbk[i];

            
            sfbands = ip->sbk_sfb_top[i];
            for (j=0; j < ip->sfb_per_sbk[i]; j++)
                ip->bk_sfb_top[j+k] = sfbands[j] + n;

            n += ip->bins_per_sbk[i];
            k += ip->sfb_per_sbk[i];
	}	    
 
	if (debug['I']) {
	    PRINT(SE,"\nsampling rate %d\n", sip->samp_rate);
	    PRINT(SE,"win_info\t%d has %d windows\n", ws, ip->nsbk);
	    PRINT(SE,"\tbins_per_bk\t%d\n", ip->bins_per_bk);
	    PRINT(SE,"\tsfb_per_bk\t%d\n", ip->sfb_per_bk);
	    for (i=0; i<ip->nsbk; i++) {
		PRINT(SE,"window\t%d\n", i);
		PRINT(SE,"\tbins_per_sbk\t%d\n", ip->bins_per_sbk[i]);
		PRINT(SE,"\tsfb_per_sbk	%d\n", ip->sfb_per_sbk[i]);
	    }
	    if (ip->sfb_width_128 != NULL) {
		PRINT(SE,"sfb top and width\n");
		for (i=0; i<ip->sfb_per_sbk[0]; i++)
		    PRINT(SE,"%6d %6d %6d\n", i, 
			ip->sbk_sfb_top[0][i], 
			ip->sfb_width_128[i]);
	    }	   
	}	 
    }	    
}



 
void
s_huffbookinit(int samp_rate_idx)
{
    int i;

    hufftab(&s_book[1], s_book1, 4, 1, HUF1SGN);
    hufftab(&s_book[2], s_book2, 4, 1, HUF2SGN);
    hufftab(&s_book[3], s_book3, 4, 2, HUF3SGN);
    hufftab(&s_book[4], s_book4, 4, 2, HUF4SGN);
    hufftab(&s_book[5], s_book5, 2, 4, HUF5SGN);
    hufftab(&s_book[6], s_book6, 2, 4, HUF6SGN);
    hufftab(&s_book[7], s_book7, 2, 7, HUF7SGN);
    hufftab(&s_book[8], s_book8, 2, 7, HUF8SGN);
    hufftab(&s_book[9], s_book9, 2, 12, HUF9SGN);
    hufftab(&s_book[10], s_book10, 2, 12, HUF10SGN);
    hufftab(&s_book[11], s_book11, 2, 16, HUF11SGN);

    hufftab(&s_book[BOOKSCL], s_bookscl, 1, 60, 1);

    for(i = 0; i < TEXP; i++){
      s_exptable[i]  = pow( 2.0,  0.25*i);
    }
    s_maxfac = TEXP;

    for(i = 0; i < MAX_IQ_TBL; i++){
	s_iq_exp_tbl[i] = pow((double)i, 4./3.);
    }

    infoinit(samp_rate_idx);
}

