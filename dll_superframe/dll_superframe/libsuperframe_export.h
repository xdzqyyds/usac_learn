#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BUILDING_LIBSUPERFRAME
#define LIBSUPERFRAME_API __declspec(dllexport)
#else
#define LIBSUPERFRAME_API __declspec(dllimport)
#endif

    LIBSUPERFRAME_API typedef struct {
        uint8_t* data;              // 超级帧数据缓冲区
        uint32_t capacity;          // ( header + payload + directory ) 由比特率*(200ms或400ms)计算
        uint32_t filled;            // payload中已填充字节数
        uint32_t frame_count;       // 当前帧数量
        uint32_t* frame_borders;    // border index 位置数组
        uint8_t reservoir_level;    // 比特储备级别
        bool last_delayed_border;   // 上个超级帧是否发生填充机制
        bool now_delayed_border;    // 本次超级帧是否发生填充机制
        uint32_t last_filled_bytes; // 上个超帧中最后一帧填充的字节数
        uint8_t* delayed_buf;       // 上个超级帧中最后一帧填充后偏移后的数据指针
        uint32_t delayed_bytes;     // 上个超级帧中最后一帧填充后剩余数据大小
    } superframe_encoder_t;

    LIBSUPERFRAME_API typedef struct {
        uint32_t capacity;           // 超级帧大小
        uint8_t* delayed_buf;        // 上个超级帧中最后的边界索引
        uint32_t delayed_bytes;      // 上个超级帧中最后的边界索引到边界的字节数

    } superframe_decoder_t;

    LIBSUPERFRAME_API superframe_encoder_t* create_superframe_encoder(uint32_t super_frame_bytes);

    LIBSUPERFRAME_API void destroy_superframe_encoder(superframe_encoder_t* sf);

    LIBSUPERFRAME_API int encode_superframe(superframe_encoder_t* sf, uint8_t* pb_out_buf, int i_out_bytes, uint8_t* superframe_output);

    LIBSUPERFRAME_API superframe_decoder_t* create_superframe_decoder(uint32_t super_frame_bytes);

    LIBSUPERFRAME_API void destroy_superframe_decoder(superframe_decoder_t* sd);

    LIBSUPERFRAME_API void* decode_superframe(void* super_frame, superframe_decoder_t* decoder, uint32_t* p_total_size, uint32_t* p_frame_sizes);


#ifdef __cplusplus
}
#endif
