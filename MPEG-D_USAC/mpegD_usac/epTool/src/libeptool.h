#ifndef __LIBEPTOOL_H__
#define __LIBEPTOOL_H__

/* opaque declaration of EP Config structure */
struct  tagEpConfig;
typedef struct tagEpConfig   *HANDLE_EP_CONFIG;
typedef struct tagClassFrame *HANDLE_CLASS_FRAME;

typedef struct tagClassFrame {
  int             maxClasses;
  long            maxBufferSize;
  int             nrValidBuffer;  /* number of valid classBuffers); 
                                     range: 0...MAX_NR_OF_CLASSES */
  int             choiceOfPred;   /* choice of predefinedSet, for dynamic switching, -1 means automagic selection */
  unsigned char **classBuffer;    /* class buffers  
                                     classBuffer[MAX__NR_OF_CLASSES][MAX_BUFFER_SIZE]*/
  long           *classLength;    /* number of valid bits in 
                                     each classBuffer 
                                     classLength[MAX_NR_OF_CLASSES] */
  long           *classCodeRate;  /* SRCPC code rate */
  long           *classCRCLength; /* CRC length for each class */
  short          *crcError;       /* CRC status for each 
                                     classBuffer (0: OK, 1: ERROR)
                                     classLength[MAX_NR_OF_CLASSES] */
} T_CLASS_FRAME;

/* function prototypes */
int EPTool_CreateExpl ( HANDLE_EP_CONFIG *hEpConfig, 
                        const char *predFileName,
                        unsigned char **configBuffer, /* output buffer containing
                                                           ErrorProtectionSpecificConfig() */
                        long *configLength,  /* corresponding length in bit */
                        int reorderFlag,
                        int bitFlag /* if (bitflag) in- and output in bits and *not* in bytes */
                        );

/* encode one frame
   if the concatenation tool does not yet have enough frames buffered to create
   an output epFrame, a "2" will be returned and outLength set to zero!

   return values:
   ==0 means everything went fine, new frame successfully encoded
   ==2 means the concatenation tool is waiting for more frames, no output yet!
   anything else means error!
*/
int EPTool_EncodeFrame( HANDLE_EP_CONFIG hEpConfig, 
                        HANDLE_CLASS_FRAME inBuffer, 
                        unsigned char *outBuffer, 
                        int *outLength , /* number of bits */
                        int bitFlag /* if (bitflag) in- and output in bits and *not* in bytes */
                        );

/* decode and get one frame
   return number of frames left from concatenation or negative number for error

   to repeat: 
    < 0  means error
    > 0  means the next frame has to be accessed with EPTool_GetDecodedFrame()
    ==0  means the next frame has to be accessed with EPTool_DecodeFrame()
*/
int EPTool_DecodeFrame( HANDLE_EP_CONFIG hEpConfig, 
                        HANDLE_CLASS_FRAME outBuffer, 
                        unsigned char *inBuffer, 
                        int inLength, /* number of bits */
                        int bitFlag /* if (bitflag) in- and output in bits and *not* in bytes */
                        ); 

/* get the next buffer in case multiple frames were concatenated
   return number of frames left from concatenation or negative number for error

   to repeat: 
    < 0  means error
    > 0  means the next frame has to be accessed with EPTool_GetDecodedFrame()
    ==0  means the next frame has to be accessed with EPTool_DecodeFrame()
*/
int EPTool_GetDecodedFrame(HANDLE_EP_CONFIG hEpConfig,
                           HANDLE_CLASS_FRAME outBuffer,
                           int bitFlag);

HANDLE_CLASS_FRAME EPTool_GetClassFrame ( HANDLE_EP_CONFIG hEpConfig, 
                                          int bitFlag /* if (bitflag) in- and output in bits and *not* in bytes */
                                          );

int EPTool_CalculateOverhead( HANDLE_EP_CONFIG hEpConfig, 
                              HANDLE_CLASS_FRAME inBuffer, 
                              int *outLength /* number of bits */
                              );

int EPTool_Destroy( HANDLE_EP_CONFIG hEpConfig );

int EPTool_CreateBufferEPSpecificConfig( unsigned char **outbuffer, 
                                         const char *predFileName
                                         );

int EPTool_DestroyBufferEPSpecificConfig(unsigned char **outBuffer);

void EPTool_GetClassLength(HANDLE_EP_CONFIG self, int choiceOfPred, unsigned int* class_length);

/* select a predefinition set in the configuration */
int EPTool_selectPredSet_byClasses(HANDLE_EP_CONFIG hEpConfig,
                                   int num,
                                   const unsigned int class_lengths[],
                                   int *choiceOfPred,
                                   int *choiceOfTransPred);
int EPTool_selectPredSet_forClassFrame(HANDLE_EP_CONFIG hEpConfig,
                                       HANDLE_CLASS_FRAME buffer,
                                       int *choiceOfPred,
                                       int *choiceOfTransPred);
int EPTool_selectPredSet_byLength(HANDLE_EP_CONFIG hEpConfig,
                                  int length,
                                  int *choiceOfPred,
                                  int *choiceOfTransPred);

/* return the maximum number of classes in the configuration */
int EPTool_GetMaximumNumberOfClasses(HANDLE_EP_CONFIG hEpConfig);


#endif /*__LIBEPTOOL_H__ */
