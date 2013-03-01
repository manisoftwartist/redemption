/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARIO *ICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2013
   Author(s): Christophe Grosjean

   new Check RedTransport class

*/

#ifndef _REDEMPTION_LIBS_RIO_CHECK_H_
#define _REDEMPTION_LIBS_RIO_CHECK_H_

#include "rio_constants.h"

struct RIOCheck {
    size_t current;
    uint8_t * data;
    size_t len;
    bool status;
    RIO_ERROR err;
};

extern "C" {
    /* This method does not allocate space for object itself, 
        but initialize it's properties
        and allocate and initialize it's subfields if necessary
    */
    static inline RIO_ERROR rio_m_RIOCheck_constructor(RIOCheck * self, const void * data, size_t len)
    {
        self->data = (uint8_t *)malloc(len);
        if (!self->data) { return RIO_ERROR_MALLOC; }
        self->len = len;
        self->current = 0;
        self->status = true;
        self->err = RIO_ERROR_OK;
        memcpy(self->data, data, len);
        return RIO_ERROR_OK;
    }

    /* This method deallocate any space used for subfields if any
    */
    static inline RIO_ERROR rio_m_RIOCheck_destructor(RIOCheck * self)
    {
        free(self->data);
        return RIO_ERROR_OK;
    }

    /* This method close ressource without calling destructor
    */
    static inline void rio_m_RIOCheck_close(RIOCheck * self)
    {
    }

    /* This method receive len bytes of data into buffer
       target buffer *MUST* be large enough to contains len data
       returns len actually received (may be 0),
       or negative value to signal some error.
       If an error occurs after reading some data the amount read will be returned
       and an error returned on subsequent call.
    */
    static inline ssize_t rio_m_RIOCheck_recv(RIOCheck * self, void * data, size_t len)
    {
         self->status = false;
         self->err = RIO_ERROR_SEND_ONLY;
         return -self->err;
    }

    /* This method send len bytes of data from buffer to current transport
       buffer must actually contains the amount of data requested to send.
       returns len actually sent (may be 0),
       or negative value to signal some error.
       If an error occurs after sending some data the amount sent will be returned
       and an error returned on subsequent call.
    */
    static inline ssize_t rio_m_RIOCheck_send(RIOCheck * self, const void * data, size_t len)
    {
        if (!(self->status)) {
            return -self->err;
        }
        size_t available_len = (self->current + len > self->len)?(self->len - self->current):len;
        if (0 != memcmp(data, (const char *)(&self->data[self->current]), available_len)){
            // data differs
            self->status = false;
            self->err = RIO_ERROR_DATA_MISMATCH;
            // find where
            uint32_t differs = 0;
            for (size_t i = 0; i < available_len ; i++){
                if (((const char *)data)[i] != (((const char *)(self->data))[self->current+i])){
                    differs = i;
                    break;
                }
            }
            TODO("define and use C equivalent of log functions and hexdump functions")
            LOG(LOG_INFO, "=============== Common Part =======");
            hexdump_c(&(((const char *)data)[0]), differs);
            LOG(LOG_INFO, "=============== Expected ==========");
            hexdump(&((const char *)self->data)[self->current+differs], available_len - differs);
            LOG(LOG_INFO, "=============== Got ===============");
            hexdump_c(&(((const char *)data)[differs]), available_len - differs);
            return differs;
        }
        self->current += available_len;
        if (available_len != len){
            LOG(LOG_INFO, "Check transport out of reference data available=%u len=%u", available_len, len);
            LOG(LOG_INFO, "=============== Common Part =======");
            hexdump_c(&(((const char *)data)[0]), available_len);
            LOG(LOG_INFO, "=============== Got Unexpected Data ==========");
            hexdump_c(&(((const char *)data)[available_len]), len - available_len);
            self->status = false;
            self->err = RIO_ERROR_TRAILING_DATA;
        }
        return available_len;
    }

};

#endif

