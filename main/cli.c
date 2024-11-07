#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "dl_errors.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * configs: 
 *   LOG_COLORS [0|1] whether to use colors if terminal supports it
 *   STORE_HISTORY [0|1] whether to store command history in flash memory
 *   CONFIG_ESP_CONSOLE_UART_NUM [0-1] which UART number to use for console output
 * 
 */

/**
 * @brief string to prepend to log entries
 */
static const char* TAG = "datalogger";

/**
 * @brief path to non-volatile filesystem for storing command history
 */
#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

/**
 * 
 */
static DL_ERROR_T initialize_filesystem(void)
{
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true
    };

    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_vfs_fat_spiflash_mount_rw_wl() failed: %s", esp_err_to_name(err));
        return DL_ERROR_INIT_FS_FAIL;
    }

    ESP_LOGI(TAG, "Filesystem initialized");
    return DL_ERROR_NONE;
}

/**
 * 
 */
static DL_ERROR_T initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        err = nvs_flash_erase();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "nvs_flash_erase() failed: %s", esp_err_to_name(err));
            return DL_ERROR_INIT_NVS_FAIL;
        }
        err = nvs_flash_init();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "nvs_flash_init() failed: %s", esp_err_to_name(err));
            return DL_ERROR_INIT_NVS_FAIL;            
        }
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_flash_init() failed: %s", esp_err_to_name(err));
        return DL_ERROR_INIT_NVS_FAIL;            
    }

    ESP_LOGI(TAG, "Flash storage initialized");
    return DL_ERROR_NONE;
}

/**
 * 
 */
static DL_ERROR_T initialize_console(void)
{
    // Drain stdout before reconfiguring it
    fflush(stdout);
    fsync(fileno(stdout));

    // Disable buffering on stdin
    setvbuf(stdin, NULL, _IONBF, 0);

    // Minicom, screen, idf_monitor send CR when ENTER key is pressed
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);

    // Move the caret to the beginning of the next line on '\n'
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    // Install UART driver for interrupt-driven reads and writes
    esp_err_t esp_err = uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0);
    if (esp_err != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_driver_install() failed: %s", esp_err_to_name(esp_err));
        return DL_ERROR_INIT_CONSOLE_FAIL;
    }

    // Configure UART.
    // Note that REF_TICK is used so that the baud rate remains
    // correct while APB frequency is changing in light sleep mode.
    const uart_config_t uart_config = {
            .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
    #if SOC_UART_SUPPORT_REF_TICK
            .source_clk = UART_SCLK_REF_TICK,
    #elif SOC_UART_SUPPORT_XTAL_CLK
            .source_clk = UART_SCLK_XTAL,
    #endif
    };
    esp_err = uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config);
    if (esp_err != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_param_config() failed: %s", esp_err_to_name(esp_err));
        return DL_ERROR_INIT_CONSOLE_FAIL;
    }

    // Tell VFS to use UART driver
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    // Initialize the console
    esp_console_config_t console_config = {
            .max_cmdline_args = 8,
            .max_cmdline_length = 256,
    #if CONFIG_LOG_COLORS
            .hint_color = atoi(LOG_COLOR_CYAN)
    #endif
    };
    esp_err = esp_console_init(&console_config);
    if (esp_err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_console_init() failed: %s", esp_err_to_name(esp_err));
        return DL_ERROR_INIT_CONSOLE_FAIL;
    }

    // Configure linenoise line completion library 
    // Enable multiline editing. If not set, long commands will scroll within single line.
    linenoiseSetMultiLine(1);

    // Tell linenoise where to get command completions and hints
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    // Set command history size
    linenoiseHistorySetMaxLen(100);

    // Set command maximum length
    linenoiseSetMaxLineLen(console_config.max_cmdline_length);

    // Don't return empty lines
    linenoiseAllowEmpty(false);

#if CONFIG_STORE_HISTORY
    // Load command history from filesystem
    linenoiseHistoryLoad(HISTORY_PATH);
#endif

    return DL_ERROR_NONE;
}

/**
 * 
 */
static void cli_task(void* args)
{
    const char* prompt = LOG_COLOR_I "> " LOG_RESET_COLOR;

    printf("\nDatalogger\n");

    // check if terminal supports escape sequences
    int probe_status = linenoiseProbe();

    if (probe_status != DL_ERROR_NONE)
    {
        printf("\nEscape sequences not supported by this terminal.\n");
        linenoiseSetDumbMode(1);

        // replace prompt with no colors
        prompt = "> ";
    }

    while (true)
    {
        // get line from terminal
        char* line = linenoise(prompt);

        // ignore EOF or error
        if (line == NULL)
        {
            continue;
        }

        // Add the command to the history if not empty
        if (strlen(line) > 0)
        {
            linenoiseHistoryAdd(line);
#if CONFIG_STORE_HISTORY
            // Save command history to filesystem
            linenoiseHistorySave(HISTORY_PATH);
#endif
        }

        // Process command
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND)
        {
            printf("Unrecognized command\n");
        }
        else if (err == ESP_ERR_INVALID_ARG)
        {
            // command was empty
        }
        else if (err == ESP_OK && ret != ESP_OK)
        {
            printf("Command returned error code: 0x%x\n", ret);
        }
        else if (err != ESP_OK)
        {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        
        // linenoise allocates line buffer on the heap, so need to free it
        linenoiseFree(line);
    }
}

DL_ERROR_T cli_init()
{
    DL_ERROR_T dl_err;

    // initialize flash memory
    dl_err = initialize_nvs();
    if (dl_err != DL_ERROR_NONE)
        return dl_err;

    // initialize flash filesystem
    dl_err = initialize_filesystem();
    if (dl_err != DL_ERROR_NONE)
        return dl_err;

    // initialize the UART for console communication
    dl_err = initialize_console();
    if (dl_err != DL_ERROR_NONE)
        return dl_err;

    // register the 'help' commnad
    esp_console_register_help_command();

    return DL_ERROR_NONE;
}

DL_ERROR_T cli_start()
{
    // launch CLI thread
    static const uint32_t CLI_TASK_STACK_DEPTH_BYTES = 2048;
    static const char * CLI_TASK_NAME = "CLI";
    static const uint32_t CLI_TASK_PRIORITY = 2;
    static TaskHandle_t h_cli_task = NULL;

   xTaskCreatePinnedToCore(cli_task, CLI_TASK_NAME, CLI_TASK_STACK_DEPTH_BYTES, NULL, CLI_TASK_PRIORITY, &h_cli_task, tskNO_AFFINITY);
   if (h_cli_task == NULL)
   {
        ESP_LOGE(TAG, "xTaskCreatePinnedToCore() failed");
        return DL_ERROR_CLI_START_FAIL;
   }

   return DL_ERROR_NONE;
}

