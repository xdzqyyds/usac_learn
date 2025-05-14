#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int bitrate;            // 编码码率(推荐20000)
    int ui_pcm_wd_sz;       // pcm格式(pcm_16le)
    int ui_samp_freq;       // 采样率
    int ui_num_chan;        // ͨ通道数量
    int sbr_flag;           // 是否使用SBR(固定开启)
    int mps_flag;           // 是否使用MPS(根据其他参数自动开启)
    int Super_frame_mode;   // 超级帧格式（400ms 或 200ms）
    int input_file_flag;    // 开启内部读取文件的函数，封装后不再使用，始终为0
    int output_file_flag;   // 开启内部写入文件的函数，输出压缩后的.mp4文件
} encode_para;

typedef struct {
    /*** 控制参数 ***/
    int sbrenable;
    int numChannel;
    int frameNumSample;
    int downSampleRatio;
    int upSampleRatio;
    int delayTotal;
    int NcoefRS;
    int NcoefUS;
    int delayBufferSamples;
    int addlCoreDelay;
    int delayEncFrames;
    int numAPRframes;
    int frame;
    int requestIPF;
    int enableIPF;
    int numChannelBS;
    int track_count_total;
    int err;
    int nSamplesProcessed;
    int input_file_flag;
    int output_file_flag;

    /*** 滤波器指针 ***/
    float* hRS;
    float* hUS;

    /*** 通道缓冲区 ***/
    float** sampleBuf; 
    float** sampleBufRS;
    float** sampleBufRSold;
    float** sampleBufUS;
    float** sampleBufUSold;
    float** additionalCoreDelayBuffer;
    float** tmpBufRS;
    float** tmpBufUS;
    float** encSampleBuf;
    float** reSampleBuf;

    /*** 编码帧数据与输出 ***/
    ENC_FRAME_DATA frameData;
    HANDLE_BSBITBUFFER* au_buffers;
    HANDLE_STREAMPROG outprog;

    /*** 文件与采样信息 ***/
    AudioFile* audioFile;
    HANDLE_STREAMFILE outfile;
    long fSampleLong;
} encode_obj;



encode_obj* xheaace_create(encode_para* encode_para);

int xheaace_encode_frame(encode_obj* encode_obj, const unsigned char* raw_pcm_frame);

int xheaace_delete(encode_obj* ctx, char* encoded_file);


#ifdef __cplusplus
}
#endif