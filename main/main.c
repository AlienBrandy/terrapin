#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "cli.h"

void app_main(void)
{
    DL_ERROR_T dl_err;
    dl_err = cli_init();
    if (dl_err != DL_ERROR_NONE)
    {
        ESP_LOGE("datalogger", "cli_init() failed");        
        return;
    }

    dl_err = cli_start();
    if (dl_err != DL_ERROR_NONE)
    {
        ESP_LOGE("datalogger", "cli_start() failed");        
        return;
    }

    while (1) {}
}
