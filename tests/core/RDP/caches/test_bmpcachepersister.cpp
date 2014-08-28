/*
    This program is free software; you can redistribute it and/or modify it
     under the terms of the GNU General Public License as published by the
     Free Software Foundation; either version 2 of the License, or (at your
     option) any later version.

    This program is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
     Public License for more details.

    You should have received a copy of the GNU General Public License along
     with this program; if not, write to the Free Software Foundation, Inc.,
     675 Mass Ave, Cambridge, MA 02139, USA.

    Product name: redemption, a FLOSS RDP proxy
    Copyright (C) Wallix 2014
    Author(s): Christophe Grosjean, Raphael Zhou
*/

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestBmpCachePersister
#include <boost/test/auto_unit_test.hpp>

#define LOGNULL
//#define LOGPRINT
#include "log.hpp"

#include "RDP/caches/bmpcachepersister.hpp"
#include "test_transport.hpp"

BOOST_AUTO_TEST_CASE(TestBmpCachePersister)
{
    uint8_t  bpp              = 8;
    bool     use_waiting_list = false;
    uint32_t verbose          = 1;

    struct BmpCacheParams {
        uint16_t entiers;
        int      size;
        bool     persistent;
    } bmp_cache_params [] = {
        { 120,  nbbytes(bpp) * 16 * 16, false },
        { 120,  nbbytes(bpp) * 32 * 32, false },
        { 2553, nbbytes(bpp) * 64 * 64, true  }
    };

    BmpCache bmp_cache( BmpCache::Recorder, bpp, 3, use_waiting_list
                      , BmpCache::CacheOption(bmp_cache_params[0].entiers, bmp_cache_params[0].size, bmp_cache_params[0].persistent)
                      , BmpCache::CacheOption(bmp_cache_params[1].entiers, bmp_cache_params[1].size, bmp_cache_params[1].persistent)
                      , BmpCache::CacheOption(bmp_cache_params[2].entiers, bmp_cache_params[2].size, bmp_cache_params[2].persistent)
                      , BmpCache::CacheOption()
                      , BmpCache::CacheOption()
                      , verbose
                      );

    BGRPalette   palette;
    Bitmap     * bmp;
    int          result;


    uint8_t raw_palette_0[] = {
/* 0000 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00,  // ................
/* 0010 */ 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0xc0, 0xc0, 0xc0, 0x00,  // ................
/* 0020 */ 0xc0, 0xdc, 0xc0, 0x00, 0xc8, 0xd0, 0xd4, 0x00, 0xd8, 0x2d, 0x41, 0x00, 0x7e, 0x00, 0x02, 0x00,  // .........-A.~...
/* 0030 */ 0x01, 0x00, 0x00, 0x00, 0x60, 0x00, 0x01, 0x00, 0x64, 0xfa, 0x12, 0x00, 0xe7, 0x14, 0x40, 0x00,  // ....`...d.....@.
/* 0040 */ 0x25, 0x9e, 0x94, 0x00, 0x00, 0x00, 0x49, 0x00, 0xfd, 0xff, 0xff, 0x00, 0x70, 0xf9, 0x12, 0x00,  // %.....I.....p...
/* 0050 */ 0x8c, 0xf6, 0x12, 0x00, 0x53, 0xeb, 0xda, 0x00, 0x38, 0x7e, 0x17, 0x00, 0x78, 0x01, 0x15, 0x00,  // ....S...8~..x...
/* 0060 */ 0x43, 0xe7, 0xda, 0x00, 0x40, 0x06, 0x15, 0x00, 0x48, 0xcf, 0x16, 0x00, 0xbe, 0xb8, 0xf4, 0x00,  // C...@...H.......
/* 0070 */ 0x24, 0xf8, 0x12, 0x00, 0xca, 0xaf, 0x17, 0x00, 0x04, 0x00, 0x00, 0x00, 0x38, 0xb1, 0x17, 0x00,  // $...........8...
/* 0080 */ 0x58, 0x01, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x40, 0xf7, 0x12, 0x00, 0xa8, 0xae, 0x17, 0x00,  // X...l...@.......
/* 0090 */ 0x20, 0xf8, 0x12, 0x00, 0xad, 0x9d, 0x94, 0x00, 0x78, 0x0d, 0x15, 0x00, 0xc9, 0x9d, 0x94, 0x00,  //  .......x.......
/* 00a0 */ 0x20, 0x51, 0x4d, 0x00, 0xb0, 0xae, 0x17, 0x00, 0x60, 0x01, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00,  //  QM.....`...,...
/* 00b0 */ 0x80, 0xf7, 0x01, 0x00, 0x08, 0x50, 0x18, 0x00, 0x7a, 0x00, 0x00, 0x00, 0xb8, 0xf7, 0x12, 0x00,  // .....P..z.......
/* 00c0 */ 0xe0, 0x80, 0x94, 0x00, 0xd0, 0x9d, 0x94, 0x00, 0x00, 0x00, 0x15, 0x00, 0xa0, 0x7b, 0x16, 0x00,  // .............{..
/* 00d0 */ 0x78, 0xf8, 0x12, 0x00, 0x9b, 0xa6, 0x94, 0x00, 0x58, 0xf8, 0x12, 0x00, 0x58, 0xb3, 0x17, 0x00,  // x.......X...X...
/* 00e0 */ 0xb8, 0x7c, 0x16, 0x00, 0xa8, 0x7b, 0x16, 0x00, 0x6b, 0xc8, 0x00, 0x00, 0x34, 0xf8, 0x12, 0x00,  // .|...{..k...4...
/* 00f0 */ 0xba, 0xde, 0xda, 0x00, 0x90, 0x02, 0x15, 0x00, 0xd8, 0x7e, 0x17, 0x00, 0x08, 0x00, 0x00, 0x00,  // .........~......
/* 0100 */ 0x28, 0x01, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0xd8, 0xaf, 0x17, 0x00, 0x70, 0x8d, 0x17, 0x00,  // (...%.......p...
/* 0110 */ 0x00, 0xf9, 0x12, 0x00, 0xa8, 0x07, 0x15, 0x00, 0x5c, 0xf9, 0x12, 0x00, 0xba, 0x00, 0x01, 0x00,  // ................
/* 0120 */ 0x00, 0x04, 0x00, 0x00, 0x68, 0x01, 0x15, 0x00, 0x28, 0x02, 0x00, 0x00, 0x30, 0xb1, 0x17, 0x00,  // ....h...(...0...
/* 0130 */ 0x60, 0xf8, 0x12, 0x00, 0x94, 0x9f, 0x94, 0x00, 0x48, 0x07, 0x15, 0x00, 0x88, 0xfa, 0x12, 0x00,  // `.......H.......
/* 0140 */ 0x28, 0x9f, 0x94, 0x00, 0x6c, 0x9f, 0x94, 0x00, 0x18, 0xea, 0x15, 0x00, 0x9a, 0xae, 0x94, 0x00,  // (...l...........
/* 0150 */ 0x03, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x49, 0x00, 0x06, 0x00, 0x00, 0x00,  // ..........I.....
/* 0160 */ 0x84, 0xf8, 0x12, 0x00, 0xa0, 0xae, 0x94, 0x00, 0x78, 0x8d, 0x17, 0x00, 0xcc, 0xae, 0x94, 0x00,  // ........x.......
/* 0170 */ 0x30, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x84, 0xf9, 0x12, 0x00, 0x5c, 0xf9, 0x01, 0x00,  // 0... ...........
/* 0180 */ 0x30, 0xf8, 0x12, 0x00, 0xb8, 0xfb, 0x12, 0x00, 0x86, 0xc3, 0xf3, 0x00, 0x79, 0x7b, 0x94, 0x00,  // 0...........y{..
/* 0190 */ 0x06, 0x1d, 0x82, 0x00, 0xd8, 0x00, 0x00, 0x00, 0x2b, 0x1d, 0x82, 0x00, 0x24, 0x00, 0x00, 0x00,  // ........+...$...
/* 01a0 */ 0x80, 0x0f, 0x05, 0x00, 0x00, 0x50, 0xfd, 0x00, 0x2c, 0xf9, 0x12, 0x00, 0x10, 0x00, 0x00, 0x00,  // .....P..,.......
/* 01b0 */ 0xd8, 0xf9, 0x12, 0x00, 0x48, 0x1a, 0x82, 0x00, 0x30, 0x1d, 0x82, 0x00, 0x75, 0x1c, 0x82, 0x00,  // ....H...0...u...
/* 01c0 */ 0x29, 0x76, 0x94, 0x00, 0xa5, 0x19, 0x82, 0x00, 0xe8, 0xf9, 0x12, 0x00, 0x65, 0x38, 0x49, 0x00,  // )v..........e8I.
/* 01d0 */ 0x85, 0x6e, 0x49, 0x00, 0xa4, 0x00, 0xdc, 0x00, 0x00, 0x00, 0xdc, 0x00, 0x64, 0x50, 0x4d, 0x00,  // .nI.........dPM.
/* 01e0 */ 0x07, 0x00, 0x01, 0x00, 0xfc, 0x24, 0x41, 0x00, 0x80, 0xfa, 0x12, 0x00, 0xd8, 0xe2, 0x45, 0x00,  // .....$A.......E.
/* 01f0 */ 0x18, 0xfa, 0x12, 0x00, 0xe3, 0xb6, 0xf4, 0x00, 0x11, 0x01, 0x00, 0x00, 0xcd, 0xab, 0xba, 0x00,  // ................
/* 0200 */ 0x90, 0xfa, 0x12, 0x00, 0x74, 0xb8, 0xf4, 0x00, 0x7f, 0x01, 0x3f, 0x00, 0x7f, 0x01, 0x5f, 0x00,  // ....t.....?..._.
/* 0210 */ 0x7f, 0x01, 0x7f, 0x00, 0x7f, 0x01, 0x9f, 0x00, 0x7f, 0x01, 0xbf, 0x00, 0x7f, 0x01, 0xdf, 0x00,  // ................
/* 0220 */ 0x7f, 0x1f, 0x01, 0x00, 0x7f, 0x1f, 0x1f, 0x00, 0x7f, 0x1f, 0x3f, 0x00, 0x7f, 0x1f, 0x5f, 0x00,  // ..........?..._.
/* 0230 */ 0x7f, 0x1f, 0x7f, 0x00, 0x7f, 0x1f, 0x9f, 0x00, 0x7f, 0x1f, 0xbf, 0x00, 0x7f, 0x1f, 0xdf, 0x00,  // ................
/* 0240 */ 0x7f, 0x3f, 0x01, 0x00, 0x7f, 0x3f, 0x1f, 0x00, 0x7f, 0x3f, 0x3f, 0x00, 0x7f, 0x3f, 0x5f, 0x00,  // .?...?...??..?_.
/* 0250 */ 0x7f, 0x3f, 0x7f, 0x00, 0x7f, 0x3f, 0x9f, 0x00, 0x7f, 0x3f, 0xbf, 0x00, 0x7f, 0x3f, 0xdf, 0x00,  // .?...?...?...?..
/* 0260 */ 0x7f, 0x5f, 0x01, 0x00, 0x7f, 0x5f, 0x1f, 0x00, 0x7f, 0x5f, 0x3f, 0x00, 0x7f, 0x5f, 0x5f, 0x00,  // ._..._..._?..__.
/* 0270 */ 0x7f, 0x5f, 0x7f, 0x00, 0x7f, 0x5f, 0x9f, 0x00, 0x7f, 0x5f, 0xbf, 0x00, 0x7f, 0x5f, 0xdf, 0x00,  // ._..._..._..._..
/* 0280 */ 0x7f, 0x7f, 0x01, 0x00, 0x7f, 0x7f, 0x1f, 0x00, 0x7f, 0x7f, 0x3f, 0x00, 0x7f, 0x7f, 0x5f, 0x00,  // ..........?..._.
/* 0290 */ 0x7f, 0x7f, 0x7f, 0x00, 0x7f, 0x7f, 0x9f, 0x00, 0x7f, 0x7f, 0xbf, 0x00, 0x7f, 0x7f, 0xdf, 0x00,  // ................
/* 02a0 */ 0x7f, 0x9f, 0x01, 0x00, 0x7f, 0x9f, 0x1f, 0x00, 0x7f, 0x9f, 0x3f, 0x00, 0x7f, 0x9f, 0x5f, 0x00,  // ..........?..._.
/* 02b0 */ 0x7f, 0x9f, 0x7f, 0x00, 0x7f, 0x9f, 0x9f, 0x00, 0x7f, 0x9f, 0xbf, 0x00, 0x7f, 0x9f, 0xdf, 0x00,  // ................
/* 02c0 */ 0x7f, 0xbf, 0x01, 0x00, 0x7f, 0xbf, 0x1f, 0x00, 0x7f, 0xbf, 0x3f, 0x00, 0x7f, 0xbf, 0x5f, 0x00,  // ..........?..._.
/* 02d0 */ 0x7f, 0xbf, 0x7f, 0x00, 0x7f, 0xbf, 0x9f, 0x00, 0x7f, 0xbf, 0xbf, 0x00, 0x7f, 0xbf, 0xdf, 0x00,  // ................
/* 02e0 */ 0x7f, 0xdf, 0x01, 0x00, 0x7f, 0xdf, 0x1f, 0x00, 0x7f, 0xdf, 0x3f, 0x00, 0x7f, 0xdf, 0x5f, 0x00,  // ..........?..._.
/* 02f0 */ 0x7f, 0xdf, 0x7f, 0x00, 0x7f, 0xdf, 0x9f, 0x00, 0x7f, 0xdf, 0xbf, 0x00, 0x7f, 0xdf, 0xdf, 0x00,  // ................
/* 0300 */ 0xbf, 0x01, 0x01, 0x00, 0xbf, 0x01, 0x1f, 0x00, 0xbf, 0x01, 0x3f, 0x00, 0xbf, 0x01, 0x5f, 0x00,  // ..........?..._.
/* 0310 */ 0xbf, 0x01, 0x7f, 0x00, 0xbf, 0x01, 0x9f, 0x00, 0xbf, 0x01, 0xbf, 0x00, 0xbf, 0x01, 0xdf, 0x00,  // ................
/* 0320 */ 0xbf, 0x1f, 0x01, 0x00, 0xbf, 0x1f, 0x1f, 0x00, 0xbf, 0x1f, 0x3f, 0x00, 0xbf, 0x1f, 0x5f, 0x00,  // ..........?..._.
/* 0330 */ 0xbf, 0x1f, 0x7f, 0x00, 0xbf, 0x1f, 0x9f, 0x00, 0xbf, 0x1f, 0xbf, 0x00, 0xbf, 0x1f, 0xdf, 0x00,  // ................
/* 0340 */ 0xbf, 0x3f, 0x01, 0x00, 0xbf, 0x3f, 0x1f, 0x00, 0xbf, 0x3f, 0x3f, 0x00, 0xbf, 0x3f, 0x5f, 0x00,  // .?...?...??..?_.
/* 0350 */ 0xbf, 0x3f, 0x7f, 0x00, 0xbf, 0x3f, 0x9f, 0x00, 0xbf, 0x3f, 0xbf, 0x00, 0xbf, 0x3f, 0xdf, 0x00,  // .?...?...?...?..
/* 0360 */ 0xbf, 0x5f, 0x01, 0x00, 0xbf, 0x5f, 0x1f, 0x00, 0xbf, 0x5f, 0x3f, 0x00, 0xbf, 0x5f, 0x5f, 0x00,  // ._..._..._?..__.
/* 0370 */ 0xbf, 0x5f, 0x7f, 0x00, 0xbf, 0x5f, 0x9f, 0x00, 0xbf, 0x5f, 0xbf, 0x00, 0xbf, 0x5f, 0xdf, 0x00,  // ._..._..._..._..
/* 0380 */ 0xbf, 0x7f, 0x01, 0x00, 0xbf, 0x7f, 0x1f, 0x00, 0xbf, 0x7f, 0x3f, 0x00, 0xbf, 0x7f, 0x5f, 0x00,  // ..........?..._.
/* 0390 */ 0xbf, 0x7f, 0x7f, 0x00, 0xbf, 0x7f, 0x9f, 0x00, 0xbf, 0x7f, 0xbf, 0x00, 0xbf, 0x7f, 0xdf, 0x00,  // ................
/* 03a0 */ 0xbf, 0x9f, 0x01, 0x00, 0xbf, 0x9f, 0x1f, 0x00, 0xbf, 0x9f, 0x3f, 0x00, 0xbf, 0x9f, 0x5f, 0x00,  // ..........?..._.
/* 03b0 */ 0xbf, 0x9f, 0x7f, 0x00, 0xbf, 0x9f, 0x9f, 0x00, 0xbf, 0x9f, 0xbf, 0x00, 0xbf, 0x9f, 0xdf, 0x00,  // ................
/* 03c0 */ 0xbf, 0xbf, 0x01, 0x00, 0xbf, 0xbf, 0x1f, 0x00, 0xbf, 0xbf, 0x3f, 0x00, 0xbf, 0xbf, 0x5f, 0x00,  // ..........?..._.
/* 03d0 */ 0xbf, 0xbf, 0x7f, 0x00, 0xbf, 0xbf, 0x9f, 0x00, 0xf0, 0xfb, 0xff, 0x00, 0x74, 0x6f, 0x66, 0x00,  // ............tof.
/* 03e0 */ 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00,  // ................
/* 03f0 */ 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00,  // ................
    };
    uint8_t raw_bitmap_0[] = {
/* 0000 */ 0xc0, 0x30, 0xf7, 0xf0, 0x3c, 0x01, 0xd1, 0xe5, 0x01, 0x00, 0x1b, 0xf0, 0x40, 0x01, 0x60, 0x1d,  // .0..<.......@.`.
/* 0010 */ 0xf7, 0x00, 0xa0, 0xf3, 0x81, 0x01, 0xf7, 0x41, 0x07, 0x00, 0x17, 0x41, 0x0d, 0x00, 0x18, 0x60,  // .......A...A...`
/* 0020 */ 0x62, 0xf7, 0xf9, 0x00, 0x17, 0x00, 0x1f, 0x41, 0x09, 0x00, 0x1a, 0x60, 0x1e, 0xf7, 0x00, 0x20,  // b......A...`...
/* 0030 */ 0x41, 0x0d, 0x00, 0x19, 0xf3, 0x03, 0x08, 0xf7,                          // A.......
    };
    memcpy(palette, raw_palette_0, sizeof(palette));
    bmp = new Bitmap(8, 8, &palette, 64, 64, raw_bitmap_0, sizeof(raw_bitmap_0), true);
    result = bmp_cache.cache_bitmap(*bmp);
    delete bmp;


    uint8_t raw_palette_1[] = {
/* 0000 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00,  // ................
/* 0010 */ 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0xc0, 0xc0, 0xc0, 0x00,  // ................
/* 0020 */ 0xc0, 0xdc, 0xc0, 0x00, 0xc8, 0xd0, 0xd4, 0x00, 0xd8, 0x2d, 0x41, 0x00, 0x7e, 0x00, 0x02, 0x00,  // .........-A.~...
/* 0030 */ 0x01, 0x00, 0x00, 0x00, 0x60, 0x00, 0x01, 0x00, 0x64, 0xfa, 0x12, 0x00, 0xe7, 0x14, 0x40, 0x00,  // ....`...d.....@.
/* 0040 */ 0x25, 0x9e, 0x94, 0x00, 0x00, 0x00, 0x49, 0x00, 0xfd, 0xff, 0xff, 0x00, 0x70, 0xf9, 0x12, 0x00,  // %.....I.....p...
/* 0050 */ 0x8c, 0xf6, 0x12, 0x00, 0x53, 0xeb, 0xda, 0x00, 0x38, 0x7e, 0x17, 0x00, 0x78, 0x01, 0x15, 0x00,  // ....S...8~..x...
/* 0060 */ 0x43, 0xe7, 0xda, 0x00, 0x40, 0x06, 0x15, 0x00, 0x48, 0xcf, 0x16, 0x00, 0xbe, 0xb8, 0xf4, 0x00,  // C...@...H.......
/* 0070 */ 0x24, 0xf8, 0x12, 0x00, 0xca, 0xaf, 0x17, 0x00, 0x04, 0x00, 0x00, 0x00, 0x38, 0xb1, 0x17, 0x00,  // $...........8...
/* 0080 */ 0x58, 0x01, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x40, 0xf7, 0x12, 0x00, 0xa8, 0xae, 0x17, 0x00,  // X...l...@.......
/* 0090 */ 0x20, 0xf8, 0x12, 0x00, 0xad, 0x9d, 0x94, 0x00, 0x78, 0x0d, 0x15, 0x00, 0xc9, 0x9d, 0x94, 0x00,  //  .......x.......
/* 00a0 */ 0x20, 0x51, 0x4d, 0x00, 0xb0, 0xae, 0x17, 0x00, 0x60, 0x01, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00,  //  QM.....`...,...
/* 00b0 */ 0x80, 0xf7, 0x01, 0x00, 0x08, 0x50, 0x18, 0x00, 0x7a, 0x00, 0x00, 0x00, 0xb8, 0xf7, 0x12, 0x00,  // .....P..z.......
/* 00c0 */ 0xe0, 0x80, 0x94, 0x00, 0xd0, 0x9d, 0x94, 0x00, 0x00, 0x00, 0x15, 0x00, 0xa0, 0x7b, 0x16, 0x00,  // .............{..
/* 00d0 */ 0x78, 0xf8, 0x12, 0x00, 0x9b, 0xa6, 0x94, 0x00, 0x58, 0xf8, 0x12, 0x00, 0x58, 0xb3, 0x17, 0x00,  // x.......X...X...
/* 00e0 */ 0xb8, 0x7c, 0x16, 0x00, 0xa8, 0x7b, 0x16, 0x00, 0x6b, 0xc8, 0x00, 0x00, 0x34, 0xf8, 0x12, 0x00,  // .|...{..k...4...
/* 00f0 */ 0xba, 0xde, 0xda, 0x00, 0x90, 0x02, 0x15, 0x00, 0xd8, 0x7e, 0x17, 0x00, 0x08, 0x00, 0x00, 0x00,  // .........~......
/* 0100 */ 0x28, 0x01, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0xd8, 0xaf, 0x17, 0x00, 0x70, 0x8d, 0x17, 0x00,  // (...%.......p...
/* 0110 */ 0x00, 0xf9, 0x12, 0x00, 0xa8, 0x07, 0x15, 0x00, 0x5c, 0xf9, 0x12, 0x00, 0xba, 0x00, 0x01, 0x00,  // ................
/* 0120 */ 0x00, 0x04, 0x00, 0x00, 0x68, 0x01, 0x15, 0x00, 0x28, 0x02, 0x00, 0x00, 0x30, 0xb1, 0x17, 0x00,  // ....h...(...0...
/* 0130 */ 0x60, 0xf8, 0x12, 0x00, 0x94, 0x9f, 0x94, 0x00, 0x48, 0x07, 0x15, 0x00, 0x88, 0xfa, 0x12, 0x00,  // `.......H.......
/* 0140 */ 0x28, 0x9f, 0x94, 0x00, 0x6c, 0x9f, 0x94, 0x00, 0x18, 0xea, 0x15, 0x00, 0x9a, 0xae, 0x94, 0x00,  // (...l...........
/* 0150 */ 0x03, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x49, 0x00, 0x06, 0x00, 0x00, 0x00,  // ..........I.....
/* 0160 */ 0x84, 0xf8, 0x12, 0x00, 0xa0, 0xae, 0x94, 0x00, 0x78, 0x8d, 0x17, 0x00, 0xcc, 0xae, 0x94, 0x00,  // ........x.......
/* 0170 */ 0x30, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x84, 0xf9, 0x12, 0x00, 0x5c, 0xf9, 0x01, 0x00,  // 0... ...........
/* 0180 */ 0x30, 0xf8, 0x12, 0x00, 0xb8, 0xfb, 0x12, 0x00, 0x86, 0xc3, 0xf3, 0x00, 0x79, 0x7b, 0x94, 0x00,  // 0...........y{..
/* 0190 */ 0x06, 0x1d, 0x82, 0x00, 0xd8, 0x00, 0x00, 0x00, 0x2b, 0x1d, 0x82, 0x00, 0x24, 0x00, 0x00, 0x00,  // ........+...$...
/* 01a0 */ 0x80, 0x0f, 0x05, 0x00, 0x00, 0x50, 0xfd, 0x00, 0x2c, 0xf9, 0x12, 0x00, 0x10, 0x00, 0x00, 0x00,  // .....P..,.......
/* 01b0 */ 0xd8, 0xf9, 0x12, 0x00, 0x48, 0x1a, 0x82, 0x00, 0x30, 0x1d, 0x82, 0x00, 0x75, 0x1c, 0x82, 0x00,  // ....H...0...u...
/* 01c0 */ 0x29, 0x76, 0x94, 0x00, 0xa5, 0x19, 0x82, 0x00, 0xe8, 0xf9, 0x12, 0x00, 0x65, 0x38, 0x49, 0x00,  // )v..........e8I.
/* 01d0 */ 0x85, 0x6e, 0x49, 0x00, 0xa4, 0x00, 0xdc, 0x00, 0x00, 0x00, 0xdc, 0x00, 0x64, 0x50, 0x4d, 0x00,  // .nI.........dPM.
/* 01e0 */ 0x07, 0x00, 0x01, 0x00, 0xfc, 0x24, 0x41, 0x00, 0x80, 0xfa, 0x12, 0x00, 0xd8, 0xe2, 0x45, 0x00,  // .....$A.......E.
/* 01f0 */ 0x18, 0xfa, 0x12, 0x00, 0xe3, 0xb6, 0xf4, 0x00, 0x11, 0x01, 0x00, 0x00, 0xcd, 0xab, 0xba, 0x00,  // ................
/* 0200 */ 0x90, 0xfa, 0x12, 0x00, 0x74, 0xb8, 0xf4, 0x00, 0x7f, 0x01, 0x3f, 0x00, 0x7f, 0x01, 0x5f, 0x00,  // ....t.....?..._.
/* 0210 */ 0x7f, 0x01, 0x7f, 0x00, 0x7f, 0x01, 0x9f, 0x00, 0x7f, 0x01, 0xbf, 0x00, 0x7f, 0x01, 0xdf, 0x00,  // ................
/* 0220 */ 0x7f, 0x1f, 0x01, 0x00, 0x7f, 0x1f, 0x1f, 0x00, 0x7f, 0x1f, 0x3f, 0x00, 0x7f, 0x1f, 0x5f, 0x00,  // ..........?..._.
/* 0230 */ 0x7f, 0x1f, 0x7f, 0x00, 0x7f, 0x1f, 0x9f, 0x00, 0x7f, 0x1f, 0xbf, 0x00, 0x7f, 0x1f, 0xdf, 0x00,  // ................
/* 0240 */ 0x7f, 0x3f, 0x01, 0x00, 0x7f, 0x3f, 0x1f, 0x00, 0x7f, 0x3f, 0x3f, 0x00, 0x7f, 0x3f, 0x5f, 0x00,  // .?...?...??..?_.
/* 0250 */ 0x7f, 0x3f, 0x7f, 0x00, 0x7f, 0x3f, 0x9f, 0x00, 0x7f, 0x3f, 0xbf, 0x00, 0x7f, 0x3f, 0xdf, 0x00,  // .?...?...?...?..
/* 0260 */ 0x7f, 0x5f, 0x01, 0x00, 0x7f, 0x5f, 0x1f, 0x00, 0x7f, 0x5f, 0x3f, 0x00, 0x7f, 0x5f, 0x5f, 0x00,  // ._..._..._?..__.
/* 0270 */ 0x7f, 0x5f, 0x7f, 0x00, 0x7f, 0x5f, 0x9f, 0x00, 0x7f, 0x5f, 0xbf, 0x00, 0x7f, 0x5f, 0xdf, 0x00,  // ._..._..._..._..
/* 0280 */ 0x7f, 0x7f, 0x01, 0x00, 0x7f, 0x7f, 0x1f, 0x00, 0x7f, 0x7f, 0x3f, 0x00, 0x7f, 0x7f, 0x5f, 0x00,  // ..........?..._.
/* 0290 */ 0x7f, 0x7f, 0x7f, 0x00, 0x7f, 0x7f, 0x9f, 0x00, 0x7f, 0x7f, 0xbf, 0x00, 0x7f, 0x7f, 0xdf, 0x00,  // ................
/* 02a0 */ 0x7f, 0x9f, 0x01, 0x00, 0x7f, 0x9f, 0x1f, 0x00, 0x7f, 0x9f, 0x3f, 0x00, 0x7f, 0x9f, 0x5f, 0x00,  // ..........?..._.
/* 02b0 */ 0x7f, 0x9f, 0x7f, 0x00, 0x7f, 0x9f, 0x9f, 0x00, 0x7f, 0x9f, 0xbf, 0x00, 0x7f, 0x9f, 0xdf, 0x00,  // ................
/* 02c0 */ 0x7f, 0xbf, 0x01, 0x00, 0x7f, 0xbf, 0x1f, 0x00, 0x7f, 0xbf, 0x3f, 0x00, 0x7f, 0xbf, 0x5f, 0x00,  // ..........?..._.
/* 02d0 */ 0x7f, 0xbf, 0x7f, 0x00, 0x7f, 0xbf, 0x9f, 0x00, 0x7f, 0xbf, 0xbf, 0x00, 0x7f, 0xbf, 0xdf, 0x00,  // ................
/* 02e0 */ 0x7f, 0xdf, 0x01, 0x00, 0x7f, 0xdf, 0x1f, 0x00, 0x7f, 0xdf, 0x3f, 0x00, 0x7f, 0xdf, 0x5f, 0x00,  // ..........?..._.
/* 02f0 */ 0x7f, 0xdf, 0x7f, 0x00, 0x7f, 0xdf, 0x9f, 0x00, 0x7f, 0xdf, 0xbf, 0x00, 0x7f, 0xdf, 0xdf, 0x00,  // ................
/* 0300 */ 0xbf, 0x01, 0x01, 0x00, 0xbf, 0x01, 0x1f, 0x00, 0xbf, 0x01, 0x3f, 0x00, 0xbf, 0x01, 0x5f, 0x00,  // ..........?..._.
/* 0310 */ 0xbf, 0x01, 0x7f, 0x00, 0xbf, 0x01, 0x9f, 0x00, 0xbf, 0x01, 0xbf, 0x00, 0xbf, 0x01, 0xdf, 0x00,  // ................
/* 0320 */ 0xbf, 0x1f, 0x01, 0x00, 0xbf, 0x1f, 0x1f, 0x00, 0xbf, 0x1f, 0x3f, 0x00, 0xbf, 0x1f, 0x5f, 0x00,  // ..........?..._.
/* 0330 */ 0xbf, 0x1f, 0x7f, 0x00, 0xbf, 0x1f, 0x9f, 0x00, 0xbf, 0x1f, 0xbf, 0x00, 0xbf, 0x1f, 0xdf, 0x00,  // ................
/* 0340 */ 0xbf, 0x3f, 0x01, 0x00, 0xbf, 0x3f, 0x1f, 0x00, 0xbf, 0x3f, 0x3f, 0x00, 0xbf, 0x3f, 0x5f, 0x00,  // .?...?...??..?_.
/* 0350 */ 0xbf, 0x3f, 0x7f, 0x00, 0xbf, 0x3f, 0x9f, 0x00, 0xbf, 0x3f, 0xbf, 0x00, 0xbf, 0x3f, 0xdf, 0x00,  // .?...?...?...?..
/* 0360 */ 0xbf, 0x5f, 0x01, 0x00, 0xbf, 0x5f, 0x1f, 0x00, 0xbf, 0x5f, 0x3f, 0x00, 0xbf, 0x5f, 0x5f, 0x00,  // ._..._..._?..__.
/* 0370 */ 0xbf, 0x5f, 0x7f, 0x00, 0xbf, 0x5f, 0x9f, 0x00, 0xbf, 0x5f, 0xbf, 0x00, 0xbf, 0x5f, 0xdf, 0x00,  // ._..._..._..._..
/* 0380 */ 0xbf, 0x7f, 0x01, 0x00, 0xbf, 0x7f, 0x1f, 0x00, 0xbf, 0x7f, 0x3f, 0x00, 0xbf, 0x7f, 0x5f, 0x00,  // ..........?..._.
/* 0390 */ 0xbf, 0x7f, 0x7f, 0x00, 0xbf, 0x7f, 0x9f, 0x00, 0xbf, 0x7f, 0xbf, 0x00, 0xbf, 0x7f, 0xdf, 0x00,  // ................
/* 03a0 */ 0xbf, 0x9f, 0x01, 0x00, 0xbf, 0x9f, 0x1f, 0x00, 0xbf, 0x9f, 0x3f, 0x00, 0xbf, 0x9f, 0x5f, 0x00,  // ..........?..._.
/* 03b0 */ 0xbf, 0x9f, 0x7f, 0x00, 0xbf, 0x9f, 0x9f, 0x00, 0xbf, 0x9f, 0xbf, 0x00, 0xbf, 0x9f, 0xdf, 0x00,  // ................
/* 03c0 */ 0xbf, 0xbf, 0x01, 0x00, 0xbf, 0xbf, 0x1f, 0x00, 0xbf, 0xbf, 0x3f, 0x00, 0xbf, 0xbf, 0x5f, 0x00,  // ..........?..._.
/* 03d0 */ 0xbf, 0xbf, 0x7f, 0x00, 0xbf, 0xbf, 0x9f, 0x00, 0xf0, 0xfb, 0xff, 0x00, 0x74, 0x6f, 0x66, 0x00,  // ............tof.
/* 03e0 */ 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00,  // ................
/* 03f0 */ 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00,  // ................
    };
    uint8_t raw_bitmap_1[] = {
/* 0000 */ 0xc0, 0x30, 0xf7, 0x00, 0xe0, 0xd0, 0xa7, 0xe5, 0x47, 0x1c, 0x0f, 0x11, 0x0e, 0x07, 0x00, 0x00,  // .0......G.......
/* 0010 */ 0x0f, 0x3e, 0x1e, 0x00, 0x9f, 0x0f, 0x00, 0x00, 0x08, 0x20, 0x80, 0x03, 0x90, 0x18, 0x4e, 0x0f,  // .>....... ....N.
/* 0020 */ 0x3c, 0x00, 0x00, 0x1e, 0x0f, 0x00, 0x00, 0x07, 0x1c, 0x00, 0x00, 0x8e, 0x07, 0x00, 0x01, 0x05,  // <...............
/* 0030 */ 0x53, 0x9b, 0x00, 0x20, 0x00, 0x00, 0xbc, 0xfa, 0x78, 0x80, 0x7d, 0x3e, 0x00, 0x00, 0x9c, 0x72,  // S.. ....x.}>...r
/* 0040 */ 0x30, 0x51, 0x39, 0x1c, 0x00, 0x38, 0x41, 0x07, 0x00, 0x19, 0xf3, 0x79, 0x01, 0xf7, 0x51, 0xc3,  // 0Q9..8A....y..Q.
/* 0050 */ 0xc7, 0xb0, 0xc7, 0xe3, 0xc1, 0x98, 0x31, 0xa5, 0x0b, 0x68, 0xab, 0xd5, 0x02, 0x00, 0x80, 0x60,  // ......1..h.....`
/* 0060 */ 0x00, 0x13, 0x05, 0x43, 0x01, 0x02, 0x14, 0x00, 0x0a, 0x65, 0xf7, 0x42, 0x13, 0x06, 0x00, 0x03,  // ...C.....e.B....
/* 0070 */ 0x07, 0x65, 0xf7, 0x42, 0x11, 0x5c, 0x00, 0x05, 0x05, 0x65, 0xf7, 0x41, 0x19, 0x1f, 0x0d, 0x42,  // .e.B.....e.A...B
/* 0080 */ 0x01, 0x40, 0x00, 0x04, 0x0d, 0x43, 0x81, 0x01, 0x03, 0x1a, 0x14, 0x50, 0xa5, 0x0b, 0x57, 0xab,  // .@...C.....P..W.
/* 0090 */ 0xd5, 0x22, 0x00, 0x82, 0xc3, 0xc7, 0x8f, 0xc7, 0xe3, 0xc1, 0x98, 0x31, 0xf0, 0x00, 0x08,     // .".........1...
    };
    memcpy(palette, raw_palette_1, sizeof(palette));
    bmp = new Bitmap(8, 8, &palette, 64, 64, raw_bitmap_1, sizeof(raw_bitmap_1), true);
    result = bmp_cache.cache_bitmap(*bmp);
    delete bmp;


    uint8_t raw_palette_2[] = {
/* 0000 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00,  // ................
/* 0010 */ 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0xc0, 0xc0, 0xc0, 0x00,  // ................
/* 0020 */ 0xc0, 0xdc, 0xc0, 0x00, 0xc8, 0xd0, 0xd4, 0x00, 0xd8, 0x2d, 0x41, 0x00, 0x7e, 0x00, 0x02, 0x00,  // .........-A.~...
/* 0030 */ 0x01, 0x00, 0x00, 0x00, 0x60, 0x00, 0x01, 0x00, 0x64, 0xfa, 0x12, 0x00, 0xe7, 0x14, 0x40, 0x00,  // ....`...d.....@.
/* 0040 */ 0x25, 0x9e, 0x94, 0x00, 0x00, 0x00, 0x49, 0x00, 0xfd, 0xff, 0xff, 0x00, 0x70, 0xf9, 0x12, 0x00,  // %.....I.....p...
/* 0050 */ 0x8c, 0xf6, 0x12, 0x00, 0x53, 0xeb, 0xda, 0x00, 0x38, 0x7e, 0x17, 0x00, 0x78, 0x01, 0x15, 0x00,  // ....S...8~..x...
/* 0060 */ 0x43, 0xe7, 0xda, 0x00, 0x40, 0x06, 0x15, 0x00, 0x48, 0xcf, 0x16, 0x00, 0xbe, 0xb8, 0xf4, 0x00,  // C...@...H.......
/* 0070 */ 0x24, 0xf8, 0x12, 0x00, 0xca, 0xaf, 0x17, 0x00, 0x04, 0x00, 0x00, 0x00, 0x38, 0xb1, 0x17, 0x00,  // $...........8...
/* 0080 */ 0x58, 0x01, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x40, 0xf7, 0x12, 0x00, 0xa8, 0xae, 0x17, 0x00,  // X...l...@.......
/* 0090 */ 0x20, 0xf8, 0x12, 0x00, 0xad, 0x9d, 0x94, 0x00, 0x78, 0x0d, 0x15, 0x00, 0xc9, 0x9d, 0x94, 0x00,  //  .......x.......
/* 00a0 */ 0x20, 0x51, 0x4d, 0x00, 0xb0, 0xae, 0x17, 0x00, 0x60, 0x01, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00,  //  QM.....`...,...
/* 00b0 */ 0x80, 0xf7, 0x01, 0x00, 0x08, 0x50, 0x18, 0x00, 0x7a, 0x00, 0x00, 0x00, 0xb8, 0xf7, 0x12, 0x00,  // .....P..z.......
/* 00c0 */ 0xe0, 0x80, 0x94, 0x00, 0xd0, 0x9d, 0x94, 0x00, 0x00, 0x00, 0x15, 0x00, 0xa0, 0x7b, 0x16, 0x00,  // .............{..
/* 00d0 */ 0x78, 0xf8, 0x12, 0x00, 0x9b, 0xa6, 0x94, 0x00, 0x58, 0xf8, 0x12, 0x00, 0x58, 0xb3, 0x17, 0x00,  // x.......X...X...
/* 00e0 */ 0xb8, 0x7c, 0x16, 0x00, 0xa8, 0x7b, 0x16, 0x00, 0x6b, 0xc8, 0x00, 0x00, 0x34, 0xf8, 0x12, 0x00,  // .|...{..k...4...
/* 00f0 */ 0xba, 0xde, 0xda, 0x00, 0x90, 0x02, 0x15, 0x00, 0xd8, 0x7e, 0x17, 0x00, 0x08, 0x00, 0x00, 0x00,  // .........~......
/* 0100 */ 0x28, 0x01, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0xd8, 0xaf, 0x17, 0x00, 0x70, 0x8d, 0x17, 0x00,  // (...%.......p...
/* 0110 */ 0x00, 0xf9, 0x12, 0x00, 0xa8, 0x07, 0x15, 0x00, 0x5c, 0xf9, 0x12, 0x00, 0xba, 0x00, 0x01, 0x00,  // ................
/* 0120 */ 0x00, 0x04, 0x00, 0x00, 0x68, 0x01, 0x15, 0x00, 0x28, 0x02, 0x00, 0x00, 0x30, 0xb1, 0x17, 0x00,  // ....h...(...0...
/* 0130 */ 0x60, 0xf8, 0x12, 0x00, 0x94, 0x9f, 0x94, 0x00, 0x48, 0x07, 0x15, 0x00, 0x88, 0xfa, 0x12, 0x00,  // `.......H.......
/* 0140 */ 0x28, 0x9f, 0x94, 0x00, 0x6c, 0x9f, 0x94, 0x00, 0x18, 0xea, 0x15, 0x00, 0x9a, 0xae, 0x94, 0x00,  // (...l...........
/* 0150 */ 0x03, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x49, 0x00, 0x06, 0x00, 0x00, 0x00,  // ..........I.....
/* 0160 */ 0x84, 0xf8, 0x12, 0x00, 0xa0, 0xae, 0x94, 0x00, 0x78, 0x8d, 0x17, 0x00, 0xcc, 0xae, 0x94, 0x00,  // ........x.......
/* 0170 */ 0x30, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x84, 0xf9, 0x12, 0x00, 0x5c, 0xf9, 0x01, 0x00,  // 0... ...........
/* 0180 */ 0x30, 0xf8, 0x12, 0x00, 0xb8, 0xfb, 0x12, 0x00, 0x86, 0xc3, 0xf3, 0x00, 0x79, 0x7b, 0x94, 0x00,  // 0...........y{..
/* 0190 */ 0x06, 0x1d, 0x82, 0x00, 0xd8, 0x00, 0x00, 0x00, 0x2b, 0x1d, 0x82, 0x00, 0x24, 0x00, 0x00, 0x00,  // ........+...$...
/* 01a0 */ 0x80, 0x0f, 0x05, 0x00, 0x00, 0x50, 0xfd, 0x00, 0x2c, 0xf9, 0x12, 0x00, 0x10, 0x00, 0x00, 0x00,  // .....P..,.......
/* 01b0 */ 0xd8, 0xf9, 0x12, 0x00, 0x48, 0x1a, 0x82, 0x00, 0x30, 0x1d, 0x82, 0x00, 0x75, 0x1c, 0x82, 0x00,  // ....H...0...u...
/* 01c0 */ 0x29, 0x76, 0x94, 0x00, 0xa5, 0x19, 0x82, 0x00, 0xe8, 0xf9, 0x12, 0x00, 0x65, 0x38, 0x49, 0x00,  // )v..........e8I.
/* 01d0 */ 0x85, 0x6e, 0x49, 0x00, 0xa4, 0x00, 0xdc, 0x00, 0x00, 0x00, 0xdc, 0x00, 0x64, 0x50, 0x4d, 0x00,  // .nI.........dPM.
/* 01e0 */ 0x07, 0x00, 0x01, 0x00, 0xfc, 0x24, 0x41, 0x00, 0x80, 0xfa, 0x12, 0x00, 0xd8, 0xe2, 0x45, 0x00,  // .....$A.......E.
/* 01f0 */ 0x18, 0xfa, 0x12, 0x00, 0xe3, 0xb6, 0xf4, 0x00, 0x11, 0x01, 0x00, 0x00, 0xcd, 0xab, 0xba, 0x00,  // ................
/* 0200 */ 0x90, 0xfa, 0x12, 0x00, 0x74, 0xb8, 0xf4, 0x00, 0x7f, 0x01, 0x3f, 0x00, 0x7f, 0x01, 0x5f, 0x00,  // ....t.....?..._.
/* 0210 */ 0x7f, 0x01, 0x7f, 0x00, 0x7f, 0x01, 0x9f, 0x00, 0x7f, 0x01, 0xbf, 0x00, 0x7f, 0x01, 0xdf, 0x00,  // ................
/* 0220 */ 0x7f, 0x1f, 0x01, 0x00, 0x7f, 0x1f, 0x1f, 0x00, 0x7f, 0x1f, 0x3f, 0x00, 0x7f, 0x1f, 0x5f, 0x00,  // ..........?..._.
/* 0230 */ 0x7f, 0x1f, 0x7f, 0x00, 0x7f, 0x1f, 0x9f, 0x00, 0x7f, 0x1f, 0xbf, 0x00, 0x7f, 0x1f, 0xdf, 0x00,  // ................
/* 0240 */ 0x7f, 0x3f, 0x01, 0x00, 0x7f, 0x3f, 0x1f, 0x00, 0x7f, 0x3f, 0x3f, 0x00, 0x7f, 0x3f, 0x5f, 0x00,  // .?...?...??..?_.
/* 0250 */ 0x7f, 0x3f, 0x7f, 0x00, 0x7f, 0x3f, 0x9f, 0x00, 0x7f, 0x3f, 0xbf, 0x00, 0x7f, 0x3f, 0xdf, 0x00,  // .?...?...?...?..
/* 0260 */ 0x7f, 0x5f, 0x01, 0x00, 0x7f, 0x5f, 0x1f, 0x00, 0x7f, 0x5f, 0x3f, 0x00, 0x7f, 0x5f, 0x5f, 0x00,  // ._..._..._?..__.
/* 0270 */ 0x7f, 0x5f, 0x7f, 0x00, 0x7f, 0x5f, 0x9f, 0x00, 0x7f, 0x5f, 0xbf, 0x00, 0x7f, 0x5f, 0xdf, 0x00,  // ._..._..._..._..
/* 0280 */ 0x7f, 0x7f, 0x01, 0x00, 0x7f, 0x7f, 0x1f, 0x00, 0x7f, 0x7f, 0x3f, 0x00, 0x7f, 0x7f, 0x5f, 0x00,  // ..........?..._.
/* 0290 */ 0x7f, 0x7f, 0x7f, 0x00, 0x7f, 0x7f, 0x9f, 0x00, 0x7f, 0x7f, 0xbf, 0x00, 0x7f, 0x7f, 0xdf, 0x00,  // ................
/* 02a0 */ 0x7f, 0x9f, 0x01, 0x00, 0x7f, 0x9f, 0x1f, 0x00, 0x7f, 0x9f, 0x3f, 0x00, 0x7f, 0x9f, 0x5f, 0x00,  // ..........?..._.
/* 02b0 */ 0x7f, 0x9f, 0x7f, 0x00, 0x7f, 0x9f, 0x9f, 0x00, 0x7f, 0x9f, 0xbf, 0x00, 0x7f, 0x9f, 0xdf, 0x00,  // ................
/* 02c0 */ 0x7f, 0xbf, 0x01, 0x00, 0x7f, 0xbf, 0x1f, 0x00, 0x7f, 0xbf, 0x3f, 0x00, 0x7f, 0xbf, 0x5f, 0x00,  // ..........?..._.
/* 02d0 */ 0x7f, 0xbf, 0x7f, 0x00, 0x7f, 0xbf, 0x9f, 0x00, 0x7f, 0xbf, 0xbf, 0x00, 0x7f, 0xbf, 0xdf, 0x00,  // ................
/* 02e0 */ 0x7f, 0xdf, 0x01, 0x00, 0x7f, 0xdf, 0x1f, 0x00, 0x7f, 0xdf, 0x3f, 0x00, 0x7f, 0xdf, 0x5f, 0x00,  // ..........?..._.
/* 02f0 */ 0x7f, 0xdf, 0x7f, 0x00, 0x7f, 0xdf, 0x9f, 0x00, 0x7f, 0xdf, 0xbf, 0x00, 0x7f, 0xdf, 0xdf, 0x00,  // ................
/* 0300 */ 0xbf, 0x01, 0x01, 0x00, 0xbf, 0x01, 0x1f, 0x00, 0xbf, 0x01, 0x3f, 0x00, 0xbf, 0x01, 0x5f, 0x00,  // ..........?..._.
/* 0310 */ 0xbf, 0x01, 0x7f, 0x00, 0xbf, 0x01, 0x9f, 0x00, 0xbf, 0x01, 0xbf, 0x00, 0xbf, 0x01, 0xdf, 0x00,  // ................
/* 0320 */ 0xbf, 0x1f, 0x01, 0x00, 0xbf, 0x1f, 0x1f, 0x00, 0xbf, 0x1f, 0x3f, 0x00, 0xbf, 0x1f, 0x5f, 0x00,  // ..........?..._.
/* 0330 */ 0xbf, 0x1f, 0x7f, 0x00, 0xbf, 0x1f, 0x9f, 0x00, 0xbf, 0x1f, 0xbf, 0x00, 0xbf, 0x1f, 0xdf, 0x00,  // ................
/* 0340 */ 0xbf, 0x3f, 0x01, 0x00, 0xbf, 0x3f, 0x1f, 0x00, 0xbf, 0x3f, 0x3f, 0x00, 0xbf, 0x3f, 0x5f, 0x00,  // .?...?...??..?_.
/* 0350 */ 0xbf, 0x3f, 0x7f, 0x00, 0xbf, 0x3f, 0x9f, 0x00, 0xbf, 0x3f, 0xbf, 0x00, 0xbf, 0x3f, 0xdf, 0x00,  // .?...?...?...?..
/* 0360 */ 0xbf, 0x5f, 0x01, 0x00, 0xbf, 0x5f, 0x1f, 0x00, 0xbf, 0x5f, 0x3f, 0x00, 0xbf, 0x5f, 0x5f, 0x00,  // ._..._..._?..__.
/* 0370 */ 0xbf, 0x5f, 0x7f, 0x00, 0xbf, 0x5f, 0x9f, 0x00, 0xbf, 0x5f, 0xbf, 0x00, 0xbf, 0x5f, 0xdf, 0x00,  // ._..._..._..._..
/* 0380 */ 0xbf, 0x7f, 0x01, 0x00, 0xbf, 0x7f, 0x1f, 0x00, 0xbf, 0x7f, 0x3f, 0x00, 0xbf, 0x7f, 0x5f, 0x00,  // ..........?..._.
/* 0390 */ 0xbf, 0x7f, 0x7f, 0x00, 0xbf, 0x7f, 0x9f, 0x00, 0xbf, 0x7f, 0xbf, 0x00, 0xbf, 0x7f, 0xdf, 0x00,  // ................
/* 03a0 */ 0xbf, 0x9f, 0x01, 0x00, 0xbf, 0x9f, 0x1f, 0x00, 0xbf, 0x9f, 0x3f, 0x00, 0xbf, 0x9f, 0x5f, 0x00,  // ..........?..._.
/* 03b0 */ 0xbf, 0x9f, 0x7f, 0x00, 0xbf, 0x9f, 0x9f, 0x00, 0xbf, 0x9f, 0xbf, 0x00, 0xbf, 0x9f, 0xdf, 0x00,  // ................
/* 03c0 */ 0xbf, 0xbf, 0x01, 0x00, 0xbf, 0xbf, 0x1f, 0x00, 0xbf, 0xbf, 0x3f, 0x00, 0xbf, 0xbf, 0x5f, 0x00,  // ..........?..._.
/* 03d0 */ 0xbf, 0xbf, 0x7f, 0x00, 0xbf, 0xbf, 0x9f, 0x00, 0xf0, 0xfb, 0xff, 0x00, 0x74, 0x6f, 0x66, 0x00,  // ............tof.
/* 03e0 */ 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00,  // ................
/* 03f0 */ 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00,  // ................
    };
    uint8_t raw_bitmap_2[] = {
/* 0000 */ 0xc0, 0x30, 0xf7, 0xf0, 0x00, 0x05, 0xd3, 0xe5, 0x0f, 0xe6, 0x07, 0x00, 0x09, 0x43, 0x0b, 0xc0,  // .0...........C..
/* 0010 */ 0x03, 0x00, 0x14, 0xfa, 0x00, 0x19, 0xfa, 0x00, 0x1b, 0x00, 0x1e, 0x41, 0x01, 0x00, 0x10, 0x09,  // ...........A....
/* 0020 */ 0xfa, 0x00, 0x10, 0x00, 0x1f, 0x41, 0xc1, 0x00, 0x12, 0x43, 0x8b, 0xd0, 0x02, 0x00, 0x07, 0xf3,  // .....A...C......
/* 0030 */ 0x40, 0x08, 0xf7,                                         // @..
    };
    memcpy(palette, raw_palette_2, sizeof(palette));
    bmp = new Bitmap(8, 8, &palette, 64, 64, raw_bitmap_2, sizeof(raw_bitmap_2), true);
    result = bmp_cache.cache_bitmap(*bmp);
(void)result;
    delete bmp;
    // result is to use !
    (void)result;
    //LogTransport t;

    #include "fixtures/persistent_disk_bitmap_cache.hpp"
    CheckTransport t(outdata, sizeof(outdata), verbose);

    BmpCachePersister::save_all_to_disk(bmp_cache, t, verbose);
}

