#include <stdio.h>
#include "pico/stdlib.h"

int main() {
    // start uart
    stdio_init_all();
    
    while(true) {
        printf("SD Test!\n");
        sleep_ms(1000);
    }


    return 0;
}