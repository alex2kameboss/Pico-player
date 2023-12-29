#include <stdio.h>
#include "pico/stdlib.h"
#include <fatfs/ff.h>
#include <pico/mem_ops.h>

#include <pico/audio_i2s.h>
#include "pico/binary_info.h"
bi_decl(bi_3pins_with_names(PICO_AUDIO_I2S_DATA_PIN, "I2S DIN", PICO_AUDIO_I2S_CLOCK_PIN_BASE, "I2S BCK", PICO_AUDIO_I2S_CLOCK_PIN_BASE+1, "I2S LRCK"));

#define MAD_MAX_RESULT_BUFFER_SIZE 1152
#include <MP3DecoderMAD.h>

#include <string.h>


#define BUF_MAX 2 * 1024

using namespace libmad;

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

struct audio_buffer_pool *ap = NULL;

void pcmDataCallback(MadAudioInfo &info, int16_t *pwm_buffer, size_t len) {
    printf("New data: %d\n", len);
    audio_buffer_t *buffer = take_audio_buffer(ap, true);
    int16_t* samples = (int16_t*) buffer->buffer->bytes;
    buffer->sample_count = 0;
    for (size_t i=0; i<len; i++){
        if (buffer->sample_count == buffer->max_sample_count) {
            printf("Play buffer: %d\n", buffer->sample_count);
            give_audio_buffer(ap, buffer);
            buffer = take_audio_buffer(ap, true);
            samples = (int16_t*) buffer->buffer->bytes;
            buffer->sample_count = 0;
        }
        samples[i] = pwm_buffer[i];
        buffer->sample_count++;
    }
    printf("Play buffer: %d\n", buffer->sample_count);
    give_audio_buffer(ap, buffer);
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

    ap = init_audio();
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

    MP3DecoderMAD mp3(pcmDataCallback);
    mp3.begin();

    while (true) {
        f_read(&fil, input_buf, BUF_MAX, &read_size);

        if (buf_size + read_size > BUF_MAX) {
            int jump = mp3.write(decode_buf, buf_size);
            if (jump == 0)
                jump = BUF_MAX;
            buf_size -= BUF_MAX;
            memcpy(decode_buf, decode_buf + jump, buf_size);

        }
        memcpy(decode_buf + buf_size, input_buf, read_size);
        buf_size += read_size;

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