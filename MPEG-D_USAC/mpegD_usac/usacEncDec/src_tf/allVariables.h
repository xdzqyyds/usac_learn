/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS and edited by

Yoshiaki Oikawa (Sony Corporation),
Mitsuyuki Hatanaka (Sony Corporation),
Ralph Sperschneider (Fraunhofer Gesellschaft IIS)
Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
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
Copyright(c)1996, 1997, 1998.
 *                                                                           *
 ****************************************************************************/

#ifndef	_allVariables_h_
#define _allVariables_h_

extern	long		bno;

extern	Huffman		book1[];
extern	Huffman		book2[];
extern	Huffman		book3[];
extern	Huffman		book4[];
extern	Huffman		book5[];
extern	Huffman		book6[];
extern	Huffman		book7[];
extern	Huffman		book8[];
extern	Huffman		book9[];
extern	Huffman		book10[];
extern	Huffman		book11[];
extern	Huffman		bookscl[];
  
/* RVL scalefactor coding */
extern	Huffman		bookRvlc[];
extern	Huffman		bookEscape[];

extern	Hcb		book[NSPECBOOKS+2+32]; /*TM 990527*/
extern	int		debug[256];
extern	Info		eight_short_info;
extern	Float		exptable[TEXP];
extern	Float*		hvals[NSPECBOOKS];
extern	Float		iq_exp_tbl[MAX_IQ_TBL];
extern	int		*lpflag[Chans];
extern	Info		only_long_info;
extern	short		sfbwidthShort[];
extern	SR_Info		samp_rate_info[];
extern  const int	sampling_boundaries[(1<<LEN_SAMP_IDX)];


extern	Info*		winmap[NUM_WIN_SEQ];
extern	Info		*win_seq_info[NUM_WIN_SEQ];

extern	int		default_config;
extern	int		current_program;
extern	ProgConfig	prog_config;
extern	MC_Info		mc_info;

#endif	/* _allVariables_h_ */
