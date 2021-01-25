
#include <SwitchMatrixScanner.h>
#include <SparkFun_Qwiic_Twist_Arduino_Library.h>
#include <SparkFunSX1509.h>
#include <Adafruit_SSD1306.h>
#include <Keyboard.h>

namespace
{
constexpr size_t SCREEN_WIDTH  = 128;  // OLED display width, in pixels
constexpr size_t SCREEN_HEIGHT = 32;   // OLED display height, in pixels
constexpr size_t ROWS          = 2;
constexpr size_t COLS          = 7;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
TWIST            twist;  // Create instance of this object
bool             dirty = true;
bool             HID = false;

char keymap[]                       = {'1', '2', '3', '4', '5', '6', '7', 'a', 'b', 'c', 'd', 'e', 'f', 'g'};
char display_string[ROWS][COLS + 3] = {{'_', '_', '_', '_', '_', '_', '_', ' ', ' ', 0}, {'_', '_', '_', '_', '_', '_', '_', ' ', ' ', 0}};
gh::thirtytwobits::SwitchMatrixScanner<ROWS, COLS> kbread({A0, A1}, {A2, 4, 5, 6, 7, 8, 9}, true, true);


void writeDisplayString(bool clearAndDisplay = true)
{
    if (clearAndDisplay)
    {
        display.clearDisplay();
    }
    display.setTextSize(1);               // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);  // Draw white text
    display.setCursor(0, 0);              // Start at top-left corner
    display.println(display_string[0]);
    display.print(display_string[1]);
    if (clearAndDisplay)
    {
        display.display();
    }
}

void onKeyUp(const uint16_t (&scancodes)[decltype(kbread)::event_buffer_size], size_t scancodes_len)
{
    for (size_t i = 0; i < scancodes_len; ++i)
    {
        const uint16_t scanindex                           = scancodes[i] - 1;
        display_string[scanindex / COLS][scanindex % COLS] = '_';
        dirty                                              = true;
        if (HID)
        {
            Keyboard.release(keymap[scanindex]);
        }
    }
}

void onKeyDown(const uint16_t (&scancodes)[decltype(kbread)::event_buffer_size], size_t scancodes_len)
{
    for (size_t i = 0; i < scancodes_len; ++i)
    {
        const uint16_t scanindex                           = scancodes[i] - 1;
        display_string[scanindex / COLS][scanindex % COLS] = 'X';
        dirty                                              = true;
        if (HID)
        {
            Keyboard.press(keymap[scanindex]);
        }
    }
}

// Check the mode switch position and change between USB-HID mode and Serial mode
void checkMode()
{
    if (HID)
    {
        if (!digitalRead(A3))
        {
            HID = false;
            Keyboard.releaseAll();
            Keyboard.end();
            display_string[0][COLS+1] = ' ';
            dirty = true;
        }
    }
    else
    {
        if (digitalRead(A3))
        {
            HID = true;
            Keyboard.begin();
            display_string[0][COLS + 1] = 'h';
            dirty = true;
        }
    }
}

}  // namespace

void setup()
{
    Serial.begin(115200);
    kbread.setup(onKeyDown, onKeyUp);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {  // Address 0x3C for 128x32
        Serial.println(F("SSD1306 allocation failed"));
        while (1)
            ;  // Don't proceed, loop forever
    }
    if (!twist.begin(Wire, 0x3D))
    {
        Serial.println(F("QWIIC twist allocation failed"));
        display_string[1][COLS + 1] = 'e';
    }
    pinMode(A3, INPUT_PULLUP);          // initialize mode switch pin
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
