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

*/

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestRedTransportLibrary
#include <boost/test/auto_unit_test.hpp>

#define LOGPRINT
#include "log.hpp"

#include "../libs/rio.h"
#include "../libs/rio_impl.h"


BOOST_AUTO_TEST_CASE(TestOutSequenceTransport_OutfilenameSequence)
{
// 3rd simplest sequence is "intracker" sequence
// - Behavior is identical to infilename sequence except the input pattern is
// a Transport that contains the list of the input files.
// - sq_get_trans() open an infile if necessary using the name it got from tracker
//   and return it on subsequent calls until it is closed (reach EOF)
// - sq_next() close the current outfile and step to the next filename wich will 
//    be used by the next sq_get_trans to create an outfile transport.

    RIO_ERROR status = RIO_ERROR_OK;
    const char trackdata[] = 
        "800 600\n"
        "0\n"
        "\n"
        "./tests/fixtures/sample0.wrm 1352304810 1352304870\n"
        "./tests/fixtures/sample1.wrm 1352304870 1352304930\n"
        "./tests/fixtures/sample2.wrm 1352304930 1352304990\n"
        ;

    RIO * tracker = rio_new_generator(&status, trackdata, sizeof(trackdata)-1);

    status = RIO_ERROR_OK;
    SQ * sequence = sq_new_intracker(&status, tracker);

    status = RIO_ERROR_OK;
    RIO * rt = rio_new_insequence(&status, sequence);

    char buffer[1024] = {};
    unsigned len = 1471394 + 444578 + 290245;
    while (len > 1024){
        BOOST_CHECK_EQUAL(1024, rio_recv(rt, buffer, sizeof(buffer)));
        len -= 1024;
    }
    BOOST_CHECK_EQUAL(521, rio_recv(rt, buffer, sizeof(buffer)));
    BOOST_CHECK_EQUAL(0, rio_recv(rt, buffer, sizeof(buffer)));

    rio_delete(rt);
    sq_delete(sequence);
    rio_delete(tracker);
}


// 5th simplest sequence is "meta" sequence
// - Behavior is identical to intacker sequence except the input pattern is the name of the tracker file
// - sq_get_trans() open an infile if necessary using the name it got from tracker
//   and return it on subsequent calls until it is closed (reach EOF)
// - sq_next() close the current outfile and step to the next filename wich will 
//    be used by the next sq_get_trans to create an outfile transport.

BOOST_AUTO_TEST_CASE(TestSequenceMeta)
{
    RIO_ERROR status = RIO_ERROR_OK;
    SQ * sequence = sq_new_inmeta(&status, "./tests/fixtures/TESTOFS", ".mwrm");

    status = RIO_ERROR_OK;
    RIO * rt = rio_new_insequence(&status, sequence);

    char buffer[1024] = {};
    BOOST_CHECK_EQUAL(10, rio_recv(rt, buffer, 10));
    BOOST_CHECK_EQUAL(0, buffer[10]);
    if (0 != memcmp(buffer, "AAAAXBBBBX", 10)){
        LOG(LOG_ERR, "expected \"AAAAXBBBBX\" got \"%s\"\n", buffer);
    }
    BOOST_CHECK_EQUAL(5, rio_recv(rt, buffer + 10, 1024));
    BOOST_CHECK_EQUAL(0, memcmp(buffer, "AAAAXBBBBXCCCCX", 15));
    BOOST_CHECK_EQUAL(0, buffer[15]);
    BOOST_CHECK_EQUAL(0, rio_recv(rt, buffer + 15, 1024));
    rio_clear(rt);
    rio_delete(rt);
    sq_delete(sequence);
}

// Outmeta is a transport that manage file opening and chunking by itself
// We provide a base filename and it creates an outfilename sequence based on it
// A trace of this sequence is kept in an independant journal file that will
// be used later to reopen the same sequence as an input transport.
// chunking is performed externally, using the independant seq object created by constructor.
// metadata can be attached to individual chunks through seq object.

// The seq object memory allocation is performed by Outmeta,
// hence returned seq *must not* be explicitely deleted
// deleting transport will take care of it.

BOOST_AUTO_TEST_CASE(TestOutMeta)
{
    // cleanup of possible previous test files
    {
        const char * file[] = {"TESTOFS.mwrm", "TESTOFS-000000.wrm", "TESTOFS-000001.wrm"};
        for (size_t i = 0 ; i < sizeof(file)/sizeof(char*) ; ++i){
            ::unlink(file[i]);
        }
    }

    RIO_ERROR status = RIO_ERROR_OK;
    SQ * seq  = NULL;
    struct timeval tv;
    tv.tv_usec = 0;
    tv.tv_sec = 1352304810;
    RIO * rt = rio_new_outmeta(&status, &seq, "TESTOFS", ".mwrm", "800 600\n", "\n", "\n", &tv);

    BOOST_CHECK_EQUAL( 5, rio_send(rt, "AAAAX",  5));
    tv.tv_sec += 100;
    sq_timestamp(seq, &tv);
    BOOST_CHECK_EQUAL(RIO_ERROR_OK, sq_next(seq));
    BOOST_CHECK_EQUAL(10, rio_send(rt, "BBBBXCCCCX", 10));
    tv.tv_sec += 100;
    sq_timestamp(seq, &tv);
    BOOST_CHECK_EQUAL(RIO_ERROR_OK, sq_next(seq));

    rio_clear(rt);
    rio_delete(rt);

    {
        RIO_ERROR status = RIO_ERROR_OK;
        SQ * sequence = sq_new_inmeta(&status, "./tests/fixtures/TESTOFS", ".mwrm");

        status = RIO_ERROR_OK;
        RIO * rt = rio_new_insequence(&status, sequence);

        char buffer[1024] = {};
        BOOST_CHECK_EQUAL(10, rio_recv(rt, buffer, 10));
        BOOST_CHECK_EQUAL(0, buffer[10]);
        if (0 != memcmp(buffer, "AAAAXBBBBX", 10)){
            LOG(LOG_ERR, "expected \"AAAAXBBBBX\" got \"%s\"\n", buffer);
        }
        BOOST_CHECK_EQUAL(5, rio_recv(rt, buffer + 10, 1024));
        BOOST_CHECK_EQUAL(0, memcmp(buffer, "AAAAXBBBBXCCCCX", 15));
        BOOST_CHECK_EQUAL(0, buffer[15]);
        BOOST_CHECK_EQUAL(0, rio_recv(rt, buffer + 15, 1024));
        rio_clear(rt);
        rio_delete(rt);
        sq_delete(sequence);
    }

    const char * file[] = {
        "TESTOFS.mwrm",
        "TESTOFS-000000.wrm",
        "TESTOFS-000001.wrm"
    };
    for (size_t i = 0 ; i < sizeof(file)/sizeof(char*) ; ++i){
        if (::unlink(file[i]) < 0){
            BOOST_CHECK(false);
            LOG(LOG_ERR, "failed to unlink %s", file[i]);
        }
    }
}


