/*
    MIT License

    Copyright (c) 2022 Scott A Dixon

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

#ifndef IO_THIRTYTWOBITS_KEYLIGHT_SX1509_H
#define IO_THIRTYTWOBITS_KEYLIGHT_SX1509_H

#include <SparkFunSX1509.h>

namespace io
{
namespace thirtytwobits
{
/*
   SX1509's capabilities:

   pins ->  |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | 15 |
   ---------+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
   PWM      |  x |  x |  x |  x |  x |  x |  x |  x |  x |  x |  x |  x |  x |  x |  x |  x |
   Blink    |  x |  x |  x |  x |  x |  x |  x |  x |  x |  x |  x |  x |  x |  x |  x |  x |
   Breath   |    |    |    |    |  x |  x |  x |  x |    |    |    |    |  x |  x |  x |  x |
*/

/**
 * Implementation of a KeyPad::KeyLight interface using the SX1509 IO expander.
 */
class KeyLightSX1509 final
{
public:
    KeyLightSX1509(SX1509& expander, int pin)
        : m_expander(expander)
        , m_pin(pin)
    {}

    bool setup()
    {
        m_expander.pinMode(m_pin, OUTPUT);
        m_expander.ledDriverInit(m_pin);
        return true;
    }

    bool digitalWrite(byte level)
    {
        m_expander.digitalWrite(m_pin, level);
        return true;
    }

    bool analogWrite(byte level)
    {
        m_expander.analogWrite(m_pin, level);
        return true;
    }

    bool breathe(byte level)
    {
        return false;
    }

private:
    SX1509& m_expander;
    const int m_pin;
};

} // namespace thirtytwobits
} // namespace io

#endif // IO_THIRTYTWOBITS_KEYLIGHT_SX1509_H