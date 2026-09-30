
        #include <stdio.h>
        #include "pico/stdlib.h"
        #include <time.h>
        #include "ibus.h"
        #include"lights.h"
        #include "turn_light.h"
        #include "to_mfc.h"
        #include "beacon.h"

        #define DBG


#define LED_PIN 25




int main(void) {
    stdout_uart_init();
//    stdio_usb_init();
    printf("Starting...\n");
    gpio_set_function_masked((1<<LED_PIN),GPIO_FUNC_SIO);

    //pre AUX

    gpio_set_dir_out_masked((1<<LED_PIN));

    gpio_put(LED_PIN,1);
    busy_wait_ms(500);
    gpio_put(LED_PIN,0);
    busy_wait_ms(500);
    gpio_put(LED_PIN,1);
    busy_wait_ms(500);
    gpio_put(LED_PIN,0);
    busy_wait_ms(500);
    gpio_put(LED_PIN,1);


    //init iBUS
    ibus_init();
    while (!ibus_data_valid()) {
        ibus_service();
    }
    to_mfc_init();
    lights_init();
    turn_light_init();
    beacon_init();

    while(1) {
        ibus_service();

        //****************************************
        //              lights + rear light
        //****************************************
        light_service();

        //****************************************
        //              turn_lights
        //****************************************
        turn_light_service();

        //****************************************
        //              beacons
        //****************************************
        beacon_service();

    } //while 1

} //main
