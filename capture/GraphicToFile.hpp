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
   Author(s): Christophe Grosjean, Jonathan Poelen

   RDPGraphicDevice is an abstract class that describe a device able to
   proceed RDP Drawing Orders. How the drawing will be actually done
   depends on the implementation.
   - It may be sent on the wire,
   - Used to draw on some internal bitmap,
   - etc.

*/

#if !defined(__GRAPHIC_TO_FILE_HPP__)
#define __GRAPHIC_TO_FILE_HPP__

#include <sys/time.h>
#include <ctime>

#include "RDP/x224.hpp"
#include "RDP/mcs.hpp"
#include "RDP/rdp.hpp"
#include "RDP/sec.hpp"
#include "RDP/lic.hpp"
#include "RDP/RDPSerializer.hpp"
#include "difftimeval.hpp"
#include "png.hpp"
#include "error.hpp"
#include "config.hpp"
#include "RDP/caches/bmpcache.hpp"
#include "colors.hpp"

#include "RDP/RDPDrawable.hpp"

class WRMChunk_Send
{
    public:
    WRMChunk_Send(Stream & stream, uint16_t chunktype, uint16_t data_size, uint16_t count)
    {
        stream.out_uint16_le(chunktype);
        stream.out_uint32_le(8 + data_size);
        stream.out_uint16_le(count);
        stream.mark_end();
    } 
};

template <size_t SZ>
class OutChunkedBufferingTransport : public Transport
{
public:
    Transport * trans;
    size_t max;
    BStream stream;
    OutChunkedBufferingTransport(Transport * trans)
        : trans(trans)
        , max(SZ-8)
        , stream(SZ)
    {
    }

    using Transport::recv;
    virtual void recv(char ** pbuffer, size_t len) throw (Error) {
        // CheckTransport does never receive anything
        throw Error(ERR_TRANSPORT_OUTPUT_ONLY_USED_FOR_SEND);
    }

    using Transport::send;
    virtual void send(const char * const buffer, size_t len) throw (Error)
    {   
        size_t to_buffer_len = len;
        while (this->stream.size() + to_buffer_len > max){
            BStream header(8);
            WRMChunk_Send chunk(header, PARTIAL_IMAGE_CHUNK, max, 1);
            this->trans->send(header.data, header.size());
            this->trans->send(this->stream.data, this->stream.size());
            size_t to_send = max - this->stream.size();
            this->trans->send(buffer + len - to_buffer_len, to_send);
            to_buffer_len -= to_send;
            this->stream.reset();    
        }
        this->stream.out_copy_bytes(buffer + len - to_buffer_len, to_buffer_len);
        this->stream.mark_end();
    }
    virtual void flush() {
        this->stream.mark_end();
        if (this->stream.size() > 0){
            BStream header(8);
            WRMChunk_Send chunk(header, LAST_IMAGE_CHUNK, this->stream.size(), 1);
            this->trans->send(header.data, header.size());
            this->trans->send(this->stream.data, this->stream.size());
        }
    }
};

