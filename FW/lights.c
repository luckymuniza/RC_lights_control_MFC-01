//
// Created by all on 16. 12. 2025.
//

#include "lights.h"
#include "pico/stdlib.h"
#include "ibus.h"
#include "stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "stdio.h"

#define REAR_LIGHT_OUT_PWM_PIN 8
#define REAR_LIGHT_IN_PIN 26  //ADC0
#define HEAD_LIGHTS_OUT_PIN 12
#define SIDE_LIGHTS_OUT_PIN 10

//input levels for brake lights
//#define REAR_LIGHT_OFF_LEVEL 3000
//#define REAR_LIGHT_ON_LEVEL 1500
#define REAR_LIGHT_BRAKE_LEVEL 980    //945 brzdy,   1020 svetla

#define REAR_LIGHT_OFF_PWM_LEVEL 0 //0-2000
#define REAR_LIGHT_ON_PWM_LEVEL 500 //0-2000
#define REAR_LIGHT_BRAKE_PWM_LEVEL 2000 //0-2000

#define LIGHTS_SW_LEVEL 1600 // switch value for lights ON/OFF

uint pwm_slice_num_rearlight;  //slice
uint pwm_chan_num_rearlight;  //channel A/B

bool rear_light_is_on = false;
bool brake_light_is_on = false;

static inline void my_adc_start_conversion(void) {
    hw_set_bits(&adc_hw->cs, ADC_CS_START_ONCE_BITS);
}


//lights and brake lights.
inline void rear_light_on (void){
    if (brake_light_is_on == false) {
        pwm_set_chan_level(pwm_slice_num_rearlight, pwm_chan_num_rearlight, REAR_LIGHT_ON_PWM_LEVEL); //ON
    }
    rear_light_is_on = true;

}
inline void rear_light_off (void){
    if (brake_light_is_on == false) {
        pwm_set_chan_level(pwm_slice_num_rearlight, pwm_chan_num_rearlight, REAR_LIGHT_OFF_PWM_LEVEL); //OFF
    }
    rear_light_is_on = false;
}
static void rear_light_brake_on (void){
    pwm_set_chan_level(pwm_slice_num_rearlight, pwm_chan_num_rearlight, REAR_LIGHT_BRAKE_PWM_LEVEL); //Brakes ON
    brake_light_is_on = true;
}
static void rear_light_brake_off (void){
    if (rear_light_is_on) {
        pwm_set_chan_level(pwm_slice_num_rearlight, pwm_chan_num_rearlight, REAR_LIGHT_ON_PWM_LEVEL); //Rear lifghts ON
    }
    else {
        pwm_set_chan_level(pwm_slice_num_rearlight, pwm_chan_num_rearlight, REAR_LIGHT_OFF_PWM_LEVEL); //rear lights OFF
    }
    brake_light_is_on = false;


}



//Moving average /32
static uint32_t my_adc_get_data(void) {
    static uint16_t buf[32] = {0};
    static uint8_t idx = 0; //kde sa ma prave zapisat
    static uint32_t sum = 0;

    if (adc_hw->cs & ADC_CS_READY_BITS) {
        sum -= buf[idx];
        buf[idx] = (uint16_t ) adc_hw->result;;
        sum += buf[idx++];
        if (idx == 32) {idx =0;}
        my_adc_start_conversion();
    }
    return sum >> 5;
}


void lights_init(void) {
    //side and head lights
    gpio_set_function_masked((1<<HEAD_LIGHTS_OUT_PIN) | (1<<SIDE_LIGHTS_OUT_PIN),GPIO_FUNC_SIO);
    gpio_set_dir_out_masked((1<<HEAD_LIGHTS_OUT_PIN) | (1<<SIDE_LIGHTS_OUT_PIN));
    gpio_put_masked((1<<HEAD_LIGHTS_OUT_PIN) | (1<<SIDE_LIGHTS_OUT_PIN),0);


    //init PWM pins2
    gpio_set_function_masked(
        (1<<REAR_LIGHT_OUT_PWM_PIN), GPIO_FUNC_PWM);
    pwm_slice_num_rearlight = pwm_gpio_to_slice_num(REAR_LIGHT_OUT_PWM_PIN);
    pwm_chan_num_rearlight = pwm_gpio_to_channel(REAR_LIGHT_OUT_PWM_PIN);
    pwm_config c = pwm_get_default_config();
    //zadne a brzdove
    //pre PWM 500Hz: f = fsys/((1999+1) x clkdiv) = 125000000/((1999+1) x 125)
    //perioda = 2ms -> 2000
    //rozsah 0 - 2ms -> 0 - 2000
    pwm_config_set_clkdiv_int(&c, 125);
    pwm_config_set_clkdiv_mode(&c, PWM_DIV_FREE_RUNNING);
    pwm_config_set_wrap(&c, 1999);
    pwm_config_set_output_polarity(&c,false,false);
    pwm_init(pwm_slice_num_rearlight, &c, true);
    rear_light_off(); //vypnute

    //ADC init for rearlight
    adc_init();
    adc_gpio_init(REAR_LIGHT_IN_PIN);
    adc_select_input(0);
    my_adc_start_conversion();
}



void light_service(void) {
    static uint32_t last_val;
    //brakes lights service
    uint32_t tmp = my_adc_get_data();
    if (tmp != last_val) {
        last_val = tmp;
        //printf("Rear Light: %u\n", (uint) tmp);
    }
    if (tmp < REAR_LIGHT_BRAKE_LEVEL) {
        //brakes on
        rear_light_brake_on();
    }
    else {
        //brakes off
        rear_light_brake_off();
    }
    //lights service

    if (ibus_get_channel(IBUS_CHAN_LIGHTS_SW) > LIGHTS_SW_LEVEL) {
        //lights on
        rear_light_on();
        gpio_put(HEAD_LIGHTS_OUT_PIN,1);
        gpio_put(SIDE_LIGHTS_OUT_PIN,1);
    }
    else {
        //lights off
        rear_light_off();
        gpio_put(HEAD_LIGHTS_OUT_PIN,0);
        gpio_put(SIDE_LIGHTS_OUT_PIN,0);
    }
}
