/**
 * prompt.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "prompt.h"
#include "esp_log.h"
#include "linenoise_lite.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "console_windows.h"
#include "menu.h"

static struct linenoiseState ls;

/**
 * 
 */
static void prompt_task(void* args)
{
    while (true)
    {
        // refresh terminal size
        int rows, cols;
        console_windows_update_size();
        console_windows_get_size(&rows, &cols);

        // display prompt
        const char* prompt = PROJECT_NAME "> ";
        linenoiseEditStart(&ls, prompt, cols);

        // loop until we receive a full line from user
        char* line; 
        while (1)
        {            
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(ls.ifd, &readfds);

            // block on stdin
            int retval = select(ls.ifd+1, &readfds, NULL, NULL, NULL);
            if (retval == -1)
            {
                // failed
                return;
            }
            if (retval == 0)
            {
                // timeout
                continue;
            }

            // character received
            line = linenoiseEditFeed(&ls);
            if (line != linenoiseEditMore)
            {
                // <CR>, <Ctrl+C>, or <Ctrl+D> received
                break;
            }

            // line not yet complete, loop for next character
        }

        // line complete, reset editor
        linenoiseEditStop(&ls);
 
        // Process command
        static char command[MENU_COMMAND_MAX_BYTES];
        strncpy(command, line, sizeof(command));
        menu_send_command(command);
    }
}

PROMPT_ERR_T prompt_init()
{
    // initialize line editing module
    if (linenoiseInit(&ls, 256) != 0)
    {
        return PROMPT_ERR_INIT_FAIL;
    }

    ESP_LOGI(PROJECT_NAME, "Prompt initialized");
    return PROMPT_ERR_NONE;
}

PROMPT_ERR_T prompt_start()
{
    // Launch thread to monitor for user input.
    //
    static const uint32_t PROMPT_TASK_STACK_DEPTH_BYTES = 4096;
    static const char * PROMPT_TASK_NAME = "PROMPT";
    static const uint32_t PROMPT_TASK_PRIORITY = 2;
    static TaskHandle_t h_prompt_task = NULL;

   xTaskCreatePinnedToCore(prompt_task, PROMPT_TASK_NAME, PROMPT_TASK_STACK_DEPTH_BYTES, NULL, PROMPT_TASK_PRIORITY, &h_prompt_task, tskNO_AFFINITY);
   if (h_prompt_task == NULL)
   {
        ESP_LOGE(PROJECT_NAME, "prompt_task create failed");
        return PROMPT_ERR_TASK_START_FAIL;
   }

   return PROMPT_ERR_NONE;
}

