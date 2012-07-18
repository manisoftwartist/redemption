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
 *   Author(s): Christophe Grosjean, Jonathan Poelen
 */

#if !defined(__WRM_RECORDER_HPP__)
#define __WRM_RECORDER_HPP__

#include <errno.h>
#include "transport.hpp"
#include "RDP/RDPGraphicDevice.hpp"
#include "meta_file.hpp"
#include "RDP/RDPDrawable.hpp"
#include "bitmap.hpp"
#include "stream.hpp"
#include "png.hpp"
#include "error.hpp"

class WRMRecorder
{
    InFileTransport trans;

public:
    RDPUnserializer reader;

private:
    Drawable * redrawable;

public:
    std::size_t idx_file;

private:
    std::string path;
    std::size_t base_path_len;

public:
    bool only_filename;

private:
    static int open(const char * filename)
    {
        LOG(LOG_INFO, "Recording to file : %s", filename);
        int fd = ::open(filename, O_RDONLY);
        if (-1 == fd){
            LOG(LOG_ERR, "Error opening wrm reader file : %s", strerror(errno));
           throw Error(ERR_WRM_RECORDER_OPEN_FAILED);
        }
        return fd;
    }

    void check_idx_wrm(std::size_t idx_wrm)
    {
        if (this->meta().files.size() <= idx_wrm)
        {
            LOG(LOG_ERR, "WRMRecorder : idx(%d) not found in meta", (int)idx_wrm);
            throw Error(ERR_RECORDER_META_REFERENCE_WRM);
        }
    }

public:
    WRMRecorder()
    : trans(0)
    , reader(&trans, 0, Rect())
    , redrawable(0)
    , idx_file(0)
    , path()
    , base_path_len(0)
    , only_filename(false)
    {}

    WRMRecorder(int fd, const std::string basepath = "")
    : trans(fd)
    , reader(&trans, 0, Rect())
    , redrawable(0)
    , idx_file(0)
    , path(basepath)
    , base_path_len(basepath.length())
    , only_filename(false)
    {
        this->normalize_path();
    }

    WRMRecorder(const std::string& filename, const std::string basepath = "")
    : trans(0)
    , reader(&trans, 0, Rect())
    , redrawable(0)
    , idx_file(0)
    , path(basepath)
    , base_path_len(basepath.length())
    , only_filename(false)
    {
        this->normalize_path();
        std::size_t pos = filename.find_last_not_of('.');
        if (pos != std::string::npos
            && pos < filename.size()
            && filename[pos] == 'm'
        ) {
            this->open_meta_followed_wrm(filename.c_str());
        }
        else {
            if (!this->open_wrm_followed_meta(filename.c_str()))
                throw Error(ERR_RECORDER_META_REFERENCE_WRM, errno);
        }
    }

    void open_meta_followed_wrm(const char * filename)
    {
        if (!this->open_meta(filename))
            throw Error(ERR_RECORDER_FAILED_TO_OPEN_TARGET_FILE, errno);
        if (this->meta().files.empty())
            throw Error(ERR_RECORDER_META_REFERENCE_WRM);
        this->open_wrm_only(this->get_cpath(this->meta().files[0].first.c_str()));
        ++this->idx_file;
        if (this->selected_next_order() && this->is_meta_chunk())
            this->ignore_chunks();
    }

    ~WRMRecorder()
    {
        ::close(this->trans.fd);
    }

private:
    void normalize_path()
    {
        if (this->base_path_len && this->path[this->base_path_len - 1] != '/')
        {
            this->path += '/';
            ++this->base_path_len;
        }
    }

public:
    void set_basepath(const std::string basepath)
    {
        this->base_path_len = basepath.length();
        this->path = basepath;
        this->normalize_path();
    }

    bool open_meta(const char* filename)
    {
        return this->reader.load_data(filename);
    }

    bool open_meta_with_path(const char* filename)
    {
        return this->open_meta(this->get_cpath(filename));
    }

    const char * get_cpath(const char * filename)
    {
        if (this->only_filename)
        {
            const char * tmp = filename;
            while (1)
            {
                while (*tmp && *tmp != '/')
                    ++tmp;
                if (!*tmp)
                    break;
                filename = ++tmp;
            }
        }
        if (!this->base_path_len)
            return filename;
        this->path.erase(this->base_path_len);
        this->path += filename;
        return this->path.c_str();
    }

public:
    void open_wrm_only(const char* filename)
    {
        this->trans.fd = WRMRecorder::open(filename);
    }

    void ignore_chunks()
    {
        this->reader.stream.p = this->reader.stream.end;
        this->reader.remaining_order_count = 0;
    }

