/*
    _______ _______ _______ _______ _______ _______ _______ _______
    |\     /|\     /|\     /|\     /|\     /|\     /|\     /|\     /|
    | +---+ | +---+ | +---+ | +---+ | +---+ | +---+ | +---+ | +---+ |
    | |   | | |   | | |   | | |   | | |   | | |   | | |   | | |   | |
    | |K  | | |e  | | |y  | | |b  | | |o  | | |a  | | |r  | | |d  | |
    | +---+ | +---+ | +---+ | +---+ | +---+ | +---+ | +---+ | +---+ |
    |/_____\|/_____\|/_____\|/_____\|/_____\|/_____\|/_____\|/_____\|


    _______ _______ _______ _______ _______ _______
    |\     /|\     /|\     /|\     /|\     /|\     /|
    | +---+ | +---+ | +---+ | +---+ | +---+ | +---+ |
    | |   | | |   | | |   | | |   | | |   | | |   | |
    | |M  | | |a  | | |t  | | |r  | | |i  | | |x  | |
    | +---+ | +---+ | +---+ | +---+ | +---+ | +---+ |
    |/_____\|/_____\|/_____\|/_____\|/_____\|/_____\|


    _______ _______ _______ _______ _______ _______
    |\     /|\     /|\     /|\     /|\     /|\     /|
    | +---+ | +---+ | +---+ | +---+ | +---+ | +---+ |
    | |   | | |   | | |   | | |   | | |   | | |   | |
    | |R  | | |e  | | |a  | | |d  | | |e  | | |r  | |
    | +---+ | +---+ | +---+ | +---+ | +---+ | +---+ |
    |/_____\|/_____\|/_____\|/_____\|/_____\|/_____\|

    MIT License

    Copyright (c) 2021 Scott A Dixon

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef IO_THIRTYTWOBITS_KEYBOARD_READER_H
#define IO_THIRTYTWOBITS_KEYBOARD_READER_H

#include <stdint.h>
#include <stddef.h>

namespace io
{
namespace thirtytwobits
{
namespace
{
enum KeyState : uint8_t
{
    UNKNOWN = 0,
    UP      = 1,
    DOWN    = 2
};

struct KeyDef
{
    uint16_t  scancode;
    KeyState state;
    uint8_t  sample_buffer;
};
};  // namespace

typedef void (*KeyHandler)(uint16_t scancode);

/**
 * Raw key scanning logic with debounce. Another class should have the responsibility of
 * providing higher-level functionality like mapping, keylock, chords, long-press, etc.
 */
template <size_t ROW_COUNT, size_t COL_COUNT>
class KeyboardReader final
{
public:

    KeyboardReader(const uint8_t (&row_pins)[ROW_COUNT],
                   const uint8_t (&column_pins)[COL_COUNT],
                   const bool enable_pullups = true,
                   const bool enable_software_debounce = true)
        : m_key_map()
        , m_row_pins()
        , m_col_pins()
        , m_keyhandler_down(nullptr)
        , m_keyhandler_up(nullptr)
        , m_column_input_type((enable_pullups) ? INPUT_PULLUP : INPUT)
        , m_enable_software_debounce(enable_software_debounce)
    {
        memcpy(m_row_pins, row_pins, sizeof(row_pins));
        memcpy(m_col_pins, column_pins, sizeof(column_pins));
        uint16_t scancode = 1;
        for (size_t r = 0; r < ROW_COUNT; ++r)
        {
            for (size_t c = 0; c < COL_COUNT; ++c)
            {
                m_key_map[r][c] = {scancode++, KeyState::UNKNOWN, 0};
            }
        }
    }

    ~KeyboardReader() {}
    KeyboardReader(const KeyboardReader&)  = delete;
    KeyboardReader(const KeyboardReader&&) = delete;
    KeyboardReader& operator=(const KeyboardReader&) = delete;
    KeyboardReader& operator=(const KeyboardReader&&) = delete;

    void setup(KeyHandler keydown_handler = nullptr, KeyHandler keyup_handler = nullptr)
    {
        m_keyhandler_down = keydown_handler;
        m_keyhandler_up   = keyup_handler;

        for (size_t r = 0; r < ROW_COUNT; ++r)
        {
            const uint8_t row_pin = m_row_pins[r];
            pinMode(row_pin, INPUT);
        }
        for (size_t c = 0; c < COL_COUNT; ++c)
        {
            const uint8_t col_pin = m_col_pins[c];
            pinMode(col_pin, m_column_input_type);
        }
        return true;
    }

    void scan()
    {
        for (size_t r = 0; r < ROW_COUNT; ++r)
        {
            const uint8_t rowpin = m_row_pins[r];
            pinMode(rowpin, OUTPUT);
            digitalWrite(rowpin, LOW);
            for (size_t c = 0; c < COL_COUNT; ++c)
            {
                KeyDef& key = m_key_map[r][c];
                // Always sample to ensure the timing is stable despite hyteresis settings.
                const bool value = digitalRead(m_col_pins[c]);
                const bool is_key_pressed = (value == LOW);
                if (m_enable_software_debounce)
                {
                    handleSoftwareDebounce(key, is_key_pressed);
                }
                else
                {
                    key.sample_buffer = (is_key_pressed) ? 0xFFFF : 0x0000;
                }
                handleKeyState(key);
            }
            // High-impedence
            pinMode(rowpin, INPUT);
        }
    }

