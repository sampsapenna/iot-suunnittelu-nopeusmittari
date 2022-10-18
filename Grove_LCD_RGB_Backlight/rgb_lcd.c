/*
    rgb_lcd.c
    2013 Copyright (c) Seeed Technology Inc.  All right reserved.
    Author:Loovee
    2013-9-18
    add rgb backlight fucnction @ 2013-10-15
    The MIT License (MIT)
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.1  USA

    Additionally code re-written to port

    rgb_lcd.c – C port for raspberry pico
    2022 Copyright (c) Sampsa Penna. All right reserved.
    Author:Sampsa Penna
    2022-10-18
    The MIT License (MIT)
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.1  USA
*/


#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>

#include "rgb_lcd.h"

// predeclaration for setReg
void setReg(struct Displaystate *disp, unsigned char reg, unsigned char dat);

// Construct a displaystate struct
struct Displaystate *rgb_lcd(void) {
    struct Displaystate *disp = malloc(sizeof(struct Displaystate));
    disp->_displayfunction = 0;
    disp->_displaycontrol = 0;
    disp->_displaymode = 0;
    disp->_initialized = 0;
    disp->_numlines = 0;
    disp->_currline = 0;
    return disp;
}

void begin(struct Displaystate *disp, uint32_t cols, uint32_t rows, uint32_t charsize) {
    disp->_displayfunction |= LCD_2LINE;

    disp->_numlines = 2;
    disp->_currline = 0;

    disp->_displaycontrol |= LCD_5x8DOTS;

    // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
    // according to datasheet, we need at least 40ms after power rises above 2.7V
    // before sending commands. Sleep for 50ms just to be sure.
    sleep_ms(50);

    // this is according to the hitachi HD44780 datasheet
    // page 45 figure 23

    // Send function set command sequence
    command(disp, LCD_FUNCTIONSET | disp->_displayfunction);
    sleep_us(4500);

    // second try
    command(disp, LCD_FUNCTIONSET | disp->_displayfunction);
    sleep_us(150);

    // third try
    command(disp, LCD_FUNCTIONSET | disp->_displayfunction);

    // finally, set # lines, font size, etc.
    command(disp, LCD_FUNCTIONSET | disp->_displayfunction);

    // turnb the display on with no cursor or blinking default
    disp->_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    display(disp);

    // clear it off
    clear(disp);

    // Initialize to default text direction (for romance languages)
    disp->_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    // set the entry mode
    command(disp, LCD_ENTRYMODESET | disp->_displaymode);

    // check rgb chip model
    int32_t ret;
    uint8_t rxdata;
    ret = i2c_read_blocking(i2c_default, RGB_ADDRESS_V5, &rxdata, 1, false);
    if (ret > 0) {
        disp->rgb_chip_addr = RGB_ADDRESS_V5;
        setReg(disp, 0x00, 0x07); // reset the chip
        sleep_us(200); // wait 200 us to complete
        setReg(disp, 0x04, 0x15); // set all led always on
    } else {
        disp->rgb_chip_addr = RGB_ADDRESS;
        // backlight init
        setReg(disp, REG_MODE1, 0);
        // set LEDs controllable by both PWM and GRPPWM registers
        setReg(disp, REG_OUTPUT, 0xFF);
        // set MODE2 values
        // 0010 0000 -> 0x20  (DMBLNK to 1, ie blinky mode)
        setReg(disp, REG_MODE2, 0x20);
    }

    setColorWhite(disp);
}

/********** high level commands, for the user! */
void clear(struct Displaystate *disp) {
    command(disp, LCD_CLEARDISPLAY);        // clear display, set cursor position to zero
    sleep_us(2000);          // this command takes a long time!
}

void home(struct Displaystate *disp) {
    command(disp, LCD_RETURNHOME);        // set cursor position to zero
    sleep_us(2000);        // this command takes a long time!
}

void setCursor(struct Displaystate *disp, uint8_t col, uint8_t row) {

    col = (row == 0 ? col | 0x80 : col | 0xc0);
    uint8_t dta[2] = {0x80, col};

    i2c_write_blocking(i2c_default, LCD_ADDRESS, &dta, 2, false);
}

// Turn the display on/off (quickly)
void noDisplay(struct Displaystate *disp) {
    disp->_displaycontrol &= ~LCD_DISPLAYON;
    command(disp, LCD_DISPLAYCONTROL | disp->_displaycontrol);
}

void display(struct Displaystate *disp) {
    disp->_displaycontrol |= LCD_DISPLAYON;
    command(disp, LCD_DISPLAYCONTROL | disp->_displaycontrol);
}

// Turns the underline cursor on/off
void noCursor(struct Displaystate *disp) {
    disp->_displaycontrol &= ~LCD_CURSORON;
    command(disp, LCD_DISPLAYCONTROL | disp->_displaycontrol);
}

void cursor(struct Displaystate *disp) {
    disp->_displaycontrol |= LCD_CURSORON;
    command(disp, LCD_DISPLAYCONTROL | disp->_displaycontrol);
}

