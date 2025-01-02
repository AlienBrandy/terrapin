/** 
 * linenoise_lite.c
 *
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 *
 * ------------------------------------------------------------------------
 *
 * Copyright (c) 2010-2023, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010-2013, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ------------------------------------------------------------------------
 */

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "linenoise_lite.h"
#include "console_windows.h"
#include "ring_buffer.h"

#define REFRESH_CLEAN   (1<<0) // RefreshSingleLine flag: Clean the old prompt from the screen
#define REFRESH_WRITE   (1<<1) // RefreshSingleLine flag: Rewrite the prompt on the screen.
#define REFRESH_ALL     (REFRESH_CLEAN | REFRESH_WRITE) // Do both.

#define PROMPT_COLOR       "\x1b[0;32m"   // ESC sequence to set the prompt color
#define PROMPT_COLOR_RESET "\x1b[0m"      // ESC sequence to reset the color

static void linenoiseAtExit(void);
static int  initHistory(void);
static int  addToHistory(const char *line);
static void freeHistory(void);
static void prevFromHistory(struct linenoiseState *l);
static void nextFromHistory(struct linenoiseState *l);

static void refreshLineWithFlags(struct linenoiseState *l, int flags);
static void refreshSingleLine(struct linenoiseState *l, int flags);
static void refreshLine(struct linenoiseState *l);

static struct termios orig_termios; /* In order to restore at exit.*/
static int maskmode = 0; /* Show "***" instead of input. For passwords. */
static int rawmode = 0; /* For atexit() function to check if restore is needed*/
static int atexit_registered = 0; /* Register atexit just 1 time. */

static ring_buffer_handle_t history_buffer = NULL;

enum KEY_ACTION {
	KEY_NULL  = 0,	    /* NULL */
	CTRL_A    = 1,      /* Ctrl+a */
	CTRL_B    = 2,      /* Ctrl-b */
	CTRL_C    = 3,      /* Ctrl-c */
	CTRL_D    = 4,      /* Ctrl-d */
	CTRL_E    = 5,      /* Ctrl-e */
	CTRL_F    = 6,      /* Ctrl-f */
	CTRL_H    = 8,      /* Ctrl-h */
	TAB       = 9,      /* Tab */
	CTRL_K    = 11,     /* Ctrl+k */
	CTRL_L    = 12,     /* Ctrl+l */
	ENTER     = 13,     /* Enter */
	CTRL_N    = 14,     /* Ctrl-n */
	CTRL_P    = 16,     /* Ctrl-p */
	CTRL_T    = 20,     /* Ctrl-t */
	CTRL_U    = 21,     /* Ctrl+u */
	CTRL_W    = 23,     /* Ctrl+w */
	ESC       = 27,     /* Escape */
	BACKSPACE = 127     /* Backspace */
};

/* ======================= Low level terminal handling ====================== */

/* Enable "mask mode". When it is enabled, instead of the input that
 * the user is typing, the terminal will just display a corresponding
 * number of asterisks, like "****". This is useful for passwords and other
 * secrets that should not be displayed. */
void linenoiseMaskModeEnable(void)
{
    maskmode = 1;
}

/* Disable mask mode. */
void linenoiseMaskModeDisable(void)
{
    maskmode = 0;
}

/* Raw mode: 1960 magic shit. */
static int enableRawMode(int fd)
{
    struct termios raw;

    if (!isatty(STDIN_FILENO)) goto fatal;
    if (!atexit_registered) {
        atexit(linenoiseAtExit);
        atexit_registered = 1;
    }
    if (tcgetattr(fd,&orig_termios) == -1) goto fatal;

    raw = orig_termios;  /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer.
     * We want read to return every single byte, without timeout. */
    raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

    /* put terminal in raw mode after flushing */
    if (tcsetattr(fd,TCSAFLUSH,&raw) < 0) goto fatal;
    rawmode = 1;
    return 0;

fatal:
    errno = ENOTTY;
    return -1;
}

static void disableRawMode(int fd)
{
    /* Don't even check the return value as it's too late. */
    if (rawmode && tcsetattr(fd,TCSAFLUSH,&orig_termios) != -1)
        rawmode = 0;
}

