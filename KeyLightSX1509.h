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

#include "KeyPad.h"
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
template<int SSX1509Pin>
class KeyLightSX1509 : public KeyLight
{
public:
    KeyLightSX1509(SX1509& expander)
        : pin(SSX1509Pin)
        , m_expander(expander)
    {}

    virtual bool setup() override
    {
        m_expander.pinMode(SSX1509Pin, OUTPUT);
        m_expander.ledDriverInit(SSX1509Pin);
    }

    virtual bool digitalWrite(byte level) override
    {
        m_expander.digitalWrite(SSX1509Pin, level);
        return true;
    }
    virtual bool analogWrite(byte level) override
    {
        m_expander.analogWrite(SSX1509Pin, level);
        return true;
    }
    virtual bool breathe(byte level) override
    {
        return false;
    }

    const int pin;
private:
    SX1509& m_expander;
};

} // namespace thirtytwobits
} // namespace io

#endif // IO_THIRTYTWOBITS_KEYLIGHT_SX1509_H