/**
 * rgb_led.h
 * 
 * device logic for the SK68XX tricolor LED
 * 
 * The SK68XX is an intelligent three-color LED module. There are separate light elements for
 * red, green, and blue, each of which can be set to a different intensity from 0=off to
 * 255=full brightness. Communication with the module is via a unipolar RZ (return-to-zero)
 * single-wire serial interface.
 * 
 * In the unipolar RZ protocol, a ONE is transmitted as a long high pulse followed by a shorter
 * low pulse, and a ZERO is transmitted as a short high pulse followed by a longer low pulse.
 * The module is self-timed; there is only a single transmission line for data with no clock.
 * 
 * This module uses the data line of an SPI interface to communicate with the LED. A command
 * packet is constructed by first packing the three RGB intensity values into 24 bits. Each bit
 * is then encoded for transmission by expanding it into three SPI data bits. A value of one is
 * encoded as b110 and a zero is encoded as b100. Zeroes are appended to the start and end of
 * the packet to generate a reset condition. The module changes state as soon as it receives
 * the second reset code.
 *
 *  <---zero code--->     <---one code---->      <---reset code--->
 *   _____                  __________         __                  __
 *  |     |                |          |          |                |
 *  |  1  |  0    0        |  1    1  |  0       |      >80 us    |
 * _|     |__________     _|          |_____     |________________|
 * 
 * SPDX-FileCopyrightText: Copyright © 2025 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#pragma once

#include <stdint.h>

/**
 * @brief initialize the communication interface with the RGB LED.
 * 
 * Call this function once at startup before attempting to write to the LED.
 * 
 * @returns true if the LED was successfully initialized, false otherwise
 */
bool rgb_led_init(void);

/**
 * @brief write a new RGB value to the LED.
 * 
 * The RGB value is a 24-bit integer with the red, green, and blue components packed into
 * the least significant bytes of the unsigned int. The LED will immediately change to the
 * new color when this function is called.
 * 
 * @param RGB the new color value.
 * @returns true if the LED was successfully written, false otherwise.
 */
bool rgb_led_write(uint32_t RGB);
