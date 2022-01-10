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

/**
 * KeyLight interface.
 */
struct KeyLight
{
    virtual ~KeyLight() = default;
    virtual bool setup() = 0;
    virtual bool digitalWrite(byte level) = 0;
    virtual bool analogWrite(byte level) = 0;
    virtual bool breathe(byte level) = 0;
};

// For now. We'll make this more complex so we can do unicode, macros, etc.
using KeyType = char;

template <size_t ROW_COUNT, size_t COL_COUNT>
class KeyPad final
{
public:
    static constexpr size_t row_count         = ROW_COUNT;
    static constexpr size_t col_count         = COL_COUNT;

    KeyPad(KeyLight* (&key_lights)[ROW_COUNT][COL_COUNT],
           KeyType (&key_map)[ROW_COUNT * COL_COUNT]
           )
        : m_key_lights{key_lights}
        , m_key_map{key_map}
        , m_keyboard_enabled(false)
    {
    }
    ~KeyPad() = default;

    KeyPad(const KeyPad&)  = delete;
    KeyPad(const KeyPad&&) = delete;
    KeyPad& operator=(const KeyPad&) = delete;
    KeyPad& operator=(const KeyPad&&) = delete;

    bool setup(bool enable_keyboard = false)
    {
        m_keyboard_enabled = enable_keyboard;
        if (m_keyboard_enabled)
        {
            Keyboard.begin();
        }

        for (size_t row = 0; row < row_count; ++row)
        {
            for (size_t col = 0; col < col_count; ++col)
            {
                KeyLight* const light = m_key_lights[row][col];
                if (!light || !light->setup())
                {
                    return false;
                }
            }
        }
        return true;
    }

    void onKeyDown(const gh::thirtytwobits::ScanCodeType scancode)
    {
        if (scancode == 0)
        {
            // illegal scancode
            return;
        }
        const size_t scanindex = scancode - 1;
        KeyLight* light = gh::thirtytwobits::itemAt<KeyLight, row_count, col_count>(scancode, m_key_lights);
        if (light)
        {
            light->digitalWrite(HIGH);
        }
        if (m_keyboard_enabled)
        {
            Keyboard.release(m_key_map[scanindex]);
        }
    }

    void onKeyUp(const gh::thirtytwobits::ScanCodeType scancode)
    {
        if (scancode == 0)
        {
            // illegal scancode
            return;
        }
        const size_t scanindex = scancode - 1;
        KeyLight* light = gh::thirtytwobits::itemAt<KeyLight, row_count, col_count>(scancode, m_key_lights);
        if (light)
        {
            light->digitalWrite(LOW);
        }
        if (m_keyboard_enabled)
        {
            Keyboard.press(m_key_map[scanindex]);
        }
    }

    bool setKeyboardEnabled(bool enable_keyboad)
    {
        if (m_keyboard_enabled == enable_keyboad)
        {
            if (!enable_keyboad)
            {
                Keyboard.releaseAll();
                Keyboard.end();
            }
            else
            {
                Keyboard.begin();
            }
            m_keyboard_enabled = enable_keyboad;
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

private:
    KeyLight* (&m_key_lights)[ROW_COUNT][COL_COUNT];
    KeyType (&m_key_map)[ROW_COUNT * COL_COUNT];
    bool m_keyboard_enabled;
};
};  // namespace thirtytwobits
};  // namespace io

#endif  // IO_THIRTYTWOBITS_KEYPAD_H