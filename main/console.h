#pragma once

/**
 * 
 * components/console:
 * copied from esp-idf components directory to here so it could be overridden. The build system doesn't provide
 * for overriding a subset of files within a component. In this case, we wanted to update the linenoise library
 * with a more recent version, but not muck with the files in the esp-idf directory directly.
 * 
 * Linenoise.c
 * new version of this library replaces the older one in components/console. The new one provides for async access
 * which we want to leverage in the cli manager. The new version however, needs to be modified because it calls
 * ioctl commands that are neither supported or even defined in the esp-idf system.
 * 
 * New version allocates buffer for received line on the stack. Since the buffer is 4096 bytes, this blows the stack.
 * Changed buffers to static allocations which should be okay for our use of the library.
 * 
 * console.c: manages the debug terminal
 *    initializes the uart and console
 *    launches the prompt thread
 *    launches the menu thread
 * 
 * prompt.c: manipulates the interactive command area
 *    uses linenoise line editing library
 *    starts thread to query for input from uart (linenoise <- stdin <- vfs uart layer <- uart rx)
 *                                               (linenoise (set_window 1, then return to 2) -> stdout -> (window manager) -> vfs uart layer -> uart tx) **this technique requires switching window context on every character input
 *                                               (menu      (always goes to window 2)                  -> stdout -> (window manager) -> vfs uart layer -> uart tx)
 *    rx input from uart should be directed to stdin and read by linenoise.
 *       stdin comes from users typing commands.
 *       stdin comes from terminal responding to terminal commands. linenoise is modal when waiting for these replies.
 *    output from linenoise (char echo, tab completion printfs) should be directed to uart tx but only show up in prompt area (use custom vfs driver to send to cli which wraps output with cursor commands if necessary) 
 *    output from menus should be directed to uart tx but only show up in menu area (use custom vfs driver to send to cli which wraps output with cursor commands. cursor commands get offsets applied.)
 *    output from debug printf statements to stdout, stderr should be directed to special area via stdout
 *    commands go to menu state machine
 * 
 * menu.c: controls the output window
 *    shows menus and submenus and launch commands ()
 *    maintains a state machine for active submenu
 * 
 * 
 *  --------------------------
 * |> help                   | <--- prompt (interactive, single-line)
 * |-------------------------|
 * | menu:                   | <--- menu (write-only, scrolling)
 * |  wifi                   |
 * |  rtos                   |
 * |  nvs                    |
 * |                         |
 *  --------------------------
 */

typedef enum {
    CONSOLE_ERR_NONE,
    CONSOLE_ERR_PROMPT_INIT_FAIL,
    CONSOLE_ERR_MENU_INIT_FAIL,
    CONSOLE_ERR_TERMINAL_INIT_FAIL,
} CONSOLE_ERR_T;

CONSOLE_ERR_T console_init(void);
CONSOLE_ERR_T console_start(void);
