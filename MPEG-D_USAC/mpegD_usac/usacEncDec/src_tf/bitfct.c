/*****************************************************************************
 *                                                                           *
"This software module was originally developed by 

Martin Dietz (Fraunhofer Gesellschaft IIS)

and edited by

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
Copyright(c)1998.
 *                                                                           *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "bitfct.h"
/****************************************************************************
 ***    
 ***    TransferBitsBetweenBitBuf()
 ***    
 ***    transfers 'NoOfBits' bits from the bitstream-buffer 'in' in the
 ***    bitstream-buffer 'out', MSB first
 ***    
 ***    R. Sperschneider     FhG IIS/INEL                                          
 ***                                                                      
 ****************************************************************************/

void TransferBitsBetweenBitBuf ( BIT_BUF *in, BIT_BUF *out, unsigned long noOfBits )
{
  while ( noOfBits )
    {
      unsigned long transferBits;
      unsigned long tempCodeword;
      transferBits = MIN ( noOfBits, UINT_BITLENGTH );
      tempCodeword = ReadBitBuf ( in, transferBits );
      WriteBitBuf ( out, tempCodeword, transferBits );
      noOfBits -= transferBits;
    }
}

/****************************************************************************
 ***    
 ***    WriteBitBuf()
 ***    
 ***    writes 'no_of_bits' bits from the variable 'value' in the
 ***    bitstream-buffer 'data', MSB first
 ***    
 ***    M.Dietz     FhG IIS/INEL                                          
 ***                                                                      
 ****************************************************************************/

void WriteBitBuf( BIT_BUF *data, unsigned long value, unsigned long no_of_bits )

{
    unsigned long mask;             /* bit mask                         */

    if( no_of_bits == 0 ) 
      return;

    mask = 0x1;
    mask <<= no_of_bits - 1;

    data->bit_count += no_of_bits;

    while( no_of_bits > 0 )
      {
        while( no_of_bits > 0 && data->valid_bits < 8 ) 
	  {
            data->byte <<= 1;
            if( value & mask ) data->byte |= 0x1;
            value <<= 1;
            no_of_bits--;
            data->valid_bits++;
	  }
        if( data->valid_bits == 8 ) 
	  {
            *data->byte_ptr++ = data->byte;
            data->byte = 0;
            data->valid_bits = 0;
          }
      }
}



/****************************************************************************
 ***    
 ***    InitWriteBitBuf()
 ***    
 ***    inititalizes a bitstream buffer
 ***    
 ***    M.Dietz     FhG IIS/INEL                                          
 ***                                                                      
 ****************************************************************************/

void InitWriteBitBuf( BIT_BUF *bit_buf, unsigned char *buffer )
{
    bit_buf->byte_ptr = buffer;
    bit_buf->byte = 0;
    bit_buf->valid_bits = 0;
    bit_buf->bit_count = 0;
}


/****************************************************************************
 ***    
 ***    FlushWriteBitBuf()
 ***    
 ***    flushes the last, possibly not stored byte of a bitstream written
 ***    with writeBitBuf
 ***    
 ***    M.Dietz     FhG IIS/INEL                                          
 ***                                                                      
 ****************************************************************************/

unsigned long FlushWriteBitBuf( BIT_BUF *bit_buf )
{
    unsigned long flushbits;
    flushbits = 8 - bit_buf->bit_count % 8;
    if( flushbits < 8 ) {
      WriteBitBuf( bit_buf, 0x0, flushbits );
      bit_buf->bit_count -= flushbits; 
    }

    return bit_buf->bit_count;
}

/****************************************************************************
 ***    
 ***    GetBitCount()
 ***    
 ***    returns the number of bits written/read
 ***    
 ***    M.Dietz     FhG IIS/INEL                                          
 ***                                                                      
 ****************************************************************************/

unsigned long GetBitCount( BIT_BUF *bit_buf )
{
    return bit_buf->bit_count;
}



/****************************************************************************
 ***
 ***    InitReadBitBuf()
 ***
 ***    inititalizes a bitstream buffer for reading
 ***
 ***    M.Dietz     FhG IIS/INEL
 ***
 ****************************************************************************/

void InitReadBitBuf( BIT_BUF *bit_buf, unsigned char *buffer )
{
    bit_buf->byte_ptr   =  buffer;
    bit_buf->byte       = *buffer;
    bit_buf->valid_bits = 8;
    bit_buf->bit_count  = 0;
}



/****************************************************************************
 ***
 ***    ReadBitBuf()
 ***
 ***    reads 'no_of_bits' bits from the bitstream buffer 'data'
 ***    in the variable 'value'
 ***
 ***    M.Dietz     FhG IIS/INEL                                          
 ***                                                                      
 ****************************************************************************/

unsigned long ReadBitBuf ( BIT_BUF*        data,       /* bitstream-buffer                     */
                           unsigned long   no_of_bits) /* number of bits to read               */
{
    unsigned long mask;             /* bit mask */
    unsigned long value;

    mask  = 0x80;
    value = 0;
    if ( no_of_bits == 0 ) 
      return ( value );

    data->bit_count += no_of_bits;

    while ( no_of_bits > 0 )
      {
        while ( no_of_bits > 0 && data->valid_bits > 0 ) 
	  {
	    value <<= 1;
            if ( data->byte & mask ) 
              value |= 0x1;
            data->byte <<= 1;
            no_of_bits--;
            data->valid_bits--;
	  }
        if ( data->valid_bits == 0 ) 
	  {
            data->byte_ptr ++;
            data->byte = *data->byte_ptr;
            data->valid_bits = 8;
	  }
    }
    return ( value );
}

