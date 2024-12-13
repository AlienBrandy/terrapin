/**
 * menu.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "menu.h"
#include "main_menu.h"
#include "console_windows.h"

static const int MENU_QUEUE_LENGTH = 5;
static QueueHandle_t h_queue;
static menu_item_t *current_menu_item = NULL;

static const char *delims = " ,";
#define MENU_MAX_PARAMS 10
typedef char *ARGV_T[MENU_MAX_PARAMS];

static void parse_command(char *cmd, int *argc, ARGV_T *argv)
{
    // initialize counter to zero
    *argc = 0;

    // store pointers of all tokens found in cmd string
    char *token = strtok(cmd, delims);
    while (token && *argc < MENU_MAX_PARAMS - 1)
    {
        (*argv)[(*argc)++] = token;
        token = strtok(0, delims);
    }

    // last pointer is always NULL to follow C environment convention
    (*argv)[*argc] = 0;
}

MENU_ERR_T menu_register_item(menu_item_t *menu_item, menu_item_t **list, int *list_size)
{
    menu_item_t *ptr = (menu_item_t *)realloc(*list, (*list_size + 1) * sizeof(menu_item_t));
    if (ptr == NULL)
    {
        return MENU_ERR_REGISTER_FAIL;
    }
    *list = ptr;
    (*list)[*list_size] = *menu_item;
    (*list_size)++;

    return MENU_ERR_NONE;
}

MENU_ERR_T menu_init(void)
{
    // initialize main menu
    current_menu_item = main_menu_init();

    // create command queue
    h_queue = xQueueCreate(MENU_QUEUE_LENGTH, MENU_COMMAND_MAX_BYTES);
    if (h_queue == NULL)
    {
        ESP_LOGE(PROJECT_NAME, "menu_queue create failed");
        return MENU_ERR_QUEUE_CREATE_FAIL;
    }

    return MENU_ERR_NONE;
}

MENU_ERR_T menu_send_command(const char *command)
{
    if (h_queue == NULL)
    {
        ESP_LOGE(PROJECT_NAME, "menu_send_command fail; not initialized");
        return MENU_ERR_NOT_INITIALIZED;
    }

    int retc = xQueueSend(h_queue, command, 0);
    if (retc != pdTRUE)
    {
        ESP_LOGE(PROJECT_NAME, "menu_send_command fail; queue full");
        return MENU_ERR_QUEUE_CREATE_FAIL;
    }

    return MENU_ERR_NONE;
}

static void menu_task(void *args)
{
    char command[MENU_COMMAND_MAX_BYTES];

    while (true)
    {
        // wait for messages in queue
        xQueueReceive(h_queue, &command, portMAX_DELAY);

        // process command
        int argc = 0;
        ARGV_T argv;
        parse_command(command, &argc, &argv);
        menu_item_t *new_menu_item = (*current_menu_item->func)(argc, argv);
        if (new_menu_item != NULL)
        {
            current_menu_item = new_menu_item;
        }
    }
}

MENU_ERR_T menu_start(void)
{
    // Launch menu state machine thread
    static const uint32_t MENU_TASK_STACK_DEPTH_BYTES = 4096;
    static const char *MENU_TASK_NAME = "MENU";
    static const uint32_t MENU_TASK_PRIORITY = 2;
    static TaskHandle_t h_menu_task = NULL;

    xTaskCreatePinnedToCore(menu_task, MENU_TASK_NAME, MENU_TASK_STACK_DEPTH_BYTES, NULL, MENU_TASK_PRIORITY, &h_menu_task, tskNO_AFFINITY);
    if (h_menu_task == NULL)
    {
        ESP_LOGE(PROJECT_NAME, "menu_task create failed");
        return MENU_ERR_TASK_START_FAIL;
    }

    return MENU_ERR_NONE;
}
