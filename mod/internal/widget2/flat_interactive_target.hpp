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
 *   Copyright (C) Wallix 2010-2014
 *   Author(s): Christophe Grosjean, Raphael Zhou, Meng Tan
 */

#if !defined(REDEMPTION_MOD_INTERNAL_WIDGET2_FLAT_INTERACTIVE_TARGET_HPP)
#define REDEMPTION_MOD_INTERNAL_WIDGET2_FLAT_INTERACTIVE_TARGET_HPP

#include "edit.hpp"
#include "edit_valid.hpp"
#include "label.hpp"
#include "password.hpp"
#include "multiline.hpp"
#include "widget2_rect.hpp"
#include "composite.hpp"
#include "flat_button.hpp"
#include "translation.hpp"
#include "theme.hpp"


class FlatInteractiveTarget : public WidgetParent
{
public:
    WidgetLabel     caption_label;
    WidgetLabel     device_label;
    WidgetLabel     device;
    WidgetEditValid device_edit;
    WidgetLabel     login_label;
    WidgetLabel     login;
    WidgetEditValid login_edit;
    WidgetLabel     password_label;
    WidgetEditValid password_edit;
    WidgetRect separator;
    Widget2 *       last_interactive;

    int fgcolor;
    int bgcolor;

    CompositeArray composite_array;

    // ASK DEVICE YES/NO
    // ASK CRED : LOGIN+PASSWORD/PASSWORD/NO

