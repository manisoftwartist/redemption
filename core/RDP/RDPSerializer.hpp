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
   Copyright (C) Wallix 2011
   Author(s): Christophe Grosjean

   RDPSerializer is an implementation of RDPGraphicDevice that know how to serialize RDP Orders
   and send them on the wire or store them in a file (actual storage will be provided as a Transport class).
   Serialized RDP orders are put in a chunk and sent when flush is called (either explicit call or because
   the provided buffer is full).

*/

#if !defined(__RDP_SERIALIZER_HPP__)
#define __RDP_SERIALIZER_HPP__

#include "RDP/RDPGraphicDevice.hpp"

#include "transport.hpp"

#include "RDP/caches/bmpcache.hpp"
#include "difftimeval.hpp"
#include "stream.hpp"
#include "rect.hpp"
#include "colors.hpp"
#include "meta_file.hpp"

struct WRMChunk {
    static const uint16_t TIMESTAMP = 1008;
    static const uint16_t BREAKPOINT = 1005;
    static const uint16_t META_FILE = 1006;
    static const uint16_t NEXT_FILE_ID = 1007;
};

struct RDPSerializer : public RDPGraphicDevice
{
    Stream * pstream;

//    uint8_t padding[65536];

    Transport * trans;
    const Inifile * ini;
    const int bitmap_cache_version;
    const int use_bitmap_comp;
    const int op2;

    // Internal state of orders
    RDPOrderCommon common;
    RDPDestBlt destblt;
    RDPPatBlt patblt;
    RDPScrBlt scrblt;
    RDPOpaqueRect opaquerect;
    RDPMemBlt memblt;
    RDPLineTo lineto;
    RDPGlyphIndex glyphindex;
    // state variables for gathering batch of orders
    size_t chunk_count;
    size_t chunk_flags;
    uint32_t offset_order_count;
    BmpCache bmp_cache;


    RDPSerializer(Transport * trans
          , Stream * pstream
          , const Inifile * ini
          , const uint8_t  bpp
          , uint32_t small_entries, uint32_t small_size
          , uint32_t medium_entries, uint32_t medium_size
          , uint32_t big_entries, uint32_t big_size
          , const int bitmap_cache_version
          , const int use_bitmap_comp
          , const int op2)
        : pstream(pstream),
        trans(trans),
        ini(ini),
        bitmap_cache_version(bitmap_cache_version),
        use_bitmap_comp(use_bitmap_comp),
        op2(op2),
        // Internal state of orders
        common(RDP::PATBLT, Rect(0, 0, 1, 1)),
        destblt(Rect(), 0),
        patblt(Rect(), 0, 0, 0, RDPBrush()),
        scrblt(Rect(), 0, 0, 0),
        opaquerect(Rect(), 0),
        memblt(0, Rect(), 0, 0, 0, 0),
        lineto(0, 0, 0, 0, 0, 0, 0, RDPPen(0, 0, 0)),
        glyphindex(0, 0, 0, 0, 0, 0, Rect(0, 0, 1, 1), Rect(0, 0, 1, 1), RDPBrush(), 0, 0, 0, (uint8_t*)""),
        // state variables for a batch of orders
        chunk_count(0),
        offset_order_count(0),
        bmp_cache(bpp, small_entries, small_size, medium_entries, medium_size, big_entries, big_size)
     {}
    ~RDPSerializer() {}
    virtual void flush() = 0;

    /*****************************************************************************/
    // check if the next order will fit in available packet size
    // if not send previous orders we got and init a new packet
    void reserve_order(size_t asked_size)
    {
        size_t max_packet_size = std::min(this->pstream->capacity, (size_t)16384);
        size_t used_size = this->pstream->get_offset();
        if (this->ini && this->ini->globals.debug.primary_orders > 63){
            LOG(LOG_INFO, "GraphicsUpdatePDU::reserve_order[%u](%u) remains=%u", this->chunk_count, asked_size, max_packet_size - used_size - 100);
        }
        if (asked_size + 100 > max_packet_size){
            LOG(LOG_ERR, "asked size (%u) > order batch capacity (%u)", asked_size + 100, max_packet_size);
            throw Error(ERR_STREAM_MEMORY_TOO_SMALL);
        }
        const size_t max_order_batch = 4096;
        if ((this->chunk_count >= max_order_batch) || (used_size + asked_size + 100) > max_packet_size) {
            this->flush();
        }
        this->chunk_count++;
    }

    virtual void draw(const RDPOpaqueRect & cmd, const Rect & clip)
    {
        this->reserve_order(23);
        RDPOrderCommon newcommon(RDP::RECT, clip);
        cmd.emit(*this->pstream, newcommon, this->common, this->opaquerect);
        this->common = newcommon;
        this->opaquerect = cmd;

        if (this->ini && this->ini->globals.debug.primary_orders){
            cmd.log(LOG_INFO, common.clip);
        }
    }

