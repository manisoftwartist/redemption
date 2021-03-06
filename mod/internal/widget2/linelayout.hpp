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
 *   Copyright (C) Wallix 2010-2014
 *   Author(s): Christophe Grosjean, Raphael Zhou, Jonathan Poelen,
 *              Meng Tan
 *
 */

#ifndef _REDEMPTION_MOD_INTERNAL_WIDGET2_LINELAYOUT_HPP_
#define _REDEMPTION_MOD_INTERNAL_WIDGET2_LINELAYOUT_HPP_

#include "widget.hpp"
#include "layout.hpp"

struct WidgetLineLayout : public WidgetLayout {

    WidgetLineLayout(DrawApi & drawable, const Rect & rect, Widget2 & parent,
                     NotifyApi * notifier, int group_id = 0)
        : WidgetLayout(drawable, rect, parent, notifier, group_id)
    {
    }

    virtual ~WidgetLineLayout() {
    }

    virtual void rearrange(size_t origin = 0) {
        size_t index = origin;
        int pos_x = this->rect.x;
        if (index > 0) {
            pos_x = this->items[index - 1]->lx();
        }
        for (; index < this->nb_items; index++) {
            Widget2 * w = this->items[index];
            w->set_xy(pos_x, this->rect.y);
            pos_x += this->items[index]->cx();
            if (w->cy() > this->rect.cy) {
                this->rect.cy = w->cy();
            }
        }
        this->rect.cx = pos_x - this->rect.x;
    }

};


#endif
