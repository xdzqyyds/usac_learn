/**********************************************************************

todo: add disclaimer

**********************************************************************/
#ifndef _sls_h_
#define _sls_h_

/* ----------------------------------------------------------
   SLS module interface
  ----------------------------------------------------------- */

/* #include "block.h" */

/* --- Init AAC-decoder --- */
/* HANDLE_SLSDECODER SLSDecodeInit (void); */

/* ----- Decode one frame with the SLS-decoder  ----- */
int SLSDecodeFrame ( 
                    /* HANDLE_SLSDECODER hDec */
                    HANDLE_AACDECODER        hDec,
                    TF_DATA*                 tfData,
                    double*                  spectral_line_vector[MAX_TIME_CHANNELS], 
                    WINDOW_SEQUENCE          windowSequence[MAX_TIME_CHANNELS],
                    WINDOW_SHAPE             window_shape[MAX_TIME_CHANNELS],
                    byte                     max_sfb[Winds],
                    int                      numChannels,
                    int                      commonWindow,
                    int                      msMask[8][60],
                    FRAME_DATA*              fd,
                    HANDLE_DECODER_GENERAL   hFault,
                    int                      layer
                    );

/* void SLSDecodeFree (HANDLE_SLSDECODER hDec); */

void slsAUDecode (
                  int numChannels,
                  FRAME_DATA* frameData,
                  TF_DATA* tfData,
                  HANDLE_DECODER_GENERAL hFault,
                  enum AAC_BIT_STREAM_TYPE bitStreamType
                  );


/* void decodeBlockType( int coded_types, int granules, int gran_block_type[], int act_gran ); */

/* TNS_frame_info GetTnsData( void *paacDec, int index ); */
/* int  GetTnsDataPresent( void *paacDec, int index ); */
/* void SetTnsDataPresent( void *paacDec, int index, int value ); */
/* int  GetTnsExtPresent(  void *paacDec, int index ); */
/* void SetTnsExtPresent(  void *paacDec, int index, int value ); */

/* int GetBlockSize( void *paacDec ); */
/* long UpdateBufferFullness( void* paacDec, Float consumedBits, int decoderBufferSize, int incomingBits); */


#endif
