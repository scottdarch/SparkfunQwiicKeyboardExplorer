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
#include "board.h"

namespace io
{
namespace thirtytwobits
{
namespace hotkeys
{
template <size_t PageCountForInstance,
          size_t LineCountForInstance,
          typename IODriverClass,
          typename PageDisplayType = board::PageDisplayType,
          size_t ScreenWidth       = board::PageScreenWidth,
          size_t ScreenHeight      = board::PageScreenHeight>
class PageScreen final
{
    PageScreen(IODriverClass& ioDriver)
        : m_current_page(0)
        , m_display(ScreenWidth, ScreenHeight, &ioDriver)
        , m_em_box(0)
    {}

public:
    ~PageScreen() = default;

    PageScreen(const PageScreen&)  = delete;
    PageScreen(const PageScreen&&) = delete;
    PageScreen& operator=(const PageScreen&) = delete;
    PageScreen& operator=(const PageScreen&&) = delete;

    static constexpr size_t PageCount = PageCountForInstance;
    static constexpr size_t LineCount = LineCountForInstance;

    bool begin()
    {
        static_assert(PageCountForInstance > 0, "PageCountForInstance must be at least 1");

        if(!m_display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
        {
            return false;
        }
        m_display.setTextColor(SSD1306_WHITE);
        int16_t dont_care[3];
        m_display.getTextBounds("M", 0, 0, &dont_care[0], &dont_care[1], &dont_care[2], &m_em_box);
        m_display.setTextWrap(false);
        return true;
    }

    void clearDisplay()
    {
        m_display.clearDisplay();
    }

    void display()
    {
        m_display.setCursor(0, LineCount * m_em_box);
        m_display.print(m_current_page);
        m_display.display();
    }

    int println(size_t page, size_t line, const char* text)
    {
        if (page >= PageCount)
        {
            return -1;
        }
        if (line >= LineCount)
        {
            return -2;
        }
        m_display.setCursor(0, m_em_box * line);
        // TODO: store the text in a page buffer and restore if/when page
        // flips occur.
        m_display.print(text);
        return 1;
    }

    bool setPage(int pageNumber)
    {
        const size_t old_page_number = m_current_page;
        m_current_page = abs(pageNumber) % PageCount;
        return (m_current_page != old_page_number);
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
    size_t m_current_page;
    // Declaration for an Adafruit-compatible display connected to ioDriver
    PageDisplayType m_display;

    int16_t m_em_box;
    
};
};  // namespace hotkeys
};  // namespace thirtytwobits
};  // namespace io

#endif  // IO_THIRTYTWOBITS_PAGESCREEN_H