/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Product name: redemption, a FLOSS RDP proxy
 *   Copyright (C) Wallix 2010-2012
 *   Author(s): Christophe Grosjean, Dominique Lafages, Jonathan Poelen,
 *              Meng Tan
 */

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestWidgetGrid
#include <boost/test/auto_unit_test.hpp>

#define LOGNULL
//#define LOGPRINT

#include "config.hpp"
#include "internal/widget2/label.hpp"
#include "internal/widget2/grid.hpp"
#include "internal/widget2/screen.hpp"
#include "internal/widget2/flat_button.hpp"
#include "check_sig.hpp"

#undef OUTPUT_FILE_PATH
#define OUTPUT_FILE_PATH "./"

#include "fake_draw.hpp"

BOOST_AUTO_TEST_CASE(TraceWidgetGrid)
{
    TestDraw drawable(800, 600);

    Inifile ini(FIXTURES_PATH "/dejavu-sans-10.fv1");

    // WidgetLabel is a label widget at position 0,0 in it's parent context
    WidgetScreen parent(drawable, 800, 600, ini.font);
    NotifyApi * notifier = NULL;
    int fg_color = RED;
    int bg_color = YELLOW;
    int id = 0;
    bool auto_resize = true;
    int16_t x = 10;
    int16_t y = 10;
    // int xtext = 4;
    // int ytext = 1;

    const uint16_t line_number   = 5;
    const uint16_t column_number = 4;
    const uint16_t grid_border   = 2;

    TODO("I believe users of this widget may wish to control text position and behavior inside rectangle"
         "ie: text may be centered, aligned left, aligned right, or even upside down, etc"
         "these possibilities (and others) are supported in RDPGlyphIndex")
    WidgetGrid wgrid(drawable, Rect(x, y, 640, 480), parent, notifier, line_number, column_number,
        PALE_BLUE, BLACK, LIGHT_BLUE, BLACK, WINBLUE, WHITE, MEDIUM_BLUE, WHITE,
        grid_border, id);

/*
    wgrid.set_sizing_strategy(0, 50, 150);
    wgrid.set_sizing_strategy(1, 150, 800);
    wgrid.set_sizing_strategy(2, 50, 150);
    wgrid.set_sizing_strategy(3, 50, 100);
*/

    Widget2  * widgetTable[128] = { 0 };
    uint16_t   widget_count     = 0;

    for (uint16_t line_index = 0; line_index < line_number; line_index++) {
        for (uint16_t column_index = 0; column_index < column_number; column_index++) {
            char text[256];
            snprintf(text, sizeof(text), "Label %ux%u", line_index, column_index);
            if ((line_index == 2) && (column_index == 3)) {
                widgetTable[widget_count] = new WidgetFlatButton(drawable, 0, 0, wgrid, notifier,
                                                            text, auto_resize, id, WHITE, MEDIUM_BLUE, LIGHT_BLUE, ini.font, 2, 2);
            }
            else {
                widgetTable[widget_count] = new WidgetLabel(drawable, 0, 0, wgrid, notifier,
                                                            text, auto_resize, id, fg_color, bg_color, ini.font);
            }
            wgrid.set_widget(line_index, column_index, widgetTable[widget_count]);
            widget_count++;
        }
    }

    ColumnWidthStrategy column_width_strategies[] = {
        { 50, 150 }, { 150, 800 }, { 50, 150 }, { 50, 100 }
    };

    uint16_t row_height[GRID_NB_ROWS_MAX]      = { 0 };
    uint16_t column_width[GRID_NB_COLUMNS_MAX] = { 0 };

    compute_format(wgrid, column_width_strategies, row_height, column_width);
    apply_format(wgrid, row_height, column_width);

    wgrid.set_selection(2, static_cast<uint16_t>(-1));

    // ask to widget to redraw at it's current position
    wgrid.rdp_input_invalidate(Rect(0 + wgrid.dx(),
                                    0 + wgrid.dy(),
                                    wgrid.cx(),
                                    wgrid.cy()));

    // drawable.save_to_png(OUTPUT_FILE_PATH "grid.png");

    char message[1024];
    if (!check_sig(drawable.gd.impl(), message,
        "\xf0\x99\xa7\x5c\xa8\xd4\x21\xc2\x60\x19\xa7\xda\x26\x16\x59\x2d\x73\x1f\xbb\x6b"
    )){
        BOOST_CHECK_MESSAGE(false, message);
    }


    wgrid.set_selection(4, static_cast<uint16_t>(-1));

    // ask to widget to redraw at it's current position
    wgrid.rdp_input_invalidate(Rect(0 + wgrid.dx(),
                                    0 + wgrid.dy(),
                                    wgrid.cx(),
                                    wgrid.cy()));
    // drawable.save_to_png(OUTPUT_FILE_PATH "grid2.png");

    if (!check_sig(drawable.gd.impl(), message,
        "\x58\x42\xcd\x8e\xde\x43\x0b\x31\x58\x0b\x0e\x77\xdf\x31\xee\x76\x8d\x8b\x29\xbb"
    )){
        BOOST_CHECK_MESSAGE(false, message);
    }


    uint16_t mouse_x = wgrid.dx() + 50;
    uint16_t mouse_y = widgetTable[5]->dy();

    wgrid.rdp_input_mouse(MOUSE_FLAG_BUTTON1 | MOUSE_FLAG_DOWN, mouse_x, mouse_y, 0);
    wgrid.rdp_input_mouse(MOUSE_FLAG_BUTTON1, mouse_x, mouse_y, 0);
    // ask to widget to redraw at it's current position
    wgrid.rdp_input_invalidate(Rect(0 + wgrid.dx(),
                                    0 + wgrid.dy(),
                                    wgrid.cx(),
                                    wgrid.cy()));
    //drawable.draw(RDPOpaqueRect(Rect(mouse_x, mouse_y, 2, 2), PINK), wgrid.rect);
    // drawable.save_to_png(OUTPUT_FILE_PATH "grid3.png");

    if (!check_sig(drawable.gd.impl(), message,
        "\xbe\x20\xc9\xff\x05\x59\x01\x79\x47\x82\xa0\x63\xf6\x9a\x42\x9e\xf5\x0b\xb0\xf6"
    )){
        BOOST_CHECK_MESSAGE(false, message);
    }


    Keymap2 keymap;
    keymap.init_layout(0x040C);

    keymap.push_kevent(Keymap2::KEVENT_LEFT_ARROW);
    wgrid.rdp_input_scancode(0,0,0,0, &keymap);
    keymap.push_kevent(Keymap2::KEVENT_DOWN_ARROW);
    wgrid.rdp_input_scancode(0,0,0,0, &keymap);
    keymap.push_kevent(Keymap2::KEVENT_DOWN_ARROW);
    wgrid.rdp_input_scancode(0,0,0,0, &keymap);
    keymap.push_kevent(Keymap2::KEVENT_DOWN_ARROW);
    wgrid.rdp_input_scancode(0,0,0,0, &keymap);

    // ask to widget to redraw at it's current position
    wgrid.rdp_input_invalidate(Rect(0 + wgrid.dx(),
                                    0 + wgrid.dy(),
                                    wgrid.cx(),
                                    wgrid.cy()));
    // drawable.save_to_png(OUTPUT_FILE_PATH "grid4.png");

    if (!check_sig(drawable.gd.impl(), message,
        "\xe8\xbb\x3b\xe6\xe9\xd7\x66\x9d\x72\xab\xab\x98\xd1\x9e\x8e\xba\x80\xe4\xbe\x56"
    )){
        BOOST_CHECK_MESSAGE(false, message);
    }

    wgrid.clear();

    for (Widget2 ** w = widgetTable; *w; w++) {
        delete (*w);
    }
}

