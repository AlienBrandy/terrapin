/**
 * network_manager.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "network_manager.h"
#include "state_machine.h"
#include "wifi.h"
#include "known_networks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "config.h"
#include "mqtt.h"

typedef enum {
    SIGNAL_INITIALIZE,
    SIGNAL_CONNECT_TO,
    SIGNAL_CONNECT,
    SIGNAL_DISCONNECT,
    SIGNAL_CONTINUE,
    SIGNAL_POLL_TIMER,
    SIGNAL_CONNECTION_LOST,
} network_manager_signal_t;

typedef struct {
    state_machine_handle_t state_machine;
    state_machine_message_t active_message;
    uint8_t known_network_index;
    TimerHandle_t poll_timer;
    bool objects_created;
    char* current_state;
} network_manager_t;

static network_manager_t me;

static void state_uninitialized(state_machine_message_t* message);
static void state_not_connected(state_machine_message_t* message);
static void state_scanning(state_machine_message_t* message);
static void state_pausing(state_machine_message_t* message);
static void state_connecting(state_machine_message_t* message);
static void state_connected(state_machine_message_t* message);

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        static state_machine_message_t msg = {.signal = SIGNAL_CONNECTION_LOST};
        state_machine_post(me.state_machine, &msg);
    }
}

static void poll_timer_callback(TimerHandle_t xTimer)
{
    (void)xTimer;
    static state_machine_message_t msg = {.signal = SIGNAL_POLL_TIMER};

    state_machine_post(me.state_machine, &msg);
}

static void send_reply(state_machine_message_t* message, NETWORK_MANAGER_ERR_T reply)
{
    if (message->caller == NULL)
    {
        return;
    }
    xTaskNotify(message->caller, reply, eSetValueWithOverwrite);

    // ensure we only send one reply per message.
    message->caller = NULL;
}

static void state_uninitialized(state_machine_message_t* message)
{
    switch (message->signal)
    {
        case SIGNAL_ENTRY:
        {
            me.current_state = "UNINITIALIZED";
            return;
        }
        case SIGNAL_EXIT:
        {
            return;
        }
        case SIGNAL_INITIALIZE:
        {
            // initialize wifi module
            WIFI_ERR_T wifi_err = wifi_init();
            if (wifi_err != WIFI_ERR_NONE)
            {
                send_reply(message, NETWORK_MANAGER_ERR_INITIALIZATION_FAILED);
                return;
            }

            // initialize list of known networks
            KNOWN_NETWORKS_ERR_T known_networks_err = known_networks_init();
            if (known_networks_err != KNOWN_NETWORKS_ERR_NONE)
            {
                send_reply(message, NETWORK_MANAGER_ERR_INITIALIZATION_FAILED);
                return;
            }

            // register event handler
            esp_err_t esp_err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &event_handler, NULL);
            if (esp_err != ESP_OK)
            {
                send_reply(message, NETWORK_MANAGER_ERR_INITIALIZATION_FAILED);
                return;
            }

            // initialization complete
            state_machine_set_state(me.state_machine, state_not_connected);
            send_reply(message, NETWORK_MANAGER_ERR_NONE);

            // check auto-connect on bootup
            if (config_get_boolean(CONFIG_KEY_NETWORK_AUTOCONNECT))
            {
                static state_machine_message_t msg = {.signal = SIGNAL_CONNECT};
                state_machine_post(me.state_machine, &msg);
            }
            return;
        }
        default:
        {
            send_reply(message, NETWORK_MANAGER_ERR_NOT_INITIALIZED);
            return;
        }
    }
}

void state_not_connected(state_machine_message_t* message)
{
    switch (message->signal)
    {
        case SIGNAL_ENTRY:
        {
            me.current_state = "NOT_CONNECTED";

            return;
        }
        case SIGNAL_EXIT:
        {
            return;
        }
        case SIGNAL_CONNECT:
        {
            // reply immediately; auto connecting is always an async operation
            send_reply(message, NETWORK_MANAGER_ERR_NONE);
            state_machine_set_state(me.state_machine, state_scanning);
            return;
        }
        case SIGNAL_CONNECT_TO:
        {
            // store message for later reply
            me.active_message = *message;

            // add network to list
            known_network_entry_t* net = (known_network_entry_t*)message->data;
            known_networks_add(net->ssid, net->pwd);

            // attempt connection to new entry which will be at the top of the list
            me.known_network_index = 0;
            state_machine_set_state(me.state_machine, state_connecting);
            return;
        }
        case SIGNAL_DISCONNECT:
        {
            // already disconnected
            send_reply(message, NETWORK_MANAGER_ERR_NONE); 
            return;
        }
        default:
        {
            send_reply(message, NETWORK_MANAGER_ERR_COMMAND_IGNORED);
            return;
        }
    }
}

void state_scanning(state_machine_message_t* message)
{
    switch (message->signal)
    {
        case SIGNAL_ENTRY:
        {
            me.current_state = "SCANNING";
            static state_machine_message_t msg = {.signal = SIGNAL_CONTINUE};
            state_machine_post(me.state_machine, &msg);
            return;
        }
        case SIGNAL_EXIT:
        {
            return;
        }
        case SIGNAL_CONTINUE:
        {
            WIFI_ERR_T code = wifi_scan();
            if (code != WIFI_ERR_NONE)
            {
                // scan failed
                state_machine_set_state(me.state_machine, state_pausing);
                return;
            }
            uint16_t num_networks = wifi_get_number_of_networks();
            if (num_networks == 0)
            {
                // no networks found       
                state_machine_set_state(me.state_machine, state_pausing);
                return;
            }
            uint8_t num_known_networks = known_networks_get_number_of_entries();
            if (num_known_networks == 0)
            {
                // no known networks recorded
                state_machine_set_state(me.state_machine, state_pausing);
                return;
            }
            for (uint16_t i = 0; i < num_networks; i++)
            {
                wifi_network_record_t network_record;
                wifi_get_network_record(i, &network_record);
                for (uint8_t j = 0; j < num_known_networks; j++)
                {
                    known_network_entry_t known_network;
                    known_networks_get_entry(j, &known_network);
                    if (strncmp(network_record.ssid, known_network.ssid, WIFI_SSID_FIELD_SIZE) == 0)
                    {
                        // network identified
                        me.known_network_index = j;
                        state_machine_set_state(me.state_machine, state_connecting);
                        return;
                    }
                }

            }
            // no matching network found
            state_machine_set_state(me.state_machine, state_pausing);
            return;
        }
        case SIGNAL_DISCONNECT:
        {
            send_reply(message, NETWORK_MANAGER_ERR_NONE);
            state_machine_set_state(me.state_machine, state_not_connected);
            return;
        }
        default:
        {
            send_reply(message, NETWORK_MANAGER_ERR_COMMAND_IGNORED);
            return;
        }
    }
}

void state_pausing(state_machine_message_t* message)
{
    switch (message->signal)
    {
        case SIGNAL_ENTRY:
        {
            me.current_state = "PAUSING";
            xTimerStart(me.poll_timer, 0);
            return;
        }
        case SIGNAL_EXIT:
        {
            xTimerStop(me.poll_timer, 0);
            return;
        }
        case SIGNAL_POLL_TIMER:
        {
            state_machine_set_state(me.state_machine, state_scanning);
            return;
        }
        case SIGNAL_DISCONNECT:
        {
            send_reply(message, NETWORK_MANAGER_ERR_NONE);
            state_machine_set_state(me.state_machine, state_not_connected);
            return;
        }
        default:
        {
            send_reply(message, NETWORK_MANAGER_ERR_COMMAND_IGNORED);
            return;
        }
    }
}

void state_connecting(state_machine_message_t* message)
{
    switch (message->signal)
    {
        case SIGNAL_ENTRY:
        {
            me.current_state = "CONNECTING";
            static state_machine_message_t msg = {.signal = SIGNAL_CONTINUE};
            state_machine_post(me.state_machine, &msg);
            return;
        }
        case SIGNAL_EXIT:
        {
            return;
        }
        case SIGNAL_CONTINUE:
        {
            known_network_entry_t net;
            KNOWN_NETWORKS_ERR_T net_err = known_networks_get_entry(me.known_network_index, &net);
            if (net_err != KNOWN_NETWORKS_ERR_NONE)
            {
                send_reply(&me.active_message, NETWORK_MANAGER_ERR_CONNECT_FAILED);
                state_machine_set_state(me.state_machine, state_pausing);
                return;
            }
            WIFI_ERR_T wifi_err = wifi_connect(net.ssid, net.pwd, 10000);
            if (wifi_err != WIFI_ERR_NONE)
            {
                send_reply(&me.active_message, NETWORK_MANAGER_ERR_CONNECT_FAILED);
                state_machine_set_state(me.state_machine, state_pausing);
                return;
            }

            send_reply(&me.active_message, NETWORK_MANAGER_ERR_NONE);
            state_machine_set_state(me.state_machine, state_connected);
            return;
        }
        case SIGNAL_DISCONNECT:
        {
            wifi_disconnect();
            send_reply(message, NETWORK_MANAGER_ERR_NONE);
            state_machine_set_state(me.state_machine, state_not_connected);
            return;
        }
        default:
        {
            send_reply(message, NETWORK_MANAGER_ERR_COMMAND_IGNORED);
            return;
        }
    }
}

void state_connected(state_machine_message_t* message)
{
    switch (message->signal)
    {
        case SIGNAL_ENTRY:
        {
            me.current_state = "CONNECTED";
            if (config_get_boolean(CONFIG_KEY_MQTT_ENABLE))
            {
                mqtt_start();
            }
            return;
        }
        case SIGNAL_EXIT:
        {
            mqtt_stop();
            return;
        }
        case SIGNAL_CONNECT:
        {
            // already connected
            send_reply(message, NETWORK_MANAGER_ERR_NONE); 
            return;
        }
        case SIGNAL_DISCONNECT:
        {
            wifi_disconnect();
            send_reply(message, NETWORK_MANAGER_ERR_NONE);
            state_machine_set_state(me.state_machine, state_not_connected);
            return;
        }
        case SIGNAL_CONNECTION_LOST:
        {
            state_machine_set_state(me.state_machine, state_pausing);
            return;
        }
        default:
        {
            send_reply(message, NETWORK_MANAGER_ERR_COMMAND_IGNORED);
            return;
        }
    }
}

NETWORK_MANAGER_ERR_T network_manager_init(bool wait)
{
    if (!me.objects_created)
    {
        if (state_machine_init(&me.state_machine, "network manager", 2, state_uninitialized) != STATE_MACHINE_ERR_NONE)
        {
            return NETWORK_MANAGER_ERR_INITIALIZATION_FAILED;
        }

        me.poll_timer = xTimerCreate("network poll", pdMS_TO_TICKS(5000), false, NULL, poll_timer_callback);
        if (me.poll_timer == NULL)
        {
            return NETWORK_MANAGER_ERR_INITIALIZATION_FAILED;
        }
        
        me.objects_created = true;
    }

    static state_machine_message_t msg;
    msg.signal = SIGNAL_INITIALIZE;
    msg.caller = wait ? xTaskGetCurrentTaskHandle() : NULL;

    if (state_machine_post(me.state_machine, &msg) != STATE_MACHINE_ERR_NONE)
    {
        return NETWORK_MANAGER_ERR_POST_FAILED;
    }

    if (wait)
    {
        uint32_t retc = 0;
        xTaskNotifyWait(0, 0, &retc, portMAX_DELAY);
        return (NETWORK_MANAGER_ERR_T)retc;
    }

    return NETWORK_MANAGER_ERR_NONE;
}

NETWORK_MANAGER_ERR_T network_manager_connect_to(char* ssid, char* pwd, bool wait)
{
    static state_machine_message_t msg;
    msg.signal = SIGNAL_CONNECT_TO;
    msg.caller = wait ? xTaskGetCurrentTaskHandle() : NULL;

    assert(sizeof(known_network_entry_t) <= sizeof(msg.data));    
    known_network_entry_t* net = (known_network_entry_t*)msg.data;
    memcpy(net->ssid, ssid, KNOWN_NETWORKS_MAX_SSID);
    memcpy(net->pwd, pwd, KNOWN_NETWORKS_MAX_PWD);

    if (state_machine_post(me.state_machine, &msg) != STATE_MACHINE_ERR_NONE)
    {
        return NETWORK_MANAGER_ERR_POST_FAILED;
    }

    if (wait)
    {
        uint32_t retc = 0;
        xTaskNotifyWait(0, 0, &retc, portMAX_DELAY);
        return (NETWORK_MANAGER_ERR_T)retc;
    }

    return NETWORK_MANAGER_ERR_NONE;
}

NETWORK_MANAGER_ERR_T network_manager_connect(bool wait)
{
    static state_machine_message_t msg;
    msg.signal = SIGNAL_CONNECT;
    msg.caller = wait ? xTaskGetCurrentTaskHandle() : NULL;

    if (state_machine_post(me.state_machine, &msg) != STATE_MACHINE_ERR_NONE)
    {
        return NETWORK_MANAGER_ERR_POST_FAILED;
    }

    if (wait)
    {
        uint32_t retc = 0;
        xTaskNotifyWait(0, 0, &retc, portMAX_DELAY);
        return (NETWORK_MANAGER_ERR_T)retc;
    }

    return NETWORK_MANAGER_ERR_NONE;
}

NETWORK_MANAGER_ERR_T network_manager_disconnect(bool wait)
{
    static state_machine_message_t msg;
    msg.signal = SIGNAL_DISCONNECT;
    msg.caller = wait ? xTaskGetCurrentTaskHandle() : NULL;

    if (state_machine_post(me.state_machine, &msg) != STATE_MACHINE_ERR_NONE)
    {
        return NETWORK_MANAGER_ERR_POST_FAILED;
    }

    if (wait)
    {
        uint32_t retc = 0;
        xTaskNotifyWait(0, 0, &retc, portMAX_DELAY);
        return (NETWORK_MANAGER_ERR_T)retc;
    }

    return NETWORK_MANAGER_ERR_NONE;
}

const char* network_manager_get_error_string(NETWORK_MANAGER_ERR_T code)
{
    static const char * error_string[NETWORK_MANAGER_ERR_MAX] = 
    {
        #define X(A, B) B,
        NETWORK_MANAGER_ERROR_LIST
        #undef X
    };
    if (code < NETWORK_MANAGER_ERR_MAX)
    {
        return error_string[code];        
    }
    return "unknown error";
}

const char* network_manager_get_current_state(void)
{
    if (me.current_state)
    {
        return me.current_state;
    }
    return "UNKNOWN";
}
