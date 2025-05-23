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
        signed int bitrate;
        signed int samp_freq;
        signed int pcm_wd_sz;
        signed int num_chan;
        signed int sbr_flag;
        signed int mps_flag;
        signed int Super_frame_mode;
    } decode_para;

    LIBXAACDEC_API typedef struct decode_obj_ decode_obj;

    LIBXAACDEC_API decode_obj* xheaacd_create(decode_para* decode_para);
    
    LIBXAACDEC_API int xheaacd_decode_frame(decode_obj* ctx, void* audioframe, int i_bytes_to_read);

    LIBXAACDEC_API int xheaacd_delete(decode_obj* ctx);


#ifdef __cplusplus
}
#endif
