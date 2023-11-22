#include <stdio.h>
#include "pico/stdlib.h"
#include <fatfs/ff.h>
#include <pico/mem_ops.h>

#include <pico/audio_i2s.h>
#include "pico/binary_info.h"
bi_decl(bi_3pins_with_names(PICO_AUDIO_I2S_DATA_PIN, "I2S DIN", PICO_AUDIO_I2S_CLOCK_PIN_BASE, "I2S BCK", PICO_AUDIO_I2S_CLOCK_PIN_BASE+1, "I2S LRCK"));

#define MINIMP3_ONLY_MP3
//#define MINIMP3_ONLY_SIMD
#define MINIMP3_NO_SIMD
#define MINIMP3_NONSTANDARD_BUT_LOGICAL
//#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include <minimp3/minimp3.h>

#define BUF_MAX 2 * 1024

struct audio_buffer_pool *init_audio() {

    static audio_format_t audio_format = {
            .sample_freq = 44100,
            .format = AUDIO_BUFFER_FORMAT_PCM_S16,
            .channel_count = 1,
    };

    static struct audio_buffer_format producer_format = {
            .format = &audio_format,
            .sample_stride = 4
    };

    struct audio_buffer_pool *producer_pool = audio_new_producer_pool(&producer_format, 3,
                                                                      PICO_AUDIO_I2S_BUFFER_SAMPLE_LENGTH); // todo correct size
    bool __unused ok;
    const struct audio_format *output_format;
    struct audio_i2s_config config = {
            .data_pin = PICO_AUDIO_I2S_DATA_PIN,
            .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
            .dma_channel = 0,
            .pio_sm = 0,
    };

    output_format = audio_i2s_setup(&audio_format, &config);
    if (!output_format) {
        panic("PicoAudio: Unable to open audio device.\n");
    }

    ok = audio_i2s_connect(producer_pool);
    assert(ok);
    audio_i2s_set_enabled(true);
    return producer_pool;
}


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

    struct audio_buffer_pool *ap = init_audio();

    unsigned char input_buf[BUF_MAX];
    unsigned char decode_buf[BUF_MAX * 2];
    mp3dec_frame_info_t info;
    int16_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
    int file_size = 0;
    int skip = 0;
    int buf_size = 0;
    UINT read_size;

    int count = 0;
    int errors = 0;

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
                count++;
                int samples = mp3dec_decode_frame(&mp3d, decode_buf + skip, buf_size - skip, pcm, &info);
                if (samples == 0) {
                    errors++;
                    printf("Decode Error. Frame: %d, error count: %d\n", count, errors);
                } else if(samples == MINIMP3_MAX_SAMPLES_PER_FRAME) {
                    struct audio_buffer *buffer = take_audio_buffer(ap, false);
                    int16_t *s = (int16_t *) buffer->buffer->bytes;
                    buffer->sample_count = samples >> 1;
                    for (int i = 0; i < buffer->sample_count; ++i) {
                        s[i] = pcm[i << 1] * 390;
                    }
                    give_audio_buffer(ap, buffer);
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