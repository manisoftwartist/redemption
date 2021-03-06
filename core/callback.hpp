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
   Copyright (C) Wallix 2010
   Author(s): Christophe Grosjean, Javier Caverni

   This class implement abstract callback used by front layer
   it is used to avoid explicit dependency between front layer
   and session (to enable to use front layer in tests).

*/

#ifndef _REDEMPTION_CORE_CALLBACK_HPP_
#define _REDEMPTION_CORE_CALLBACK_HPP_

#include "darray.hpp"
#include "rect.hpp"

class Stream;
class Keymap2;

enum {
    RDP_INPUT_SYNCHRONIZE          = 0,
    RDP_INPUT_CODEPOINT            = 1,
    RDP_INPUT_VIRTKEY              = 2,
    RDP_INPUT_SCANCODE             = 4,
    RDP_INPUT_MOUSE                = 0x8001
};

/* Device flags */
enum {
    KBD_FLAG_RIGHT                 = 0x0001,
    KBD_FLAG_EXT                   = 0x0100,
    KBD_FLAG_QUIET                 = 0x1000,
    KBD_FLAG_DOWN                  = 0x4000,
    KBD_FLAG_UP                    = 0x8000
};

/* These are for synchronization; not for keystrokes */
enum {
    KBD_FLAG_SCROLL                = 0x0001,
    KBD_FLAG_NUMLOCK               = 0x0002,
    KBD_FLAG_CAPITAL               = 0x0004
};

enum {
    MOUSE_FLAG_MOVE                = 0x0800,
    MOUSE_FLAG_BUTTON1             = 0x1000,
    MOUSE_FLAG_BUTTON2             = 0x2000,
    MOUSE_FLAG_BUTTON3             = 0x4000,
    MOUSE_FLAG_BUTTON4             = 0x0280,
    MOUSE_FLAG_BUTTON5             = 0x0380,
    MOUSE_FLAG_DOWN                = 0x8000
};

struct RdpInput
{
    virtual ~RdpInput() {}
    virtual void rdp_input_scancode(long param1, long param2, long param3, long param4, Keymap2 * keymap) = 0;
    virtual void rdp_input_mouse(int device_flags, int x, int y, Keymap2 * keymap) = 0;
    virtual void rdp_input_synchronize(uint32_t time, uint16_t device_flags, int16_t param1, int16_t param2) = 0;
    virtual void rdp_input_invalidate(const Rect & r) = 0;
    virtual void rdp_input_invalidate2(const DArray<Rect> & vr) {
        for (size_t i = 0; i < vr.size(); i++) {
            if (!vr[i].isempty()) {
                this->rdp_input_invalidate(vr[i]);
            }
        }
    }
    // Client calls this member function when it became up and running.
    virtual void rdp_input_up_and_running() { /* LOG(LOG_ERR, "CB:UP_AND_RUNNING"); */}
};

struct Callback : RdpInput
{
    virtual void send_to_mod_channel(const char * const front_channel_name, Stream & chunk, std::size_t length, uint32_t flags)
    {
    }
    // Interface for session to send back to mod_rdp for tse virtual channel target data (asked previously)
    virtual void send_auth_channel_data(const char * data) {}
};

#endif
