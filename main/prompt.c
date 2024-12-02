#include <stdio.h>
#include <string.h>
#include "prompt.h"
#include "esp_log.h"
#include "linenoise_lite.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "filesystem.h"
#include "console_windows.h"
#include "menu.h"

/**
 * @brief path to non-volatile storage of cli history buffer
 */
#define HISTORY_PATH FILESYSTEM_MOUNT_PATH "/history.txt"

/**
 * 
 */
static void prompt_task(void* args)
{
    const char* prompt = PROJECT_NAME "> ";

    while (true)
    {
        struct linenoiseState ls;
        static char buf[LINENOISE_MAX_LINE];    // contains input characters
        static char abuf[LINENOISE_MAX_LINE];   // scratch pad for assembling terminal command string

        // refresh terminal size
        int rows, cols;
        console_windows_update_size();
        console_windows_get_size(&rows, &cols);

        // display prompt
        linenoiseEditStart(&ls, buf, sizeof(buf), abuf, sizeof(abuf), prompt, cols);

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

        // Add the command to the history if not empty
        if (strlen(line) > 0)
        {
            linenoiseHistoryAdd(line);
            linenoiseHistorySave(HISTORY_PATH);
        }
 
        // Process command
        menu_command_t command;
        strncpy(command.command_string, line, MENU_COMMAND_MAX_BYTES);
        menu_send_command(&command);
    }
}

PROMPT_ERR_T prompt_init()
{
    // Load command history from filesystem
    linenoiseHistoryLoad(HISTORY_PATH);

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

