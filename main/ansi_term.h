/**
 * ansi_term.h
 * 
 * ANSI terminal control sequences.
 * This collection of methods sends ANSI escape sequences to a terminal for controlling
 * cursor position, cursor and text attributes, and scroll regions. An init function configures
 * UARTO and directs the STDIN and STDOUT filestreams to the UART. These low-level methods are
 * intended as building blocks for a terminal-based CLI.
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ANSI_COLOR_BLACK        = 30,
    ANSI_COLOR_RED          = 31,
    ANSI_COLOR_GREEN        = 32,
    ANSI_COLOR_YELLOW       = 33,
    ANSI_COLOR_BLUE         = 34,
    ANSI_COLOR_MAGENTA      = 35,
    ANSI_COLOR_CYAN         = 36,
    ANSI_COLOR_WHITE        = 37,
} ANSI_TERM_COLOR_T;

typedef enum {
    ANSI_ATTRIB_NORMAL      = 0,
    ANSI_ATTRIB_BOLD        = 1,
    ANSI_ATTRIB_DIM         = 2,
    ANSI_ATTRIB_UNDERSCORE  = 4,
    ANSI_ATTRIB_BLINK       = 5,
    ANSI_ATTRIB_REVERSE     = 7,
    ANSI_ATTRIB_HIDDEN      = 8,
} ANSI_TERM_ATTRIB_T;

typedef enum {
    ANSI_CURSOR_DEFAULT     = 0, // Default
    ANSI_CURSOR_BBLOCK      = 1, // Block (blinking)
    ANSI_CURSOR_BLOCK       = 2, //	Block (steady)
    ANSI_CURSOR_BUNDERLINE  = 3, // Underline (blinking)
    ANSI_CURSOR_UNDERLINE   = 4, // Underline (steady)
    ANSI_CURSOR_BBAR        = 5, //	Bar (blinking)
    ANSI_CURSOR_BAR         = 6, //	Bar (steady)
} ANSI_TERM_CURSOR_STYLE_T;

/**
 * @brief configures STDOUT and STDIN filestreams to use UART0.
 * 
 * The UART settings are hard-coded to 8/N/1, 115200 baud.
 */
bool ansi_term_init(void);

/**
 * @brief retrieve the current terminal size.
 * 
 * @param rows receives the number of rows
 * @param cols receives the number of colums
 * @returns true if the terminal size was successfully received, false if there was an error
 * and the rows, cols parameters were not updated.
 */
bool ansi_term_get_terminal_size(int* rows, int* cols);

/**
 * @brief retrieve the current cursor position.
 * 
 * @param rows receives the current row
 * @param cols receives the current column
 * @returns true if the cursor position was successfully received, false if there was an error
 * and the row, col parameters were not updated.
 */
bool ansi_term_get_cursor_pos(int* row, int* col);

/**
 * @brief configure the terminal so text scrolls within a certain range of rows.
 * 
 * The row count starts at the top of the screen. The top row is number 1.
 * @param top first row of scroll region, inclusive
 * @param bottom last row of scroll region, inclusive
 */
void ansi_term_set_scroll_region(int top, int bottom);

/**
 * @brief clear the terminal screen.
 */
void ansi_term_erase_screen(void);

/**
 * @brief move the cursor to the indicated position.
 * 
 * @param row target row
 * @param col target column
 */
void ansi_term_set_cursor_pos(int row, int col);

/**
 * @brief set text attributes and color.
 * 
 * Only a single attribute and color can be active at a time. This is a limitation of this API,
 * not of the ANSI control sequence for setting attributes.
 * 
 * @param color text color, from the enumerated list
 * @param attribute text attributes, from the enumerated list
 */
void ansi_term_set_attributes(ANSI_TERM_COLOR_T color, ANSI_TERM_ATTRIB_T attribute);

/**
 * @brief reset text attributes and color to 'normal'.
 */
void ansi_term_reset_attributes(void);

/**
 * @brief hide the cursor to reduce flicker during redraws.
 */
void ansi_term_hide_cursor(void);

/**
 * @brief shows a cursor that was previously hidden.
 */
void ansi_term_show_cursor(void);

/**
 * @brief set the cursor style.
 * 
 * @param style cursor style, from the enumerated list
 */
void ansi_term_set_cursor_style(ANSI_TERM_CURSOR_STYLE_T style);
