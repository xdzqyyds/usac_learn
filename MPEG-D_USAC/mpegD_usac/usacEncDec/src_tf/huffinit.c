/**********************************************************************
MPEG-4 Audio VM



This software module was originally developed by



and edited by

Ali Nowbakht-Irani  (Fraunhofer Gesellschaft IIS)
Ralph Sperschneider (Fraunhofer Gesellschaft IIS)

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1997,1998,1999.

*/

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "allHandles.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "allVariables.h"        /* variables */

#include "huffdec3.h"
#include "port.h"

void huffbookinit(int  block_size_samples, int sampling_rate_idx)
{
    int i;

    hufftab ( &book[1],       book1,   4,  1,    1,  HUF1SGN,            11 );
    hufftab ( &book[2],       book2,   4,  1,    1,  HUF2SGN,             9 );
    hufftab ( &book[3],       book3,   4,  2,    2,  HUF3SGN,          16+4 );
    hufftab ( &book[4],       book4,   4,  2,    2,  HUF4SGN,          12+4 );
    hufftab ( &book[5],       book5,   2,  4,    4,  HUF5SGN,            13 );
    hufftab ( &book[6],       book6,   2,  4,    4,  HUF6SGN,            11 );
    hufftab ( &book[7],       book7,   2,  7,    7,  HUF7SGN,          12+2 );
    hufftab ( &book[8],       book8,   2,  7,    7,  HUF8SGN,          10+2 );
    hufftab ( &book[9],       book9,   2, 12,   12,  HUF9SGN,          15+2 );
    hufftab ( &book[10],      book10,  2, 12,   12, HUF10SGN,          12+2 );
    hufftab ( &book[11],      book11,  2, 16, 8191, HUF11SGN, 5+2+2*(2*8+5) );

    hufftab ( &book[BOOKSCL], bookscl, 1, 60,   60,        1,            19 );

    hufftab ( &book[16],      book11,  2, 16,   15, HUF11SGN,          12+2 ); /*14*/
    hufftab ( &book[17],      book11,  2, 16,   31, HUF11SGN, 5+2+2*(2*0+5) ); /*17*/
    hufftab ( &book[18],      book11,  2, 16,   47, HUF11SGN, 5+2+2*(2*1+5) ); /*21*/
    hufftab ( &book[19],      book11,  2, 16,   63, HUF11SGN, 5+2+2*(2*1+5) ); /*21*/
    hufftab ( &book[20],      book11,  2, 16,   95, HUF11SGN, 5+2+2*(2*2+5) ); /*25*/
    hufftab ( &book[21],      book11,  2, 16,  127, HUF11SGN, 5+2+2*(2*2+5) ); /*25*/
    hufftab ( &book[22],      book11,  2, 16,  159, HUF11SGN, 5+2+2*(2*3+5) ); /*29*/
    hufftab ( &book[23],      book11,  2, 16,  191, HUF11SGN, 5+2+2*(2*3+5) ); /*29*/
    hufftab ( &book[24],      book11,  2, 16,  223, HUF11SGN, 5+2+2*(2*3+5) ); /*29*/
    hufftab ( &book[25],      book11,  2, 16,  255, HUF11SGN, 5+2+2*(2*3+5) ); /*29*/
    hufftab ( &book[26],      book11,  2, 16,  319, HUF11SGN, 5+2+2*(2*4+5) ); /*33*/
    hufftab ( &book[27],      book11,  2, 16,  383, HUF11SGN, 5+2+2*(2*4+5) ); /*33*/
    hufftab ( &book[28],      book11,  2, 16,  511, HUF11SGN, 5+2+2*(2*4+5) ); /*33*/
    hufftab ( &book[29],      book11,  2, 16,  767, HUF11SGN, 5+2+2*(2*5+5) ); /*37*/
    hufftab ( &book[30],      book11,  2, 16, 1023, HUF11SGN, 5+2+2*(2*5+5) ); /*37*/
    hufftab ( &book[31],      book11,  2, 16, 2047, HUF11SGN, 5+2+2*(2*6+5) ); /*41*/

    /* RVL scalefector coding */
    qsort(bookRvlc, 23, sizeof(Huffman), huffcmp);
    qsort(bookEscape, 54, sizeof(Huffman), huffcmp);

    for(i = 0; i < TEXP; i++){
	exptable[i]  = pow( 2.0,  0.25*i);
    }

    for(i = 0; i < MAX_IQ_TBL; i++){
	iq_exp_tbl[i] = pow(i, 4./3.);
    }

    infoinit(&samp_rate_info[sampling_rate_idx], block_size_samples);
}

