/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2010-2013
   Author(s): Christophe Grosjean, Javier Caverni, Meng Tan, Raphael Zhou
*/

#ifndef _REDEMPTION_MOD_MOD_API_HPP_
#define _REDEMPTION_MOD_MOD_API_HPP_

#include <ctime>

#include "callback.hpp"
#include "draw_api.hpp"
#include "font.hpp"
#include "text_metrics.hpp"
#include "wait_obj.hpp"
#include "RDP/caches/glyphcache.hpp"
#include "RDP/orders/RDPOrdersCommon.hpp"
#include "RDP/orders/RDPOrdersPrimaryGlyphIndex.hpp"

class RDPGraphicDevice;
class Inifile;
class Stream;
class HStream;

enum {
    BUTTON_STATE_UP   = 0,
    BUTTON_STATE_DOWN = 1
};

class Timeout {
    time_t timeout;

public:
    typedef enum {
        TIMEOUT_REACHED,
        TIMEOUT_NOT_REACHED,
        TIMEOUT_INACTIVE
    } timeout_result_t;

    Timeout(time_t now, time_t length = 0)
        : timeout(length ? (now + length) : 0) {}

    timeout_result_t check(time_t now) const {
        if (this->timeout) {
            if (now > this->timeout) {
                return TIMEOUT_REACHED;
            }
            else {
                return TIMEOUT_NOT_REACHED;
            }
        }
        return TIMEOUT_INACTIVE;
    }

    bool is_cancelled() const {
        return (this->timeout == 0);
    }

    long timeleft_sec(time_t now) const {
        return (this->timeout - now);
    }

    void cancel_timeout() {
        this->timeout = 0;
    }

    void restart_timeout(time_t now, time_t length) {
        this->timeout = now + length;
    }
};

class mod_api : public Callback, public DrawApi {
protected:
    wait_obj           event;
    RDPPen             pen;
    RDPGraphicDevice * gd;

    uint16_t front_width;
    uint16_t front_height;

public:
    mod_api(const uint16_t front_width, const uint16_t front_height)
    : gd(this)
    , front_width(front_width)
    , front_height(front_height) {
        this->event.set(0);
    }

    virtual ~mod_api() {}

    virtual wait_obj& get_event() { return this->event; }

    uint16_t get_front_width() const { return this->front_width; }
    uint16_t get_front_height() const { return this->front_height; }

    virtual void text_metrics(Font const & font, const char * text, int & width, int & height)
    {
        ::text_metrics(font, text, width, height,
            [](uint32_t charnum) {
                LOG(LOG_WARNING, "mod_api::text_metrics() - character not defined >0x%02x<", charnum);
            }
        );
    }

    TODO("implementation of the server_draw_text function below is a small subset of possibilities text can be packed (detecting duplicated strings). See MS-RDPEGDI 2.2.2.2.1.1.2.13 GlyphIndex (GLYPHINDEX_ORDER)")
    virtual void server_draw_text(Font const & font, int16_t x, int16_t y, const char * text,
                                  uint32_t fgcolor, uint32_t bgcolor, const Rect & clip)
    {
        static GlyphCache mod_glyph_cache;

        UTF8toUnicodeIterator unicode_iter(text);
        while (*unicode_iter) {
            int total_width = 0;
            int total_height = 0;
            uint8_t data[256];
            auto data_begin = std::begin(data);
            const auto data_end = std::end(data)-2;

            const int cacheId = 7;
            int distance_from_previous_fragment = 0;
            while (data_begin != data_end) {
                const uint32_t charnum = *unicode_iter;
                if (!charnum) {
                    break ;
                }
                ++unicode_iter;

                int cacheIndex = 0;
                FontChar const & font_item = font.glyph_defined(charnum) && font.font_items[charnum]
                ? font.font_items[charnum]
                : [&]() {
                    LOG(LOG_WARNING, "mod_api::server_draw_text() - character not defined >0x%02x<", charnum);
                    return std::ref(font.font_items[static_cast<unsigned>('?')]);
                }().get();
                TODO(" avoid passing parameters by reference to get results")
                const GlyphCache::t_glyph_cache_result cache_result =
                    mod_glyph_cache.add_glyph(font_item, cacheId, cacheIndex);
(void)cache_result;

                *data_begin = cacheIndex;
                ++data_begin;
                *data_begin = distance_from_previous_fragment;
                ++data_begin;
                distance_from_previous_fragment = font_item.incby;
                total_width += font_item.incby;
                total_height = std::max(total_height, font_item.height);
            }

            const Rect bk(x, y, total_width + 1, total_height + 1);

            RDPGlyphIndex glyphindex(
                cacheId,            // cache_id
                0x03,               // fl_accel
                0x0,                // ui_charinc
                1,                  // f_op_redundant,
                fgcolor,            // BackColor (text color)
                bgcolor,            // ForeColor (color of the opaque rectangle)
                bk,                 // bk
                bk,                 // op
                // brush
                RDPBrush(0, 0, 3, 0xaa,
                    (const uint8_t *)"\xaa\x55\xaa\x55\xaa\x55\xaa\x55"),
                x,                  // glyph_x
                y + total_height,   // glyph_y
                data_begin - data,  // data_len in bytes
                data                // data
            );

            x += total_width;

            this->gd->draw(glyphindex, clip, &mod_glyph_cache);
        }
    }

protected:
    static RDPGraphicDevice * get_gd(mod_api const & mod)
    {
        return mod.gd;
    }

    static void set_gd(mod_api & mod, RDPGraphicDevice * gd)
    {
        mod.gd = gd;
    }

public:
    virtual void send_to_front_channel(const char * const mod_channel_name,
        uint8_t* data, size_t length, size_t chunk_size, int flags) = 0;

    // draw_event is run when mod socket received some data (drawing order)
    // or auto-generated by modules, say to comply to some refresh.
    // draw event decodes incoming traffic from backend and eventually calls front to draw things
    // may raise an exception (say if connection to server is closed), but returns nothings
    virtual void draw_event(time_t now) = 0;

    // used when context changed to avoid creating a new module
    // it usually perform some task identical to what constructor does
    // henceforth it should often be called by constructors
    virtual void refresh_context(Inifile & ini) {}

    virtual bool is_up_and_running() { return false; }

    virtual void send_fastpath_data(Stream & data) {}
    virtual void send_data_indication_ex(uint16_t channelId, HStream & stream) {}
    virtual void disconnect() {}
};

#endif
