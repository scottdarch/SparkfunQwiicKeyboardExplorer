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

#ifndef IO_THIRTYTWOBITS_SWITCH_MATRIX_SCANNER_H
#define IO_THIRTYTWOBITS_SWITCH_MATRIX_SCANNER_H

#include <stdint.h>
#include <stddef.h>

namespace io
{
namespace thirtytwobits
{
namespace
{
enum SwitchState : uint8_t
{
    UNKNOWN              = 0,
    OPEN                 = 1,
    CLOSED               = 2,
    OPEN_EVENT_PENDING   = 17,
    CLOSED_EVENT_PENDING = 34
};

struct SwitchDef
{
    uint16_t    scancode;
    SwitchState state;
    uint8_t     sample_buffer;
};
};  // namespace

typedef void (*SwitchHandler)(uint16_t scancode);

/**
 * Raw swtch matrix scanning logic with optional software debounce.
 * 
 * TODO: vector version of callback events.
 */
template <size_t ROW_COUNT, size_t COL_COUNT>
class SwitchMatrixScanner final
{
public:
    SwitchMatrixScanner(const uint8_t (&row_pins)[ROW_COUNT],
                        const uint8_t (&column_pins)[COL_COUNT],
                        const bool enable_pullups           = true,
                        const bool enable_software_debounce = true)
        : m_switch_map()
        , m_row_pins()
        , m_col_pins()
        , m_switchhandler_closed(nullptr)
        , m_switchhandler_open(nullptr)
        , m_column_input_type((enable_pullups) ? INPUT_PULLUP : INPUT)
        , m_enable_software_debounce(enable_software_debounce)
        , m_event_high_water_mark{0, 0}
    {
        memcpy(m_row_pins, row_pins, sizeof(row_pins));
        memcpy(m_col_pins, column_pins, sizeof(column_pins));
        uint16_t scancode = 1;
        for (size_t r = 0; r < ROW_COUNT; ++r)
        {
            for (size_t c = 0; c < COL_COUNT; ++c)
            {
                m_switch_map[r][c] = {scancode++, SwitchState::UNKNOWN, 0};
            }
        }
    }

    ~SwitchMatrixScanner() {}
    SwitchMatrixScanner(const SwitchMatrixScanner&)  = delete;
    SwitchMatrixScanner(const SwitchMatrixScanner&&) = delete;
    SwitchMatrixScanner& operator=(const SwitchMatrixScanner&) = delete;
    SwitchMatrixScanner& operator=(const SwitchMatrixScanner&&) = delete;

    void setup(SwitchHandler switchclosed_handler = nullptr, SwitchHandler switchopen_handler = nullptr)
    {
        m_switchhandler_closed = switchclosed_handler;
        m_switchhandler_open   = switchopen_handler;

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

    void scan(bool flush_events_in_call = true)
    {
        for (size_t r = 0; r < ROW_COUNT; ++r)
        {
            const uint8_t rowpin = m_row_pins[r];
            pinMode(rowpin, OUTPUT);
            digitalWrite(rowpin, LOW);
            for (size_t c = 0; c < COL_COUNT; ++c)
            {
                SwitchDef& swtch = m_switch_map[r][c];
                // Always sample to ensure the timing is stable despite hyteresis settings.
                const bool value             = digitalRead(m_col_pins[c]);
                const bool is_switch_pressed = (value == LOW);
                if (m_enable_software_debounce)
                {
                    handleSoftwareDebounce(swtch, is_switch_pressed);
                }
                else
                {
                    swtch.sample_buffer = (is_switch_pressed) ? 0xFFFF : 0x0000;
                }
                if (handleSwitchState(swtch))
                {
                    m_event_high_water_mark[0] = r + 1;
                    m_event_high_water_mark[1] = c + 1;
                }
            }
            // High-impedence
            pinMode(rowpin, INPUT);
        }
        if (flush_events_in_call)
        {
            flush_events();
        }
    }

    void flush_events()
    {
        for (size_t r = 0; r < m_event_high_water_mark[0]; ++r)
        {
            for (size_t c = 0; c < m_event_high_water_mark[1]; ++c)
            {
                SwitchDef& swtch = m_switch_map[r][c];
                if (swtch.state == SwitchState::OPEN_EVENT_PENDING)
                {
                    onSwitchOpen(swtch);
                    swtch.state = SwitchState::OPEN;
                }
                else if (swtch.state == SwitchState::CLOSED_EVENT_PENDING)
                {
                    onSwitchClosed(swtch);
                    swtch.state = SwitchState::CLOSED;
                }
            }
        }
        m_event_high_water_mark[0] = 0;
        m_event_high_water_mark[1] = 0;
    }

    bool isSwitchClosed(uint16_t scancode)
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
        return (static_cast<SwitchState>(m_switch_map[row][col].state & 0xF) == SwitchState::CLOSED);
    }

private:
    // +----------------------------------------------------------------------+
    // | SOFTWARE DEBOUNCING PARAMETERS :: ADJUSTABLE
    // +----------------------------------------------------------------------+

