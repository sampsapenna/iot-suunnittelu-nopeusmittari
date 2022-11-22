#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "rgb_lcd.h"
#include "DHT20.h"

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

#define I2C0_SDA 8
#define I2C0_SCL 9

struct Displaystate *disp = NULL;
DHT20 *sens;

const char speedo[12] = "Speedometer";
const char starting[12] = "starting...";
const char speed_str[8] = "Speed: ";
char cur[5] = "0.00";
const char kmh[5] = "km/h";
char th[10] = "0.00 00.0";

bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

int setup() {
    // Set up stdio
    stdio_init_all();
    sleep_ms(5000);
    printf("Running controller setup.\n");

#ifndef PICO_DEFAULT_LED_PIN
#warning blink example requires a board with a regular LED
#else
    // set internal led as debug output for pulse
    printf("Adding default led pin as pulse flag.\n");
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    printf("Added default led pin as pulse flag.\n");
#endif

    printf("Initializing optoencoder input pin.\n");
    // set GP22 as input for optoencoder pulse
    gpio_init(OPTOENCODER_GPIO_PIN);
    gpio_set_dir(OPTOENCODER_GPIO_PIN, GPIO_IN);
    gpio_set_pulls(OPTOENCODER_GPIO_PIN, false, false);
    printf("Enabling optoencoder pin input hysteresis.\n");
    gpio_set_input_hysteresis_enabled(OPTOENCODER_GPIO_PIN, true);
    printf("Initialized optoencoder input pin.\n");

    printf("Setting up i2c\n");
    // setup i2c
    i2c_init(i2c0, 100 * 1000);
    gpio_set_function(I2C0_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA);
    gpio_pull_up(I2C0_SCL);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(I2C0_SDA, I2C0_SCL, GPIO_FUNC_I2C));
    printf("Set up i2c.\n");

    // Setup Grove LCD monitor
    printf("Create grove lcd display struct.\n");
    disp = rgb_lcd();
    printf("Initialize grove lcd display struct.\n");
    begin(disp, 16, 2);

    // Set up temp and humid sensor
    printf("Initialize DHT20.\n");
    DHT20_init(sens);
    printf("Initialized DHT20.\n");

    // Write initialization message
    printf("Initialize cursor to start.\n");
    setCursor(disp, 0, 0);

    printf("Blink display.\n");
    noDisplay(disp);
    sleep_ms(1000);
    display(disp);
    printf("Blinked display.\n");

    printf("Setting up scrolling.\n");
    leftToRight(disp);
    noAutoscroll(disp);
    printf("Set up scrolling.\n");

    printf("Writing startup message.\n");

    for (uint32_t c = 0; c < 11; c++) {
        write(disp, speedo[c]);
        sleep_ms(100);
    }

    setCursor(disp, 0, 1);
    for (uint32_t c = 0; c < 11; c++) {
        write(disp, starting[c]);
        sleep_ms(100);
    }

    sleep_ms(2000);

    clear(disp);
    setCursor(disp, 0, 0);

    printf("Speedometer initialization successful.\n");

    return 0;
}

int loop() {
    int opto_status = 0;
    int elapsed_time = 0;
    int trigger_refresh = 0;
    bool last_tick_opto_status = false;
    float speed_ms = 0;

    while(1) {
        opto_status = gpio_get(OPTOENCODER_GPIO_PIN);

        gpio_put(PICO_DEFAULT_LED_PIN, opto_status > 0 ? true : false);

        if (opto_status) {
            if (!last_tick_opto_status) {
                last_tick_opto_status = true;
                speed_ms = (float)LENGTH_PER_ROTATION / (float)PULSES_PER_ROTATION / (float)elapsed_time * 3.6;
                if (elapsed_time > 0) {
                    sprintf(cur, "%.2f", speed_ms);
                } else {
                    strcpy(cur, "0.00");
                }
                elapsed_time = 0;
                printf("Current speed is: %s\n", cur);
            }
        } else {
            last_tick_opto_status = false;
        }

        if (trigger_refresh >= 500) {
            trigger_refresh = 0;
            clear(disp);
            setCursor(disp, 0, 0);
            resetSensor(sens);
            sleep_ms(10);
            read(sens);
            sprintf(th, "%3.2f %3.1f", getTemperature(sens), getHumidity(sens));
            printf("Current temp vals are %s\n", th);
            printf("Temp bits: \n");
            printf(
                "%d %d %d %d %d %d %d \n",
                sens->_bits[0],
                sens->_bits[1],
                sens->_bits[2],
                sens->_bits[3],
                sens->_bits[4],
                sens->_bits[5],
                sens->_bits[6]
            );

            for (uint32_t c = 0; c < 7; c++) {
                write(disp, speed_str[c]);
            }

            for (uint32_t c = 0; c < 4; c++) {
                write(disp, cur[c]);
            }

            for (uint32_t c = 0; c < 4; c++) {
                write(disp, kmh[c]);
            }

            setCursor(disp, 0, 1);
            for (uint32_t c = 0; c < 10; c++) {
                write(disp, th[c]);
            }
        }

        sleep_ms(1);
        elapsed_time++;
        trigger_refresh++;
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
