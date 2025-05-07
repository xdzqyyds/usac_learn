/**********************************************************************
MPEG-4 Audio VM

This software module was originally developed by
  Y.B. Thomas Kim and S.H. Park (Samsung AIT)
and edited by
  Y.B. Thomas Kim (Samsung AIT) on 1997-11-06

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

Copyright (c) 1997.

**********************************************************************/

#ifndef	_sam_scf_coding_h_
#define	_sam_scf_coding_h_

  static int scf2index[128] = {
  68,   69,   70,   71,   75,   76,   77,   78,
  79,   80,   81,   82,   83,   84,   85,   86,
  87,   88,   89,   72,   90,   73,   65,   66,
  58,   67,   59,   60,   61,   62,   54,   55,
  46,   47,   48,   49,   50,   51,   41,   42,
  35,   36,   37,   29,   38,   30,   23,   24,
  25,   19,   20,   14,   15,   16,   11,    7,
  8,    5,    2,    1,    0,    3,    4,    6,
  9,   10,   12,   13,   17,   18,   21,   22,
  26,   27,   28,   31,   32,   33,   34,   39,
  40,   43,   44,   45,   52,   53,   63,   56,
  64,   57,   74,   91,   92,   93,   94,   95,
  96,   97,   98,   99,  100,  101,  102,  103,
  104,  105,  106,  107,  108,  109,  110,  111,
  112,  113,  114,  115,  116,  117,  118,  119,
  120,  121,  122,  123,  124,  125,  126,  127,
  };
  
static int index2scf[128] = {
   60,   59,   58,   61,   62,   57,   63,   55,
   56,   64,   65,   54,   66,   67,   51,   52,
   53,   68,   69,   49,   50,   70,   71,   46,
   47,   48,   72,   73,   74,   43,   45,   75,
   76,   77,   78,   40,   41,   42,   44,   79,
   80,   38,   39,   81,   82,   83,   32,   33,
   34,   35,   36,   37,   84,   85,   30,   31,
   87,   89,   24,   26,   27,   28,   29,   86,
   88,   22,   23,   25,    0,    1,    2,    3,
   19,   21,   90,    4,    5,    6,    7,    8,
    9,   10,   11,   12,   13,   14,   15,   16,
   17,   18,   20,   91,   92,   93,   94,   95,
   96,   97,   98,   99,  100,  101,  102,  103,
  104,  105,  106,  107,  108,  109,  110,  111,
  112,  113,  114,  115,  116,  117,  118,  119,
  120,  121,  122,  123,  124,  125,  126,  127,
};

#endif /* #ifndef	_sam_scf_coding_h_ */
