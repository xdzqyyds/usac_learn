#include <stdint.h>
#include <stdbool.h>
#include "CRC.h"
#include "libsuperframe_export.h"

extern "C" {
#include "libsuperframe_export.h"
}

#define MAX_FRAMES 15

superframe_encoder_t* create_superframe_encoder(uint32_t super_frame_bytes) {
    superframe_encoder_t* sf = (superframe_encoder_t*)malloc(sizeof(superframe_encoder_t));
    if(!sf) return NULL;
    
    sf->data = (uint8_t*)malloc(super_frame_bytes);
    if(!sf->data) {
        free(sf);
        return NULL;
    }
    
    sf->frame_borders = (uint32_t*)calloc(16, sizeof(uint32_t));  // 预分配16帧空间(最多支持15帧)
    if(!sf->frame_borders) {
        free(sf->data);
        free(sf);
        return NULL;
    }

    sf->delayed_buf = (uint8_t*)malloc(super_frame_bytes);
    if (!sf->delayed_buf) {
        free(sf);
        return NULL;
    }
    sf->capacity = super_frame_bytes;
    sf->filled = 0;
    sf->frame_count = 0;
    sf->reservoir_level = 0; 
    sf->last_delayed_border = false;
    sf->now_delayed_border = false;
    sf->last_filled_bytes = 0;
    sf->delayed_bytes = 0;
    
    return sf;
}

void destroy_superframe_encoder(superframe_encoder_t* sf) {
    if(sf) {
        free(sf->data);
        free(sf->frame_borders);
        free(sf->delayed_buf);
        free(sf);
    }
}

int encode_superframe(superframe_encoder_t* sf, uint8_t* pb_out_buf, int i_out_bytes , uint8_t* superframe_output) {

    CCRC frameCRC, headerCRC;
    const int HEADER_SIZE = 2;
    const int BORDER_DESC_SIZE = 2;

    if (sf->delayed_bytes) {
        memcpy(sf->data + HEADER_SIZE + sf->filled, sf->delayed_buf, sf->delayed_bytes);
        sf->filled += sf->delayed_bytes;
        sf->delayed_bytes = 0;
    }

    //payload frame CRC16
    frameCRC.Reset(16);
    for(int i = 0; i < i_out_bytes; i++) {
        frameCRC.AddByte(pb_out_buf[i]);
    }
    uint16_t frame_crc = frameCRC.GetCRC();

    // 准备完整数据 (frame + CRC)
    uint8_t* complete_data = (uint8_t*)malloc(i_out_bytes + 2);
    if (complete_data == NULL) {
        printf("fatal error: libxaac super frame encoder: Memory allocation failed");
        return -1;
    }
    memcpy(complete_data, pb_out_buf, i_out_bytes);
    complete_data[i_out_bytes] = (frame_crc >> 8) & 0xFF;
    complete_data[i_out_bytes + 1] = frame_crc & 0xFF;
 
    uint32_t required_space = i_out_bytes + 2;
    uint32_t available_space = sf->capacity - sf->filled - (HEADER_SIZE + sf->frame_count * BORDER_DESC_SIZE);
    
    // 空间不足,填充后生成超级帧
    if (required_space + BORDER_DESC_SIZE >= available_space) {

        sf->frame_borders[sf->frame_count++] = sf->filled;

        uint32_t filled_space;
        if (available_space < 3) {
            filled_space = available_space;
            sf->now_delayed_border = true;
        } else {
            filled_space = available_space - BORDER_DESC_SIZE;
        }

        if (sf->last_delayed_border) {
                memmove(&sf->frame_borders[1], &sf->frame_borders[0], sf->frame_count * sizeof(sf->frame_borders[0]));
                sf->frame_borders[0] = 0;
                sf->frame_count++;
            }
        if (sf->now_delayed_border) sf->frame_count--;

        //build header
        uint8_t header = (sf->frame_count << 4) | (sf->reservoir_level & 0x0F);
        
        //header CRC8
        headerCRC.Reset(8);
        headerCRC.AddByte(header);
        uint8_t header_crc = headerCRC.GetCRC();

        //write header
        sf->data[0] = header;
        sf->data[1] = header_crc;

        memcpy(sf->data + HEADER_SIZE + sf->filled, complete_data, filled_space);
        sf->delayed_bytes = (i_out_bytes + 2) - filled_space;
        memcpy(sf->delayed_buf, complete_data + filled_space, sf->delayed_bytes);
        

        uint32_t dir_pos = sf->capacity - (sf->frame_count * BORDER_DESC_SIZE);
        //directory逆序写入超级帧
        for (int i = sf->frame_count - 1; i >= 0; i--) {
            uint16_t border_desc = 0;
            if(i == 0 && sf->last_delayed_border) {
                if(sf->last_filled_bytes == 2) {
                    border_desc = (0xFFE << 4) | sf->frame_count;
                } else if (sf->last_filled_bytes == 1) {
                    border_desc = (0xFFF << 4) | sf->frame_count;
                }
            } else {
                border_desc = (sf->frame_borders[i] << 4) | sf->frame_count;
            }
            sf->data[dir_pos++] = (border_desc >> 8) & 0xFF;
            sf->data[dir_pos++] = border_desc & 0xFF;
        }

        //将sf->data的内容写入superframe_output
        memcpy(superframe_output, sf->data, sf->capacity);
        free(complete_data);

        //重置编码器状态
        sf->filled = 0;
        sf->frame_count = 0;
        sf->last_delayed_border = sf->now_delayed_border;
        sf->now_delayed_border = false;
        sf->last_filled_bytes = filled_space;
        
        return 1; // 超级帧完成

    } else {
        sf->frame_borders[sf->frame_count++] = sf->filled;
        memcpy(sf->data + HEADER_SIZE + sf->filled, complete_data, i_out_bytes + 2);
        sf->filled += (i_out_bytes + 2);

        free(complete_data);
        if (sf->frame_count > (MAX_FRAMES - 1)) return -2; //超出最大帧限制

        return 0; // 继续收集帧
    }

}

