/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Product name: redemption, a FLOSS RDP proxy
 *   Copyright (C) Wallix 2010-2013
 *   Author(s): Christophe Grosjean, Xiaopeng Zhou, Jonathan Poelen, Meng Tan
 */

#ifndef REDEMPTION_MOD_INTERNAL_FLAT_LOGIN_MOD_HPP
#define REDEMPTION_MOD_INTERNAL_FLAT_LOGIN_MOD_HPP

#include "version.hpp"
#include "front_api.hpp"
#include "config.hpp"
#include "widget2/flat_login.hpp"
#include "internal_mod.hpp"
#include "widget2/notify_api.hpp"
#include "translation.hpp"
#include "copy_paste.hpp"

class FlatLoginMod : public InternalMod, public NotifyApi
{
    FlatLogin login;

    CopyPaste copy_paste;

public:
    Inifile & ini;

    FlatLoginMod(Inifile & ini, FrontAPI & front, uint16_t width, uint16_t height)
        : InternalMod(front, width, height, ini.font, &ini)
        , login(*this, width, height, this->screen, this, "Redemption " VERSION,
                ini.account.username[0] != 0,
                0, 0, 0, TR("login", ini), TR("password", ini), ini)
        , ini(ini)
    {
        this->screen.add_widget(&this->login);

        this->login.login_edit.set_text(this->ini.account.username);
        this->login.password_edit.set_text(this->ini.account.password);

        this->screen.set_widget_focus(&this->login, Widget2::focus_reason_tabkey);

        this->login.set_widget_focus(&this->login.login_edit, Widget2::focus_reason_tabkey);
        if (ini.account.username[0] != 0){
            this->login.set_widget_focus(&this->login.password_edit, Widget2::focus_reason_tabkey);
        }

        this->screen.refresh(this->screen.rect);
    }

    virtual ~FlatLoginMod()
    {
        this->screen.clear();
    }

    virtual void notify(Widget2* sender, notify_event_t event)
    {
        switch (event) {
        case NOTIFY_SUBMIT:
            this->ini.parse_username(this->login.login_edit.get_text());
            this->ini.context_set_value(AUTHID_PASSWORD, this->login.password_edit.get_text());
            this->event.signal = BACK_EVENT_NEXT;
            this->event.set();
            break;
        case NOTIFY_CANCEL:
            this->event.signal = BACK_EVENT_STOP;
            this->event.set();
            break;
        default:
            if (this->copy_paste) {
                copy_paste_process_event(this->copy_paste, *reinterpret_cast<WidgetEdit*>(sender), event);
            }
            break;
        }
    }

    virtual void draw_event(time_t now)
    {
        if (!this->copy_paste && event.waked_up_by_time) {
            this->copy_paste.ready(this->front);
        }
        this->event.reset();
    }

    virtual void send_to_mod_channel(const char * front_channel_name, Stream& chunk, size_t length, uint32_t flags)
    {
        if (this->copy_paste) {
            this->copy_paste.send_to_mod_channel(chunk, flags);
        }
    }
};

#endif
