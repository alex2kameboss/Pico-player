#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// for SD
#include <fatfs/ff.h>
FATFS fs;

// for MP3
#include <cstring>
#define BUF_MAX 2 * 1024
#define MAX_DECODE_BUF 2 * BUF_MAX + 1024
#define MAD_MAX_RESULT_BUFFER_SIZE 1152
#include <MP3DecoderMAD.h>

// for I2S
#include <pico/audio_i2s.h>
#include "pico/binary_info.h"
bi_decl(bi_3pins_with_names(PICO_AUDIO_I2S_DATA_PIN, "I2S DIN", PICO_AUDIO_I2S_CLOCK_PIN_BASE, "I2S BCK", PICO_AUDIO_I2S_CLOCK_PIN_BASE + 1, "I2S LRCK"));
static struct audio_buffer_pool *ap = NULL;

// for wifi
#include <pico/cyw43_arch.h>
#include <lwip/apps/lwiperf.h>
#include <lwip/ip4_addr.h>
#include <lwip/netif.h>
#include <lwip/sockets.h>
#include <messages/utils.h>

// qeuues
QueueHandle_t sdToMp3Data, sdToMp3Size, mp3ToI2s;

static TaskHandle_t sdTask = NULL, mp3Task = NULL, i2sTask = NULL, wifiTask = NULL;

SemaphoreHandle_t songNameSemaphore;
ResponseSongMessage responseSongMessage("Not playing!");

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
    printf("Stop init MP3\n");

    // for I2S
    printf("Start init I2S\n");
    ap = init_audio();
    printf("Stop init I2S\n");

    // for wifi
    printf("Start init WiFi\n");
    printf("Stop init WiFi\n");
}

static void sd(void *pvParameters) {
    unsigned char input_buf[BUF_MAX];
    int file_size = 0;

    FIL fil;
    FRESULT fr;
    UINT read_size;

    xSemaphoreTake(songNameSemaphore, portMAX_DELAY);
    responseSongMessage.setSongName("test.mp3");
    xSemaphoreGive(songNameSemaphore);

    fr = f_open(&fil, "test.mp3", FA_READ);

    while (true) {
        f_read(&fil, input_buf, BUF_MAX, &read_size);

        // TODO: send data to mp3 deccode
        xQueueSend(sdToMp3Size, &read_size, portMAX_DELAY);
        xQueueSend(sdToMp3Data, input_buf, portMAX_DELAY);

        file_size += read_size;
        if (read_size < BUF_MAX) {
            if (f_eof(&fil)) {
                //printf("EOF reach\n");
            } else {
                //printf("ERROR\n");
            }
            printf("File size: %f kB, %f MB\n", (float)file_size / 1024.0, (float)file_size / 1024.0 / 1024.0);
            break;
        }
    }
}

void pcmDataCallback(libmad::MadAudioInfo &info, int16_t *pwm_buffer, size_t len) {
    if (len == 1152) {
        xQueueSend(mp3ToI2s, pwm_buffer, portMAX_DELAY);
    }
}


static void mp3(void *pvParameters) {
    unsigned char decode_buf[MAX_DECODE_BUF];
    int buf_size = 0;
    unsigned char input_buf[BUF_MAX];
    UINT read_size;
    unsigned int jump;
    libmad::MP3DecoderMAD mp3(pcmDataCallback);
    mp3.begin();

    while (true) {
        xQueueReceive(sdToMp3Size, &read_size, portMAX_DELAY);
        xQueueReceive(sdToMp3Data, input_buf, portMAX_DELAY);

        if (buf_size + read_size >= MAX_DECODE_BUF) {
            jump = mp3.give_data(decode_buf, buf_size);
            if (jump == 0)
                jump = BUF_MAX;
            buf_size -= BUF_MAX;
            memcpy(decode_buf, decode_buf + jump, buf_size);
        }
        memcpy(decode_buf + buf_size, input_buf, read_size);
        buf_size += read_size;   
    }

}

static void i2s(void *pvParameters) {
    int16_t buf_s16[1152];
    while (true) {
        // TODO: get pcm16 buffer
        xQueueReceive(mp3ToI2s, &buf_s16, portMAX_DELAY);

        audio_buffer_t *buffer = take_audio_buffer(ap, true);
        int16_t* samples = (int16_t*) buffer->buffer->bytes;
        buffer->sample_count = 0;
        for (size_t i = 0; i < 1152; ++i){
            if (buffer->sample_count == buffer->max_sample_count) {
                give_audio_buffer(ap, buffer);
                buffer = take_audio_buffer(ap, true);
                samples = (int16_t*) buffer->buffer->bytes;
                buffer->sample_count = 0;
            }
            samples[i] = buf_s16[i];
            buffer->sample_count++;
        }
        give_audio_buffer(ap, buffer);
    }
}

static void wifi(void *pvParameters) {
    printf("Start wifi task\n");
    if (cyw43_arch_init())
    {
        printf("failed to initialise\n");
        return;
    }

    cyw43_arch_enable_sta_mode();
    printf("Connecting to WiFi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("failed to connect.\n");
        exit(1);
    }
    else
    {
        printf("Connected.\n");
    }
    
    int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    struct sockaddr_in listen_addr;
    listen_addr.sin_len = sizeof(struct sockaddr_in);
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(1234);
    
    if (server_sock < 0)
    {
        printf("Unable to create socket: error %d", errno);
        return;
    }

    if (bind(server_sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0)
    {
        printf("Unable to bind socket: error %d\n", errno);
        return;
    }

    if (listen(server_sock, 3) < 0)
    {
        printf("Unable to listen on socket: error %d\n", errno);
        return;
    }

    printf("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), ntohs(listen_addr.sin_port));

    while (true)
    {
        struct sockaddr_storage remote_addr;
        socklen_t len = sizeof(remote_addr);
        int conn_sock = accept(server_sock, (struct sockaddr *)&remote_addr, &len);
        if (conn_sock >= 0) {
            IMessage* clientMessage = r(conn_sock);

            if (clientMessage->getType() == MsgId::REQ_SONG) {
                xSemaphoreTake(songNameSemaphore, portMAX_DELAY);
                s(conn_sock, &responseSongMessage);
                xSemaphoreGive(songNameSemaphore);
            }

            close(conn_sock);
            delete clientMessage;
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
    xTaskCreate(sd, "SD", 2 * BUF_MAX + 256, NULL, 30, &sdTask);
    xTaskCreate(mp3, "MP3", MAX_DECODE_BUF + BUF_MAX + 1024 + 1152 + 256, NULL, 30, &mp3Task);
    xTaskCreate(i2s, "I2S", 1152 * 2 + 256, NULL, 31, &i2sTask);
    xTaskCreate(wifi, "WiFi", 300, NULL, 30, &wifiTask);
    // create semaphore
    songNameSemaphore = xSemaphoreCreateMutex();

    // start OS
    vTaskStartScheduler();

    return 0;
}
