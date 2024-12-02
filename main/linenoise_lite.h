/**
 * linenoise_lite.h
 * 
 * Line editing module based on open-source linenoise project.
 * This fork has been modified to remove memory allocs/frees to make it
 * more suitable for an embedded application. Some functionality has been
 * removed to reduce the footprint and simplify the API. Notably, the
 * line completion and multiline features have been removed.
 * 
 * This version inherits and is subject to the same license policy
 * applicable to the original linenoise library as described below.
 *
 * ------------------------------------------------------------------------
 * 
 * linenoise.h -- VERSION 1.0
 *
 * Guerrilla line editing library against the idea that a line editing lib
 * needs to be 20,000 lines of C code.
 *
 * See linenoise.c for more information.
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
 */

#ifndef __LINENOISE_LITE_H
#define __LINENOISE_LITE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h> /* For size_t. */

/**
 * @brief
 */
#define LINENOISE_DEFAULT_HISTORY_MAX_LEN 20

/**
 * @brief
 */
#define LINENOISE_MAX_LINE 256

/**
 * returned by linenoiseEditFeed when the line isn't yet complete.
 * 
 */
extern char *linenoiseEditMore;

/* The linenoiseState structure represents the state during line editing.
 * We pass this state to functions implementing specific editing
 * functionalities. */
struct linenoiseState {
    int ifd;            /* Terminal stdin file descriptor. */
    char *buf;          /* Edited line buffer. */
    size_t buflen;      /* Edited line buffer size. */
    char *abuf;         /* Append buffer, includes escape sequences. */
    size_t abuflen;     /* Append buffer size .*/
    const char *prompt; /* Prompt to display. */
    size_t plen;        /* Prompt length. */
    size_t pos;         /* Current cursor position. */
    size_t oldpos;      /* Previous refresh cursor position. */
    size_t len;         /* Current edited line length. */
    size_t cols;        /* Number of columns in terminal. */
    size_t oldrows;     /* Rows used by last refrehsed line (multiline mode) */
    int history_index;  /* The history index we are currently editing. */
};

/* Non blocking API. */
int linenoiseEditStart(struct linenoiseState *l, char *buf, size_t buflen, char *abuf, size_t abuflen, const char *prompt, size_t max_cols);
char *linenoiseEditFeed(struct linenoiseState *l);
void linenoiseEditStop(struct linenoiseState *l);
void linenoiseHide(struct linenoiseState *l);
void linenoiseShow(struct linenoiseState *l);

/* History API. */
int linenoiseHistoryAdd(const char *line);
int linenoiseHistorySave(const char *filename);
int linenoiseHistoryLoad(const char *filename);

/* Other utilities. */
void linenoiseMaskModeEnable(void);
void linenoiseMaskModeDisable(void);

#ifdef __cplusplus
}
#endif

#endif
