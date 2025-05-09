/********************************************************************** 
MPEG-4 Audio VM
Bit stream module



This software module was originally developed by

Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)

and edited by

Ralph Sperschneider (Fraunhofer IIS)
Thomas Schaefer (Fraunhofer IIS)
Olaf Kaehler (Fraunhofer IIS)

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

Copyright (c) 1996, 1997, 1998.



Header file: bitstream.h

 

Required modules:
common.o                common module

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>

Changes:
04-jun-96   HP    added buffers, ASCII-header and BsGetBufferAhead()
07-jun-96   HP    use CommonWarning(), CommonExit()
10-jun-96   HP    ...
13-jun-96   HP    changed BsGetBit(), BsPutBit(), BsGetBuffer(),
                  BsGetBufferAhead(), BsPutBuffer()
26-aug-96   HP    CVS
23-oct-96   HP    free for BsOpenFileRead() info string
15-nov-96   HP    changed int to long where required
                  added BsGetBitChar(), BsGetBitShort(), BsGetBitInt()
                  improved file header handling
23-dec-96   HP    renamed mode to write in *HANDLE_BSBITSTREAM
10-jan-97   HP    added BsGetBitAhead(), BsGetSkip()
19-feb-97   HP    made internal data structures invisible
04-apr-97   BT/HP added BsGetBufferAppend()
07-max-97   BT    added BsGetBitBack()
14-mrz-98   sfr   added CreateEscInstanceData(), BsReadInfoFile(), BsGetBitEP()
20-jan-99   HP    due to the nature of some of the modifications merged
                  into this code, I disclaim any responsibility for this
                  code and/or its readability -- sorry ...
21-jan-99   HP    trying to clean up a bit ...
22-jan-99   HP    added "-" stdin/stdout support
                  variable file buffer size for streaming
12-feb-99   BT/HP updated ...
19-apr-99   HP    cleaning up some header files again :-(
14-mar-03   OK    added BsRWBitWrapper()
**********************************************************************/

/**********************************************************************

Bit data is stored LSB-justified and transmitted MSB first.

Example: 5 bit stored in unsigned long
         ("1st" is the first bit transmitted, "x" indicates unused bit)

          x  ...  x  1st 2nd 3rd 4th 5th
         MSB                         LSB

Headerless bit stream files contain N*8 bits, where N is the file size in
bytes. The last byte is padded with 0s if necessary.

Example: bit stream file containing 13 bits (file size: 2 bytes)

         1st 2nd 3rd 4th 5th 6th 7th 8th   9th 10th 11th 12th 13th 0 0 0
         MSB ....... 1st byte ...... LSB   MSB ...... 2nd byte ...... LSB

A bit stream file can have an optional ASCII header consisting of a
magic string (e.g. ".mp4") followed by '\n' and an (optional) info string.
The info string might also contain '\n'. The ASCII header is
terminated by '\0'. If no magic string is specified when opening a
bit stream file, a file without ASCII-header is read/written.

Format of a bit stream file:
[ <magic string> [<info string> '\0'] ] <bit stream>

  <string> refers to string without terminal '\0'
  <bit stream> refers to data bytes as described above for headerless files

**********************************************************************/


#ifndef _bitstream_h_
#define _bitstream_h_

/* ---------- declarations ---------- */

#include <stdio.h>       /* struct FILE */
#include "allHandles.h"
#include "buffer.h"
#include "obj_descr.h"

#define MAX_NR_OF_ESC              5
#define MAX_NR_OF_INSTANCE_PER_ESC 2
#define LONG_NUMBIT 32                /* bits in unsigned long and max number of bits to read at once */

