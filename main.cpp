#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// for SD
#include <fatfs/ff.h>
FATFS fs;

// for MP3
#include <mp3_decoder/mp3_decoder.h>
#include <cstring>
#define BUF_MAX 2 * 1024

// for I2S
#include <pico/audio_i2s.h>
#include "pico/binary_info.h"
bi_decl(bi_3pins_with_names(PICO_AUDIO_I2S_DATA_PIN, "I2S DIN", PICO_AUDIO_I2S_CLOCK_PIN_BASE, "I2S BCK", PICO_AUDIO_I2S_CLOCK_PIN_BASE + 1, "I2S LRCK"));
static struct audio_buffer_pool *ap = NULL;

// qeuues
QueueHandle_t sdToMp3Data, sdToMp3Size, mp3ToI2s;

static TaskHandle_t sdTask = NULL, mp3Task = NULL, i2sTask = NULL;

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

void init() {
    // for SD
    printf("Start init SD card\n");
    FRESULT fr;
    // mount sd card
    fr = f_mount(&fs, "", 1);

    if (fr != FR_OK) {
        printf("Mount error %d\n", fr);
    }
    printf("Mount ok\n");
    printf("Stop init SD card\n");

    // for MP3
    printf("Start init MP3\n");
    ap = init_audio();
    printf("Stop init MP3\n");

    // for I2S
    printf("Start init I2S\n");
    printf("Stop init I2S\n");
}

static void sd(void *pvParameters) {
    unsigned char input_buf[BUF_MAX];
    int file_size = 0;

    FIL fil;
    FRESULT fr;
    UINT read_size;

    fr = f_open(&fil, "test.mp3", FA_READ);

    while (true) {
        printf("Read data\n");
        f_read(&fil, input_buf, BUF_MAX, &read_size);

        // TODO: send data to mp3 deccode
        printf("Read size send: %d\n", read_size);
        printf("Send free spaces: %d\n", uxQueueSpacesAvailable(sdToMp3Size));
        printf("Send read size: %d\n", xQueueSend(sdToMp3Size, &read_size, portMAX_DELAY));
        printf("Send read data: %d\n", xQueueSend(sdToMp3Data, input_buf, portMAX_DELAY));
        printf("Send free spaces after: %d\n", uxQueueSpacesAvailable(sdToMp3Size));

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
}

static void mp3(void *pvParameters) {
    unsigned char decode_buf[BUF_MAX * 2];
    int16_t buf_s16[1152];
    int skip = 0;
    int buf_size = 0;
    unsigned char input_buf[BUF_MAX];
    UINT read_size;

    while (true) {
        // TODO: get buffer
        printf("Wait data\n");
        printf("Recv size: %d\n", xQueueReceive(sdToMp3Size, &read_size, portMAX_DELAY));
        printf("Recv data: %d\n", xQueueReceive(sdToMp3Data, input_buf, portMAX_DELAY));
        printf("Read size rcv: %d\n", read_size);
        printf("Rcv free spaces: %d\n", uxQueueSpacesAvailable(sdToMp3Size));

        if (buf_size == 0) {
            memcpy(decode_buf, input_buf, read_size);
            buf_size += read_size;
        } else {
            if (read_size > 0) {
                memcpy(decode_buf + BUF_MAX, input_buf, read_size);
                buf_size += read_size;
            }
            while (true) {
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
                            // TODO: sent buf_s16 to i2s
                            xQueueSend(mp3ToI2s, &buf_s16, portMAX_DELAY);
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
    }

}

static void i2s(void *pvParameters) {
    int16_t buf_s16[1152];
    while (true) {
        // TODO: get pcm16 buffer
        xQueueReceive(mp3ToI2s, &buf_s16, portMAX_DELAY);

        int play = 1152;
        while (play > 0) {
            // printf("Create buf %d\n", play);
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
                samples[i*2+0] = buf[i*2+0] / 4;
                samples[i*2+1] = buf[i*2+1] / 4;
            }

            // printf("Play buf\n");
            give_audio_buffer(ap, buffer);
        }
    }
}

int main() {
    // start uart
    stdio_init_all();

    sleep_ms(5000);
    printf("pico Player\n");

    // init
    init();

    // create queues
    sdToMp3Data = xQueueCreate(2, BUF_MAX);
    sdToMp3Size = xQueueCreate(2, sizeof(UINT));
    mp3ToI2s = xQueueCreate(2, 1152 * 2);

    // create tasks
    xTaskCreate(sd, "SD", 5 * BUF_MAX, NULL, 31, &sdTask);
    xTaskCreate(mp3, "MP3", 6 * BUF_MAX, NULL, 31, &mp3Task);
    xTaskCreate(i2s, "I2S", 1152 * 4, NULL, 31, &i2sTask);

    // start OS
    vTaskStartScheduler();

    return 0;
}
