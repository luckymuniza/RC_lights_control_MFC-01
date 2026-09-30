//
// Created by all on 2. 9. 2026.
//

#include "beacon.h"
#include "ibus.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"



#define BEACON_OUT_PIN 11
#define FRONT_BEACON_OUT_PWM_PIN 27
#define BEACON_SW_LEVEL 1600
#define BEACON_PWM_ON_LEVEL 1800
#define BEACON_PWM_OFF_LEVEL 1200

static uint pwm_chan_num;
static uint pwm_slice_num;


void beacon_init(void) {
    gpio_set_function_masked((1<<BEACON_OUT_PIN),GPIO_FUNC_SIO);
    gpio_set_dir_out_masked((1<<BEACON_OUT_PIN));
    gpio_put_masked((1<<BEACON_OUT_PIN),0);

    //init PWM for front beacon
    gpio_set_function_masked((1<<FRONT_BEACON_OUT_PWM_PIN), GPIO_FUNC_PWM);
    pwm_slice_num = pwm_gpio_to_slice_num(FRONT_BEACON_OUT_PWM_PIN);
    pwm_chan_num = pwm_gpio_to_channel(FRONT_BEACON_OUT_PWM_PIN);

    pwm_config c = pwm_get_default_config();
    // servo PWM,
    //for PWM 50Hz: f = fsys/((19999+1) x clkdiv) = 125000000/((19999+1) x 125)
    //period = 20ms -> 20000
    //range 1 - 2ms -> 1000 - 2000
    pwm_config_set_clkdiv_int(&c, 125);
    pwm_config_set_clkdiv_mode(&c, PWM_DIV_FREE_RUNNING);
    pwm_config_set_wrap(&c, 19999);
    pwm_config_set_output_polarity(&c,false,false);
    pwm_init(pwm_slice_num, &c, true);
    pwm_set_chan_level(pwm_slice_num, pwm_chan_num, BEACON_PWM_OFF_LEVEL); //low



}

void beacon_service(void) {
    if (ibus_get_channel(IBUS_CHAN_BEACON_SW) > BEACON_SW_LEVEL) {
        //beacon on
        gpio_put(BEACON_OUT_PIN,1);
        pwm_set_chan_level(pwm_slice_num, pwm_chan_num, BEACON_PWM_ON_LEVEL); //high
    }
    else {
        //beacon_off
        gpio_put(BEACON_OUT_PIN,0);
        pwm_set_chan_level(pwm_slice_num, pwm_chan_num, BEACON_PWM_OFF_LEVEL); //low
    }
}