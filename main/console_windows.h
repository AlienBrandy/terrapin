/**
 * console_windows.h
 * 
 * windowing subsytem for ANSI terminals.
 * This module splits a terminal window vertically into separate scrolling regions, visually
 * demarcated by horizontal bars. Text is directed to a certain region using the putc(), 
 * printf(), and write() functions which include a window index.
 * 
 * The module currently defines two regions. Window 1 is at the bottom of the screen and
 * is hard-coded to four rows. Window 2 is at the top of the screen and it's height is
 * dynamically determined based on the terminal screen size. In this arrangement, window 1
 * can be used for single-line text input with a short scroll area to show previously
 * entered commands, and window 2 would be used for larger blocks of text such as menus
 * and status screens.
 * 
 * A terminal does not push updates when it's screen is resized. To detect resize events, 
 * the programmer must occasionally call console_windows_update_size() to fetch the current
 * screen size. If the size has changed, this function will repaint the screen and erase 
 * the contents of the windows.
 * 
 * Any output to the terminal through stdio functions will end up in whichever screen 
 * happened to be active. This may corrupt menus and prompts. console_windows_logf() has
 * a standard function signature and can be used to redirect stdio to the appropriate window.
 * console_windows_logf() directs output to window 2.
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdarg.h>

typedef enum {
    CONSOLE_WINDOW_1,
    CONSOLE_WINDOW_2,
    CONSOLE_WINDOW_MAX
} CONSOLE_WINDOW_T;

#define PROMPT_WINDOW CONSOLE_WINDOW_1
#define MENU_WINDOW   CONSOLE_WINDOW_2
#define LOG_WINDOW    CONSOLE_WINDOW_2

void console_windows_init(void);
void console_windows_update_size(void);
void console_windows_get_size(int* rows, int* cols);
int  console_windows_putc(CONSOLE_WINDOW_T idx, char c);
int  console_windows_printf(CONSOLE_WINDOW_T idx, const char* format, ...);
int  console_windows_write(CONSOLE_WINDOW_T idx, const char* string, int nbytes);
int  console_windows_logf(const char* format, va_list args);

