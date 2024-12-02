#include <stdio.h>
#include <string.h>
#include "menu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "console_windows.h"

static const int MENU_QUEUE_LENGTH = 5;
static QueueHandle_t h_queue;

MENU_ERR_T menu_send_command(menu_command_t* command)
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

static void menu_task(void* args)
{
    menu_command_t command;

    while (true)
    {
        // wait for messages in queue
        xQueueReceive(h_queue, &command, portMAX_DELAY);

        // process command
        console_windows_printf(CONSOLE_WINDOW_2, "Command received: [%s]\n", command.command_string);
        for (int i=0; i<10; i++) {
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
    }
}

MENU_ERR_T menu_init(void)
{
    // create command queue
    h_queue = xQueueCreate(MENU_QUEUE_LENGTH, sizeof(menu_command_t));
    if (h_queue == NULL)
    {
        ESP_LOGE(PROJECT_NAME, "menu_queue create failed");
        return MENU_ERR_QUEUE_CREATE_FAIL;
    }

    return MENU_ERR_NONE;
}

MENU_ERR_T menu_start(void)
{
    // Launch thread to handle menu state machine.
    //
    static const uint32_t MENU_TASK_STACK_DEPTH_BYTES = 4096;
    static const char * MENU_TASK_NAME = "MENU";
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