    virtual void draw(const RDPScrBlt & cmd, const Rect &clip)
    {
        this->reserve_order(25);
        RDPOrderCommon newcommon(RDP::SCREENBLT, clip);
        cmd.emit(*this->pstream, newcommon, this->common, this->scrblt);
        this->common = newcommon;
        this->scrblt = cmd;
        if (this->ini && this->ini->globals.debug.primary_orders){
            cmd.log(LOG_INFO, common.clip);
        }
    }

    virtual void draw(const RDPDestBlt & cmd, const Rect &clip)
    {
        this->reserve_order(21);
        RDPOrderCommon newcommon(RDP::DESTBLT, clip);
        cmd.emit(*this->pstream, newcommon, this->common, this->destblt);
        this->common = newcommon;
        this->destblt = cmd;
        if (this->ini && this->ini->globals.debug.primary_orders){
            cmd.log(LOG_INFO, common.clip);
        }
    }

    virtual void draw(const RDPPatBlt & cmd, const Rect &clip)
    {
        this->reserve_order(29);
        using namespace RDP;
        RDPOrderCommon newcommon(RDP::PATBLT, clip);
        cmd.emit(*this->pstream, newcommon, this->common, this->patblt);
        this->common = newcommon;
        this->patblt = cmd;
        if (this->ini && this->ini->globals.debug.primary_orders){
            cmd.log(LOG_INFO, common.clip);
        }
    }


    virtual void draw(const RDPMemBlt & cmd, const Rect & clip, const Bitmap & oldbmp)
    {
        uint32_t res = this->bmp_cache.cache_bitmap(oldbmp);
        uint8_t cache_id = (res >> 16) & 0x3;
        uint16_t cache_idx = res;

        if ((res >> 24) == BITMAP_ADDED_TO_CACHE){
            const Bitmap * bmp = this->bmp_cache.get(cache_id, cache_idx);
            RDPBmpCache cmd_cache(bmp, cache_id, cache_idx, this->ini?this->ini->globals.debug.primary_orders:0);
            this->reserve_order(cmd_cache.bmp->bmp_size + 16);
            cmd_cache.emit(*this->pstream, this->bitmap_cache_version, this->use_bitmap_comp, this->op2);

            if (this->ini && this->ini->globals.debug.secondary_orders){
                cmd_cache.log(LOG_INFO);
            }
        }

        RDPMemBlt newcmd = cmd;
        newcmd.cache_id = cache_id;
        newcmd.cache_idx = cache_idx;
        this->reserve_order(30);
        RDPOrderCommon newcommon(RDP::MEMBLT, clip);
        newcmd.emit(*this->pstream, newcommon, this->common, this->memblt);
        this->common = newcommon;
        this->memblt = newcmd;
        if (this->ini && this->ini->globals.debug.primary_orders){
            newcmd.log(LOG_INFO, common.clip);
        }
    }

    virtual void draw(const RDPLineTo& cmd, const Rect & clip)
    {
        this->reserve_order(32);
        RDPOrderCommon newcommon(RDP::LINE, clip);
        cmd.emit(*this->pstream, newcommon, this->common, this->lineto);
        this->common = newcommon;
        this->lineto = cmd;
        if (this->ini && this->ini->globals.debug.primary_orders){
            cmd.log(LOG_INFO, common.clip);
        }
    }

    virtual void draw(const RDPGlyphIndex & cmd, const Rect & clip)
    {
        this->reserve_order(297);
        RDPOrderCommon newcommon(RDP::GLYPHINDEX, clip);
        cmd.emit(*this->pstream, newcommon, this->common, this->glyphindex);
        this->common = newcommon;
        this->glyphindex = cmd;
        if (this->ini && this->ini->globals.debug.primary_orders){
            cmd.log(LOG_INFO, common.clip);
        }
    }

    virtual void draw(const RDPBrushCache & cmd)
    {
        this->reserve_order(cmd.size + 12);
        cmd.emit(*this->pstream);
        if (this->ini && this->ini->globals.debug.secondary_orders){
            cmd.log(LOG_INFO);
        }
    }

    virtual void draw(const RDPColCache & cmd)
    {
        this->reserve_order(2000);
        cmd.emit(*this->pstream);
        if (this->ini && this->ini->globals.debug.secondary_orders){
            cmd.log(LOG_INFO);
        }
    }

    virtual void draw(const RDPGlyphCache & cmd)
    {
        TODO(" compute actual size  instead of a majoration as below")
        this->reserve_order(1000);
        cmd.emit(*this->pstream);
        if (this->ini && this->ini->globals.debug.secondary_orders){
            cmd.log(LOG_INFO);
        }
    }

};


#endif
