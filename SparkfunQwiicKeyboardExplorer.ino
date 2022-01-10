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

io::thirtytwobits::KeyType keymap[] = {'1', '2', '3', '4', '5', '6', '7', 'a', 'b', 'c', 'd', 'e', 'f', 'g'};
char display_string[hotkeys::board::KeyMatrixRowCount][hotkeys::board::KeyMatrixColumnCount + 3] =
    {{'_', '_', '_', '_', '_', '_', '_', ' ', ' ', 0}, {'_', '_', '_', '_', '_', '_', '_', ' ', ' ', 0}};

/*
   PWM  PWM  PWM  PWM  PWM  PWM  PWM
   BLK  BLK  BLK  BLK  BLK  BLK  BLK
    -    -    -   BRE  BRE  BRE  BRE
  +---++---++---++---++---++---++---+
  | 8 || 9 || 10|| 12|| 13|| 14|| 15|
  +---++---++---++---++---++---++---+
  +---++---++---++---++---++---++---+
  | 0 || 1 || 2 || 4 || 5 || 6 || 7 |
  +---++---++---++---++---++---++---+
*/
SX1509 ioexpander;

auto ioex0  = io::thirtytwobits::KeyLightSX1509<0>(ioexpander);
auto ioex1  = io::thirtytwobits::KeyLightSX1509<1>(ioexpander);
auto ioex2  = io::thirtytwobits::KeyLightSX1509<2>(ioexpander);
auto ioex4  = io::thirtytwobits::KeyLightSX1509<4>(ioexpander);
auto ioex5  = io::thirtytwobits::KeyLightSX1509<5>(ioexpander);
auto ioex6  = io::thirtytwobits::KeyLightSX1509<6>(ioexpander);
auto ioex7  = io::thirtytwobits::KeyLightSX1509<7>(ioexpander);

auto ioex8  = io::thirtytwobits::KeyLightSX1509<8>(ioexpander);
auto ioex9  = io::thirtytwobits::KeyLightSX1509<9>(ioexpander);
auto ioex10 = io::thirtytwobits::KeyLightSX1509<10>(ioexpander);
auto ioex12 = io::thirtytwobits::KeyLightSX1509<12>(ioexpander);
auto ioex13 = io::thirtytwobits::KeyLightSX1509<13>(ioexpander);
auto ioex14 = io::thirtytwobits::KeyLightSX1509<14>(ioexpander);
auto ioex15 = io::thirtytwobits::KeyLightSX1509<15>(ioexpander);


io::thirtytwobits::KeyLight* keylights[hotkeys::board::KeyMatrixRowCount][hotkeys::board::KeyMatrixColumnCount] =
    {
        {
            &ioex8, &ioex9, &ioex10, &ioex12, &ioex13, &ioex14, &ioex15
        },
        {
            &ioex0, &ioex1, &ioex2, &ioex4, &ioex5, &ioex6, &ioex7
        }
    };

gh::thirtytwobits::SwitchMatrixScanner<hotkeys::board::KeyMatrixRowCount, hotkeys::board::KeyMatrixColumnCount> kbread(
    {A0, A1},
    {A2, 4, 5, 6, 7, 8, 9},
    true,
    true);

using KeyPadType = io::thirtytwobits::KeyPad<hotkeys::board::KeyMatrixRowCount, hotkeys::board::KeyMatrixColumnCount>;
gh::scottdarch::aligned_storage<sizeof(KeyPadType), alignof(KeyPadType)>::type keypad_storage;
KeyPadType*                                                                    keypad = nullptr;

void writeDisplayString(bool clearAndDisplay = true)
{
    if (clearAndDisplay)
    {
        page_screen.clearDisplay();
    }
    page_screen.println(0, 0, display_string[0]);
    page_screen.println(0, 1, display_string[1]);
    if (clearAndDisplay)
    {
        page_screen.display();
    }
}

void onKeyUp(const gh::thirtytwobits::ScanCodeType (&scancodes)[decltype(kbread)::event_buffer_size],
             size_t scancodes_len)
{
    for (size_t i = 0; i < scancodes_len; ++i)
    {
        const gh::thirtytwobits::ScanCodeType scanindex = scancodes[i] - 1;
        display_string[scanindex / hotkeys::board::KeyMatrixColumnCount]
                      [scanindex % hotkeys::board::KeyMatrixColumnCount] = '_';
        dirty                                                            = true;
    }
}

void onKeyDown(const gh::thirtytwobits::ScanCodeType (&scancodes)[decltype(kbread)::event_buffer_size],
               size_t scancodes_len)
{
    for (size_t i = 0; i < scancodes_len; ++i)
    {
        const gh::thirtytwobits::ScanCodeType scanindex = scancodes[i] - 1;
        display_string[scanindex / hotkeys::board::KeyMatrixColumnCount]
                      [scanindex % hotkeys::board::KeyMatrixColumnCount] = 'X';
        dirty                                                            = true;
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
        display_string[0][hotkeys::board::KeyMatrixColumnCount + 1] = ' ';
        dirty                                                       = true;
    }
    else
    {
        display_string[0][hotkeys::board::KeyMatrixColumnCount + 1] = 'h';
        dirty                                                       = true;
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
    twist.connectRed(10);
    twist.setLimit(254);

    if (!ioexpander.begin(0x3E))
    {
        Serial.println(F("QWIIC SX1509 allocation failed"));
    }
    pinMode(A3, INPUT_PULLUP);  // initialize mode switch pin

    keypad = new (keypad_storage.data) KeyPadType(keylights, keymap);

    if (!keypad->setup())
    {
        Serial.println(F("KeyPad initialization failed"));
    }
    else
    {
        Serial.println(F("OK"));
    }
}

unsigned schedule       = 0;
int16_t  last_count     = 0;
uint8_t  last_red_level = 0;

void loop()
{
    kbread.scan();
    schedule += 1;
    const int16_t count = twist.getCount();
    if (count != last_count)
    {
        Serial.println(count);
        last_count              = count;
        const uint8_t red_level = twist.getRed();
        if (last_red_level != red_level)
        {
            Serial.print("red: ");
            Serial.println(red_level);
            for (size_t row = 0; row < hotkeys::board::KeyMatrixRowCount; ++row)
            {
                for (size_t col = 0; col < hotkeys::board::KeyMatrixColumnCount; ++col)
                {
                    ioexpander.analogWrite(keylights[row][col], red_level);
                }
            }
            last_red_level = red_level;
        }
    }
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
