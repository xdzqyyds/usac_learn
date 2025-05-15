#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BUILDING_LIBXAACENC
#define LIBXAACENC_API __declspec(dllexport)
#else
#define LIBXAACENC_API __declspec(dllimport)
#endif

    LIBXAACENC_API typedef struct {
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


    LIBXAACENC_API typedef struct encode_obj_ encode_obj;

    LIBXAACENC_API encode_obj* xheaace_create(encode_para* encode_para);

    LIBXAACENC_API int xheaace_encode_frame(encode_obj* encode_obj, const unsigned char* raw_pcm_frame, unsigned char** out_encoded_data, int* out_encoded_size);

    LIBXAACENC_API int xheaace_delete(encode_obj* ctx, char* encoded_file);


#ifdef __cplusplus
}
#endif