    bool isKeyDown(uint16_t scancode)
    {
        if (scancode == 0)
        {
            return false;
        }
        const uint16_t scanindex = scancode - 1;
        if (scanindex >= ROW_COUNT * COL_COUNT)
        {
            return false;
        }
        const uint16_t row = scanindex / COL_COUNT;
        const uint16_t col = scanindex - (row * COL_COUNT);
        return (m_key_map[row][col].state == KeyState::DOWN);
    }

    void setEnableSoftwareDebounce(bool enable)
    {
        m_enable_software_debounce = enable;
    }

    bool getEnableSoftwareDebounce() const
    {
        return m_enable_software_debounce;
    }

private:
    // +----------------------------------------------------------------------+
    // | SOFTWARE DEBOUNCING PARAMETERS :: ADJUSTABLE
    // +----------------------------------------------------------------------+

    static constexpr const uint8_t DebounceSettleCount = 4;
    static constexpr const uint8_t DebounceSampleCount = 3;
    static constexpr const uint8_t DebounceSettleBits = 3;
    static constexpr const uint8_t DebounceSampleBits = 5;

    // +----------------------------------------------------------------------+
    // | SOFTWARE DEBOUNCING PARAMETERS :: VALIDATION
    // +----------------------------------------------------------------------+
    static_assert(DebounceSettleCount <= (1 << DebounceSettleBits) - 1, "DebounceSettleCount must fit in DebounceSettleBits bits.");
    static_assert(DebounceSampleCount <= DebounceSampleBits, "DebounceSampleCount must be <= DebounceSampleBits");
    static_assert(DebounceSettleBits + DebounceSampleBits <= 8, "DebounceSampleBits + DebounceSettleBits must be <=8");

    // +----------------------------------------------------------------------+
    // | SOFTWARE DEBOUNCING PARAMETERS :: COMPUTED
    // +----------------------------------------------------------------------+
    static constexpr const uint8_t DebounceSettleShift = DebounceSampleBits;
    static constexpr const uint8_t DebounceSettleMask = ((1 << DebounceSettleBits) - 1) << DebounceSettleShift;
    static constexpr const uint8_t DebounceSampleMask = ((1 << DebounceSampleBits) - 1);
    // No sample shift since we always located it in the LSbs.

    // +----------------------------------------------------------------------+
    // | HELPERS
    // +----------------------------------------------------------------------+
    static constexpr bool is_down(const uint8_t sample_mask, const uint8_t sample_buffer)
    {
        return (sample_mask & sample_buffer) == sample_mask;
    }

    static constexpr bool is_up(const uint8_t sample_mask, const uint8_t sample_buffer)
    {
        return (sample_mask & sample_buffer) == 0;
    }

    static constexpr void reset_sample_count(KeyDef& key)
    {
        key.sample_buffer = (DebounceSampleMask & key.sample_buffer);
    }

    void handleSoftwareDebounce(KeyDef& key, bool is_key_pressed)
    {
        const uint8_t settle_count = (key.sample_buffer & DebounceSettleMask) >> DebounceSettleShift;
        if (settle_count < DebounceSettleCount)
        {
            // We're still in the settle window. Increment the settle count.
            key.sample_buffer = ((settle_count + 1) << DebounceSettleShift) | (DebounceSampleMask & key.sample_buffer);
        }
        else
        {
            key.sample_buffer = (settle_count << DebounceSettleShift) | (DebounceSampleMask & (key.sample_buffer << 1)) | ((is_key_pressed) ? 1 : 0);
        }
    }

    void handleKeyState(KeyDef& key)
    {
        if (is_down(DebounceSampleMask, key.sample_buffer) && key.state != KeyState::DOWN)
        {
            const KeyState oldState = key.state;
            key.state               = KeyState::DOWN;
            onKeyDown(key, oldState);
            reset_sample_count(key);
        }
        else if (is_up(DebounceSampleMask, key.sample_buffer) && key.state != KeyState::UP)
        {
            const KeyState oldState = key.state;
            key.state               = KeyState::UP;
            onKeyUp(key, oldState);
            reset_sample_count(key);
        }
    }

    void onKeyDown(const KeyDef& key, KeyState oldState)
    {
        // If the key is down on power-up then we do emit an onKeyDown.
        (void) oldState;
        const KeyHandler keyhandler_down = m_keyhandler_down;
        if (keyhandler_down != nullptr)
        {
            keyhandler_down(key.scancode);
        }
    }

    void onKeyUp(const KeyDef& key, KeyState oldState)
    {
        if (oldState == KeyState::UNKNOWN)
        {
            // we discard the first key up transition.
            return;
        }
        const KeyHandler keyhandler_up = m_keyhandler_up;
        if (keyhandler_up != nullptr)
        {
            keyhandler_up(key.scancode);
        }
    }

    KeyDef     m_key_map[ROW_COUNT][COL_COUNT];
    uint8_t    m_row_pins[ROW_COUNT];
    uint8_t    m_col_pins[COL_COUNT];
    KeyHandler m_keyhandler_down;
    KeyHandler m_keyhandler_up;
    uint8_t    m_column_input_type;
    bool       m_enable_software_debounce;
};

};  // namespace thirtytwobits
};  // namespace io

#endif  // IO_THIRTYTWOBITS_KEYBOARD_READER_H