// Turn on and off the blinking cursor
void noBlink(struct Displaystate *disp) {
    disp->_displaycontrol &= ~LCD_BLINKON;
    command(disp, LCD_DISPLAYCONTROL | disp->_displaycontrol);
}
void blink(struct Displaystate *disp) {
    disp->_displaycontrol |= LCD_BLINKON;
    command(disp, LCD_DISPLAYCONTROL | disp->_displaycontrol);
}

// These commands scroll the display without changing the RAM
void scrollDisplayLeft(struct Displaystate *disp) {
    command(disp, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void scrollDisplayRight(struct Displaystate *disp) {
    command(disp, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void leftToRight(struct Displaystate *disp) {
    disp->_displaymode |= LCD_ENTRYLEFT;
    command(disp, LCD_ENTRYMODESET | disp->_displaymode);
}

// This is for text that flows Right to Left
void rightToLeft(struct Displaystate *disp) {
    disp->_displaymode &= ~LCD_ENTRYLEFT;
    command(disp, LCD_ENTRYMODESET | disp->_displaymode);
}

// This will 'right justify' text from the cursor
void autoscroll(struct Displaystate *disp) {
    disp->_displaymode |= LCD_ENTRYSHIFTINCREMENT;
    command(disp, LCD_ENTRYMODESET | disp->_displaymode);
}

// This will 'left justify' text from the cursor
void noAutoscroll(struct Displaystate *disp) {
    disp->_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
    command(disp, LCD_ENTRYMODESET | disp->_displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void createChar(struct Displaystate *disp, uint8_t location, uint8_t charmap[]) {

    location &= 0x7; // we only have 8 locations 0-7
    command(disp, LCD_SETCGRAMADDR | (location << 3));

    uint8_t dta[9];
    dta[0] = 0x40;
    for (int i = 0; i < 8; i++) {
        dta[i + 1] = charmap[i];
    }
    i2c_write_blocking(i2c_default, LCD_ADDRESS, &dta, 9, false);
}

// Control the backlight LED blinking
void blinkLED(struct Displaystate *disp) {
    if (disp->rgb_chip_addr == RGB_ADDRESS_V5)
    {
        // attach all led to pwm1
        // blink period in seconds = (<reg 1> + 2) *0.128s
        // pwm1 on/off ratio = <reg 2> / 256
        setReg(disp, 0x04, 0x2a);  // 0010 1010
        setReg(disp, 0x01, 0x06);  // blink every second
        setReg(disp, 0x02, 0x7f);  // half on, half off
    }
    else
    {
        // blink period in seconds = (<reg 7> + 1) / 24
        // on/off ratio = <reg 6> / 256
        setReg(disp, 0x07, 0x17);  // blink every second
        setReg(disp, 0x06, 0x7f);  // half on, half off
    }
    

}

void noBlinkLED(struct Displaystate *disp) {
    if (disp->rgb_chip_addr == RGB_ADDRESS_V5)
    {
        setReg(disp, 0x04, 0x15);  // 0001 0101
    }
    else
    {
        setReg(disp, 0x07, 0x00);
        setReg(disp, 0x06, 0xff);
    }
}

/*********** mid level commands, for sending data/cmds */

// send command
void command(struct Displaystate *disp, uint8_t value) {
    uint8_t dta[2] = {0x80, value};
    i2c_write_blocking(i2c_default, LCD_ADDRESS, &dta, 2, false);
}

// send data
size_t write(struct Displaystate *disp, uint8_t value) {
    uint8_t dta[2] = {0x40, value};
    i2c_write_blocking(i2c_default, LCD_ADDRESS, &dta, 2, false);
    return 1; // assume success
}

void setReg(struct Displaystate *disp, uint8_t reg, uint8_t dat) {
    uint8_t dta[2] = {reg, dat};
    i2c_write_blocking(i2c_default, disp->rgb_chip_addr, &dta, 2, false);
}

void setRGB(struct Displaystate *disp, uint8_t r, uint8_t g, uint8_t b) {
    if (disp->rgb_chip_addr == RGB_ADDRESS_V5)
    {
        setReg(disp, 0x06, r);
        setReg(disp, 0x07, g);
        setReg(disp, 0x08, b);
    }
    else
    {
        setReg(disp, 0x04, r);
        setReg(disp, 0x03, g);
        setReg(disp, 0x02, b);
    }
}

void setPWM(struct Displaystate *disp, uint8_t color, uint8_t pwm) {
    switch (color)
    {
        case WHITE:
            setRGB(disp, pwm, pwm, pwm);
            break;
        case RED:
            setRGB(disp, pwm, 0, 0);
            break;
        case GREEN:
            setRGB(disp, 0, pwm, 0);
            break;
        case BLUE:
            setRGB(disp, 0, 0, pwm);
            break;
        default:
            break;
    }
}

const unsigned char color_define[4][3] = {
    {255, 255, 255},            // white
    {255, 0, 0},                // red
    {0, 255, 0},                // green
    {0, 0, 255},                // blue
};

void setColor(struct Displaystate *disp, unsigned char color) {
    if (color > 3) {
        return ;
    }
    setRGB(disp, color_define[color][0], color_define[color][1], color_define[color][2]);
}