    bool interpret_meta_chunk()
    {
        char filename[1024];
        this->get_order_file(filename);
        --this->reader.remaining_order_count;
        return this->open_meta_with_path(filename);
    }

    bool interpret_meta_chunk_and_force_meta(const char* filename)
    {
        this->ignore_chunks();
        return this->open_meta_with_path(filename);
    }

    bool open_wrm_followed_meta(const char* filename)
    {
        this->open_wrm_only(filename);
        if (!this->selected_next_order()){
            return false;
        }
        if (this->is_meta_chunk()) {
            return this->interpret_meta_chunk();
        }
        return true;
    }

    bool open_wrm_followed_meta(const char* filename, const char* filename_meta)
    {
        this->open_wrm_only(filename);
        if (!this->selected_next_order()){
            return false;
        }
        if (this->is_meta_chunk()) {
            return this->interpret_meta_chunk_and_force_meta(filename_meta);
        }
        return true;
    }

    bool is_meta_chunk() const
    { return this->reader.chunk_type == WRMChunk::META_FILE; }

    const DataMetaFile& meta() const
    {
        return this->reader.data_meta;
    }

    DataMetaFile& meta()
    {
        return this->reader.data_meta;
    }

private:
    void next_file(const char * filename)
    {
        ::close(this->trans.fd);
        this->trans.fd = -1;
        this->trans.fd = open(this->get_cpath(filename)); //can throw exception
        this->trans.total_received = 0;
        this->trans.last_quantum_received = 0;
        this->trans.total_sent = 0;
        this->trans.last_quantum_sent = 0;
        this->trans.quantum_count = 0;
        this->ignore_chunks();
    }

public:
    void get_order_file(char * filename)
    {
        size_t len = this->reader.stream.in_uint32_le();
        this->reader.stream.in_copy_bytes((uint8_t*)filename, len);
        filename[len] = 0;
    }

    std::size_t reference_wrm_size() const
    {
        return this->meta().files.size();
    }

    std::size_t get_idx_file(const char* wrm_name) const
    {
        std::size_t i = 0;
        while (i < this->meta().files.size() && this->meta().files[i].first != wrm_name){
            ++i;
        }
        return i;
    }

    void consumer(RDPGraphicDevice * consumer)
    {
        this->reader.consumer = consumer;
    }

    void redraw_consumer(Drawable* consumer)
    {
        this->redrawable = consumer;
    }

    RDPGraphicDevice * consumer()
    {
        return this->reader.consumer;
    }

    bool selected_next_order()
    {
        return this->reader.selected_next_order();
    }

    uint16_t& chunk_type()
    {
        return this->reader.chunk_type;
    }

    uint16_t chunk_type() const
    {
        return this->reader.chunk_type;
    }

    uint16_t& remaining_order_count()
    {
        return this->reader.remaining_order_count;
    }

private:
    void recv_rect(Rect& rect)
    {
        rect.x = this->reader.stream.in_uint16_le();
        rect.y = this->reader.stream.in_uint16_le();
        rect.cx = this->reader.stream.in_uint16_le();
        rect.cy = this->reader.stream.in_uint16_le();
    }

    void recv_brush(RDPBrush& brush)
    {
        brush.org_x = this->reader.stream.in_uint8();
        brush.org_y = this->reader.stream.in_uint8();
        brush.style = this->reader.stream.in_uint8();
        brush.hatch = this->reader.stream.in_uint8();
        brush.extra[0] = this->reader.stream.in_uint8();
        brush.extra[1] = this->reader.stream.in_uint8();
        brush.extra[2] = this->reader.stream.in_uint8();
        brush.extra[3] = this->reader.stream.in_uint8();
        brush.extra[4] = this->reader.stream.in_uint8();
        brush.extra[5] = this->reader.stream.in_uint8();
        brush.extra[6] = this->reader.stream.in_uint8();
    }

