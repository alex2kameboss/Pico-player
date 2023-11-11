#include <stdio.h>
#include "pico/stdlib.h"
#include <fatfs/ff.h>
#include <pico/mem_ops.h>

#define MINIMP3_ONLY_MP3
//#define MINIMP3_ONLY_SIMD
#define MINIMP3_NO_SIMD
#define MINIMP3_NONSTANDARD_BUT_LOGICAL
//#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include <minimp3/minimp3.h>

#define BUF_MAX 2 * 1024

int main() {
    // start uart
    stdio_init_all();
    
    sleep_ms(5000);

    printf("MP3 decode test\n");

    FATFS fs;
    FIL fil;
    FRESULT fr;
    UINT br;
    UINT bw;

    fr = f_mount(&fs, "", 1);

    if (fr != FR_OK) {
        printf("mount error %d\n", fr);
    }
    printf("mount ok\n");

    fr = f_open(&fil, "test.mp3", FA_READ);

    static mp3dec_t mp3d;
    mp3dec_init(&mp3d);

    unsigned char input_buf[BUF_MAX];
    unsigned char decode_buf[BUF_MAX * 2];
    mp3dec_frame_info_t info;
    short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
    int file_size = 0;
    int skip = 0;
    int buf_size = 0;
    UINT read_size;
    while (true) {
        f_read(&fil, input_buf, BUF_MAX, &read_size);

        if (buf_size == 0) {
            memcpy(decode_buf, input_buf, read_size);
            buf_size += read_size;
        } else {
            if (read_size > 0) {
                memcpy(decode_buf + BUF_MAX, input_buf, read_size);
                buf_size += read_size;
            }
            while (true) {
                int samples = mp3dec_decode_frame(&mp3d, decode_buf + skip, buf_size - skip, pcm, &info);
                if (samples == 0) {
                    printf("Decode Error\n");
                }
                //printf("samples: %d, frame: %d\n", samples, info.frame_bytes);
                if (skip >= BUF_MAX) {
                    memcpy(decode_buf, decode_buf + BUF_MAX, BUF_MAX);
                    skip -= BUF_MAX;
                    buf_size -= BUF_MAX;
                    break;
                }
                skip += info.frame_bytes;
            }
        }

        file_size += read_size;
        if (read_size < BUF_MAX) {
            if (f_eof(&fil)) {
                printf("EOF reach\n");
                while (true) {
                    int samples = mp3dec_decode_frame(&mp3d, decode_buf + skip, buf_size - skip, pcm, &info);
                    if (samples == 0) {
                        break;
                    }
                    skip += info.frame_bytes;
                }
            } else {
                printf("ERROR\n");
            }
            printf("File size: %f kB, %f MB\n", (float)file_size / 1024.0, (float)file_size / 1024.0 / 1024.0);
            break;
        }
        
    }

    f_close(&fil);

    return 0;
}