    static constexpr const uint8_t DebounceSettleCount = 4;
    static constexpr const uint8_t DebounceSampleCount = 3;
    static constexpr const uint8_t DebounceSettleBits  = 3;
    static constexpr const uint8_t DebounceSampleBits  = 5;

    // +----------------------------------------------------------------------+
    // | SOFTWARE DEBOUNCING PARAMETERS :: VALIDATION
    // +----------------------------------------------------------------------+
    static_assert(DebounceSettleCount <= (1 << DebounceSettleBits) - 1,
                  "DebounceSettleCount must fit in DebounceSettleBits bits.");
    static_assert(DebounceSampleCount <= DebounceSampleBits, "DebounceSampleCount must be <= DebounceSampleBits");
    static_assert(DebounceSettleBits + DebounceSampleBits <= 8, "DebounceSampleBits + DebounceSettleBits must be <=8");

    // +----------------------------------------------------------------------+
    // | SOFTWARE DEBOUNCING PARAMETERS :: COMPUTED
    // +----------------------------------------------------------------------+
    static constexpr const uint8_t DebounceSettleShift = DebounceSampleBits;
    static constexpr const uint8_t DebounceSettleMask  = ((1 << DebounceSettleBits) - 1) << DebounceSettleShift;
    static constexpr const uint8_t DebounceSampleMask  = ((1 << DebounceSampleBits) - 1);
    // No sample shift since we always located it in the LSbs.

    // +----------------------------------------------------------------------+
    // | VALIDATE TEMPLATE PARAMS
    // +----------------------------------------------------------------------+
    static constexpr const size_t ScanCodeMax = 0xFFFF;

    static_assert(ROW_COUNT * COL_COUNT < ScanCodeMax, "This class can only scan up to ScanCodeMax switches.");
    static_assert(ROW_COUNT > 0, "ROW_COUNT cannot be 0");
    static_assert(COL_COUNT > 0, "COL_COUNT cannot be 0");

    // +----------------------------------------------------------------------+
    // | HELPERS
    // +----------------------------------------------------------------------+
    static constexpr bool is_closed(const uint8_t sample_mask, const uint8_t sample_buffer)
    {
        return (sample_mask & sample_buffer) == sample_mask;
    }

    static constexpr bool is_open(const uint8_t sample_mask, const uint8_t sample_buffer)
    {
        return (sample_mask & sample_buffer) == 0;
    }

    static constexpr void reset_sample_count(SwitchDef& swtch)
    {
        swtch.sample_buffer = (DebounceSampleMask & swtch.sample_buffer);
    }

    void handleSoftwareDebounce(SwitchDef& swtch, bool is_switch_pressed)
    {
        const uint8_t settle_count = (swtch.sample_buffer & DebounceSettleMask) >> DebounceSettleShift;
        if (settle_count < DebounceSettleCount)
        {
            // We're still in the settle window. Increment the settle count.
            swtch.sample_buffer =
                ((settle_count + 1) << DebounceSettleShift) | (DebounceSampleMask & swtch.sample_buffer);
        }
        else
        {
            swtch.sample_buffer = (settle_count << DebounceSettleShift) |
                                  (DebounceSampleMask & (swtch.sample_buffer << 1)) | ((is_switch_pressed) ? 1 : 0);
        }
    }

    bool handleSwitchState(SwitchDef& swtch)
    {
        bool pending_event = false;
        if (is_closed(DebounceSampleMask, swtch.sample_buffer) && swtch.state != SwitchState::CLOSED)
        {
            const SwitchState oldState = swtch.state;
            swtch.state                = SwitchState::CLOSED_EVENT_PENDING;
            reset_sample_count(swtch);
            pending_event = true;
        }
        else if (is_open(DebounceSampleMask, swtch.sample_buffer) && swtch.state != SwitchState::OPEN)
        {
            const SwitchState oldState = swtch.state;
            if (oldState == SwitchState::UNKNOWN)
            {
                swtch.state   = SwitchState::OPEN_EVENT_PENDING;
                pending_event = true;
            }
            else
            {
                swtch.state = SwitchState::OPEN;
            }
            reset_sample_count(swtch);
        }
        return pending_event;
    }

    void onSwitchClosed(const SwitchDef& swtch)
    {
        const SwitchHandler switchhandler_closed = m_switchhandler_closed;
        if (switchhandler_closed != nullptr)
        {
            switchhandler_closed(swtch.scancode);
        }
    }

    void onSwitchOpen(const SwitchDef& swtch)
    {
        const SwitchHandler switchhandler_open = m_switchhandler_open;
        if (switchhandler_open != nullptr)
        {
            switchhandler_open(swtch.scancode);
        }
    }

    SwitchDef     m_switch_map[ROW_COUNT][COL_COUNT];
    const uint8_t m_row_pins[ROW_COUNT];
    const uint8_t m_col_pins[COL_COUNT];
    SwitchHandler m_switchhandler_closed;
    SwitchHandler m_switchhandler_open;
    const uint8_t m_column_input_type;
    const bool    m_enable_software_debounce;
    uint8_t       m_event_high_water_mark[2];
};

};  // namespace thirtytwobits
};  // namespace io

#endif  // IO_THIRTYTWOBITS_SWITCH_MATRIX_SCANNER_H
