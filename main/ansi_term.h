/**
 * ansi_term.h
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

/**
 * 
 */
bool ansi_term_init(void);

/**
 * 
 */
bool ansi_term_get_terminal_size(int* rows, int* cols);

/**
 * 
 */
bool ansi_term_get_cursor_pos(int* row, int* col);

/**
 * 
 */
void ansi_term_set_scroll_region(int top, int bottom);

/**
 * 
 */
void ansi_term_erase_screen(void);

/**
 * 
 */
void ansi_term_set_cursor_pos(int row, int col);

/**
 * 
 */
void ansi_term_set_attributes(ANSI_TERM_COLOR_T color, uint8_t attribute);

/**
 * 
 */
void ansi_term_reset_attributes(void);

/**
 * 
 */
void ansi_term_hide_cursor(void);

/**
 * 
 */
void ansi_term_show_cursor(void);
