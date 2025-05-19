#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "libxaace_export.h"
#include "libxaacd_export.h"
#include "libsuperframe_export.h"


long find_data_chunk(FILE* wav_file) {
    char chunk_id[5] = { 0 };
    uint32_t chunk_size;
    if (!wav_file) return -1;



    fseek(wav_file, 12, SEEK_SET);  // Skip RIFF header (12 bytes)

    while (fread(chunk_id, 1, 4, wav_file) == 4) {
        fread(&chunk_size, sizeof(uint32_t), 1, wav_file);

        if (strncmp(chunk_id, "data", 4) == 0) {
            return ftell(wav_file);
        }
        else {
            fseek(wav_file, chunk_size, SEEK_CUR);
        }
    }

    return -1;
}




#define AUDIO_BITRATE        64000
#define AUDIO_SAMPLE_RATE    44100
#define AUDIO_CHANNELS       2
#define AUDIO_PCM_WIDTH      16
#define AUDIO_SBR_FLAG       1
#define AUDIO_MPS_FLAG       0
#define AUDIO_SUPERFRAME     400

#define Handle_frame_length  256*AUDIO_CHANNELS*AUDIO_PCM_WIDTH 
#define Super_frame_length   AUDIO_BITRATE*AUDIO_SUPERFRAME/8000



int main() {
    const char* input_wav_path = "input.wav";
    const char* encoded_wav_path = "encoded.mp4";
    FILE* input_wav_file = fopen(input_wav_path, "rb");
    FILE* encoded_wav_file = NULL;
    encode_obj* ctx;
    encode_para enc_para;
    unsigned char* encoded_data = NULL;
    int encoded_size = 0;

    unsigned char* buffer = (unsigned char*)malloc(Handle_frame_length);
    int i = 0;
    long data_offset = find_data_chunk(input_wav_file);
    superframe_encoder_t* super_ctx;
    uint8_t* superframe_output = (uint8_t*)malloc(Super_frame_length);


    memset(&enc_para, 0, sizeof(encode_para));
    enc_para.bitrate = AUDIO_BITRATE;
    enc_para.ui_pcm_wd_sz = AUDIO_PCM_WIDTH;
    enc_para.ui_samp_freq = AUDIO_SAMPLE_RATE;
    enc_para.ui_num_chan = AUDIO_CHANNELS;
    enc_para.sbr_flag = AUDIO_SBR_FLAG;
    enc_para.mps_flag = AUDIO_MPS_FLAG;
    enc_para.Super_frame_mode = AUDIO_SUPERFRAME;
    enc_para.input_file_flag = 0;
    enc_para.output_file_flag = 1;
    if (enc_para.output_file_flag) {
        encoded_wav_file = fopen(encoded_wav_path, "wb");
    }
    ctx = xheaace_create(&enc_para);
    super_ctx = create_superframe_encoder(Super_frame_length);



    decode_para dec_para;
    memset(&dec_para, 0, sizeof(decode_para));
    dec_para.bitrate = AUDIO_BITRATE;
    dec_para.pcm_wd_sz = AUDIO_PCM_WIDTH;
    dec_para.samp_freq = AUDIO_SAMPLE_RATE;
    dec_para.num_chan = AUDIO_CHANNELS;
    dec_para.sbr_flag = AUDIO_SBR_FLAG;
    dec_para.mps_flag = AUDIO_MPS_FLAG;
    dec_para.Super_frame_mode = AUDIO_SUPERFRAME;
    strcpy(dec_para.output_path, "audio_frame_decoded_output.wav");
    dec_para.pf_out = fopen(dec_para.output_path, "wb");
    superframe_decoder_t* super_decoder = create_superframe_decoder(Super_frame_length);
    decode_obj* decoder = (decode_obj*)xheaacd_create(&dec_para);
    if (!decoder) {
        printf("xheaacd_create error\n");
        return 1;
    }

    int mm = 0;

    fseek(input_wav_file, data_offset, SEEK_SET);

    while (i < 600) {
        fread(buffer, 1, Handle_frame_length, input_wav_file);
        if (xheaace_encode_frame(ctx, buffer, &encoded_data, &encoded_size) == 1) {
            if (encoded_size > 0) {
                xheaacd_decode_frame(decoder, encoded_data, encoded_size);
            }

            //if (encode_superframe(super_ctx, encoded_data, encoded_size, superframe_output) == 1) {
            //    fprintf(stdout, "superframe successful\n");
            //    fflush(stdout);
            //    uint32_t total_size = 0;
            //    uint32_t frame_sizes[17] = { 0 };
            //    uint32_t offset = 0;
            //    uint32_t i = 0;
            //    /*decode one superframe into multiple audio frames*/
            //    uint8_t* audio_frame = (uint8_t*)decode_superframe(superframe_output, super_decoder, &total_size, frame_sizes);
            //    int fff = 0;
            //    
            //    //uint8_t* pcm_buffer = (uint8_t*)malloc(Handle_frame_length * 50);
            //    //uint32_t pcm_offset = 0;
            //    while (offset < total_size && i < 17) {
            //        if (frame_sizes[i] > 0) {
            //            /*decoded audio frame*/
            //            xheaacd_decode_frame(decoder, audio_frame + offset, frame_sizes[i]);
            //            /*audio player, The input data is "decoder->pb_out_buf" ,the size is "Handle_frame_length"*/
            //        }
            //        offset += frame_sizes[i];
            //        i++;
            //    }
            //}
        }
        fprintf(stdout, "\rframe %4d\n", xheaace_get_frame_count(ctx));
        fflush(stdout);
        i++;
    }

    free(encoded_data);
    free(superframe_output);
    xheaace_delete(ctx, encoded_wav_path);
    destroy_superframe_encoder(super_ctx);
    fclose(input_wav_file);
    if (enc_para.output_file_flag) {
        fclose(encoded_wav_file);
    }
    free(buffer);
    return 0;
}