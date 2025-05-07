/*****************************************************************************
 *                                                                           *
"This software module was originally developed by 

Martin Dietz (Fraunhofer Gesellschaft IIS)

and edited by

Ralph Sperschneider (Fraunhofer Gesellschaft IIS)

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
Copyright(c)1998.
 *                                                                           *
 ****************************************************************************/

#ifndef _bitfct_h_
#define _bitfct_h_

#define UINT_BITLENGTH            32    /* Length in bits of an (unsigned) long variable */

typedef struct 
{
    unsigned char  *byte_ptr;           /* pointer to next byte             */
    unsigned char  byte;                /* next byte to write in buffer     */
    unsigned short valid_bits;          /* number of valid bits in byte     */
    unsigned long bit_count;            /* counts encoded bits              */
                                 
} BIT_BUF;    


#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

void          WriteBitBuf( BIT_BUF*      data, 
                           unsigned long value, 
                           unsigned long no_of_bits );

void          InitWriteBitBuf( BIT_BUF*       bit_buf, 
                               unsigned char* buffer );

unsigned long FlushWriteBitBuf( BIT_BUF* bit_buf );

unsigned long GetBitCount( BIT_BUF* bit_buf );

void          InitReadBitBuf( BIT_BUF*      bit_buf, 
                              unsigned char* buffer );

unsigned long ReadBitBuf( BIT_BUF*      data, 
                          unsigned long no_of_bits );

void          TransferBitsBetweenBitBuf ( BIT_BUF *in, BIT_BUF *out, unsigned long noOfBits );

#endif /* _bitfct_h_ */
