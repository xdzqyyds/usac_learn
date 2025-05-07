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

#include <stdio.h>
#include <string.h>
#include "interface.h"

#define LF	"%-25s %d\n"

typedef struct {
    int num_ele;
    int ele_is_cpe[(1<<LEN_TAG)];
    int ele_tag[(1<<LEN_TAG)];
} EleList;

typedef struct {
    int present;
    int ele_tag;
    int pseudo_enab;
} MIXdown;

typedef struct {
    int profile;
    int sampling_rate_idx;
    EleList front;
    EleList side;
    EleList back;
    EleList lfe;
    EleList data;
    EleList coupling;
    MIXdown mono_mix;
    MIXdown stereo_mix;
    MIXdown matrix_mix;
    char comments[(1<<LEN_PC_COMM)+1];
    long    buffer_fullness;	/* put this transport level info here */
} ProgConfig;

typedef struct {
    char    adif_id[LEN_ADIF_ID+1];
    int     copy_id_present;
    char    copy_id[LEN_COPYRT_ID+1];
    int     original_copy;
    int     home;
    int     bitstream_type;
    long    bitrate;
    int     num_pce;
    int     prog_tags[(1<<LEN_TAG)];
} ADIF_Header;

FILE		*bin;
long		cword = 0;
int		nbits = 0;
ADIF_Header     adif_header;
ProgConfig      prog_config;


long
getshort(void)
{
    int c1, c2;

    c1 = getc(bin);
    c2 = getc(bin);
    if(c2 < 0)
        c2 = 0;
    if(c1 < 0){
        c1 = 0;
    }
    return (c1<<8) | c2;
}

long
GetBits(int n)
{
    while (nbits <= 16){
        cword = (cword<<16) | getshort();
        nbits += 16;
    }

    nbits -= n;
    return (cword>>nbits) & ((1<<n)-1);
}

int
byte_align(void)
{
    nbits &= (~7);
}

/*
 * program configuration element
 */
static void
get_ele_list(EleList *p, int enable_cpe)
{  
    int i, j;
    for (i=0, j=p->num_ele; i<j; i++) {
	if (enable_cpe)
	    p->ele_is_cpe[i] = GetBits(LEN_ELE_IS_CPE);
	p->ele_tag[i] = GetBits(LEN_TAG);
    }
}

int
get_prog_config(ProgConfig *p)
{
    int i, j, tag;

    tag = GetBits(LEN_TAG);
    p->profile = GetBits(LEN_PROFILE);
    p->sampling_rate_idx = GetBits(LEN_SAMP_IDX);
    p->front.num_ele = GetBits(LEN_NUM_ELE);
    p->side.num_ele = GetBits(LEN_NUM_ELE);
    p->back.num_ele = GetBits(LEN_NUM_ELE);
    p->lfe.num_ele = GetBits(LEN_NUM_LFE);
    p->data.num_ele = GetBits(LEN_NUM_DAT);
    p->coupling.num_ele = GetBits(LEN_NUM_CCE);
    if ((p->mono_mix.present = GetBits(LEN_MIX_PRES)) == 1)
	p->mono_mix.ele_tag = GetBits(LEN_TAG);
    if ((p->stereo_mix.present = GetBits(LEN_MIX_PRES)) == 1)
	p->stereo_mix.ele_tag = GetBits(LEN_TAG);
    if ((p->matrix_mix.present = GetBits(LEN_MIX_PRES)) == 1) {
	p->matrix_mix.ele_tag = GetBits(LEN_MMIX_IDX);
	p->matrix_mix.pseudo_enab = GetBits(LEN_PSUR_ENAB);
    }
    get_ele_list(&p->front, 1);
    get_ele_list(&p->side, 1);
    get_ele_list(&p->back, 1);
    get_ele_list(&p->lfe, 0);
    get_ele_list(&p->data, 0);
    get_ele_list(&p->coupling, 1);
    
    byte_align();
    j = GetBits(LEN_COMMENT_BYTES);
    for (i=0; i<j; i++)
	p->comments[i] = GetBits(LEN_BYTE);
    p->comments[i] = 0;	/* null terminator for string */
   
    return tag;
}

/*
 * adif_header
 */
int
get_adif_header(void)
{
    int i, n, tag;
    ADIF_Header *p = &adif_header;
    
    /* adif header */
    for (i=0; i<LEN_ADIF_ID; i++)
	p->adif_id[i] = GetBits(LEN_BYTE); 
    p->adif_id[i] = 0;	    /* null terminated string */
    /* test for id */
    if (strncmp(p->adif_id, "ADIF", 4) != 0) {
	printf( "Bad ADIF header ID\n");
	return -1;
    }
	
    /* copyright string */	
    if ((p->copy_id_present = GetBits(LEN_COPYRT_PRES)) == 1) {
	for (i=0; i<LEN_COPYRT_ID; i++)
	    p->copy_id[i] = GetBits(LEN_BYTE); 
	p->copy_id[i] = 0;  /* null terminated string */
    }
    p->original_copy = GetBits(LEN_ORIG);
    p->home = GetBits(LEN_HOME);
    p->bitstream_type = GetBits(LEN_BS_TYPE);
    p->bitrate = GetBits(LEN_BIT_RATE);

    printf( "\nADIF header:\n");
    printf( LF, "p->copy_id_present", p->copy_id_present);
    printf( LF, "p->original_copy", p->original_copy);
    printf( LF, "p->home", p->home);
    printf( LF, "p->bitstream_type", p->bitstream_type);
    printf( LF, "p->bitrate", p->bitrate);
    
    /* program config elements */ 
    n = GetBits(LEN_NUM_PCE) + 1;
    printf( "%s %d\n", "number of PCE", n);
    
    for (i=0; i<n; i++) {
	ProgConfig *p = &prog_config;
	
	p->buffer_fullness =
	    (adif_header.bitstream_type == 0) ? GetBits(LEN_ADIF_BF) : 0;
	tag = get_prog_config(p);
	printf( "\n");
	printf( LF, "PCE number", n);
	printf( LF, "tag", tag);
	printf( LF, "p->profile", p->profile);
	printf( LF, "p->sampling_rate_idx", p->sampling_rate_idx);
	printf( LF, "p->front.num_ele", p->front.num_ele);
	printf( LF, "p->side.num_ele", p->side.num_ele);
	printf( LF, "p->back.num_ele", p->back.num_ele);
	printf( LF, "p->lfe.num_ele", p->lfe.num_ele);
	printf( LF, "p->data.num_ele", p->data.num_ele);
	printf( LF, "p->coupling.num_ele", p->coupling.num_ele);
	printf( LF, "p->mono_mix.present", p->mono_mix.present);
	printf( LF, "p->stereo_mix.present", p->stereo_mix.present);
	printf( LF, "p->matrix_mix.present", p->matrix_mix.present);
	printf( "Comment: %s\n", prog_config.comments);
    }
    return 1;
}

/* program to print ADIF header 
 * 
 */
void
main(int argc, char *argv[])
{
    if (argc == 1) {
	printf("usage: %s <filename>\n", argv[0]);
	return;
    }
    
    if ((bin = fopen(argv[1], "rb")) == 0) {
        printf( "cant open %s\n", argv[1]);
        return;
    }
    
    get_adif_header();  
}