BOOST_AUTO_TEST_CASE(TestBmpCachePersister1)
{
    uint8_t  bpp              = 8;
    bool     use_waiting_list = false;
    uint32_t verbose          = 1;

    struct BmpCacheParams {
        uint16_t entiers;
        int      size;
        bool     persistent;
    } bmp_cache_params [] = {
        { 120,  nbbytes(bpp) * 16 * 16, false },
        { 120,  nbbytes(bpp) * 32 * 32, false },
        { 2553, nbbytes(bpp) * 64 * 64, true  }
    };

    BmpCache bmp_cache( BmpCache::Recorder, bpp, 3, use_waiting_list
                      , BmpCache::CacheOption(bmp_cache_params[0].entiers, bmp_cache_params[0].size, bmp_cache_params[0].persistent)
                      , BmpCache::CacheOption(bmp_cache_params[1].entiers, bmp_cache_params[1].size, bmp_cache_params[1].persistent)
                      , BmpCache::CacheOption(bmp_cache_params[2].entiers, bmp_cache_params[2].size, bmp_cache_params[2].persistent)
                      , BmpCache::CacheOption()
                      , BmpCache::CacheOption()
                      , verbose
                      );

    #include "fixtures/persistent_disk_bitmap_cache.hpp"
    GeneratorTransport t(outdata, sizeof(outdata));

    BmpCachePersister bmp_cache_persister(bmp_cache, t, "fixtures/persistent_disk_bitmap_cache.hpp", verbose);

    RDP::BitmapCachePersistentListEntry persistent_list[] = {
        { 0x99E1C40C, 0x17C187AF },
        { 0x03E8896E, 0x5C267FC8 },
        { 0xABABABAB, 0xCDCDCDCD },
        { 0x63D8DC64, 0x0A888EF6 }
    };
    uint8_t  cache_id          = 2;
    uint16_t number_of_entries = sizeof(persistent_list) / sizeof(persistent_list[0]);
    uint16_t first_entry_index = 0;
    bmp_cache_persister.process_key_list(cache_id, persistent_list, number_of_entries, first_entry_index);

    BOOST_CHECK((bmp_cache.get_cache(cache_id)[0].sig.sig_32[0] == 0x99E1C40C) && (bmp_cache.get_cache(cache_id)[0].sig.sig_32[1] == 0x17C187AF));
    BOOST_CHECK((bmp_cache.get_cache(cache_id)[1].sig.sig_32[0] == 0x03E8896E) && (bmp_cache.get_cache(cache_id)[1].sig.sig_32[1] == 0x5C267FC8));

    BOOST_CHECK(!bmp_cache.get_cache(cache_id)[2]);

    BOOST_CHECK((bmp_cache.get_cache(cache_id)[3].sig.sig_32[0] == 0x63D8DC64) && (bmp_cache.get_cache(cache_id)[3].sig.sig_32[1] == 0x0A888EF6));

    BOOST_CHECK(!bmp_cache.get_cache(cache_id)[4]);
}

