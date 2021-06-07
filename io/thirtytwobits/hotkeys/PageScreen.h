/*
 *                                                              [h][o][t][ ]
 *                                                              [k][e][y][s]
 * ----------------------------------------------------------------------------
 * Copyright (c) 2021 Scott A Dixon.
 * All Rights Reserved.
 * 
 * This software is distributed under the terms of the MIT License.
 */

#ifndef IO_THIRTYTWOBITS_PAGESCREEN_H
#define IO_THIRTYTWOBITS_PAGESCREEN_H

#include <stdint.h>
#include <stddef.h>
#include "io/thirtytwobits/hotkeys/board.h"

namespace io
{
namespace thirtytwobits
{
namespace hotkeys
{
template <typename IODriverClass,
          typename PageDisplayType = board::PageDisplayType,
          size_t ScreenWidth = board::PageDisplayWidth,
          size_t ScreenHeight = board::PageDisplayHeight>
class PageScreen final
{
    PageScreen(IODriverClass& ioDriver)
        : m_display(ScreenWidth, ScreenHeight, &ioDriver)
    {}
public:
    ~PageScreen() = default;

    PageScreen(const PageScreen&)  = delete;
    PageScreen(const PageScreen&&) = delete;
    PageScreen& operator=(const PageScreen&) = delete;
    PageScreen& operator=(const PageScreen&&) = delete;

    bool begin()
    {
        return m_display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    }

    void clearDisplay()
    {
        m_display.clearDisplay();
    }

    // ------------------------------------------------------------------------
    //  SINGLETON
    // ------------------------------------------------------------------------
    /**
     * You must declare the storage space for this in your sketch. For example:
     * ```
     *     using namespace io::thirtytwobits
     *
     *     // HomeNet singleton storage.
     *     hotkeys::PageScreen hotkeys::PageScreen::singleton;
     *
     *     // alias the singleton to something if you want to avoid the clunkly
     *     // static access syntax.
     *     constexpr hotkeys::PageScreen& page_screen = hotkeys::PageScreen::singleton;
     * ```
     * Why?
     * Because you don't want your firmware to be executing constructors in include statements.
     * This is particularly bad because subsequent includes can alias things after the class
     * is constructed and the order that the singleton is constructed can change based
     * on include guards and which other header is first to include this one. Yes Arduino
     * does this a lot but it's just a bad plan.
     */
    static PageScreen singleton;

private:
    // Declaration for an Adafruit-compatible display connected to ioDriver
    PageDisplayType m_display;
};
};  // namespace hotkeys
};  // namespace thirtytwobits
};  // namespace io

#endif  // IO_THIRTYTWOBITS_PAGESCREEN_H