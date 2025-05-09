#ifndef _CLASSIFICATION_H_
#define _CLASSIFICATION_H_

#define FD_MODE		2
#define TD_MODE		0

typedef struct 
{
  int    coding_mode;		/* coding mode of the frame */
  int    pre_mode;		/* coding mode of the previous frame */
  
  float  input_samples[3840 * 2];
  int    n_buffer_samples;  
  int    class_buf[10];  
  int    n_buf_class;
  int    n_class_frames;
  
  int    isSwitchMode;		
} CLASSIFICATION;

extern CLASSIFICATION classfyData;
void init_classification(CLASSIFICATION *classfy, int  bitrate, int codecMode);
void classification (CLASSIFICATION *classfy);

#endif /* _CLASSIFICATION_H_ */
