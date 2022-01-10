/*
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

#ifndef IO_THIRTYTWOBITS_KEYPAD_H
#define IO_THIRTYTWOBITS_KEYPAD_H

#include <SwitchMatrixScanner.h>
#include <Keyboard.h>

namespace io
{
namespace thirtytwobits
{

template <typename KeyLightType, typename KeyType, size_t ROW_COUNT, size_t COL_COUNT>
class KeyPad final
{
private:
    static void key_dn(const gh::thirtytwobits::ScanCodeType (&scancodes)[gh::thirtytwobits::SwitchMatrixScanner<ROW_COUNT, COL_COUNT>::event_buffer_size],
                       size_t scancodes_len,
                       void* userdata)
    {
        auto self = reinterpret_cast<KeyPad<KeyLightType, KeyType, ROW_COUNT, COL_COUNT>*>(userdata);
        if (self)
        {
            for (size_t i = 0; i < scancodes_len; ++i)
            {
                self->onKeyDownInternal(scancodes[i]);
            }
        }
    }

    static void key_up(const gh::thirtytwobits::ScanCodeType (&scancodes)[gh::thirtytwobits::SwitchMatrixScanner<ROW_COUNT, COL_COUNT>::event_buffer_size],
                       size_t scancodes_len,
                       void* userdata)
    {
        auto self = reinterpret_cast<KeyPad<KeyLightType, KeyType, ROW_COUNT, COL_COUNT>*>(userdata);
        if (self)
        {
            for (size_t i = 0; i < scancodes_len; ++i)
            {
                self->onKeyUpInternal(scancodes[i]);
            }
        }
    }

public:
    static constexpr size_t row_count         = ROW_COUNT;
    static constexpr size_t col_count         = COL_COUNT;
    static constexpr size_t scancode_max      = ROW_COUNT * COL_COUNT;

    KeyPad(KeyLightType (&key_lights)[ROW_COUNT * COL_COUNT],
           KeyType (&key_map)[ROW_COUNT * COL_COUNT],
           const uint8_t (&row_pins)[ROW_COUNT],
           const uint8_t (&column_pins)[COL_COUNT],
           const bool enable_pullups           = true,
           const bool enable_software_debounce = true)
        : m_kbread(row_pins, column_pins, enable_pullups, enable_software_debounce)
        , m_key_lights{key_lights}
        , m_key_map{key_map}
        , m_keyboard_enabled(false)
        , m_ambient_level(0)
    {
    }
    ~KeyPad() = default;

    KeyPad(const KeyPad&)  = delete;
    KeyPad(const KeyPad&&) = delete;
    KeyPad& operator=(const KeyPad&) = delete;
    KeyPad& operator=(const KeyPad&&) = delete;

    bool setup(bool enable_keyboard = false)
    {
        m_kbread.setup(key_dn, key_up, this);
        if (enable_keyboard != m_keyboard_enabled)
        {
            setKeyboardEnabled(enable_keyboard);
        }
        else if (m_keyboard_enabled)
        {
            Keyboard.begin();
        }

        for(size_t i = 0; i < ROW_COUNT * COL_COUNT; ++i)
        {
            if (!m_key_lights[i].setup())
            {
                return false;
            }
        }
        return true;
    }

    bool scan()
    {
        return m_kbread.scan();
    }

    bool setKeyboardEnabled(bool enable_keyboard)
    {
        if (m_keyboard_enabled != enable_keyboard)
        {
            if (!enable_keyboard)
            {
                Keyboard.releaseAll();
                Keyboard.end();
            }
            else
            {
                Keyboard.begin();
            }
            m_keyboard_enabled = enable_keyboard;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool getKeyboardEnabled() const
    {
        return m_keyboard_enabled;
    }

    bool setBacklightAmbientLevel(byte level)
    {
        if (m_ambient_level != level)
        {
            for(size_t i = 0; i < ROW_COUNT * COL_COUNT; ++i)
            {
                m_key_lights[i].analogWrite(level);
            }
            m_ambient_level = level;
            return true;
        }
        else
        {
            return false;
        }
    }

    byte getBacklightAmbientLevel() const
    {
        return m_ambient_level;
    }

    bool isKeyPressed(gh::thirtytwobits::ScanCodeType scancode)
    {
        return m_kbread.isSwitchClosed(scancode);
    }

private:

    void onKeyDownInternal(const gh::thirtytwobits::ScanCodeType scancode)
    {
        if (scancode == 0 || scancode > scancode_max)
        {
            // illegal scancode
            return;
        }
        const size_t scanindex = scancode - 1;
        KeyLightType& light = m_key_lights[scanindex];
        light.digitalWrite(HIGH);
        if (m_keyboard_enabled)
        {
            Keyboard.press(m_key_map[scanindex]);
        }
    }

    void onKeyUpInternal(const gh::thirtytwobits::ScanCodeType scancode)
    {
        if (scancode == 0 || scancode > scancode_max)
        {
            // illegal scancode
            return;
        }
        const size_t scanindex = scancode - 1;
        KeyLightType& light = m_key_lights[scanindex];
        light.digitalWrite(LOW);
        if (m_keyboard_enabled)
        {
            Keyboard.release(m_key_map[scanindex]);
        }
    }

    gh::thirtytwobits::SwitchMatrixScanner<ROW_COUNT, COL_COUNT> m_kbread;
    KeyLightType (&m_key_lights)[ROW_COUNT * COL_COUNT];
    KeyType (&m_key_map)[ROW_COUNT * COL_COUNT];
    bool m_keyboard_enabled;
    byte m_ambient_level;
};
};  // namespace thirtytwobits
};  // namespace io

#endif  // IO_THIRTYTWOBITS_KEYPAD_H