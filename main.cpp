#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#define LED0 28
#define BTN0 27

static TaskHandle_t xTask1 = NULL, xTask2 = NULL;

static void vBlinkTask0(void *pvParameters ) {
   bool state = true;
   for (;;) {
      printf("Start waiting, state = %d\n", state);
      gpio_put(LED0, state);
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      state = !state;
      printf("Got notification, state = %d\n", state);
   }
}

static void vReadButton(void *pvParameters) {
   bool last_state = true;
   for (;;) {
      bool btn = gpio_get(BTN0);
      //printf("Button value %d\n", btn);
      if(last_state == true && btn == false) {
         //printf("State change, notify\n");
         xTaskNotifyGive(xTask2);
      }
      last_state = btn;
      vTaskDelay(250);
   }
}

int main() {
   // start uart
   stdio_init_all();

   gpio_init(LED0);
   gpio_set_dir(LED0, GPIO_OUT);

   gpio_init(BTN0);
   gpio_set_dir(BTN0, GPIO_IN);
   gpio_pull_up(BTN0);

   xTaskCreate(vReadButton, "Read button", 200, NULL, tskIDLE_PRIORITY, &xTask1);
   xTaskCreate(vBlinkTask0, "Blink Task 0", 200, NULL, tskIDLE_PRIORITY, &xTask2);

   vTaskStartScheduler();

   return 0;
}