superframe_decoder_t* create_superframe_decoder(uint32_t super_frame_bytes) {
    superframe_decoder_t* sd = (superframe_decoder_t*)malloc(sizeof(superframe_decoder_t));
    if(!sd) return NULL;
    
    sd->delayed_buf = (uint8_t*)malloc(super_frame_bytes);
    if(!sd->delayed_buf) {
        free(sd);
        return NULL; 
    }
    
    sd->capacity = super_frame_bytes;
    sd->delayed_bytes = 0;
    
    return sd;
}

void destroy_superframe_decoder(superframe_decoder_t* sd) {
    if(sd) {
        free(sd->delayed_buf);
        free(sd);
    }
}

void* decode_superframe(void* super_frame, superframe_decoder_t* decoder, uint32_t* p_total_size, uint32_t* p_frame_sizes) {
    const int HEADER_SIZE = 2;
    const int BORDER_DESC_SIZE = 2;
    CCRC headerCRC, frameCRC;
    uint8_t** audio_frames_crc = (uint8_t**)malloc((MAX_FRAMES + 4) * sizeof(uint8_t*));
    uint32_t audio_frame_size[17] = {0};
    uint32_t total_size = 0;
    
    uint8_t* superframe_input = (uint8_t*)super_frame;
    
    //header CRC
    uint8_t header = superframe_input[0];
    uint8_t header_crc = superframe_input[1];
    headerCRC.Reset(8);
    headerCRC.AddByte(header);
    if(header_crc != headerCRC.GetCRC()) {

    }
    uint8_t frame_count = (header >> 4) & 0x0F;
    uint8_t reservoir_level = header & 0x0F;
    
    if(frame_count > MAX_FRAMES || frame_count == 0) {
        return NULL; 
    }
    
    //collect directory
    uint32_t dir_pos = decoder->capacity - (frame_count * BORDER_DESC_SIZE);
    uint32_t* frame_borders = (uint32_t*)malloc(frame_count * sizeof(uint32_t));
    if(!frame_borders) return NULL;
    for(int i = 0; i < frame_count; i++) {
        int idx = frame_count - 1 - i; // 逆序读取
        uint16_t border_desc = (superframe_input[dir_pos + i*2] << 8) | 
                              superframe_input[dir_pos + i*2 + 1];
        frame_borders[idx] = (border_desc >> 4);
    }

    for(int i = 0; i < frame_count; i++) {
        audio_frames_crc[i] = (uint8_t*)malloc(decoder->capacity);
        if(i == 0 && (frame_borders[i] == 0xFFE || frame_borders[i] == 0xFFF)) {
            if(frame_borders[i] == 0xFFE) {
                memcpy(audio_frames_crc[i], decoder->delayed_buf, decoder->delayed_bytes-2);
                audio_frame_size[i] = decoder->delayed_bytes - 2;
                total_size += audio_frame_size[i] - 2;

                i++;
                audio_frames_crc[i] = (uint8_t*)malloc(decoder->capacity);
                memcpy(audio_frames_crc[i], decoder->delayed_buf + decoder->delayed_bytes - 2, 2);
                memcpy(audio_frames_crc[i] + 2, superframe_input + HEADER_SIZE, frame_borders[i]);
                audio_frame_size[i] = frame_borders[i] + 2;
                total_size += audio_frame_size[i] - 2;
            } else {
                memcpy(audio_frames_crc[i], decoder->delayed_buf, decoder->delayed_bytes-1);
                audio_frame_size[i] = decoder->delayed_bytes - 1;
                total_size += audio_frame_size[i] - 2;

                i++;
                audio_frames_crc[i] = (uint8_t*)malloc(decoder->capacity);
                memcpy(audio_frames_crc[i], decoder->delayed_buf + decoder->delayed_bytes - 1, 1);
                memcpy(audio_frames_crc[i] + 1, superframe_input + HEADER_SIZE, frame_borders[i]);
                audio_frame_size[i] = frame_borders[i] + 1;
                total_size += audio_frame_size[i] - 2;
            }
        } else if (i == 0) {
            memcpy(audio_frames_crc[i], decoder->delayed_buf, decoder->delayed_bytes);
            memcpy(audio_frames_crc[i] + decoder->delayed_bytes, superframe_input + HEADER_SIZE, frame_borders[i]);
            audio_frame_size[i] = frame_borders[i] + decoder->delayed_bytes;
            total_size += (audio_frame_size[i] > 2) ? (audio_frame_size[i] - 2) : 0;
        } else {
            if (frame_borders[i] - frame_borders[i - 1] <= 0) {
                memcpy(decoder->delayed_buf, superframe_input + HEADER_SIZE + frame_borders[i-1], dir_pos - (HEADER_SIZE + frame_borders[i-1]));
                decoder->delayed_bytes = dir_pos - (HEADER_SIZE + frame_borders[i-1]);
            }
            else {
                memcpy(audio_frames_crc[i], superframe_input + HEADER_SIZE + frame_borders[i - 1], frame_borders[i] - frame_borders[i - 1]);
                audio_frame_size[i] = frame_borders[i] - frame_borders[i - 1];
                total_size += audio_frame_size[i] - 2;
                if (i == frame_count - 1) {
                    memcpy(decoder->delayed_buf, superframe_input + HEADER_SIZE + frame_borders[i], dir_pos - (HEADER_SIZE + frame_borders[i]));
                    decoder->delayed_bytes = dir_pos - (HEADER_SIZE + frame_borders[i]);
                    //decoder->delayed_buf = superframe_input + HEADER_SIZE + frame_borders[i];
                }
            }
        } 
    }

    uint8_t* audio_frame = (uint8_t*)malloc(total_size);
    if (!audio_frame) {
        return NULL;
    }

    int offset = 0;

    for (int i = 0; i < frame_count ; i++) {
        if (audio_frame_size[i] == 0) continue;
        uint16_t frame_crc = (audio_frames_crc[i][audio_frame_size[i] - 2] << 8) | audio_frames_crc[i][audio_frame_size[i] -2 + 1];
        frameCRC.Reset(16);
        for (int j = 0; j < audio_frame_size[i] - 2; j++) {
            frameCRC.AddByte(audio_frames_crc[i][j]);
        }
        if (frame_crc != frameCRC.GetCRC()) {
            free(audio_frame);
            return NULL;
        }
        memcpy(audio_frame + offset, audio_frames_crc[i], audio_frame_size[i] - 2);
        offset += (audio_frame_size[i] - 2);
        p_frame_sizes[i] = audio_frame_size[i] - 2;
    }

    for (int i = 0; i < frame_count ; i++) {
        if (audio_frames_crc[i]) free(audio_frames_crc[i]);
    }
    free(audio_frames_crc);

    *p_total_size = total_size;

    return (void*)audio_frame;
}