/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif

  /* BsInit() */
  /* Init bit stream module. */

  void BsInit ( long maxReadAhead,  /* in: max num bits to be read ahead 
                                       (determines file buffer size) 
                                       (0 = default) */
                int debugLevel,     /* in: debug level
                                       0=off  1=basic  2=medium  3=full */
                int aacEOF);        /* in: AAC eof detection (default = 0) */


  /* BsOpenFileRead() */
  /* Open bit stream file for reading. */

  HANDLE_BSBITSTREAM BsOpenFileRead ( char *fileName,          /* in: file name 
                                                                  "-": stdin */
                                      char *magic,             /* in: expected magic string 
                                                                  or NULL (no ASCII header in file) */
                                      char **info );           /* out: info string 
                                                                  or NULL (no info string in file) 
                                                                  returns: 
                                                                  bit stream (handle) 
                                                                  or NULL if error */
  

  /* BsOpenFileWrite() */
  /* Open bit stream file for writing. */

  HANDLE_BSBITSTREAM BsOpenFileWrite ( char *fileName,        /* in: file name
                                                                 "-": stdout */
                                      char *magic,            /* in: magic string 
                                                                 or NULL (no ASCII header) */
                                      char *info );           /* in: info string 
                                                                 or NULL (no info string) 
                                                                 returns: 
                                                                 bit stream (handle) 
                                                                 or NULL if error */


  /* BsOpenBufferRead() */
  /* Open bit stream buffer for reading. */

  HANDLE_BSBITSTREAM BsOpenBufferRead ( HANDLE_BSBITBUFFER buffer );  /* in: bit buffer 
                                                                         returns: 
                                                                         bit stream (handle) */
  

  /* BsOpenBufferWrite() */
  /* Open bit stream buffer for writing. */
  /* (Buffer is cleared first - previous data in buffer is lost !!!) */

  HANDLE_BSBITSTREAM BsOpenBufferWrite ( HANDLE_BSBITBUFFER buffer );           /* in: bit buffer 
                                                                                 returns: 
                                                                                 bit stream (handle) */


  /* BsCurrentBit() */
  /* Get number of bits read/written from/to stream. */

  long BsCurrentBit ( HANDLE_BSBITSTREAM stream );                /* in: bit stream 
                                                                     returns: 
                                                                     number of bits read/written */


  /* BsEof() */
  /* Test if end of bit stream file occurs within the next numBit bits. */

  int BsEof ( HANDLE_BSBITSTREAM stream,         /* in: bit stream */
             long numBit );                      /* in: number of bits ahead scanned for EOF 
                                                    returns: 
                                                    0=not EOF  1=EOF */


  /* BsCloseRemove() */
  /* Close bit stream. */

  int BsCloseRemove ( HANDLE_BSBITSTREAM stream,         /* in: bit stream */
                     int remove );                       /* in: if opened with BsOpenBufferRead(): 
                                                            0 = keep buffer unchanged 
                                                            1 = remove bits read from buffer 
                                                            returns: 
                                                            0=OK  1=error */


  /* BsClose() */
  /* Close bit stream. */

  int BsClose ( HANDLE_BSBITSTREAM stream );              /* in: bit stream 
                                                             returns: 
                                                             0=OK  1=error */
  

  /* BsRWBitWrapper() */
  /* Read bits from or write bits to bit stream (depending on WriteFlag) */
  /* (Current position in bit stream is advanced !!!) */

  int BsRWBitWrapper(HANDLE_BSBITSTREAM stream,	/* in: bit stream */
                     unsigned long *data,	/* in/out: bits to read/write */
                     int numBit,		/* in: num bits to read/write */
                     int WriteFlag);		/* in: 0: read, 1: write
                                                   returns: number of bits read/written */

  /* BsGetBit() */
  /* Read bits from bit stream. */
  /* (Current position in bit stream is advanced !!!) */

  int BsGetBit ( HANDLE_BSBITSTREAM stream,      /* in: bit stream */
                 unsigned long *data,            /* out: bits read 
                                                    (may be NULL if numBit==0) */
                 int numBit );                   /* in: num bits to read 
                                                    [0..32] 
                                                    returns: 
                                                    0=OK  1=error */

  /* this function is mainly for debugging purpose, */
  /* you can call it from the debugger */
  long int BsGetBitBack ( HANDLE_BSBITSTREAM stream,     /* in: bit stream */
                         int numBit );                   /* in: num bits to read 
                                                            [0..32] 
                                                            returns: 
                                                            if numBit is positive
                                                            return the last numBits bit from stream
                                                            else
                                                            return the next -numBits from stream 
                                                            stream->currentBit is always unchanged */

  /* BsGetBitChar() */
  /* Read bits from bit stream (char). */
  /* (Current position in bit stream is advanced !!!) */

  int BsGetBitChar ( HANDLE_BSBITSTREAM stream,          /* in: bit stream */
                     unsigned char *data,                /* out: bits read 
                                                            (may be NULL if numBit==0) */
                     int numBit );                       /* in: num bits to read 
                                                            [0..8] 
                                                            returns: 
                                                            0=OK  1=error */


  /* BsGetBitShort() */
  /* Read bits from bit stream (short). */
  /* (Current position in bit stream is advanced !!!) */

  int BsGetBitShort ( HANDLE_BSBITSTREAM stream,         /* in: bit stream */
                      unsigned short *data,              /* out: bits read 
                                                            (may be NULL if numBit==0) */
                      int numBit );                      /* in: num bits to read 
                                                            [0..16] 
                                                            returns: 
                                                            0=OK  1=error */


  /* BsGetBitInt() */
  /* Read bits from bit stream (int). */
  /* (Current position in bit stream is advanced !!!) */

  int BsGetBitInt ( HANDLE_BSBITSTREAM stream,   /* in: bit stream */
                    unsigned int *data,          /* out: bits read 
                                                    (may be NULL if numBit==0) */
                    int numBit );                /* in: num bits to read 
                                                    [0..16] 
                                                    returns: 
                                                    0=OK  1=error */


  /* BsGetBitAhead() */
  /* Read bits from bit stream. */
  /* (Current position in bit stream is NOT advanced !!!) */

  int BsGetBitAhead ( HANDLE_BSBITSTREAM stream,         /* in: bit stream */
                      unsigned long *data,               /* out: bits read 
                                                           (may be NULL if numBit==0) */
                      int numBit );                      /* in: num bits to read 
                                                            [0..32] 
                                                            returns: 
                                                            0=OK  1=error */


  /* BsGetBitAheadChar() */
  /* Read bits from bit stream (char). */
  /* (Current position in bit stream is NOT advanced !!!) */

  int BsGetBitAheadChar ( HANDLE_BSBITSTREAM stream,     /* in: bit stream */
                          unsigned char *data,           /* out: bits read 
                                                            (may be NULL if numBit==0) */
                          int numBit );                  /* in: num bits to read 
                                                            [0..8] 
                                                            returns: 
                                                            0=OK  1=error */


  /* BsGetBitAheadShort() */
  /* Read bits from bit stream (short). */
  /* (Current position in bit stream is NOT advanced !!!) */

  int BsGetBitAheadShort ( HANDLE_BSBITSTREAM stream,    /* in: bit stream */
                           unsigned short *data,         /* out: bits read 
                                                            (may be NULL if numBit==0) */
                           int numBit );                 /* in: num bits to read 
                                                            [0..16] 
                                                            returns: 
                                                            0=OK  1=error */


  /* BsGetBitAheadInt() */
  /* Read bits from bit stream (int). */
  /* (Current position in bit stream is NOT advanced !!!) */

  int BsGetBitAheadInt ( HANDLE_BSBITSTREAM stream,      /* in: bit stream */
                         unsigned int *data,             /* out: bits read 
                                                            (may be NULL if numBit==0) */
                         int numBit );                   /* in: num bits to read 
                                                            [0..16] 
                                                            returns: 
                                                            0=OK  1=error */


  /* BsGetBuffer() */
  /* Read bits from bit stream to buffer. */
  /* (Current position in bit stream is advanced !!!) */
  /* (Buffer is cleared first - previous data in buffer is lost !!!) */

  int BsGetBuffer ( HANDLE_BSBITSTREAM stream,           /* in: bit stream */
                    HANDLE_BSBITBUFFER buffer,           /* out: buffer read 
                                                           (may be NULL if numBit==0) */
                    long numBit );                       /* in: num bits to read 
                                                            returns: 
                                                            0=OK  1=error */
  
  /* BsGetBufferAppend() */
  /* append bits from bit stream to buffer. */
  /* (Current position in bit stream is advanced !!!) */

  int BsGetBufferAppend ( HANDLE_BSBITSTREAM stream,     /* in: bit stream */
                          HANDLE_BSBITBUFFER buffer,     /* out: buffer read 
                                                            (may be NULL if numBit==0) */
                          int append,                    /* in: if != 0: append bits to buffer 
                                                            (don't clear buffer) */
                          long numBit );                 /* in: num bits to read 
                                                            returns: 
                                                            0=OK  1=error */


  /* BsGetBufferAhead() */
  /* Read bits ahead of current position from bit stream to buffer. */
  /* (Current position in bit stream is NOT advanced !!!) */
  /* (Buffer is cleared first - previous data in buffer is lost !!!) */

  int BsGetBufferAhead (
                        HANDLE_BSBITSTREAM stream,      /* in: bit stream */
                        HANDLE_BSBITBUFFER buffer,      /* out: buffer read 
                                                           (may be NULL if numBit==0) */
                        long numBit);                   /* in: num bits to read 
                                                           returns: 
                                                           0=OK  1=error */


  /* BsGetSkip() */
  /* Skip bits in bit stream (read). */
  /* (Current position in bit stream is advanced !!!) */

  int BsGetSkip (
                 HANDLE_BSBITSTREAM stream,     /* in: bit stream */
                 long numBit);                  /* in: num bits to skip 
                                                   returns: 
                                                   0=OK  1=error */


  /* BsPutBit() */
  /* Write bits to bit stream. */

  int BsPutBit (
                HANDLE_BSBITSTREAM stream,      /* in: bit stream */
                unsigned long data,             /* in: bits to write */
                int numBit);                    /* in: num bits to write 
                                                   [0..32] 
                                                   returns: 
                                                   0=OK  1=error */


  /* BsPutBuffer() */
  /* Write bits from buffer to bit stream. */

  int BsPutBuffer (
                   HANDLE_BSBITSTREAM stream,        /* in: bit stream */
                   HANDLE_BSBITBUFFER buffer);       /* in: buffer to write 
                                                        returns: 
                                                        0=OK  1=error */


  /* BsAllocBuffer() */
  /* Allocate bit buffer. */

  HANDLE_BSBITBUFFER BsAllocBuffer (
                                    long numBit);   /* in: buffer size in bits 
                                                       returns: 
                                                       bit buffer (handle) */


  /* BsFreeBuffer() */
  /* Free bit buffer. */

  void BsFreeBuffer (
                     HANDLE_BSBITBUFFER buffer);    /* in: bit buffer */


  /* BsBufferNumBit() */
  /* Get number of bits in buffer. */

  long BsBufferNumBit (
                       HANDLE_BSBITBUFFER buffer);  /* in: bit buffer 
                                                       returns: 
                                                       number of bits in buffer */


  /* BsClearBuffer() */
  /* Clear bit buffer (set numBit to 0). */

  void BsClearBuffer (
                      HANDLE_BSBITBUFFER buffer);  /* in: bit buffer */

  char *BsGetFileName ( HANDLE_BSBITSTREAM stream);

  void BsCheckClassBufferFullnessEPprematurely(int                            decoded_bits,
                                               const HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                               HANDLE_ESC_INSTANCE_DATA       *epInfoPtr);

  int BsCheckClassBufferFullnessEP1 ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                      HANDLE_BSBITSTREAM       stream );

  int BsCheckClassBufferFullnessEP2 ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                      HANDLE_BSBITSTREAM       stream );

  int BsGetBitEP ( CODE_TABLE                code,               /* in: code of the aac bitstream element */
                   HANDLE_ESC_INSTANCE_DATA  hEscInstanceData,   /* in: handle with info to decode EP bitstream */    
                   HANDLE_BSBITSTREAM        stream,             /* in: bit stream */
                   unsigned long*            data,               /* out: bits read 
                                                                    (may be NULL if numBit==0) */
                   int                       numBit);            /* in: num bits to read [0..32] 
                                                                    returns: 0=OK  1=error */
  
  int BsGetBufferAheadEP ( HANDLE_BSBITSTREAM     stream,          /* in: bit stream */
                           HANDLE_BSBITBUFFER     buffer,          /* out: buffer read (may be NULL if numBit==0) */
                           long                   numBit );        /* in: num bits to read */
  
  HANDLE_ESC_INSTANCE_DATA CreateEscInstanceData ( char* infoFileName, 
                                                   int   epDebugLevel );
  
  void InitAacSpecificInstanceData ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                     HANDLE_RESILIENCE        hResilience,
                                     int                      channelConfiguration,
                                     unsigned short           tnsNotUsed  );

  unsigned short GetEsc( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                         CODE_TABLE               bsElement );

  unsigned short GetEscAssignment( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                   CODE_TABLE               bsElement, 
                                   int                      assignmentScheme );

  void BsReadInfoFile ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData);

  void BsPrepareEp1Stream( HANDLE_ESC_INSTANCE_DATA hEscInstanceData, 
                           LAYER_DATA *layer, 
                           int numStreams );

  void BsPrepareScalEp1Stream( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                               LAYER_DATA *layer, 
                               int layerNumber,
                               int numStreams,
                               int firstStream );

  /* error info on frame */
  unsigned char BsGetFrameLostFlagEP ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData );

  unsigned char BsGetErrorInFrameFlagEP ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData );

  unsigned char BsGetReadErrorInFrameFlagEP ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData );

  unsigned char BsGetCrcResultInFrameFlagEP ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData );

  /* error info on instance */
  unsigned char BsGetErrorInInstanceFlagEP ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                             unsigned short           instanceOfEsc, 
                                             unsigned short           esc );

  unsigned char BsGetCrcResultInInstanceFlagEP ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                                 unsigned short           instanceOfEsc, 
                                                 unsigned short           esc );

  unsigned char BsGetReadErrorInInstanceFlagEP ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                                 unsigned short           instanceOfEsc, 
                                                 unsigned short           esc );

  /* error info on data element */
  unsigned char BsGetErrorForDataElementFlagEP ( CODE_TABLE               code,
                                                 HANDLE_ESC_INSTANCE_DATA hEscInstanceData );

  unsigned char BsGetReadErrorForDataElementFlagEP ( CODE_TABLE               code,
                                                     HANDLE_ESC_INSTANCE_DATA hEscInstanceData );

  unsigned char BsGetCrcResultForDataElementFlagEP ( CODE_TABLE               code,
                                                     HANDLE_ESC_INSTANCE_DATA hEscInstanceData );
  /* end of error info */
  int BsGetEpDebugLevel ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData );
  
  /* appended by Sony on 990616 */
  
  void BsCloseInfoFile ( HANDLE_ESC_INSTANCE_DATA );

  void BsReadInfoFileHvx ( HANDLE_ESC_INSTANCE_DATA, 
                           unsigned short * );

  /* void BsGetCrcResultInClassHvx ( HANDLE_ESC_INSTANCE_DATA ,
     unsigned short * ); */

  unsigned short BsGetNumClassHvx( HANDLE_ESC_INSTANCE_DATA );

  void BsCloseInfoFileHvx ( HANDLE_ESC_INSTANCE_DATA );

  void BsSetSynElemId (int elemID) ;

  void BsSetChannel (int chan) ;

  int  BsGetChannel (void);

  unsigned short BsGetInstanceUsed ( HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                                     unsigned short           instanceOfEsc, 
                                     unsigned short           esc  );

  int GetInstanceOfEsc_(int channel, int assignmentScheme);
  int GetInstanceOfEsc(int channel);


  /******************************************************************************/

  long BsBufferGetSize(HANDLE_BSBITBUFFER buffer);
  void BsBufferSetData(LAYER_DATA *layer, unsigned char* data, unsigned int numBits);
  unsigned char* BsBufferGetDataBegin(HANDLE_BSBITBUFFER buffer);

  long BsBufferFreeBits(HANDLE_BSBITBUFFER buffer);

  FILE *BsFileHandle(HANDLE_BSBITSTREAM stream);

  void BsManipulateSetFileHandle(FILE*              file,
                                 HANDLE_BSBITSTREAM stream);

  long BsStreamId(HANDLE_BSBITSTREAM stream);

  int BsWriteMode(HANDLE_BSBITSTREAM stream);

  void BsRemoveAllCompletelyReadBytesFromBuffer(HANDLE_BSBITSTREAM stream,
                                                HANDLE_BSBITBUFFER buffer);

  HANDLE_BSBITBUFFER BsGetBitBufferHandle(long               curBuf,
                                          HANDLE_BSBITSTREAM stream);

  long BsNumByte(HANDLE_BSBITSTREAM stream);

  void BsManipulateSetNumByte(long               numByte,
                              HANDLE_BSBITSTREAM stream);

  void BsBufferManipulateSetNumBit(long               numBit,
                                   HANDLE_BSBITBUFFER buffer);

  void BsManipulateSetCurrentBit(long               currentBit,
                                 HANDLE_BSBITSTREAM stream);

  void BsBufferManipulateSetDataElement(unsigned char      data,
                                        long               index,
                                        HANDLE_BSBITBUFFER buffer);

  HANDLE_BSBITBUFFER BsAllocPlainDirtyBuffer(unsigned char* data, 
                                             long           size);

  void BsDirtyForceRewindAndWrite(HANDLE_BSBITSTREAM stream);

  void BsDirtyDirectOnBuffer(unsigned char*     p,
                             unsigned int       byteSize,
                             int                debugLevel,
                             HANDLE_BSBITBUFFER buffer);


	void BsSetBufRPos(HANDLE_BSBITSTREAM stream, int n);	/* SAMSUNG_2005-09-30 : added	*/

#ifdef __cplusplus
}
#endif

#endif  /* _bitstream_h_ */

/* end of bitstream.h */
