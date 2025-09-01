/*
 *                                                              [h][o][t][ ]
 *                                                              [k][e][y][s]
 * ----------------------------------------------------------------------------
 * Copyright (c) 2021 Scott A Dixon.
 * All Rights Reserved.
 *
 * This software is distributed under the terms of the MIT License.
 */

#ifndef IO_THIRTYTWOBITS_HOTKEYS_BOARD_H
#define IO_THIRTYTWOBITS_HOTKEYS_BOARD_H

#include <Adafruit_SSD1306.h>

// Definitions for my hot keys rig built out of the Sparkfun QWIIC keyboard
// explorer (https://www.sparkfun.com/products/17251)

namespace io
{
namespace thirtytwobits
{
namespace hotkeys
{
namespace board
{
using PageDisplayType                 = Adafruit_SSD1306;
constexpr size_t PageScreenWidth      = 128;  // OLED display width, in pixels
constexpr size_t PageScreenHeight     = 32;   // OLED display height, in pixels
constexpr size_t KeyMatrixRowCount    = 2;
constexpr size_t KeyMatrixColumnCount = 7;
};  // namespace board
};  // namespace hotkeys
};  // namespace thirtytwobits
};  // namespace io

#endif  // IO_THIRTYTWOBITS_HOTKEYS_BOARD_H