void infoinit ( SR_Info* sip, 
                int      block_size_samples )
{ 
    int i, j, k, n, ws;
    const short *sfbands;
    Info *ip;

    /* long block info */
    ip = &only_long_info;
    win_seq_info[ONLY_LONG_SEQUENCE] = ip;
    ip->islong = 1;
    ip->nsbk = 1;
    ip->bins_per_bk =  block_size_samples ;
    ip->longFssGroups = sip->longFssGroups;
    for (i=0; i<ip->nsbk; i++) {
      ip->sectbits[i] = LONG_SECT_BITS;
      switch(block_size_samples){
      case 480:
        ip->sbk_sfb_top[i] = sip->SFbands480;
        ip->sfb_per_sbk[i] = sip->nsfb480;
        break;
      case 512:
        ip->sbk_sfb_top[i] = sip->SFbands512;
        ip->sfb_per_sbk[i] = sip->nsfb512;
        break;
      case 768:
        ip->sfb_per_sbk[i] = sip->nsfb768;
        ip->sbk_sfb_top[i] = sip->SFbands768;
        break;
      case 960:
        ip->sbk_sfb_top[i] = sip->SFbands960;
        ip->sfb_per_sbk[i] = sip->nsfb960;
        break;
      case 1024:
        ip->sfb_per_sbk[i] = sip->nsfb1024;
        ip->sbk_sfb_top[i] = sip->SFbands1024;
        break;
      default:
        assert(0);
        break;
      }
    }
    ip->sfb_width_short = NULL;
    ip->num_groups = 1;
    ip->group_len[0] = 1;
    ip->group_offs[0] = 0;
    
    /* short block info */
    ip = &eight_short_info;
    win_seq_info[EIGHT_SHORT_SEQUENCE] = ip;
    ip->islong = 0;
    ip->nsbk = NSHORT;
    ip->bins_per_bk = block_size_samples;
    ip->shortFssWidth = sip->shortFssWidth;
    for (i=0; i<ip->nsbk; i++) {
      ip->sectbits[i] = SHORT_SECT_BITS;
      switch(block_size_samples){
      case 768:
        ip->sbk_sfb_top[i] = sip->SFbands96;
        ip->sfb_per_sbk[i] = sip->nsfb96;
        break;
      case 960:
        ip->sbk_sfb_top[i] = sip->SFbands120;
        ip->sfb_per_sbk[i] = sip->nsfb120;
        break;
      case 1024:
        ip->sbk_sfb_top[i] = sip->SFbands128;
        ip->sfb_per_sbk[i] = sip->nsfb128;
        break;
      default:
        assert(0);
        break;
      }
    }
    /* construct sfb width table */
    ip->sfb_width_short = sfbwidthShort;
    for (i=0, j=0, n=ip->sfb_per_sbk[0]; i<n; i++) {
	k = ip->sbk_sfb_top[0][i];
	ip->sfb_width_short[i] = k - j; /*  insure: writing array out of range  */
	j = k;
    }
    
    /* common to long and short */
    for (ws=0; ws<NUM_WIN_SEQ; ws++) {
        if ((ip = win_seq_info[ws]) == NULL)
            continue;
	ip->sfb_per_bk = 0;   
	k = 0;
	n = 0;
        for (i=0; i<ip->nsbk; i++) {
            /* compute bins_per_sbk */
	    ip->bins_per_sbk[i] = ip->bins_per_bk / ip->nsbk;
	    
            /* compute sfb_per_bk */
            ip->sfb_per_bk += ip->sfb_per_sbk[i];

            /* construct default (non-interleaved) bk_sfb_top[] */
            sfbands = ip->sbk_sfb_top[i];
            for (j=0; j < ip->sfb_per_sbk[i]; j++)
                ip->bk_sfb_top[j+k] = sfbands[j] + n;

            n += ip->bins_per_sbk[i];
            k += ip->sfb_per_sbk[i];
	}	    
 
	if (debug['I']) {
	    fprintf(stderr,"\nsampling rate %d\n", sip->samp_rate);
	    fprintf(stderr,"win_info\t%d has %d windows\n", ws, ip->nsbk);
	    fprintf(stderr,"\tbins_per_bk\t%d\n", ip->bins_per_bk);
	    fprintf(stderr,"\tsfb_per_bk\t%d\n", ip->sfb_per_bk);
	    for (i=0; i<ip->nsbk; i++) {
		fprintf(stderr,"window\t%d\n", i);
		fprintf(stderr,"\tbins_per_sbk\t%d\n", ip->bins_per_sbk[i]);
		fprintf(stderr,"\tsfb_per_sbk	%d\n", ip->sfb_per_sbk[i]);
	    }
	    if (ip->sfb_width_short != NULL) {
		fprintf(stderr,"sfb top and width\n");
		for (i=0; i<ip->sfb_per_sbk[0]; i++)
		    fprintf(stderr,"%6d %6d %6d\n", i, 
			ip->sbk_sfb_top[0][i], 
			ip->sfb_width_short[i]);
	    }	   
	}	 
    }	    
}