/* Single line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal.
 *
 * Flags is REFRESH_* macros. The function can just remove the old
 * prompt, just write it, or both. */
static void refreshSingleLine(struct linenoiseState *l, int flags)
{
    size_t plen = l->plen;
    char *buf = l->buf;
    size_t len = l->len;
    size_t pos = l->pos;
    int written = 0;

    // skip characters if display isn't wide enough to show the entire thing.
    while((plen + pos) >= l->cols) {
        buf++;
        len--;
        pos--;
    }
    while (plen + len > l->cols) {
        len--;
    }

    // Clear the append buffer
    memset(l->abuf, 0, l->abuflen);

    // Add command to move cursor to left edge
    written += snprintf(l->abuf + written, l->abuflen - written, "\r");

    if (flags & REFRESH_WRITE) {
        // Add the prompt
        written += snprintf(l->abuf + written, l->abuflen - written, PROMPT_COLOR "%s" PROMPT_COLOR_RESET, l->prompt);

        // Add the current buffer content
        if (maskmode == 1)
        {
            while (len--) 
                written += snprintf(l->abuf + written, l->abuflen - written, "*");
        }
        else
        {
            written += snprintf(l->abuf + written, l->abuflen - written, "%s", buf);
        }
    }

    // Add command to erase to right
    written += snprintf(l->abuf + written, l->abuflen - written, "\x1b[0K");

    // Restore cursor if entire line was overwritten
    if (flags & REFRESH_WRITE)
    {
        // Move cursor to original position.
        written += snprintf(l->abuf + written, l->abuflen - written, "\r\x1b[%dC", (int)(pos+plen));
    }

    console_windows_write(PROMPT_WINDOW, l->abuf, written);
}

/* Calls the two low level functions refreshSingleLine() or
 * refreshMultiLine() according to the selected mode. */
static void refreshLineWithFlags(struct linenoiseState *l, int flags)
{
    refreshSingleLine(l,flags);
}

/* Utility function to avoid specifying REFRESH_ALL all the times. */
static void refreshLine(struct linenoiseState *l)
{
    refreshLineWithFlags(l,REFRESH_ALL);
}

/* Hide the current line, when using the multiplexing API. */
void linenoiseHide(struct linenoiseState *l)
{
    refreshSingleLine(l,REFRESH_CLEAN);
}

/* Show the current line, when using the multiplexing API. */
void linenoiseShow(struct linenoiseState *l)
{
    refreshLineWithFlags(l,REFRESH_WRITE);
}

/* Insert the character 'c' at cursor current position.
 *
 * On error writing to the terminal -1 is returned, otherwise 0. */
int linenoiseEditInsert(struct linenoiseState *l, char c)
{
    if (l->len < l->buflen)
    {
        if (l->len == l->pos)
        {
            l->buf[l->pos] = c;
            l->pos++;
            l->len++;
            l->buf[l->len] = '\0';
            if ((l->plen+l->len < l->cols))
            {
                // Avoid a full update of the line in the trivial case.
                char d = (maskmode==1) ? '*' : c;
                if (console_windows_putc(PROMPT_WINDOW, d) == -1) return -1;
            }
            else
            {
                refreshLine(l);
            }
        }
        else
        {
            memmove(l->buf+l->pos+1,l->buf+l->pos,l->len-l->pos);
            l->buf[l->pos] = c;
            l->len++;
            l->pos++;
            l->buf[l->len] = '\0';
            refreshLine(l);
        }
    }
    return 0;
}

/* Move cursor on the left. */
void linenoiseEditMoveLeft(struct linenoiseState *l)
{
    if (l->pos > 0) {
        l->pos--;
        refreshLine(l);
    }
}

/* Move cursor on the right. */
void linenoiseEditMoveRight(struct linenoiseState *l)
{
    if (l->pos != l->len) {
        l->pos++;
        refreshLine(l);
    }
}

/* Move cursor to the start of the line. */
void linenoiseEditMoveHome(struct linenoiseState *l)
{
    if (l->pos != 0) {
        l->pos = 0;
        refreshLine(l);
    }
}

/* Move cursor to the end of the line. */
void linenoiseEditMoveEnd(struct linenoiseState *l)
{
    if (l->pos != l->len) {
        l->pos = l->len;
        refreshLine(l);
    }
}