    FlatInteractiveTarget(DrawApi& drawable, uint16_t width, uint16_t height,
                          Widget2 & parent, NotifyApi* notifier,
                          int group_id, bool ask_device,
                          bool ask_login, bool ask_password,
                          Theme & theme, const char* caption,
                          const char * text_device,
                          const char * device_str,
                          const char * text_login,
                          const char * login_str,
                          const char * text_password,
                          Font const & font)
        : WidgetParent(drawable, Rect(0, 0, width, height), parent, notifier)
        , caption_label(drawable, 0, 0, *this, NULL, caption, true, -13,
                        theme.global.fgcolor, theme.global.bgcolor, font)
        , device_label(drawable, 0, 0, *this, NULL, text_device, true, -13,
                      theme.global.fgcolor, theme.global.bgcolor, font)
        , device(drawable, 0, 0, *this, NULL, device_str, true, -13,
                 theme.global.fgcolor, theme.global.bgcolor, font)
        , device_edit(drawable, 0, 0, 400, *this, this, NULL, -14,
                      theme.edit.fgcolor, theme.edit.bgcolor,
                      theme.edit.focus_color, font, -1u, 1, 1)
        , login_label(drawable, 0, 0, *this, NULL, text_login, true, -13,
                      theme.global.fgcolor, theme.global.bgcolor, font)
        , login(drawable, 0, 0, *this, NULL, login_str, true, -13,
                theme.global.fgcolor, theme.global.bgcolor, font)
        , login_edit(drawable, 0, 0, 400, *this, this, NULL, -14,
                     theme.edit.fgcolor, theme.edit.bgcolor,
                     theme.edit.focus_color, font, -1u, 1, 1)
        , password_label(drawable, 0, 0, *this, NULL, text_password, true, -13,
                         theme.global.fgcolor, theme.global.bgcolor, font)
        , password_edit(drawable, 0, 0, 400, *this, this, NULL, -14,
                        theme.edit.fgcolor, theme.edit.bgcolor,
                        theme.edit.focus_color, font, -1u, 1, 1, true)
        , separator(drawable, Rect(0, 0, width, 2), *this, this, -12,
                    theme.global.separator_color)
        , last_interactive((ask_login || ask_password)?&this->password_edit:&this->device_edit)
        , fgcolor(theme.global.fgcolor)
        , bgcolor(theme.global.bgcolor)
    {
        this->impl = &composite_array;

        ask_password = (ask_login || ask_password);

        Widget2 * device_show = &this->device;
        if (ask_device) {
            device_show = &this->device_edit;
        }
        Widget2 * login_show = &this->login;
        if (ask_login) {
            login_show = &this->login_edit;
        }
        Widget2 * password_show = NULL;
        if (ask_password) {
            password_show = &this->password_edit;
        }


        this->add_widget(&this->caption_label);
        this->add_widget(&this->device_label);
        this->add_widget(device_show);
        if (ask_device) {
            this->add_widget(&this->device);
        }
        this->add_widget(&this->login_label);
        this->add_widget(login_show);
        if (password_show) {
            this->add_widget(&this->password_label);
            this->add_widget(password_show);
        }
        this->add_widget(&this->separator);

        // Center bloc positionning
        // Device, Login and Password boxes
        int margin_w = std::max<int>(this->device_label.rect.cx,
                                     this->login_label.rect.cx);
        margin_w = std::max<int>(margin_w,
                                 this->password_label.rect.cx);

        int cbloc_w = std::max<int>(this->caption_label.rect.cx,
                                    margin_w + device_show->rect.cx + 20);
        cbloc_w = std::max<int>(cbloc_w,
                                margin_w + login_show->rect.cx + 20);
        cbloc_w = std::max<int>(cbloc_w,
                                margin_w + this->password_edit.rect.cx + 20);

        if (ask_device) {
            cbloc_w = std::max<int>(cbloc_w,
                                    margin_w + this->device.rect.cx + 20);
        }

        int extra_h = 0;
        if (password_show) {
            extra_h += std::max(this->password_label.rect.cy,
                                this->password_edit.rect.cy) + 20;
        }
        if (ask_device) {
            extra_h += this->device.rect.cy + 30;
        }
        int cbloc_h = this->caption_label.rect.cy + 20 + 30 +
            this->device_label.rect.cy + 30 +
            this->login_label.rect.cy + 30 +
            extra_h;

        int x_cbloc = (width  - cbloc_w) / 2;
        int y_cbloc = (height - cbloc_h) / 3;

        int y = y_cbloc;
        this->caption_label.set_xy((width - this->caption_label.rect.cx) / 2, y);
        this->separator.rect.cx = cbloc_w;

        y = this->caption_label.ly() + 20;
        this->separator.set_xy(x_cbloc, y);

        y = this->separator.ly() + 20;
        this->device_label.set_xy(x_cbloc, y);
        device_show->set_xy(x_cbloc + margin_w + 20, y);
        y = device_show->ly() + 20;
        if (ask_device) {
            this->device.set_xy(x_cbloc + margin_w + 20, y - 10);
            y = this->device.ly() + 20;
        }
        this->login_label.set_xy(x_cbloc, y);
        login_show->set_xy(x_cbloc + margin_w + 20, y);
        y = login_show->ly() + 20;
        this->password_label.set_xy(x_cbloc, y);
        this->password_edit.set_xy(x_cbloc + margin_w + 20, y);

        this->password_label.rect.y += (this->password_edit.cy() - this->password_label.cy()) / 2;
        this->login_label.rect.y += (login_show->cy() - this->login_label.cy()) / 2;
        this->device_label.rect.y += (device_show->cy() - this->login_label.cy()) / 2;
    }

    virtual ~FlatInteractiveTarget()
    {
        this->clear();
    }

    virtual int get_bg_color() const {
        return this->bgcolor;
    }

    virtual void notify(Widget2* widget, NotifyApi::notify_event_t event)
    {
        if ((widget == this->last_interactive)
             && event == NOTIFY_SUBMIT) {
            this->send_notify(NOTIFY_SUBMIT);
        }
        else if (event == NOTIFY_SUBMIT) {
            this->next_focus();
        }
    }

    virtual void rdp_input_scancode(long int param1, long int param2, long int param3, long int param4, Keymap2* keymap)
    {
        if (keymap->nb_kevent_available() > 0){
            switch (keymap->top_kevent()){
            case Keymap2::KEVENT_ESC:
                keymap->get_kevent();
                this->send_notify(NOTIFY_CANCEL);
                break;
            default:
                WidgetParent::rdp_input_scancode(param1, param2, param3, param4, keymap);
                break;
            }
        }
    }
};

#endif
