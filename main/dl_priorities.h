#pragma once

/**
 * Thread priorities used in datalogger application
 * larger value = higher priority
 * 
 * lowest priority = 1 (idle task)
 * highest priority = 25 (defined as configMAX_PRIORITIES)
 */

#define DL_PRIORITY_CONSOLE 2