/* Delete the character at the right of the cursor without altering the cursor
 * position. Basically this is what happens with the "Delete" keyboard key. */
void linenoiseEditDelete(struct linenoiseState *l)
{
    if (l->len > 0 && l->pos < l->len) {
        memmove(l->buf+l->pos,l->buf+l->pos+1,l->len-l->pos-1);
        l->len--;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

/* Backspace implementation. */
void linenoiseEditBackspace(struct linenoiseState *l)
{
    if (l->pos > 0 && l->len > 0) {
        memmove(l->buf+l->pos-1,l->buf+l->pos,l->len-l->pos);
        l->pos--;
        l->len--;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

/* Delete the previosu word, maintaining the cursor at the start of the
 * current word. */
void linenoiseEditDeletePrevWord(struct linenoiseState *l)
{
    size_t old_pos = l->pos;
    size_t diff;

    while (l->pos > 0 && l->buf[l->pos-1] == ' ')
        l->pos--;
    while (l->pos > 0 && l->buf[l->pos-1] != ' ')
        l->pos--;
    diff = old_pos - l->pos;
    memmove(l->buf+l->pos,l->buf+old_pos,l->len-old_pos+1);
    l->len -= diff;
    refreshLine(l);
}

int linenoiseInit(struct linenoiseState *l, size_t max_line_chars)
{
    // Create line buffer for input characters
    l->buf = calloc(1, max_line_chars);
    if (l->buf == NULL)
    {
        return 1;
    }

    // Create scratchpad for assembling terminal command string
    l->abuf = calloc(1, max_line_chars * 2);
    if (l->abuf == NULL)
    {
        free(l->buf);
        return 1;
    }

    // Initialize history buffer
    if (initHistory() != 0)
    {
        free(l->buf);
        free(l->abuf);
        return 1;
    }

    l->buflen = max_line_chars;
    l->abuflen = max_line_chars * 2;
    return 0;
}

/* This function is part of the multiplexed API of Linenoise, that is used
 * in order to implement the blocking variant of the API but can also be
 * called by the user directly in an event driven program. It will:
 *
 * 1. Initialize the linenoise state passed by the user.
 * 2. Put the terminal in RAW mode.
 * 3. Show the prompt.
 * 4. Return control to the user, that will have to call linenoiseEditFeed()
 *    each time there is some data arriving in the standard input.
 *
 * The user can also call linenoiseEditHide() and linenoiseEditShow() if it
 * is required to show some input arriving asyncronously, without mixing
 * it with the currently edited line.
 *
 * When linenoiseEditFeed() returns non-NULL, the user finished with the
 * line editing session (pressed enter CTRL-D/C): in this case the caller
 * needs to call linenoiseEditStop() to put back the terminal in normal
 * mode. This will not destroy the buffer, as long as the linenoiseState
 * is still valid in the context of the caller.
 *
 * The function returns 0 on success, or -1 if writing to standard output
 * fails. If stdin_fd or stdout_fd are set to -1, the default is to use
 * STDIN_FILENO and STDOUT_FILENO.
 */
int linenoiseEditStart(struct linenoiseState *l, const char *prompt, size_t max_cols)
{
    /* Populate the linenoise state that we pass to functions implementing
     * specific editing functionalities. */
    l->ifd = STDIN_FILENO;
    l->prompt = prompt;
    l->plen = strlen(prompt);
    l->oldpos = 0;
    l->pos = 0;
    l->len = 0;
    l->cols = max_cols;

    /* Enter raw mode. */
    if (enableRawMode(l->ifd) == -1) return -1;

    /* Buffer starts empty. */
    l->buf[0] = '\0';
    l->buflen--; /* Make sure there is always space for the nulterm */

    /* If stdin is not a tty, stop here with the initialization. We
     * will actually just read a line from standard input in blocking
     * mode later, in linenoiseEditFeed(). */
    if (!isatty(l->ifd)) return 0;

    console_windows_printf(PROMPT_WINDOW, PROMPT_COLOR "%s" PROMPT_COLOR_RESET, prompt);
    return 0;
}

char *linenoiseEditMore = "If you see this, you are misusing the API: when linenoiseEditFeed() is called, if it returns linenoiseEditMore the user is yet editing the line. See the README file for more information.";

/* This function is part of the multiplexed API of linenoise, see the top
 * comment on linenoiseEditStart() for more information. Call this function
 * each time there is some data to read from the standard input file
 * descriptor. In the case of blocking operations, this function can just be
 * called in a loop, and block.
 *
 * The function returns linenoiseEditMore to signal that line editing is still
 * in progress, that is, the user didn't yet pressed enter / CTRL-D. Otherwise
 * the function returns the pointer to the heap-allocated buffer with the
 * edited line, that the user should free with linenoiseFree().
 *
 * On special conditions, NULL is returned and errno is populated:
 *
 * EAGAIN if the user pressed Ctrl-C
 * ENOENT if the user pressed Ctrl-D
 *
 * Some other errno: I/O error.
 */
char *linenoiseEditFeed(struct linenoiseState *l)
{
    char c;
    int nread;
    char seq[3];

    nread = read(l->ifd,&c,1);
    if (nread <= 0) return NULL;

    // repositions the cursor in case it was moved between function calls.
    refreshLine(l);

    switch(c)
    {
    case ENTER:    /* enter */
        return l->buf;
    case CTRL_C:     /* ctrl-c */
        errno = EAGAIN;
        return NULL;
    case BACKSPACE:   /* backspace */
    case 8:     /* ctrl-h */
        linenoiseEditBackspace(l);
        break;
    case CTRL_D:     /* ctrl-d, remove char at right of cursor, or if the
                        line is empty, act as end-of-file. */
        if (l->len > 0) {
            linenoiseEditDelete(l);
        } else {
            errno = ENOENT;
            return NULL;
        }
        break;
    case CTRL_T:    /* ctrl-t, swaps current character with previous. */
        if (l->pos > 0 && l->pos < l->len) {
            int aux = l->buf[l->pos-1];
            l->buf[l->pos-1] = l->buf[l->pos];
            l->buf[l->pos] = aux;
            if (l->pos != l->len-1) l->pos++;
            refreshLine(l);
        }
        break;
    case CTRL_B:     /* ctrl-b */
        linenoiseEditMoveLeft(l);
        break;
    case CTRL_F:     /* ctrl-f */
        linenoiseEditMoveRight(l);
        break;
    case CTRL_P:    /* ctrl-p */
        prevFromHistory(l);
        break;
    case CTRL_N:    /* ctrl-n */
        nextFromHistory(l);
        break;
    case ESC:    /* escape sequence */
        /* Read the next two bytes representing the escape sequence.
         * Use two calls to handle slow terminals returning the two
         * chars at different times. */
        if (read(l->ifd,seq,1) == -1) break;
        if (read(l->ifd,seq+1,1) == -1) break;

        /* ESC [ sequences. */
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                /* Extended escape, read additional byte. */
                if (read(l->ifd,seq+2,1) == -1) break;
                if (seq[2] == '~') {
                    switch(seq[1]) {
                    case '3': /* Delete key. */
                        linenoiseEditDelete(l);
                        break;
                    }
                }
            } else {
                switch(seq[1]) {
                case 'A': /* Up */
                    prevFromHistory(l);
                    break;
                case 'B': /* Down */
                    nextFromHistory(l);
                    break;
                case 'C': /* Right */
                    linenoiseEditMoveRight(l);
                    break;
                case 'D': /* Left */
                    linenoiseEditMoveLeft(l);
                    break;
                case 'H': /* Home */
                    linenoiseEditMoveHome(l);
                    break;
                case 'F': /* End*/
                    linenoiseEditMoveEnd(l);
                    break;
                }
            }
        }

        /* ESC O sequences. */
        else if (seq[0] == 'O') {
            switch(seq[1]) {
            case 'H': /* Home */
                linenoiseEditMoveHome(l);
                break;
            case 'F': /* End*/
                linenoiseEditMoveEnd(l);
                break;
            }
        }
        break;
    default:
        if (linenoiseEditInsert(l,c)) return NULL;
        break;
    case CTRL_U: /* Ctrl+u, delete the whole line. */
        l->buf[0] = '\0';
        l->pos = l->len = 0;
        refreshLine(l);
        break;
    case CTRL_K: /* Ctrl+k, delete from current to end of line. */
        l->buf[l->pos] = '\0';
        l->len = l->pos;
        refreshLine(l);
        break;
    case CTRL_A: /* Ctrl+a, go to the start of the line */
        linenoiseEditMoveHome(l);
        break;
    case CTRL_E: /* ctrl+e, go to the end of the line */
        linenoiseEditMoveEnd(l);
        break;
    case CTRL_L: /* ctrl+l, clear screen */
        // not supported
        break;
    case CTRL_W: /* ctrl+w, delete previous word */
        linenoiseEditDeletePrevWord(l);
        break;
    }

    return linenoiseEditMore;
}

/* This is part of the multiplexed linenoise API. See linenoiseEditStart()
 * for more information. This function is called when linenoiseEditFeed()
 * returns something different than NULL. At this point the user input
 * is in the buffer, and we can restore the terminal in normal mode. */
void linenoiseEditStop(struct linenoiseState *l)
{
    // store line in history if not blank
    if (strlen(l->buf) > 0)
    {
        addToHistory(l->buf);
    }

    if (!isatty(l->ifd)) return;
    disableRawMode(l->ifd);
    console_windows_printf(PROMPT_WINDOW, "\n");
}

/* At exit we'll try to fix the terminal to the initial conditions. */
static void linenoiseAtExit(void)
{
    disableRawMode(STDIN_FILENO);
    freeHistory();
}

/* ================================ History ================================= */

/**
 * @brief Init the history buffer.
 * 
 * call before attempting to add/retrieve lines from history buffer 
 */
static int initHistory(void)
{
    if (history_buffer == NULL)
    {
        // create buffer for approximately 50 entries
        if (ring_buffer_create(&history_buffer, 2048) != RING_BUFFER_ERR_NONE)
        {
            return 1;
        }

        // add a blank line to history buffer as a hint to the user
        // for where the list wraps.
        addToHistory("");
    }

    return 0;
}

/**
 * @brief Free the history buffer.
 * 
 * used when we have to exit() to avoid memory leaks are reported by valgrind & co.
 */
static void freeHistory(void)
{
    ring_buffer_destroy(history_buffer);
}

/**
 * @brief Overwrite line buffer with previous entry from history.
 * 
 */
static void prevFromHistory(struct linenoiseState *l)
{
    char* line_from_history = "";

    if (ring_buffer_peek_prev(history_buffer, (uint8_t**)&line_from_history, NULL) != RING_BUFFER_ERR_NONE)
    {
        return;
    }
    strncpy(l->buf, (char*)line_from_history, l->buflen);
    l->buf[l->buflen - 1] = '\0';
    l->len = l->pos = strlen(l->buf);
    refreshLine(l);
}

/**
 * @brief Overwrite line buffer with next entry from history.
 * 
 */
static void nextFromHistory(struct linenoiseState *l)
{
    char* line_from_history = "";

    if (ring_buffer_peek_next(history_buffer, (uint8_t**)&line_from_history, NULL) != RING_BUFFER_ERR_NONE)
    {
        return;
    }
    strncpy(l->buf, (char*)line_from_history, l->buflen);
    l->buf[l->buflen - 1] = '\0';
    l->len = l->pos = strlen(l->buf);
    refreshLine(l);
}

/**
 * @brief Add a new entry in the history buffer.
 * 
 */
static int addToHistory(const char *line)
{
    // check if previous entry is identical to skip duplicates
    char* last_line = "";
    RING_BUFFER_ERR_T retc = ring_buffer_peek_tail(history_buffer, (uint8_t**)&last_line, NULL);
    if (retc == RING_BUFFER_ERR_EMPTY)
    {
        // add line (including null terminator) to history
        ring_buffer_add(history_buffer, (uint8_t*)line, strlen(line) + 1);
        return 0;
    }
    if (retc == RING_BUFFER_ERR_NONE)
    {
        if (strcmp(line, last_line) != 0)
        {
            // add line (including null terminator) to history
            ring_buffer_add(history_buffer, (uint8_t*)line, strlen(line) + 1);
        }

        // reset read pointer to head element
        ring_buffer_peek_head(history_buffer, NULL, NULL);
        return 0;
    }

    // error accessing ring buffer
    return 1;
}

