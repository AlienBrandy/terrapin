/**
 * ansi_term.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ansi_term.h"
#include <stdio.h>
#include <stdarg.h>
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "min_max.h"

/**
 * @brief uart settings for console I/O
 */
#define CONSOLE_UART 0
#define CONSOLE_UART_BAUDRATE 115200

bool ansi_term_init(void)
{
    // Drain stdout before reconfiguring it
    fflush(stdout);
    fsync(fileno(stdout));

    // Disable buffering on stdin and stdout
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    // Minicom, screen, idf_monitor send CR when ENTER key is pressed
    esp_vfs_dev_uart_port_set_rx_line_endings(CONSOLE_UART, ESP_LINE_ENDINGS_CR);

    // Move the caret to the beginning of the next line on '\n'
    esp_vfs_dev_uart_port_set_tx_line_endings(CONSOLE_UART, ESP_LINE_ENDINGS_CRLF);

    // Install UART driver for interrupt-driven reads and writes
    esp_err_t esp_err = uart_driver_install(CONSOLE_UART, 256, 0, 0, NULL, 0);
    if (esp_err != ESP_OK)
    {
        ESP_LOGE(PROJECT_NAME, "uart_driver_install() failed: %s", esp_err_to_name(esp_err));
        return false;
    }

    // Configure UART.
    // Note that REF_TICK is used so that the baud rate remains
    // correct while APB frequency is changing in light sleep mode.
    const uart_config_t uart_config = {
            .baud_rate = CONSOLE_UART_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
    #if SOC_UART_SUPPORT_REF_TICK
            .source_clk = UART_SCLK_REF_TICK,
    #elif SOC_UART_SUPPORT_XTAL_CLK
            .source_clk = UART_SCLK_XTAL,
    #endif
    };
    esp_err = uart_param_config(CONSOLE_UART, &uart_config);
    if (esp_err != ESP_OK)
    {
        ESP_LOGE(PROJECT_NAME, "uart_param_config() failed: %s", esp_err_to_name(esp_err));
        return false;
    }

    // Tell VFS to use UART driver
    // This means reads/writes will grab a mutex, fill the uart ring buffer,
    // then enable uart interrupts to service the buffer and communicate directly with the peripheral registers
    esp_vfs_dev_uart_use_driver(CONSOLE_UART);

    ESP_LOGI(PROJECT_NAME, "Terminal initialized");
    return true;
}

bool ansi_term_get_terminal_size(int* rows, int* cols)
{
    int save_row, save_col;

    // store the current cursor location to restore later
    if (!ansi_term_get_cursor_pos(&save_row, &save_col))
    {
        return false;
    }

    // move cursor to lower right
    static const char ESC_MOVE_LOWER_RIGHT[] = "\x1b[999C\x1b[999B";
    if (write(STDOUT_FILENO, ESC_MOVE_LOWER_RIGHT, sizeof(ESC_MOVE_LOWER_RIGHT)) == -1)
    {
        return false;
    }

    // get new cursor position
    if (!ansi_term_get_cursor_pos(rows, cols))
    {
        return false;
    }

    // restore original cursor position
    ansi_term_set_cursor_pos(save_row, save_col);

    return true;
}

bool ansi_term_get_cursor_pos(int* rows, int* cols)
{
    *rows = -1;
    *cols = -1;

    // Report cursor location
    static const char ANSI_TERM_GET_CURSOR_POS[] = "\x1b[6n";
    int nchars = write(STDOUT_FILENO, ANSI_TERM_GET_CURSOR_POS, sizeof(ANSI_TERM_GET_CURSOR_POS));
    if (nchars == -1)
    {
        return false;
    }

    // Read the response: ESC [ rows ; cols R
    char buf[32];
    unsigned int i = 0;
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, buf + i, 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    // Parse it
    if (buf[0] != '\x1b' || buf[1] != '[')
    {
        return false;
    }
    if (sscanf(buf + 2, "%d;%d", rows, cols) != 2)
    {
        return false;
    }

    return true;
}

void ansi_term_set_cursor_pos(int row, int col)
{
    char buf[20];
    int nbytes = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", row, col);
    write(STDOUT_FILENO, buf, nbytes);
}

void ansi_term_set_scroll_region(int top, int bottom)
{
    char buf[20];
    int nbytes = snprintf(buf, sizeof(buf), "\x1b[%d;%dr", top, bottom);
    write(STDOUT_FILENO, buf, nbytes);
}

void ansi_term_erase_screen(void)
{
    static const char ANSI_ESC_ERASE[] = "\x1b[2J";
    write(STDOUT_FILENO, ANSI_ESC_ERASE, sizeof(ANSI_ESC_ERASE));
}

void ansi_term_set_attributes(ANSI_TERM_COLOR_T color, uint8_t attribute)
{
    char buf[20];
    int nbytes = snprintf(buf, sizeof(buf), "\x1b[%d;%dm", attribute, color);
    write(STDOUT_FILENO, buf, nbytes);
}

void ansi_term_reset_attributes(void)
{
    static const char ANSI_ESC_RESET_ATTRIBUTES[] = "\x1b[0m";
    write(STDOUT_FILENO, ANSI_ESC_RESET_ATTRIBUTES, sizeof(ANSI_ESC_RESET_ATTRIBUTES));
}

void ansi_term_hide_cursor(void)
{
    static const char ANSI_ESC_HIDE_CURSOR[] = "\x1b[?25l";
    write(STDOUT_FILENO, ANSI_ESC_HIDE_CURSOR, sizeof(ANSI_ESC_HIDE_CURSOR));
}

void ansi_term_show_cursor(void)
{
    static const char ANSI_ESC_SHOW_CURSOR[] = "\x1b[?25h";
    write(STDOUT_FILENO, ANSI_ESC_SHOW_CURSOR, sizeof(ANSI_ESC_SHOW_CURSOR));
}
