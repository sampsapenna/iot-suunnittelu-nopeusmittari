#include "pico/stdlib.h"

#ifndef OPTOENCODER_GPIO_PIN
#define OPTOENCODER_GPIO_PIN 22
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

    return 0;
}

int loop() {
    int opto_status = 0;
    while(1) {
        opto_status = gpio_get(OPTOENCODER_GPIO_PIN);

        gpio_put(PICO_DEFAULT_LED_PIN, opto_status > 0 ? true : false);

        sleep_us(100);
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
