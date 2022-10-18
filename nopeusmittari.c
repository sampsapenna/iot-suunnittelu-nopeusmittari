#include "pico/stdlib.h"

#ifndef OPTOENCODER_GPIO_PIN
#define OPTOENCODER_GPIO_PIN 22
#endif

// Rim length in millimeters, distance traveled with single rotation
// defaults to 700x28c/28-622 = (635 + 28 + 28) * 3.14159 ≈ 2170
// Define to length per rotation
#ifndef LENGTH_PER_ROTATION
#define LENGTH_PER_ROTATION 2170
#endif

// Pulses per rotation, for different types of encoding disks
#ifndef PULSES_PER_ROTATION
#define PULSES_PER_ROTATION 1
#endif


int setup() {
#ifndef PICO_DEFAULT_LED_PIN
#warning blink example requires a board with a regular LED
#else
    // set internal led as debug output for pulse
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
#endif

    // set GP22 as input for optoencoder pulse
    gpio_init(OPTOENCODER_GPIO_PIN);
    gpio_set_dir(OPTOENCODER_GPIO_PIN, GPIO_IN);
    gpio_set_pulls(OPTOENCODER_GPIO_PIN, false, false);
    gpio_set_input_hysteresis_enabled(OPTOENCODER_GPIO_PIN, true);

    return 0;
}

int loop() {
    int opto_status = 0;
    int elapsed_time=0;
    bool last_tick_opto_status=false;
    float speed_ms=0;


    while(1) {
        opto_status = gpio_get(OPTOENCODER_GPIO_PIN);

        gpio_put(PICO_DEFAULT_LED_PIN, opto_status > 0 ? true : false);

        if (opto_status){
            if (!last_tick_opto_status){
                last_tick_opto_status=true;
                speed_ms=LENGTH_PER_ROTATION/elapsed_time;
                elapsed_time=0;
            }
        }else{
            last_tick_opto_status=false;
        }


        sleep_ms(1);
        elapsed_time+=1;
    }
    return 0;
}

int main() {
    int ret;
    ret = setup();
    // In case we add failure conditions after setup, handle them after
    // label "failover"
    if (ret) goto failover;
    return loop();
failover:
    return ret;
}