/*
BOOST_AUTO_TEST_CASE(TraceWidgetLabel2)
{
    TestDraw drawable(800, 600);

    // WidgetLabel is a label widget of size 100x20 at position 10,100 in it's parent context
    WidgetScreen parent(drawable, 800, 600);
    NotifyApi * notifier = NULL;
    int fg_color = RED;
    int bg_color = YELLOW;
    int id = 0;
    bool auto_resize = true;
    int16_t x = 10;
    int16_t y = 100;

    WidgetLabel wlabel(drawable, x, y, parent, notifier, "test2", auto_resize, id, fg_color, bg_color);

    // ask to widget to redraw at it's current position
    wlabel.rdp_input_invalidate(Rect(0 + wlabel.dx(),
                                     0 + wlabel.dy(),
                                     wlabel.cx(),
                                     wlabel.cy()));

    //drawable.save_to_png(OUTPUT_FILE_PATH "label2.png");

    char message[1024];
    if (!check_sig(drawable.gd.impl(), message,
        "\xc2\x24\xac\x83\xee\xdc\x69\x2d\x01\x94"
        "\xfc\xe9\x2b\x45\xa8\x4a\xa9\x89\xde\x6d")){
        BOOST_CHECK_MESSAGE(false, message);
    }
}

BOOST_AUTO_TEST_CASE(TraceWidgetLabel3)
{
    TestDraw drawable(800, 600);

    // WidgetLabel is a label widget of size 100x20 at position -10,500 in it's parent context
    WidgetScreen parent(drawable, 800, 600);
    NotifyApi * notifier = NULL;
    int fg_color = RED;
    int bg_color = YELLOW;
    int id = 0;
    bool auto_resize = true;
    int16_t x = -10;
    int16_t y = 500;

    WidgetLabel wlabel(drawable, x, y, parent, notifier, "test3", auto_resize, id, fg_color, bg_color);

    // ask to widget to redraw at it's current position
    wlabel.rdp_input_invalidate(Rect(0 + wlabel.dx(),
                                     0 + wlabel.dy(),
                                     wlabel.cx(),
                                     wlabel.cy()));

    //drawable.save_to_png(OUTPUT_FILE_PATH "label3.png");

    char message[1024];
    if (!check_sig(drawable.gd.impl(), message,
        "\x40\x9a\xff\xfd\x37\x16\x19\xa3\x3a\x92"
        "\xac\x4c\x1d\x7c\x6e\x47\xd1\x14\x33\x01")){
        BOOST_CHECK_MESSAGE(false, message);
    }
}

BOOST_AUTO_TEST_CASE(TraceWidgetLabel4)
{
    TestDraw drawable(800, 600);

    // WidgetLabel is a label widget of size 100x20 at position 770,500 in it's parent context
    WidgetScreen parent(drawable, 800, 600);
    NotifyApi * notifier = NULL;
    int fg_color = RED;
    int bg_color = YELLOW;
    int id = 0;
    bool auto_resize = true;
    int16_t x = 770;
    int16_t y = 500;

    WidgetLabel wlabel(drawable, x, y, parent, notifier, "test4", auto_resize, id, fg_color, bg_color);

    // ask to widget to redraw at it's current position
    wlabel.rdp_input_invalidate(Rect(0 + wlabel.dx(),
                                     0 + wlabel.dy(),
                                     wlabel.cx(),
                                     wlabel.cy()));

    //drawable.save_to_png(OUTPUT_FILE_PATH "label4.png");

    char message[1024];
    if (!check_sig(drawable.gd.impl(), message,
        "\x55\xb9\x08\xd3\x42\x16\x47\x4d\x62\xa7"
        "\xfc\xce\x0d\x18\x9c\x29\x82\xd6\xf2\x38")){
        BOOST_CHECK_MESSAGE(false, message);
    }
}

BOOST_AUTO_TEST_CASE(TraceWidgetLabel5)
{
    TestDraw drawable(800, 600);

    // WidgetLabel is a label widget of size 100x20 at position -20,-7 in it's parent context
    WidgetScreen parent(drawable, 800, 600);
    NotifyApi * notifier = NULL;
    int fg_color = RED;
    int bg_color = YELLOW;
    int id = 0;
    bool auto_resize = true;
    int16_t x = -20;
    int16_t y = -7;

    WidgetLabel wlabel(drawable, x, y, parent, notifier, "test5", auto_resize, id, fg_color, bg_color);

    // ask to widget to redraw at it's current position
    wlabel.rdp_input_invalidate(Rect(0 + wlabel.dx(),
                                     0 + wlabel.dy(),
                                     wlabel.cx(),
                                     wlabel.cy()));

    //drawable.save_to_png(OUTPUT_FILE_PATH "label5.png");

    char message[1024];
    if (!check_sig(drawable.gd.impl(), message,
        "\x3c\xa9\xf2\x32\x51\xc4\x70\x8c\xfe\x26"
        "\xc8\x37\xa1\xdb\x5a\xdb\x82\xad\x1f\x67")){
        BOOST_CHECK_MESSAGE(false, message);
    }
}

BOOST_AUTO_TEST_CASE(TraceWidgetLabel6)
{
    TestDraw drawable(800, 600);

    // WidgetLabel is a label widget of size 100x20 at position 760,-7 in it's parent context
    WidgetScreen parent(drawable, 800, 600);
    NotifyApi * notifier = NULL;
    int fg_color = RED;
    int bg_color = YELLOW;
    int id = 0;
    bool auto_resize = true;
    int16_t x = 760;
    int16_t y = -7;

    WidgetLabel wlabel(drawable, x, y, parent, notifier, "test6", auto_resize, id, fg_color, bg_color);

    // ask to widget to redraw at it's current position
    wlabel.rdp_input_invalidate(Rect(0 + wlabel.dx(),
                                     0 + wlabel.dy(),
                                     wlabel.cx(),
                                     wlabel.cy()));

    //drawable.save_to_png(OUTPUT_FILE_PATH "label6.png");

    char message[1024];
    if (!check_sig(drawable.gd.impl(), message,
        "\x14\x49\x6b\x6a\xf0\xb8\x40\x0d\x5f\x61"
        "\xe6\x5d\x91\x13\x34\x89\x8d\x3c\xb8\xd0")){
        BOOST_CHECK_MESSAGE(false, message);
    }
}

BOOST_AUTO_TEST_CASE(TraceWidgetLabelClip)
{
    TestDraw drawable(800, 600);

    // WidgetLabel is a label widget of size 100x20 at position 760,-7 in it's parent context
    WidgetScreen parent(drawable, 800, 600);
    NotifyApi * notifier = NULL;
    int fg_color = RED;
    int bg_color = YELLOW;
    int id = 0;
    bool auto_resize = true;
    int16_t x = 760;
    int16_t y = -7;

    WidgetLabel wlabel(drawable, x, y, parent, notifier, "test6", auto_resize, id, fg_color, bg_color);

    // ask to widget to redraw at position 780,-7 and of size 120x20. After clip the size is of 20x13
    wlabel.rdp_input_invalidate(Rect(20 + wlabel.dx(),
                                     0 + wlabel.dy(),
                                     wlabel.cx(),
                                     wlabel.cy()));

    //drawable.save_to_png(OUTPUT_FILE_PATH "label7.png");

    char message[1024];
    if (!check_sig(drawable.gd.impl(), message,
        "\x5b\x6c\x88\xf2\x0b\x35\x40\xbe\x8e\x44"
        "\xc0\x45\x4c\xed\x3a\x77\xc3\x3c\x30\x1a")){
        BOOST_CHECK_MESSAGE(false, message);
    }
}

BOOST_AUTO_TEST_CASE(TraceWidgetLabelClip2)
{
    TestDraw drawable(800, 600);

    // WidgetLabel is a label widget of size 100x20 at position 10,7 in it's parent context
    WidgetScreen parent(drawable, 800, 600);
    NotifyApi * notifier = NULL;
    int fg_color = RED;
    int bg_color = YELLOW;
    int id = 0;
    bool auto_resize = true;
    int16_t x = 0;
    int16_t y = 0;

    WidgetLabel wlabel(drawable, x, y, parent, notifier, "test6", auto_resize, id, fg_color, bg_color);

    // ask to widget to redraw at position 30,12 and of size 30x10.
    wlabel.rdp_input_invalidate(Rect(20 + wlabel.dx(),
                                     5 + wlabel.dy(),
                                     30,
                                     10));

    //drawable.save_to_png(OUTPUT_FILE_PATH "label8.png");

    char message[1024];
    if (!check_sig(drawable.gd.impl(), message,
        "\xa1\x7a\x59\x8d\x51\x87\x8f\xf5\x90\x75"
        "\x02\xec\x6e\x61\x49\xbd\xaa\x92\x8f\x01")){
        BOOST_CHECK_MESSAGE(false, message);
    }
}

BOOST_AUTO_TEST_CASE(TraceWidgetLabelEvent)
{
    TestDraw drawable(800, 600);

    struct WidgetReceiveEvent : public Widget2 {
        Widget2* sender;
        NotifyApi::notify_event_t event;

        WidgetReceiveEvent(TestDraw& drawable)
        : Widget2(drawable, Rect(), *this, NULL)
        , sender(NULL)
        , event(0)
        {}

        virtual void draw(const Rect&)
        {}

        virtual void notify(Widget2* sender, NotifyApi::notify_event_t event)
        {
            this->sender = sender;
            this->event = event;
        }
    } widget_for_receive_event(drawable);

    Widget2& parent = widget_for_receive_event;
    NotifyApi * notifier = NULL;
    bool auto_resize = false;
    int16_t x = 0;
    int16_t y = 0;

    WidgetLabel wlabel(drawable, x, y, parent, notifier, "", auto_resize, 0, BLACK, WHITE);

    wlabel.rdp_input_mouse(MOUSE_FLAG_BUTTON1|MOUSE_FLAG_DOWN, 0, 0, 0);
    BOOST_CHECK(widget_for_receive_event.sender == 0);
    BOOST_CHECK(widget_for_receive_event.event == 0);
    wlabel.rdp_input_mouse(MOUSE_FLAG_BUTTON1, 0, 0, 0);
    BOOST_CHECK(widget_for_receive_event.sender == 0);
    BOOST_CHECK(widget_for_receive_event.event == 0);

    Keymap2 keymap;
    keymap.init_layout(0x040C);
    keymap.push_char('a');

    wlabel.rdp_input_scancode(0, 0, 0, 0, &keymap);
    BOOST_CHECK(widget_for_receive_event.sender == 0);
    BOOST_CHECK(widget_for_receive_event.event == 0);
}

BOOST_AUTO_TEST_CASE(TraceWidgetLabelAndComposite)
{
    TestDraw drawable(800, 600);

    //WidgetLabel is a label widget of size 256x125 at position 0,0 in it's parent context
    WidgetScreen parent(drawable, 800, 600);
    NotifyApi * notifier = NULL;

    WidgetComposite wcomposite(drawable, Rect(0,0,800,600), parent, notifier);

    WidgetLabel wlabel1(drawable, 0,0, wcomposite, notifier,
                        "abababab", true, 0, YELLOW, BLACK);
    WidgetLabel wlabel2(drawable, 0,100, wcomposite, notifier,
                        "ggghdgh", true, 0, WHITE, BLUE);
    WidgetLabel wlabel3(drawable, 100,100, wcomposite, notifier,
                        "lldlslql", true, 0, BLUE, RED);
    WidgetLabel wlabel4(drawable, 300,300, wcomposite, notifier,
                        "LLLLMLLM", true, 0, PINK, DARK_GREEN);
    WidgetLabel wlabel5(drawable, 700,-10, wcomposite, notifier,
                        "dsdsdjdjs", true, 0, LIGHT_GREEN, DARK_BLUE);
    WidgetLabel wlabel6(drawable, -10,550, wcomposite, notifier,
                        "xxwwp", true, 0, ANTHRACITE, PALE_GREEN);

    wcomposite.add_widget(&wlabel1);
    wcomposite.add_widget(&wlabel2);
    wcomposite.add_widget(&wlabel3);
    wcomposite.add_widget(&wlabel4);
    wcomposite.add_widget(&wlabel5);
    wcomposite.add_widget(&wlabel6);

    //ask to widget to redraw at position 100,25 and of size 100x100.
    wcomposite.rdp_input_invalidate(Rect(100, 25, 100, 100));

   //drawable.save_to_png(OUTPUT_FILE_PATH "label9.png");

    char message[1024];
    if (!check_sig(drawable.gd.impl(), message,
        "\x3f\x02\x08\xad\xbd\xd8\xf2\xc7\x1b\xf8"
        "\x32\x58\x67\x66\x5d\xdb\xe5\x75\xe4\xda")){
        BOOST_CHECK_MESSAGE(false, message);
    }

    //ask to widget to redraw at it's current position
    wcomposite.rdp_input_invalidate(Rect(0, 0, wcomposite.cx(), wcomposite.cy()));

    // drawable.save_to_png(OUTPUT_FILE_PATH "label10.png");

    if (!check_sig(drawable.gd.impl(), message,
        "\x47\x60\x43\x39\x74\x53\x46\x46\xd0\x1a"
        "\x3a\x30\x71\xfd\xee\xa6\x3a\x6c\xaa\x75")){
        BOOST_CHECK_MESSAGE(false, message);
    }
    wcomposite.clear();
}

TODO("the entry point exists in module: it's rdp_input_invalidate"
     "je just have to change received values to widget messages")
*/
