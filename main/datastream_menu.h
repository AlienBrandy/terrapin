/**
 * datastream_menu.h
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "menu.h"

menu_item_t* datastream_menu(int argc, char* argv[]);
void datastream_menu_set_parent(menu_function_t parent_menu);