    void recv_pen(RDPPen& pen)
    {
        pen.color = this->reader.stream.in_uint32_le();
        pen.style = this->reader.stream.in_uint8();
        pen.width = this->reader.stream.in_uint8();
    }

public:
    timeval get_start_time_order()
    {
        timeval time = {
            this->reader.stream.in_uint64_be(),
            this->reader.stream.in_uint64_be()
        };
        return time;
    }

private:
    struct PngBuffer {
        uint8_t * data;
        PngBuffer(uint8_t * __data)
        : data(__data)
        {}
    };
    static void png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
    {
        /* with libpng15 next line causes pointer deference error; use libpng12 */
        PngBuffer* buffer = static_cast<PngBuffer*>(png_ptr->io_ptr);
        memcpy(data, buffer->data, length);
        buffer->data += length;
    }

public:
    void interpret_order()
    {
        switch (this->reader.chunk_type) {
            case WRMChunk::TIME_START:
            {
                this->ignore_chunks();
            }
            break;
            case WRMChunk::META_FILE:
            {
                this->ignore_chunks();
                //this->reader.stream.p += this->reader.stream.in_uint32_le();
                //--this->reader.remaining_order_count;
            }
            break;
            case WRMChunk::NEXT_FILE_ID:
            {
                this->idx_file = this->reader.stream.in_uint32_le();
                this->check_idx_wrm(this->idx_file);
                this->next_file(this->meta().files[this->idx_file].first.c_str());
            }
            break;
            case WRMChunk::BREAKPOINT:
            {
                uint16_t width = this->reader.stream.in_uint16_le();
                uint16_t height = this->reader.stream.in_uint16_le();
                /*uint8_t bpp = */this->reader.stream.in_uint8();
                this->reader.wait_cap.timer.sec() = this->reader.stream.in_uint64_le();
                this->reader.wait_cap.timer.usec() = this->reader.stream.in_uint64_le();
                --this->reader.remaining_order_count;

                //read screen
                if (this->redrawable) {
                    const char * filename = this->meta().files[this->idx_file].second.c_str();
                    if (std::FILE* fd = std::fopen(filename, "w+"))
                    {
                        read_png24(fd, this->redrawable->data, width, height, this->redrawable->rowsize);
                        fclose(fd);
                    } else {
                        LOG(LOG_ERR, "open context screen %s: %s", filename, strerror(errno));
                        throw Error(ERR_RECORDER_FAILED_TO_OPEN_TARGET_FILE, errno);
                    }
                }

                this->selected_next_order();

                this->reader.common.order = this->reader.stream.in_uint8();
                this->recv_rect(this->reader.common.clip);
                //this->reader.common.str(texttest, 10000);
                //std::cout << "interpret_order: " << texttest << '\n';

                this->reader.opaquerect.color = this->reader.stream.in_uint32_le();
                this->recv_rect(this->reader.opaquerect.rect);
                //std::cout << "interpret_order: ";
                //this->reader.opaquerect.print(Rect(0,0,0,0));

                this->reader.destblt.rop = this->reader.stream.in_uint8();
                this->recv_rect(this->reader.destblt.rect);
                //std::cout << "interpret_order: ";
                //this->reader.destblt.print(Rect(0,0,0,0));

                this->reader.patblt.rop = this->reader.stream.in_uint8();
                this->reader.patblt.back_color = this->reader.stream.in_uint32_le();
                this->reader.patblt.fore_color = this->reader.stream.in_uint32_le();
                this->recv_brush(this->reader.patblt.brush);
                this->recv_rect(this->reader.patblt.rect);
                //std::cout << "interpret_order: ";
                //this->reader.patblt.print(Rect(0,0,0,0));

                this->reader.scrblt.rop = this->reader.stream.in_uint8();
                this->reader.scrblt.srcx = this->reader.stream.in_uint16_le();
                this->reader.scrblt.srcy = this->reader.stream.in_uint16_le();
                this->recv_rect(this->reader.scrblt.rect);
                //std::cout << "interpret_order: ";
                //this->reader.scrblt.print(Rect(0,0,0,0));

                this->reader.memblt.rop = this->reader.stream.in_uint8();
                this->reader.memblt.srcx = this->reader.stream.in_uint16_le();
                this->reader.memblt.srcy = this->reader.stream.in_uint16_le();
                this->reader.memblt.cache_id = this->reader.stream.in_uint16_le();
                this->reader.memblt.cache_idx = this->reader.stream.in_uint16_le();
                this->recv_rect(this->reader.memblt.rect);
                //std::cout << "interpret_order: ";
                //this->reader.memblt.print(Rect(0,0,0,0));

                this->reader.lineto.rop2 = this->reader.stream.in_uint8();
                this->reader.lineto.startx = this->reader.stream.in_uint16_le();
                this->reader.lineto.starty = this->reader.stream.in_uint16_le();
                this->reader.lineto.endx = this->reader.stream.in_uint16_le();
                this->reader.lineto.endy = this->reader.stream.in_uint16_le();
                this->reader.lineto.back_mode = this->reader.stream.in_uint8();
                this->reader.lineto.back_color = this->reader.stream.in_uint32_le();
                this->recv_pen(this->reader.lineto.pen);
                //std::cout << "interpret_order: ";
                //this->reader.lineto.print(Rect(0,0,0,0));

                this->reader.glyphindex.back_color = this->reader.stream.in_uint32_le();
                this->reader.glyphindex.fore_color = this->reader.stream.in_uint32_le();
                this->reader.glyphindex.f_op_redundant = this->reader.stream.in_uint16_le();
                this->reader.glyphindex.fl_accel = this->reader.stream.in_uint16_le();
                this->reader.glyphindex.glyph_x = this->reader.stream.in_uint16_le();
                this->reader.glyphindex.glyph_y = this->reader.stream.in_uint16_le();
                this->reader.glyphindex.ui_charinc = this->reader.stream.in_uint16_le();
                this->reader.glyphindex.cache_id = this->reader.stream.in_uint8();
                this->reader.glyphindex.data_len = this->reader.stream.in_uint8();
                this->recv_rect(this->reader.glyphindex.bk);
                this->recv_rect(this->reader.glyphindex.op);
                this->recv_brush(this->reader.glyphindex.brush);
                this->reader.glyphindex.data = (uint8_t*)malloc(this->reader.glyphindex.data_len);
                this->reader.stream.in_copy_bytes(this->reader.glyphindex.data, this->reader.glyphindex.data_len);
                //std::cout << "interpret_order: ";
                //this->reader.glyphindex.print(Rect(0,0,0,0));

                this->reader.order_count = this->reader.stream.in_uint16_le();
                //std::cout << "\ninterpret_order: "  << this->reader.order_count << '\n';

                this->reader.bmp_cache.small_entries = this->reader.stream.in_uint16_le();
                this->reader.bmp_cache.small_size = this->reader.stream.in_uint16_le();
                this->reader.bmp_cache.medium_entries = this->reader.stream.in_uint16_le();
                this->reader.bmp_cache.medium_size = this->reader.stream.in_uint16_le();
                this->reader.bmp_cache.big_entries = this->reader.stream.in_uint16_le();
                this->reader.bmp_cache.big_size = this->reader.stream.in_uint16_le();
                uint32_t stamp = this->reader.stream.in_uint32_le();

                this->reader.bmp_cache.reset();
                this->reader.bmp_cache.stamp = stamp;
                this->reader.remaining_order_count = 0;

                z_stream zstrm;
                zstrm.zalloc = 0;
                zstrm.zfree = 0;
                zstrm.opaque = 0;
                int ret;
                const int Bpp = 3;
                while (1)
                {
                    this->reader.stream.init(14);
                    this->reader.trans->recv(&this->reader.stream.end, 14);
                    uint16_t idx = this->reader.stream.in_uint16_le();
                    uint32_t stamp = this->reader.stream.in_uint32_le();
                    uint16_t cx = this->reader.stream.in_uint16_le();
                    uint16_t cy = this->reader.stream.in_uint16_le();
                    uint32_t buffer_size = this->reader.stream.in_uint32_le();
                    if (idx == 8192 * 3 + 1){
                        break;
                    }

                    this->reader.stream.init(buffer_size);
                    this->reader.trans->recv(&this->reader.stream.end, buffer_size);

                    zstrm.avail_in = buffer_size;
                    zstrm.next_in = this->reader.stream.data;

                    uint8_t * data = new uint8_t[cx*cy * Bpp];
                    zstrm.avail_out = cx*cy * Bpp;
                    zstrm.next_out = data;

                    if ((ret = inflateInit(&zstrm)) != Z_OK)
                    {
                        LOG(LOG_ERR, "zlib: inflateInit: %d", ret);
                        throw Error(ERR_WRM_RECORDER_ZIP_UNCOMPRESS);
                    }

                    ret = inflate(&zstrm, Z_FINISH);
                    inflateEnd(&zstrm);

                    if (ret != Z_STREAM_END)
                    {
                        LOG(LOG_ERR, "zlib: inflate: %d", ret);
                        throw Error(ERR_WRM_RECORDER_ZIP_UNCOMPRESS);
                    }

                    uint cid = idx / 8192;
                    uint cidx = idx % 8192;
                    this->reader.bmp_cache.stamps[cid][cidx] = stamp;
                    this->reader.bmp_cache.cache[cid][cidx] = new Bitmap(24, 0, cx, cy, data, cx*cy);
                    delete [] data;
                }
            }
            break;
            default:
                this->reader.interpret_order();
                break;
        }
    }

    bool next_order()
    {
        if (this->selected_next_order())
        {
            this->interpret_order();
            return true;
        }
        return false;
    }
};

#endif
