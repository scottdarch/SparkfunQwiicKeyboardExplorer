#include <Adafruit_SSD1306.h>
#include <splash.h>

#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT_Macros.h>
#include <Adafruit_SPITFT.h>
#include <gfxfont.h>

#include "src/SwitchMatrixScanner.h"
#include <Adafruit_SSD1306.h>
#include <SparkFun_Qwiic_Twist_Arduino_Library.h>
#include <SparkFunSX1509.h>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
TWIST            twist;  // Create instance of this object

char keymap[] = {'1', '2', '3', '4', '5', '6', '7', 'a', 'b', 'c', 'd', 'e', 'f', 'g'};
io::thirtytwobits::SwitchMatrixScanner<2, 7> kbread({A0, A1}, {A2, 4, 5, 6, 7, 8, 9}, true, true);

namespace
{
void writeToDisplay(const __FlashStringHelper* words, bool clearAndDisplay = true)
{
    if (clearAndDisplay)
    {
        display.clearDisplay();
    }
    display.setTextSize(1);               // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);  // Draw white text
    display.setCursor(0, 0);              // Start at top-left corner
    display.println(words);
    if (clearAndDisplay)
    {
        display.display();
    }
}

void writeToDisplay(const char* line, bool clearAndDisplay = true)
{
    if (clearAndDisplay)
    {
        display.clearDisplay();
    }
    display.setTextSize(1);               // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);  // Draw white text
    display.setCursor(0, 0);              // Start at top-left corner
    display.println(line);
    if (clearAndDisplay)
    {
        display.display();
    }
}

void onKeyUp(const uint16_t (&scancode)[decltype(kbread)::event_buffer_size], size_t scancodes_len) {}

void onKeyDown(const uint16_t (&scancodes)[decltype(kbread)::event_buffer_size], size_t scancodes_len)
{
    static char    line_buffer[20] = {0};
    static uint8_t p               = 0;
    for (size_t i = 0; i < scancodes_len; ++i)
    {
        const uint16_t scancode = scancodes[i];
        const char     typed    = keymap[scancode - 1];
        line_buffer[p++]        = typed;
        if (p == 19)
        {
            p = 0;
        }
    }
    writeToDisplay(line_buffer);
}

}  // namespace

void setup()
{
    Serial.begin(115200);
    kbread.setup(onKeyDown, onKeyUp);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {  // Address 0x3C for 128x32
        Serial1.println(F("SSD1306 allocation failed"));
        while (1)
            ;  // Don't proceed, loop forever
    }
    Serial.println(F("OK"));
    writeToDisplay(F("OK"));
}

void loop()
{
    kbread.scan();
}
