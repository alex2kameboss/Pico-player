#include <stdio.h>
#include "pico/stdlib.h"
#include <fatfs/ff.h>
#include <pico/mem_ops.h>

#include <pico/audio_i2s.h>
#include "pico/binary_info.h"
bi_decl(bi_3pins_with_names(PICO_AUDIO_I2S_DATA_PIN, "I2S DIN", PICO_AUDIO_I2S_CLOCK_PIN_BASE, "I2S BCK", PICO_AUDIO_I2S_CLOCK_PIN_BASE+1, "I2S LRCK"));

#include <mp3_decoder/mp3_decoder.h>
#include <string.h>

#define BUF_MAX 2 * 1024

struct audio_buffer_pool *init_audio() {

    static audio_format_t audio_format = {
            .sample_freq = 44100,
            .format = AUDIO_BUFFER_FORMAT_PCM_S16,
            .channel_count = 2
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

    struct audio_buffer_pool *ap = init_audio();
    int16_t buf_s16[1152];
    uint8_t m_inBuff[1600];
    

    unsigned char input_buf[BUF_MAX];
    unsigned char decode_buf[BUF_MAX * 2];
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
                printf("Skip: %d\n", skip);
                int next_sync = MP3FindSyncWord(decode_buf + skip, buf_size - skip);
                if (next_sync == -1) {
                    printf("Syncword not found\n");
                    skip = BUF_MAX;
                } else if (next_sync == 0) {
                    int bytesLeft = buf_size - skip;
                    printf("Decode frame\n");
                    int ret = MP3Decode(decode_buf + skip, &bytesLeft, buf_s16, 0);
                    if (buf_size - skip == bytesLeft) {
                        printf("Miss syncword\n");
                        skip += 2; // miss sync bytes, skipt 2 bytes and search again
                    } else {
                        skip += buf_size - skip - bytesLeft;

                        if (ret == 0) {
                            int play = 1152;
                            while (play > 0) {
                                printf("Create buf %d\n", play);
                                audio_buffer_t *buffer = take_audio_buffer(ap, true);
                                int16_t* samples = (int16_t*) buffer->buffer->bytes;
                                int16_t* buf = buf_s16 + (1152 - play);

                                if (play >= buffer->max_sample_count) {
                                    buffer->sample_count = buffer->max_sample_count;
                                    play -= buffer->max_sample_count;
                                } else {
                                    buffer->sample_count = play;
                                    play = 0;
                                }


                                for (int i = 0; i < buffer->sample_count; i++) {
                                    samples[i*2+0] = buf[i*2+0];
                                    samples[i*2+1] = buf[i*2+1];
                                }

                                printf("Play buf\n");
                                give_audio_buffer(ap, buffer);
                            }
                        } else {
                            printf("Decode error: %d\n", ret);
                        }
                    }
                } else {
                    printf("Skip frame\n");
                    skip += next_sync;
                }
            
                //printf("samples: %d, frame: %d\n", samples, info.frame_bytes);
                if (skip >= BUF_MAX) {
                    printf("Reach buf limit\n");
                    memcpy(decode_buf, decode_buf + BUF_MAX, BUF_MAX);
                    skip -= BUF_MAX;
                    buf_size -= BUF_MAX;
                    break;
                }
            }
        }

        file_size += read_size;
        if (read_size < BUF_MAX) {
            if (f_eof(&fil)) {
                printf("EOF reach\n");
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