struct GraphicToFile : public RDPGraphicDevice
REDOC("To keep things easy all chunks have 8 bytes headers"
      " starting with chunk_type, chunk_size"
      " and order_count (whatever it means, depending on chunks")
{
    timeval last_sent_timer;
    timeval timer;
    const uint16_t width;
    const uint16_t height;
    const uint8_t  bpp;
    RDPDrawable * drawable;
    RDPSerializer * serializer;
    Transport * trans;
    BmpCache & bmp_cache;

    GraphicToFile(const timeval& now
                , Transport * trans
                , Stream * pstream
                , const Inifile * ini
                , const uint16_t width
                , const uint16_t height
                , const uint8_t  bpp
                , BmpCache & bmp_cache)
    : timer(now)
    , width(width)
    , height(height)
    , bpp(bpp)
    , trans(trans)
    , bmp_cache(bmp_cache)
    {
    
        TODO("The serializers and the drawables should probably be provided by external call, not instanciated here")
        this->serializer = new RDPSerializer(trans, pstream, ini, bpp, this->bmp_cache, 0, 1, 1);
    
        this->drawable = new RDPDrawable(width, height, true);

        last_sent_timer.tv_sec = 0;
        last_sent_timer.tv_usec = 0;
        this->serializer->order_count = 0;
        
        this->send_meta_chunk();
    }

    ~GraphicToFile(){
        delete this->serializer;
        delete this->drawable;
    }

    virtual void timestamp(const timeval& now)
    REDOC("Update timestamp but send nothing, the timestamp will be sent later with the next effective event")
    {
        uint64_t old_timer = this->timer.tv_sec * 1000000ULL + this->timer.tv_usec;
        uint64_t current_timer = now.tv_sec * 1000000ULL + now.tv_usec;
        if (old_timer < current_timer){
            this->flush();
            this->timer = now;
        }
    }

    void send_meta_chunk(void)
    {
        TODO("meta should contain some WRM version identifier")
        BStream header(8);
        BStream stream(18);
        stream.out_uint16_le(this->width);
        stream.out_uint16_le(this->height);
        stream.out_uint16_le(this->bpp);
        stream.out_uint16_le(this->bmp_cache.small_entries);
        stream.out_uint16_le(this->bmp_cache.small_size);
        stream.out_uint16_le(this->bmp_cache.medium_entries);
        stream.out_uint16_le(this->bmp_cache.medium_size);
        stream.out_uint16_le(this->bmp_cache.big_entries);
        stream.out_uint16_le(this->bmp_cache.big_size);
        stream.mark_end();

        WRMChunk_Send chunk(header, META_FILE, stream.size(), 1);

        this->trans->send(header.data, header.size());
        this->trans->send(stream.data, stream.size());
    }

    // this one is used to store some embedded image inside WRM
    void send_image_chunk(void)
    {
        OutChunkedBufferingTransport<65536> png_trans(trans);

        ::transport_dump_png24(&png_trans, this->drawable->drawable.data,
                     this->drawable->drawable.width, this->drawable->drawable.height,
                     this->drawable->drawable.rowsize
                    );
    }

    void send_timestamp_chunk(void)
    {
        uint64_t old_timer = this->last_sent_timer.tv_sec * 1000000ULL + this->last_sent_timer.tv_usec;
        uint64_t current_timer = this->timer.tv_sec * 1000000ULL + this->timer.tv_usec;
        if (old_timer < current_timer){
            BStream stream(8);
            stream.out_uint64_le(current_timer);
            stream.mark_end();

            BStream header(8);
            WRMChunk_Send chunk(header, TIMESTAMP, 8, 1);
            this->trans->send(header.data, header.size());
            this->trans->send(stream.data, stream.size());
        }
    }

    void send_save_state_chunk()
    {
        BStream stream(2048);
        // RDPOrderCommon common;
        stream.out_uint8(this->serializer->common.order);
        stream.out_uint16_le(this->serializer->common.clip.x);
        stream.out_uint16_le(this->serializer->common.clip.y);
        stream.out_uint16_le(this->serializer->common.clip.cx);
        stream.out_uint16_le(this->serializer->common.clip.cy);
        // RDPDestBlt destblt;
        stream.out_uint16_le(this->serializer->destblt.rect.x);
        stream.out_uint16_le(this->serializer->destblt.rect.y);
        stream.out_uint16_le(this->serializer->destblt.rect.cx);
        stream.out_uint16_le(this->serializer->destblt.rect.cy);
        stream.out_uint8(this->serializer->destblt.rop);
        // RDPDestBlt destblt;
        stream.out_uint16_le(this->serializer->patblt.rect.x);
        stream.out_uint16_le(this->serializer->patblt.rect.y);
        stream.out_uint16_le(this->serializer->patblt.rect.cx);
        stream.out_uint16_le(this->serializer->patblt.rect.cy);
        stream.out_uint8(this->serializer->patblt.rop);
        stream.out_uint32_le(this->serializer->patblt.back_color);
        stream.out_uint32_le(this->serializer->patblt.fore_color);
        stream.out_uint8(this->serializer->patblt.brush.org_x);
        stream.out_uint8(this->serializer->patblt.brush.org_y);
        stream.out_uint8(this->serializer->patblt.brush.style);
        stream.out_uint8(this->serializer->patblt.brush.hatch);
        stream.out_copy_bytes(this->serializer->patblt.brush.extra, 7);
        // RDPScrBlt scrblt;
        stream.out_uint16_le(this->serializer->scrblt.rect.x);
        stream.out_uint16_le(this->serializer->scrblt.rect.y);
        stream.out_uint16_le(this->serializer->scrblt.rect.cx);
        stream.out_uint16_le(this->serializer->scrblt.rect.cy);
        stream.out_uint8(this->serializer->scrblt.rop);
        stream.out_uint16_le(this->serializer->scrblt.srcx);
        stream.out_uint16_le(this->serializer->scrblt.srcy);
        // RDPOpaqueRect opaquerect;
        stream.out_uint16_le(this->serializer->opaquerect.rect.x);
        stream.out_uint16_le(this->serializer->opaquerect.rect.y);
        stream.out_uint16_le(this->serializer->opaquerect.rect.cx);
        stream.out_uint16_le(this->serializer->opaquerect.rect.cy);
        stream.out_uint8(this->serializer->opaquerect.color);
        stream.out_uint8(this->serializer->opaquerect.color >> 8);
        stream.out_uint8(this->serializer->opaquerect.color >> 16);
        // RDPMemBlt memblt;
        stream.out_uint16_le(this->serializer->memblt.cache_id);
        stream.out_uint16_le(this->serializer->memblt.rect.x);
        stream.out_uint16_le(this->serializer->memblt.rect.y);
        stream.out_uint16_le(this->serializer->memblt.rect.cx);
        stream.out_uint16_le(this->serializer->memblt.rect.cy);
        stream.out_uint8(this->serializer->memblt.srcx);
        stream.out_uint8(this->serializer->memblt.srcy);
        stream.out_uint16_le(this->serializer->memblt.cache_idx);
        //RDPLineTo lineto;
        stream.out_uint8(this->serializer->lineto.back_mode);
        stream.out_uint16_le(this->serializer->lineto.startx);
        stream.out_uint16_le(this->serializer->lineto.starty);
        stream.out_uint16_le(this->serializer->lineto.endx);
        stream.out_uint16_le(this->serializer->lineto.endy);
        stream.out_uint32_le(this->serializer->lineto.back_color);
        stream.out_uint8(this->serializer->lineto.rop2);
        stream.out_uint8(this->serializer->lineto.pen.style);
        stream.out_sint8(this->serializer->lineto.pen.width);
        stream.out_uint32_le(this->serializer->lineto.pen.color);
        // RDPGlyphIndex glyphindex;
        stream.out_uint8(this->serializer->glyphindex.cache_id);
        stream.out_sint16_le(this->serializer->glyphindex.fl_accel);
        stream.out_sint16_le(this->serializer->glyphindex.ui_charinc);
        stream.out_sint16_le(this->serializer->glyphindex.f_op_redundant);
        stream.out_uint32_le(this->serializer->glyphindex.back_color);
        stream.out_uint32_le(this->serializer->glyphindex.fore_color);
        stream.out_uint16_le(this->serializer->glyphindex.bk.x);
        stream.out_uint16_le(this->serializer->glyphindex.bk.y);
        stream.out_uint16_le(this->serializer->glyphindex.bk.cx);
        stream.out_uint16_le(this->serializer->glyphindex.bk.cy);
        stream.out_uint16_le(this->serializer->glyphindex.op.x);
        stream.out_uint16_le(this->serializer->glyphindex.op.y);
        stream.out_uint16_le(this->serializer->glyphindex.op.cx);
        stream.out_uint16_le(this->serializer->glyphindex.op.cy);
        stream.out_uint8(this->serializer->glyphindex.brush.org_x);
        stream.out_uint8(this->serializer->glyphindex.brush.org_y);
        stream.out_uint8(this->serializer->glyphindex.brush.style);
        stream.out_uint8(this->serializer->glyphindex.brush.hatch);
        stream.out_copy_bytes(this->serializer->glyphindex.brush.extra, 7);
        stream.out_sint16_le(this->serializer->glyphindex.glyph_x);
        stream.out_sint16_le(this->serializer->glyphindex.glyph_y);
        stream.out_uint8(this->serializer->glyphindex.data_len);
        memset(this->serializer->glyphindex.data 
                + this->serializer->glyphindex.data_len, 0,
            sizeof(this->serializer->glyphindex.data) 
                - this->serializer->glyphindex.data_len);
        stream.out_copy_bytes(this->serializer->glyphindex.data, 256);

        //------------------------------ missing variable length ---------------
        stream.mark_end();

        BStream header(8);
        WRMChunk_Send chunk(header, SAVE_STATE, stream.size(), 1);
        this->trans->send(header.data, header.size());
        this->trans->send(stream.data, stream.size());
    }    

    void send_caches_chunk()
    {
        this->serializer->save_bmp_caches();
        if (this->serializer->order_count > 0){
            this->send_orders_chunk();
        }
    }

    void save_bmp_caches()
    {
        this->serializer->save_bmp_caches();
    }

    void breakpoint()
    {
        this->flush();
        this->trans->next();
        this->send_meta_chunk();
        this->send_timestamp_chunk();
        this->send_save_state_chunk();
        this->send_image_chunk();
        this->send_caches_chunk();
    }


    virtual void flush()
    {
        if (this->serializer->order_count > 0){
            this->send_timestamp_chunk();
            this->send_orders_chunk();
        }
    }

    void send_orders_chunk()
    {
        this->serializer->pstream->mark_end();
        BStream header(8);
        WRMChunk_Send chunk(header, RDP_UPDATE_ORDERS, this->serializer->pstream->size(), this->serializer->order_count);
        this->trans->send(header.data, header.size());
        this->serializer->flush();
    }


    virtual void draw(const RDPOpaqueRect & cmd, const Rect & clip) 
    {
        this->serializer->draw(cmd, clip);
        this->drawable->draw(cmd, clip);
    }

    virtual void draw(const RDPScrBlt & cmd, const Rect &clip)
    {
        this->serializer->draw(cmd, clip);
        this->drawable->draw(cmd, clip);
    }

    virtual void draw(const RDPDestBlt & cmd, const Rect &clip)
    {
        this->serializer->draw(cmd, clip);
        this->drawable->draw(cmd, clip);
    }

    virtual void draw(const RDPPatBlt & cmd, const Rect &clip)
    {
        this->serializer->draw(cmd, clip);
        this->drawable->draw(cmd, clip);
    }

    virtual void draw(const RDPMemBlt & cmd, const Rect & clip, const Bitmap & bmp)
    {
        this->serializer->draw(cmd, clip, bmp);
        this->drawable->draw(cmd, clip, bmp);
    }

    virtual void draw(const RDPLineTo& cmd, const Rect & clip)
    {
        this->serializer->draw(cmd, clip);
        this->drawable->draw(cmd, clip);
    }

    virtual void draw(const RDPGlyphIndex & cmd, const Rect & clip)
    {
        this->serializer->draw(cmd, clip);
        this->drawable->draw(cmd, clip);
    }

};

#endif
