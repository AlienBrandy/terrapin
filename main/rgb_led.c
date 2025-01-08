/**
 * rgb_led.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include "rgb_led.h"
#include "driver/gpio.h"
#include "hal/spi_types.h"
#include "driver/spi_master.h"
#include "console_windows.h"

static spi_device_handle_t spi;

bool rgb_led_init(void)
{
    spi_bus_config_t buscfg =
    {
        .miso_io_num = GPIO_NUM_NC,     // not connected
        .mosi_io_num = GPIO_NUM_48,     // RGB LED connection on ESP32-S3-DevKitC-1
        .sclk_io_num = GPIO_NUM_NC,     // not connected
        .quadwp_io_num = GPIO_NUM_NC,   // not connected
        .quadhd_io_num = GPIO_NUM_NC,   // not connected
        .max_transfer_sz = 25 + 9 + 25  // in bytes; 9 for data, (2x) 25 for reset code
    };

    spi_device_interface_config_t devcfg = 
    {
        .clock_speed_hz = 2500000,      // Clock out max 833Hz symbol period, each symbol comprised of three bits
        .mode = 0,                      // SPI mode 0: CPOL:-0 and CPHA:-0
        .spics_io_num = GPIO_NUM_NC,    // CS not connected
        .queue_size = 1,                // queue size for transactions
    };

    // Initialize the SPI bus
    if (spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO) != ESP_OK)
    {
        return false;
    }

    // Attach slave device to bus
    if (spi_bus_add_device(SPI2_HOST, &devcfg, &spi) != ESP_OK)
    {
        return false;
    }

    return true;
}

bool rgb_led_write(uint32_t RGB)
{
    #define RESET_BYTES_START 25
    #define RESET_BYTES_END 25
    #define DATA_BYTES 9
    #define TX_BYTES (RESET_BYTES_START + DATA_BYTES + RESET_BYTES_END)

    // unpack rgb value into component colors and store in bits [31:24]
    uint32_t R = (RGB & 0xFF0000) << 8;
    uint32_t G = (RGB & 0xFF00) << 16;
    uint32_t B = (RGB & 0xFF) << 24;

    // convert color value into SK68XX command format, in which each bit of the 
    // the color value is expanded into a 3-bit code. Result is stored in bits [23:0]
    uint32_t zero_bitcode = 0b100;
    uint32_t one_bitcode  = 0b110;
    uint32_t mask = 1 << 24;
    for (int i = 0; i < 8; i++)
    {
        R += (R & mask) ? one_bitcode : zero_bitcode;
        G += (G & mask) ? one_bitcode : zero_bitcode;
        B += (B & mask) ? one_bitcode : zero_bitcode;
        mask <<= 1;
        one_bitcode <<= 3;
        zero_bitcode <<= 3;
    }

    // build the Tx buffer, starting with enough zeros to send a
    // 80us min reset pulse, then 9 bytes of data, then a final
    // reset pulse.
    static uint8_t buf[TX_BYTES];
    uint8_t* pdata = buf + RESET_BYTES_START;
    pdata[0] = (G & 0xFF0000) >> 16;
    pdata[1] = (G & 0xFF00) >> 8;
    pdata[2] = (G & 0xFF);
    pdata[3] = (R & 0xFF0000) >> 16;
    pdata[4] = (R & 0xFF00) >> 8;
    pdata[5] = (R & 0xFF);
    pdata[6] = (B & 0xFF0000) >> 16;
    pdata[7] = (B & 0xFF00) >> 8;
    pdata[8] = (B & 0xFF);

    spi_transaction_t trans_desc =
    {
        .tx_buffer = buf,
        .length = TX_BYTES * 8,       // data length, in bits
    };

    // send the Tx buffer
    if (spi_device_polling_transmit(spi, &trans_desc) != ESP_OK)
    {
        return false;
    }

    return true;
}


