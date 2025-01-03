/**
 * console_windows.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "console_windows.h"
#include "ansi_term.h"
#include "min_max.h"
#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/semphr.h"

typedef struct {
    int first_row;
    int last_row;
    int num_rows;
    int num_cols;
    int restore_row;
} WINDOW_T;

/**
 *01 |----------border---------|
 *02 |                         | <-- start row
 *03 |         window 2        |
 *04 |                         |
 *05 |                         | <-- restore row
 *06 |----------border---------|
 *07 |                         | <-- start row
 *08 |         window 1        |
 *09 |                         |
 *10 |                         | <-- restore row
 *11 |----------border---------| <-- max rows
 */

static WINDOW_T windows[CONSOLE_WINDOW_MAX];
static CONSOLE_WINDOW_T active_window = CONSOLE_WINDOW_MAX;
static SemaphoreHandle_t console_mutex = NULL;
static int screen_max_row = -1;
static int screen_max_col = -1;

static void draw_horizontal_border(int length, ANSI_TERM_COLOR_T color)
{
    ansi_term_set_attributes(color, ANSI_ATTRIB_NORMAL);
    for (int i = 0; i < length; i++)
    {
        static const char BORDER = '=';
        write(STDOUT_FILENO, &BORDER, 1);
    }
    ansi_term_reset_attributes();
}

static void repaint_screen(void)
{
    // start by clearing the terminal window
    ansi_term_erase_screen();

    // hide the cursor during redraw
    ansi_term_hide_cursor();

    // paint borders around windows
    for (int win = 0; win < CONSOLE_WINDOW_MAX; win++)
    {
        // print top border on line above first_row
        ansi_term_set_cursor_pos(windows[win].first_row - 1, 1);
        draw_horizontal_border(windows[win].num_cols, ANSI_COLOR_CYAN);
    }

    // print bottom border one line beyond last window, last_row
    int row = windows[CONSOLE_WINDOW_MAX - 1].last_row + 1;
    ansi_term_set_cursor_pos(row, 1);
    draw_horizontal_border(windows[CONSOLE_WINDOW_MAX - 1].num_cols, ANSI_COLOR_CYAN);

    // invalidate current window index to force refresh
    active_window = CONSOLE_WINDOW_MAX;

    // show the cursor again
    ansi_term_set_cursor_style(ANSI_CURSOR_BBAR);
    ansi_term_show_cursor();
}

static void set_active_window(CONSOLE_WINDOW_T idx)
{
    if (idx >= CONSOLE_WINDOW_MAX)
    {
        return; 
    }   

    if (active_window == idx)
    {
        // selected window already active
        return;
    }

    // set scroll region for window
    ansi_term_set_scroll_region(windows[idx].first_row, windows[idx].last_row);

    // restore cursor position for new window
    ansi_term_set_cursor_pos(windows[idx].restore_row, 1);

    active_window = idx;
}

void console_windows_init(void)
{
    // create mutex
    // this is a recursive mutex so a single thread can take it multiple times without blocking
    console_mutex = xSemaphoreCreateRecursiveMutex();

    // check size of window and repaint it
    console_windows_update_size();
}

void console_windows_update_size(void)
{
    xSemaphoreTakeRecursive(console_mutex, portMAX_DELAY);

    // check terminal size
    int max_row = screen_max_row;
    int max_col = screen_max_col;
    if (ansi_term_get_terminal_size(&max_row, &max_col) == false)
    {
        // command failed; possibly user input corrupted terminal replies to ESC sequences.
        // assume default terminal size if this is the first attempt to determine the size,
        // otherwise leave the settings as they were.
        if (max_row == -1) max_row = 100;
        if (max_col == -1) max_col = 80;
    }

    if ((max_row == screen_max_row) && (max_col == screen_max_col))
    {
        // size unchanged
        xSemaphoreGiveRecursive(console_mutex);
        return;
    }

    // store new size
    screen_max_row = max_row;
    screen_max_col = max_col;
    
    // reset window positions
    //  - first window hard-coded to four rows
    //  - second window takes remaining space
    windows[CONSOLE_WINDOW_1].num_cols    = max_col;
    windows[CONSOLE_WINDOW_1].num_rows    = 4;
    windows[CONSOLE_WINDOW_1].first_row   = max_row - windows[CONSOLE_WINDOW_1].num_rows;
    windows[CONSOLE_WINDOW_1].last_row    = max_row - 1;
    windows[CONSOLE_WINDOW_1].restore_row = windows[CONSOLE_WINDOW_1].last_row;

    windows[CONSOLE_WINDOW_2].num_cols    = max_col;
    windows[CONSOLE_WINDOW_2].num_rows    = windows[CONSOLE_WINDOW_1].first_row - 3;
    windows[CONSOLE_WINDOW_2].first_row   = 2;
    windows[CONSOLE_WINDOW_2].last_row    = windows[CONSOLE_WINDOW_2].first_row + windows[CONSOLE_WINDOW_2].num_rows - 1;
    windows[CONSOLE_WINDOW_2].restore_row = windows[CONSOLE_WINDOW_2].last_row;

    repaint_screen();

    xSemaphoreGiveRecursive(console_mutex);
}

void console_windows_get_size(int* rows, int* cols)
{
    *rows = screen_max_row;
    *cols = screen_max_col;
}

int console_windows_putc(CONSOLE_WINDOW_T idx, char c)
{
    if (idx >= CONSOLE_WINDOW_MAX)
    {
        return -1; 
    }
    xSemaphoreTakeRecursive(console_mutex, portMAX_DELAY);
    set_active_window(idx);
    int written = write(STDOUT_FILENO, &c, 1);
    xSemaphoreGiveRecursive(console_mutex);

    return written;
}

int console_windows_printf(CONSOLE_WINDOW_T idx, const char* format, ...)
{
    if (idx >= CONSOLE_WINDOW_MAX)
    {
        return -1; 
    }
    xSemaphoreTakeRecursive(console_mutex, portMAX_DELAY);
    set_active_window(idx);
    va_list args;
    va_start (args, format);
    int written = vfprintf(stdout, format, args);
    va_end (args);
    xSemaphoreGiveRecursive(console_mutex);

    return written;
}

int console_windows_logf(const char* format, va_list args)
{
    // direct log messages to big window
    xSemaphoreTakeRecursive(console_mutex, portMAX_DELAY);
    set_active_window(LOG_WINDOW);
    int written = vfprintf(stdout, format, args);
    xSemaphoreGiveRecursive(console_mutex);

    return written;
}

int console_windows_write(CONSOLE_WINDOW_T idx, const char* string, int nbytes)
{
    if (idx >= CONSOLE_WINDOW_MAX)
    {
        return -1; 
    }
    xSemaphoreTakeRecursive(console_mutex, portMAX_DELAY);
    set_active_window(idx);
    int written = write(STDOUT_FILENO, string, nbytes);
    xSemaphoreGiveRecursive(console_mutex);

    return written;
}