BOOST_AUTO_TEST_CASE(TestBmpCachePersister2)
{
    uint8_t  bpp              = 8;
    bool     use_waiting_list = false;
    uint32_t verbose          = 1;

    struct BmpCacheParams {
        uint16_t entiers;
        int      size;
        bool     persistent;
    } bmp_cache_params [] = {
        { 120,  nbbytes(bpp) * 16 * 16, false },
        { 120,  nbbytes(bpp) * 32 * 32, false },
        { 2553, nbbytes(bpp) * 64 * 64, true  }
    };

    BmpCache bmp_cache( BmpCache::Recorder, bpp, 3, use_waiting_list
                      , BmpCache::CacheOption(bmp_cache_params[0].entiers, bmp_cache_params[0].size, bmp_cache_params[0].persistent)
                      , BmpCache::CacheOption(bmp_cache_params[1].entiers, bmp_cache_params[1].size, bmp_cache_params[1].persistent)
                      , BmpCache::CacheOption(bmp_cache_params[2].entiers, bmp_cache_params[2].size, bmp_cache_params[2].persistent)
                      , BmpCache::CacheOption()
                      , BmpCache::CacheOption()
                      , verbose
                      );

    #include "fixtures/persistent_disk_bitmap_cache.hpp"
    GeneratorTransport t(outdata, sizeof(outdata));

    BmpCachePersister::load_all_from_disk(bmp_cache, t, "fixtures/persistent_disk_bitmap_cache.hpp", verbose);

    uint8_t cache_id = 2;
    BOOST_CHECK((bmp_cache.get_cache(cache_id)[0].sig.sig_32[0] == 0x99E1C40C) && (bmp_cache.get_cache(cache_id)[0].sig.sig_32[1] == 0x17C187AF));
    BOOST_CHECK((bmp_cache.get_cache(cache_id)[1].sig.sig_32[0] == 0x03E8896E) && (bmp_cache.get_cache(cache_id)[1].sig.sig_32[1] == 0x5C267FC8));
    BOOST_CHECK((bmp_cache.get_cache(cache_id)[2].sig.sig_32[0] == 0x63D8DC64) && (bmp_cache.get_cache(cache_id)[2].sig.sig_32[1] == 0x0A888EF6));

    BOOST_CHECK(!bmp_cache.get_cache(cache_id)[3]);
}
