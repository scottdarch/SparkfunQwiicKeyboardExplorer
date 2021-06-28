#include <SparkFunSX1509.h>

/*
 *                                                              [h][o][t][ ]
 *                                                              [k][e][y][s]
 * ----------------------------------------------------------------------------
 * Copyright (c) 2021 Scott A Dixon.
 * All Rights Reserved.
 *
 * This software is distributed under the terms of the MIT License.
 */

// ----------------------------------------------------------------------------
//  PLATFORM INCLUDES
// ----------------------------------------------------------------------------
#include <SwitchMatrixScanner.h>
#include <SparkFun_Qwiic_Twist_Arduino_Library.h>
#include <SparkFunSX1509.h>
#include <Adafruit_SSD1306.h>
#include <Keyboard.h>

// ----------------------------------------------------------------------------
//  LOCAL INCLUDES
// ----------------------------------------------------------------------------
#include "board.h"
#include "PageScreen.h"

using namespace io::thirtytwobits;

namespace
{
using PageScreenT = hotkeys::PageScreen<typeof(Wire)>;
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
template <>
PageScreenT            PageScreenT::singleton{Wire};
constexpr PageScreenT& page_screen = PageScreenT::singleton;

TWIST twist;  // Create instance of this object
bool  dirty         = true;
bool  HOTKEYS_HID   = false;
bool  hid_mode_init = false;

char keymap[] = {'1', '2', '3', '4', '5', '6', '7', 'a', 'b', 'c', 'd', 'e', 'f', 'g'};
char display_string[hotkeys::board::KeyMatrixRowCount][hotkeys::board::KeyMatrixColumnCount + 3] =
    {{'_', '_', '_', '_', '_', '_', '_', ' ', ' ', 0}, {'_', '_', '_', '_', '_', '_', '_', ' ', ' ', 0}};
int keylights[hotkeys::board::KeyMatrixRowCount][hotkeys::board::KeyMatrixColumnCount] = {{0, 1, 2, 3, 4, 5, 6},
                                                                                          {7, 8, 9, 10, 11, 12, 13}};
gh::thirtytwobits::SwitchMatrixScanner<hotkeys::board::KeyMatrixRowCount, hotkeys::board::KeyMatrixColumnCount> kbread(
    {A0, A1},
    {A2, 4, 5, 6, 7, 8, 9},
    true,
    true);

SX1509 ioexpander;

void writeDisplayString(bool clearAndDisplay = true)
{
    if (clearAndDisplay)
    {
        page_screen.clearDisplay();
    }
    // display.setTextSize(1);               // Normal 1:1 pixel scale
    // display.setTextColor(SSD1306_WHITE);  // Draw white text
    // display.setCursor(0, 0);              // Start at top-left corner
    // display.println(display_string[0]);
    // display.print(display_string[1]);
    // if (clearAndDisplay)
    // {
    //     display.display();
    // }
}

void onKeyUp(const uint16_t (&scancodes)[decltype(kbread)::event_buffer_size], size_t scancodes_len)
{
    for (size_t i = 0; i < scancodes_len; ++i)
    {
        const uint16_t scanindex = scancodes[i] - 1;
        display_string[scanindex / hotkeys::board::KeyMatrixColumnCount]
                      [scanindex % hotkeys::board::KeyMatrixColumnCount] = '_';
        dirty                                                            = true;
        int keylight = keylights[scanindex / hotkeys::board::KeyMatrixColumnCount]
                                [scanindex % hotkeys::board::KeyMatrixColumnCount];
        ioexpander.digitalWrite(keylight, LOW);
        if (HOTKEYS_HID)
        {
            Keyboard.release(keymap[scanindex]);
        }
    }
}

void onKeyDown(const uint16_t (&scancodes)[decltype(kbread)::event_buffer_size], size_t scancodes_len)
{
    for (size_t i = 0; i < scancodes_len; ++i)
    {
        const uint16_t scanindex = scancodes[i] - 1;
        display_string[scanindex / hotkeys::board::KeyMatrixColumnCount]
                      [scanindex % hotkeys::board::KeyMatrixColumnCount] = 'X';
        dirty                                                            = true;
        int keylight = keylights[scanindex / hotkeys::board::KeyMatrixColumnCount]
                                [scanindex % hotkeys::board::KeyMatrixColumnCount];
        ioexpander.digitalWrite(keylight, HIGH);
        if (HOTKEYS_HID)
        {
            Keyboard.press(keymap[scanindex]);
        }
    }
}

// Check the mode switch position and change between USB-HOTKEYS_HID mode and Serial mode
void checkMode()
{
    const bool hidMode = digitalRead(A3);
    if (HOTKEYS_HID != hidMode || !hid_mode_init)
    {
        hid_mode_init = true;
        HOTKEYS_HID   = hidMode;
        if (!hidMode)
        {
            Keyboard.releaseAll();
            Keyboard.end();
            display_string[0][hotkeys::board::KeyMatrixColumnCount + 1] = ' ';
            dirty                                                       = true;
        }
        else
        {
            Keyboard.begin();
            display_string[0][hotkeys::board::KeyMatrixColumnCount + 1] = 'h';
            dirty                                                       = true;
        }
    }
}

}  // namespace

void setup()
{
    Serial.begin(115200);
    checkMode();
    if (!HOTKEYS_HID)
    {
        // When not in HOTKEYS_HID mode wait for serial or 3 seconds.
        const unsigned long start = millis();
        do
        {
            if (Serial)
            {
                break;
            }
            delay(10);
        } while ((millis() - start) < 3000);
    }
    Serial.println(F("Hotkeys starting..."));

    kbread.setup(onKeyDown, onKeyUp);
    if (!page_screen.begin())
    {
        // Address 0x3C for 128x32
        Serial.println(F("PageScreen allocation failed"));
    }
    if (!twist.begin(Wire, 0x3D))
    {
        Serial.println(F("QWIIC twist allocation failed"));
        display_string[1][hotkeys::board::KeyMatrixColumnCount + 1] = 'e';
    }
    if (!ioexpander.begin(0x3E))
    {
        Serial.println(F("QWIIC SX1509 allocation failed"));
    }
    else
    {
        // hotkeys::KeyMatrixRowCount][hotkeys::KeyMatrixColumnCount
        for (size_t row = 0; row < hotkeys::board::KeyMatrixRowCount; ++row)
        {
            const size_t offset = hotkeys::board::KeyMatrixRowCount * row;
            for (size_t col = 0; col < hotkeys::board::KeyMatrixColumnCount; ++col)
            {
                ioexpander.pinMode(offset + col, OUTPUT);
            }
        }
    }
    pinMode(A3, INPUT_PULLUP);  // initialize mode switch pin
    twist.connectBlue(-10);
    Serial.println(F("OK"));
}

void loop()
{
    kbread.scan();
    checkMode();
    kbread.scan();
    if (dirty)
    {
        writeDisplayString();
    }
    else
    {
        delay(1);
    }
}
