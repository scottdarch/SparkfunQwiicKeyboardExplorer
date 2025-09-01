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
#include <SparkFun_Qwiic_Twist_Arduino_Library.h>
#include <Adafruit_SSD1306.h>
#include <new> // for placement new

// ----------------------------------------------------------------------------
//  LOCAL INCLUDES
// ----------------------------------------------------------------------------
#include "board.h"
#include "arduino_type_traits.h"
#include "PageScreen.h"
#include "KeyPad.h"
#include "KeyLightSX1509.h"

using namespace io::thirtytwobits;

namespace
{
using PageScreenT = hotkeys::PageScreen<4, 2, typeof(Wire)>;
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
template <>
PageScreenT            PageScreenT::singleton{Wire};
constexpr PageScreenT& page_screen = PageScreenT::singleton;

TWIST twist;  // Create instance of this object
bool  dirty         = true;
char  setup_error   = 0;

const char keymap[] = {'1', '2', '3', '4', '5', '6', '7', 'a', 'b', 'c', 'd', 'e', 'f', 'g'};
constexpr size_t display_columns = hotkeys::board::KeyMatrixColumnCount + 3;
char display_string[hotkeys::board::KeyMatrixRowCount * display_columns] =
    {'_', '_', '_', '_', '_', '_', '_', ' ', ' ', 0, '_', '_', '_', '_', '_', '_', '_', ' ', ' ', 0};

/*
   All pins support PWM and blink.
   Only pins with BRE support breathing.
   
   BRE  BRE  BRE  BRE  BRE  BRE  BRE
  +---++---++---++---++---++---++---+
  | 5 || 6 || 7 || 12|| 13|| 14|| 15|
  +---++---++---++---++---++---++---+
    -    -    -    -   BRE   -    -
  +---++---++---++---++---++---++---+
  | 0 || 1 || 2 || 3 || 4 || 8 || 9 |
  +---++---++---++---++---++---++---+
*/
SX1509 ioexpander;

io::thirtytwobits::KeyLightSX1509 keylights[hotkeys::board::KeyMatrixRowCount * hotkeys::board::KeyMatrixColumnCount] =
{ 
          io::thirtytwobits::KeyLightSX1509(ioexpander, 5)
        , io::thirtytwobits::KeyLightSX1509(ioexpander, 6)
        , io::thirtytwobits::KeyLightSX1509(ioexpander, 7)
        , io::thirtytwobits::KeyLightSX1509(ioexpander, 12)
        , io::thirtytwobits::KeyLightSX1509(ioexpander, 13)
        , io::thirtytwobits::KeyLightSX1509(ioexpander, 14)
        , io::thirtytwobits::KeyLightSX1509(ioexpander, 15)
        , io::thirtytwobits::KeyLightSX1509(ioexpander, 0)
        , io::thirtytwobits::KeyLightSX1509(ioexpander, 1)
        , io::thirtytwobits::KeyLightSX1509(ioexpander, 2)
        , io::thirtytwobits::KeyLightSX1509(ioexpander, 3)
        , io::thirtytwobits::KeyLightSX1509(ioexpander, 4)
        , io::thirtytwobits::KeyLightSX1509(ioexpander, 8)
        , io::thirtytwobits::KeyLightSX1509(ioexpander, 9)
};

using KeyPadType = io::thirtytwobits::KeyPad<io::thirtytwobits::KeyLightSX1509,
                                             const char,
                                             hotkeys::board::KeyMatrixRowCount,
                                             hotkeys::board::KeyMatrixColumnCount>;
gh::scottdarch::aligned_storage<sizeof(KeyPadType), alignof(KeyPadType)>::type keypad_storage;
KeyPadType*                                                                    keypad = nullptr;

void writeDisplayString(bool clearAndDisplay = true)
{
    if (clearAndDisplay)
    {
        page_screen.clearDisplay();
    }
    page_screen.println(0, 0, &display_string[0]);
    page_screen.println(0, 1, &display_string[display_columns]);
    if (clearAndDisplay)
    {
        page_screen.display();
    }
    dirty = false;
}

void checkKeyPad()
{
    for (size_t r = 0; r < hotkeys::board::KeyMatrixRowCount; ++r)
    {
        const size_t display_row_offset = (r * display_columns);
        const size_t row_offset = r * hotkeys::board::KeyMatrixColumnCount;
        for (size_t c = 0; c < hotkeys::board::KeyMatrixColumnCount; ++c)
        {
            const size_t i = display_row_offset + c;
            const bool was_pressed = (display_string[i] == 'X');
            bool is_pressed = was_pressed;
            if (keypad)
            {
                is_pressed = keypad->isKeyPressed(row_offset + c + 1);
            }
            if (is_pressed != was_pressed)
            {
                display_string[i] = (is_pressed) ? 'X' : '_';
                dirty             = true;
            }
        }
    }
}

// Check the mode switch position and change between USB-HOTKEYS_HID mode and Serial mode
bool checkMode()
{
    const bool hidMode = digitalRead(A3);
    if (keypad)
    {
        keypad->setKeyboardEnabled(hidMode);
    }
    if (!hidMode)
    {
        display_string[hotkeys::board::KeyMatrixColumnCount + 1] = ' ';
        dirty                                                    = true;
    }
    else
    {
        display_string[hotkeys::board::KeyMatrixColumnCount + 1] = 'h';
        dirty                                                    = true;
    }
    return hidMode;
}

}  // namespace

void setup()
{
    Serial.begin(115200);
    if (!checkMode())
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

    if (!page_screen.begin())
    {
        // Address 0x3C for 128x32
        Serial.println(F("PageScreen allocation failed"));
        setup_error = '1';
    }
    if (!twist.begin(Wire, 0x3D))
    {
        Serial.println(F("QWIIC twist allocation failed"));
        setup_error = '2';
    }
    twist.connectRed(10);
    twist.setLimit(254);

    if (!ioexpander.begin(0x3E))
    {
        Serial.println(F("QWIIC SX1509 allocation failed"));
        setup_error = '3';
    }
    pinMode(A3, INPUT_PULLUP);  // initialize mode switch pin

    keypad = new (keypad_storage.data) KeyPadType(keylights, 
                                                  keymap,
                                                  {A0, A1},
                                                  {A2, 4, 5, 6, 7, 8, 9},
                                                  true,
                                                  true);

    if (!keypad->setup())
    {
        Serial.println(F("KeyPad initialization failed"));
        setup_error = '4';
    }
    else
    {
        Serial.println(F("OK"));
    }

    if (setup_error != 0)
    {
        display_string[display_columns + hotkeys::board::KeyMatrixColumnCount + 1] = 'e';
        dirty = true;
    }
}

int16_t  last_count     = 0;

void loop()
{
    if (!keypad)
    {
        abort();
    }
    if (keypad->scan())
    {
        checkKeyPad();
    }
    const int16_t count = twist.getCount();
    if (count != last_count)
    {
        last_count              = count;
        const uint8_t red_level = twist.getRed();
        if (keypad->setBacklightAmbientLevel(red_level))
        {
            Serial.print("red: ");
            Serial.println(red_level);
        }
    }
    checkMode();
    if(keypad->scan())
    {
        checkKeyPad();
    }
    if (dirty)
    {
        writeDisplayString();
    }
    else
    {
        delay(1);
    }
}
