#pragma once

#include <stdarg.h>

typedef enum {
    CONSOLE_WINDOW_1,
    CONSOLE_WINDOW_2,
    CONSOLE_WINDOW_MAX
} CONSOLE_WINDOW_T;

void console_windows_init(void);
void console_windows_update_size(void);
void console_windows_get_size(int* rows, int* cols);
int  console_windows_putc(CONSOLE_WINDOW_T idx, char c);
int  console_windows_printf(CONSOLE_WINDOW_T idx, const char* format, ...);
int  console_windows_write(CONSOLE_WINDOW_T idx, const char* string, int nbytes);
int  console_windows_logf(const char* format, va_list args);

