#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#define LED0 28
#define LED1 27

void vBlinkTask0() {
   for (;;) {
      gpio_put(LED0, 1);
      vTaskDelay(250);
      gpio_put(LED0, 0);
      vTaskDelay(250);
   }
}

void vBlinkTask1() {
   for (;;) {
      gpio_put(LED1, 1);
      vTaskDelay(500);
      gpio_put(LED1, 0);
      vTaskDelay(500);
   }
}

void main() {
   gpio_init(LED0);
   gpio_set_dir(LED0, GPIO_OUT);

   gpio_init(LED1);
   gpio_set_dir(LED1, GPIO_OUT);

   xTaskCreate(vBlinkTask0, "Blink Task 0", 128, NULL, 1, NULL);
   xTaskCreate(vBlinkTask1, "Blink Task 1", 128, NULL, 1, NULL);

   vTaskStartScheduler();
}


