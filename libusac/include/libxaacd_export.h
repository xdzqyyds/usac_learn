#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BUILDING_LIBXAACDEC 
#define LIBXAACDEC_API __declspec(dllexport)
#else 
#define LIBXAACDEC_API __declspec(dllimport)
#endif


    LIBXAACDEC_API typedef struct {
        FILE* pf_out;
        char output_path[512];
        signed int bitrate;
        signed int samp_freq;
        signed int pcm_wd_sz;
        signed int num_chan;
        signed int sbr_flag;
        signed int mps_flag;
        signed int Super_frame_mode;
    } decode_para;

    LIBXAACDEC_API typedef struct {
        void* pv_ia_process_api_obj;
        decode_para* decode_para;
        signed int ui_inp_size;
        signed int i_buff_size;
        signed int i_bytes_consumed;
        signed char* pb_inp_buf;
        signed char* pb_out_buf;
    } decode_obj;


    //LIBXAACDEC_API typedef struct {
    //    unsigned int capacity;           // 超级帧大小
    //    unsigned char* delayed_buf;        // 上个超级帧中最后的边界索引
    //    unsigned int delayed_bytes;      // 上个超级帧中最后的边界索引到边界的字节数
    //} superframe_decoder_t;


    LIBXAACDEC_API decode_obj* xheaacd_create(decode_para* decode_para);

    LIBXAACDEC_API int xheaacd_decode_frame(decode_obj* pv_decode_obj, void* audioframe, int ixheaacd_i_bytes_to_read);

    //LIBXAACDEC_API superframe_decoder_t* create_superframe_decoder(unsigned int super_frame_bytes);

    //LIBXAACDEC_API void* decode_superframe(void* super_frame, superframe_decoder_t* decoder, unsigned int* p_total_size, unsigned int* p_frame_sizes);

    LIBXAACDEC_API signed int write_header(FILE* fp, signed int pcmbytes, signed int freq, signed int channels, signed int bits, signed int i_channel_mask);


#ifdef __cplusplus
}
#endif
