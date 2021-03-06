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
    Copyright (C) Wallix 2013
    Author(s): Christophe Grosjean, Raphael Zhou, Meng Tan
*/

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestNtlmMessageAuthenticate
#include <boost/test/auto_unit_test.hpp>

#define LOGNULL

#include "RDP/nla/credssp.hpp"
#include "RDP/nla/ntlm/ntlm_message_authenticate.hpp"

#include "check_sig.hpp"

BOOST_AUTO_TEST_CASE(TestAuthenticate)
{
    // ===== NTLMSSP_AUTH =====
    uint8_t packet3[] = {
        0x30, 0x82, 0x02, 0x41, 0xa0, 0x03, 0x02, 0x01,
        0x02, 0xa1, 0x82, 0x01, 0x12, 0x30, 0x82, 0x01,
        0x0e, 0x30, 0x82, 0x01, 0x0a, 0xa0, 0x82, 0x01,
        0x06, 0x04, 0x82, 0x01, 0x02, 0x4e, 0x54, 0x4c,
        0x4d, 0x53, 0x53, 0x50, 0x00, 0x03, 0x00, 0x00,
        0x00, 0x18, 0x00, 0x18, 0x00, 0x6a, 0x00, 0x00,
        0x00, 0x70, 0x00, 0x70, 0x00, 0x82, 0x00, 0x00,
        0x00, 0x08, 0x00, 0x08, 0x00, 0x48, 0x00, 0x00,
        0x00, 0x10, 0x00, 0x10, 0x00, 0x50, 0x00, 0x00,
        0x00, 0x0a, 0x00, 0x0a, 0x00, 0x60, 0x00, 0x00,
        0x00, 0x10, 0x00, 0x10, 0x00, 0xf2, 0x00, 0x00,
        0x00, 0x35, 0x82, 0x88, 0xe2, 0x05, 0x01, 0x28,
        0x0a, 0x00, 0x00, 0x00, 0x0f, 0x77, 0x00, 0x69,
        0x00, 0x6e, 0x00, 0x37, 0x00, 0x75, 0x00, 0x73,
        0x00, 0x65, 0x00, 0x72, 0x00, 0x6e, 0x00, 0x61,
        0x00, 0x6d, 0x00, 0x65, 0x00, 0x57, 0x00, 0x49,
        0x00, 0x4e, 0x00, 0x58, 0x00, 0x50, 0x00, 0xa0,
        0x98, 0x01, 0x10, 0x19, 0xbb, 0x5d, 0x00, 0xf6,
        0xbe, 0x00, 0x33, 0x90, 0x20, 0x34, 0xb3, 0x47,
        0xa2, 0xe5, 0xcf, 0x27, 0xf7, 0x3c, 0x43, 0x01,
        0x4a, 0xd0, 0x8c, 0x24, 0xb4, 0x90, 0x74, 0x39,
        0x68, 0xe8, 0xbd, 0x0d, 0x2b, 0x70, 0x10, 0x01,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc3,
        0x83, 0xa2, 0x1c, 0x6c, 0xb0, 0xcb, 0x01, 0x47,
        0xa2, 0xe5, 0xcf, 0x27, 0xf7, 0x3c, 0x43, 0x00,
        0x00, 0x00, 0x00, 0x02, 0x00, 0x08, 0x00, 0x57,
        0x00, 0x49, 0x00, 0x4e, 0x00, 0x37, 0x00, 0x01,
        0x00, 0x08, 0x00, 0x57, 0x00, 0x49, 0x00, 0x4e,
        0x00, 0x37, 0x00, 0x04, 0x00, 0x08, 0x00, 0x77,
        0x00, 0x69, 0x00, 0x6e, 0x00, 0x37, 0x00, 0x03,
        0x00, 0x08, 0x00, 0x77, 0x00, 0x69, 0x00, 0x6e,
        0x00, 0x37, 0x00, 0x07, 0x00, 0x08, 0x00, 0xa9,
        0x8d, 0x9b, 0x1a, 0x6c, 0xb0, 0xcb, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb1,
        0xd2, 0x45, 0x42, 0x0f, 0x37, 0x9a, 0x0e, 0xe0,
        0xce, 0x77, 0x40, 0x10, 0x8a, 0xda, 0xba, 0xa3,
        0x82, 0x01, 0x22, 0x04, 0x82, 0x01, 0x1e, 0x01,
        0x00, 0x00, 0x00, 0x91, 0x5e, 0xb0, 0x6e, 0x72,
        0x82, 0x53, 0xae, 0x00, 0x00, 0x00, 0x00, 0x27,
        0x29, 0x73, 0xa9, 0xfa, 0x46, 0x17, 0x3c, 0x74,
        0x14, 0x45, 0x2a, 0xd1, 0xe2, 0x92, 0xa1, 0xc6,
        0x0a, 0x30, 0xd4, 0xcc, 0xe0, 0x92, 0xf6, 0xb3,
        0x20, 0xb3, 0xa0, 0xf1, 0x38, 0xb1, 0xf4, 0xe5,
        0x96, 0xdf, 0xa1, 0x65, 0x5b, 0xd6, 0x0c, 0x2a,
        0x86, 0x99, 0xcc, 0x72, 0x80, 0xbd, 0xe9, 0x19,
        0x1f, 0x42, 0x53, 0xf6, 0x84, 0xa3, 0xda, 0x0e,
        0xec, 0x10, 0x29, 0x15, 0x52, 0x5c, 0x77, 0x40,
        0xc8, 0x3d, 0x44, 0x01, 0x34, 0xb6, 0x0a, 0x75,
        0x33, 0xc0, 0x25, 0x71, 0xd3, 0x25, 0x38, 0x3b,
        0xfc, 0x3b, 0xa8, 0xcf, 0xba, 0x2b, 0xf6, 0x99,
        0x0e, 0x5f, 0x4e, 0xa9, 0x16, 0x2b, 0x52, 0x9f,
        0xbb, 0x76, 0xf8, 0x03, 0xfc, 0x11, 0x5e, 0x36,
        0x83, 0xd8, 0x4c, 0x9a, 0xdc, 0x9d, 0x35, 0xe2,
        0xc8, 0x63, 0xa9, 0x3d, 0x07, 0x97, 0x52, 0x64,
        0x54, 0x72, 0x9e, 0x9a, 0x8c, 0x56, 0x79, 0x4a,
        0x78, 0x91, 0x0a, 0x4c, 0x52, 0x84, 0x5a, 0x4a,
        0xb8, 0x28, 0x0b, 0x2f, 0xe6, 0x89, 0x7d, 0x07,
        0x3b, 0x7b, 0x6e, 0x22, 0xcc, 0x4c, 0xff, 0xf4,
        0x10, 0x96, 0xf2, 0x27, 0x29, 0xa0, 0x76, 0x0d,
        0x4c, 0x7e, 0x7a, 0x42, 0xe4, 0x1e, 0x6a, 0x95,
        0x7d, 0x4c, 0xaf, 0xdb, 0x86, 0x49, 0x5c, 0xbf,
        0xc2, 0x65, 0xb6, 0xf2, 0xed, 0xae, 0x8d, 0x57,
        0xed, 0xf0, 0xd4, 0xcb, 0x7a, 0xbb, 0x23, 0xde,
        0xe3, 0x43, 0xea, 0xb1, 0x02, 0xe3, 0xb4, 0x96,
        0xe9, 0xe7, 0x48, 0x69, 0xb0, 0xaa, 0xec, 0x89,
        0x38, 0x8b, 0xc2, 0xbd, 0xdd, 0xf7, 0xdf, 0xa1,
        0x37, 0xe7, 0x34, 0x72, 0x7f, 0x91, 0x10, 0x14,
        0x73, 0xfe, 0x32, 0xdc, 0xfe, 0x68, 0x2b, 0xc0,
        0x08, 0xdf, 0x05, 0xf7, 0xbd, 0x46, 0x33, 0xfb,
        0xc9, 0xfc, 0x89, 0xaa, 0x5d, 0x25, 0x49, 0xc8,
        0x6e, 0x86, 0xee, 0xc2, 0xce, 0xc4, 0x8e, 0x85,
        0x9f, 0xe8, 0x30, 0xb3, 0x86, 0x11, 0xd5, 0xb8,
        0x34, 0x4a, 0xe0, 0x03, 0xe5
    };

    LOG(LOG_INFO, "=================================\n");
    BStream s;
    s.init(sizeof(packet3));
    s.out_copy_bytes(packet3, sizeof(packet3));
    s.mark_end();
    s.rewind();

    uint8_t sig[20];
    get_sig(s, sig, sizeof(sig));

    TSRequest ts_req3(s);

    BOOST_CHECK_EQUAL(ts_req3.version, 2);

    BOOST_CHECK_EQUAL(ts_req3.negoTokens.size(), 0x102);
    BOOST_CHECK_EQUAL(ts_req3.authInfo.size(), 0);
    BOOST_CHECK_EQUAL(ts_req3.pubKeyAuth.size(), 0x11e);

    BStream to_send3;

    BOOST_CHECK_EQUAL(to_send3.size(), 0);
    ts_req3.emit(to_send3);

    BOOST_CHECK_EQUAL(to_send3.size(), 0x241 + 4);

    char message[1024];
    if (!check_sig(to_send3, message, (const char *)sig)){
        BOOST_CHECK_MESSAGE(false, message);
    }

    // hexdump_c(to_send3.get_data(), to_send3.size());

    NTLMAuthenticateMessage AuthMsg;
    // AuthMsg.recv(ts_req3.negoTokens);

    StaticStream token(ts_req3.negoTokens.get_data(), ts_req3.negoTokens.size());
    AuthMsg.recv(token);

    BOOST_CHECK_EQUAL(AuthMsg.negoFlags.flags, 0xE2888235);
    // AuthMsg.negoFlags.print();

    BOOST_CHECK_EQUAL(AuthMsg.LmChallengeResponse.len, 24);
    BOOST_CHECK_EQUAL(AuthMsg.LmChallengeResponse.bufferOffset, 106);
    BOOST_CHECK_EQUAL(AuthMsg.NtChallengeResponse.len, 112);
    BOOST_CHECK_EQUAL(AuthMsg.NtChallengeResponse.bufferOffset, 130);
    BOOST_CHECK_EQUAL(AuthMsg.DomainName.len, 8);
    BOOST_CHECK_EQUAL(AuthMsg.DomainName.bufferOffset, 72);
    BOOST_CHECK_EQUAL(AuthMsg.UserName.len, 16);
    BOOST_CHECK_EQUAL(AuthMsg.UserName.bufferOffset, 80);
    BOOST_CHECK_EQUAL(AuthMsg.Workstation.len, 10);
    BOOST_CHECK_EQUAL(AuthMsg.Workstation.bufferOffset, 96);
    BOOST_CHECK_EQUAL(AuthMsg.EncryptedRandomSessionKey.len, 16);

    // LmChallengeResponse
    LMv2_Response lmResponse;
    lmResponse.recv(AuthMsg.LmChallengeResponse.Buffer);

    // LOG(LOG_INFO, "Lm Response . Response ===========\n");
    // hexdump_c(lmResponse.Response, 16);
    uint8_t lm_response_match[] =
        "\xa0\x98\x01\x10\x19\xbb\x5d\x00\xf6\xbe\x00\x33\x90\x20\x34\xb3";
    BOOST_CHECK_EQUAL(memcmp(lm_response_match,
                             lmResponse.Response,
                             16),
                      0);

    // LOG(LOG_INFO, "Lm Response . ClientChallenge ===========\n");
    // hexdump_c(lmResponse.ClientChallenge, 8);
    uint8_t lm_clientchallenge_match[] =
        "\x47\xa2\xe5\xcf\x27\xf7\x3c\x43";
    BOOST_CHECK_EQUAL(memcmp(lm_clientchallenge_match,
                             lmResponse.ClientChallenge,
                             8),
                      0);



    // NtChallengeResponse
    NTLMv2_Response ntResponse;
    ntResponse.recv(AuthMsg.NtChallengeResponse.Buffer);

    // LOG(LOG_INFO, "Nt Response . Response ===========\n");
    // hexdump_c(ntResponse.Response, 16);
    uint8_t nt_response_match[] =
        "\x01\x4a\xd0\x8c\x24\xb4\x90\x74\x39\x68\xe8\xbd\x0d\x2b\x70\x10";
    BOOST_CHECK_EQUAL(memcmp(nt_response_match,
                             ntResponse.Response,
                             16),
                      0);

    BOOST_CHECK_EQUAL(ntResponse.Challenge.RespType, 1);
    BOOST_CHECK_EQUAL(ntResponse.Challenge.HiRespType, 1);
    // LOG(LOG_INFO, "Nt Response . Challenge . Timestamp ===========\n");
    // hexdump_c(ntResponse.Challenge.Timestamp, 8);
    uint8_t nt_timestamp_match[] =
        "\xc3\x83\xa2\x1c\x6c\xb0\xcb\x01";
    BOOST_CHECK_EQUAL(memcmp(nt_timestamp_match,
                             ntResponse.Challenge.Timestamp,
                             8),
                      0);

    // LOG(LOG_INFO, "Nt Response . Challenge . ClientChallenge ===========\n");
    // hexdump_c(ntResponse.Challenge.ClientChallenge, 8);
    uint8_t nt_clientchallenge_match[] =
        "\x47\xa2\xe5\xcf\x27\xf7\x3c\x43";
    BOOST_CHECK_EQUAL(memcmp(nt_clientchallenge_match,
                             ntResponse.Challenge.ClientChallenge,
                             8),
                      0);

    // LOG(LOG_INFO, "Nt Response . Challenge . AvPairList ===========\n");
    // ntResponse.Challenge.AvPairList.print();

    // Domain Name
    // LOG(LOG_INFO, "Domain Name ===========\n");
    // hexdump_c(AuthMsg.DomainName.Buffer.get_data(), AuthMsg.DomainName.Buffer.size());
    uint8_t domainname_match[] =
        "\x77\x00\x69\x00\x6e\x00\x37\x00";
    BOOST_CHECK_EQUAL(memcmp(domainname_match,
                             AuthMsg.DomainName.Buffer.get_data(),
                             AuthMsg.DomainName.len),
                      0);

    // User Name
    // LOG(LOG_INFO, "User Name ===========\n");
    // hexdump_c(AuthMsg.UserName.Buffer.get_data(), AuthMsg.UserName.Buffer.size());
    uint8_t username_match[] =
        "\x75\x00\x73\x00\x65\x00\x72\x00\x6e\x00\x61\x00\x6d\x00\x65\x00";
    BOOST_CHECK_EQUAL(memcmp(username_match,
                             AuthMsg.UserName.Buffer.get_data(),
                             AuthMsg.UserName.len),
                      0);

    // Work Station
    // LOG(LOG_INFO, "Work Station ===========\n");
    // hexdump_c(AuthMsg.Workstation.Buffer.get_data(), AuthMsg.Workstation.Buffer.size());
    uint8_t workstation_match[] =
        "\x57\x00\x49\x00\x4e\x00\x58\x00\x50\x00";
    BOOST_CHECK_EQUAL(memcmp(workstation_match,
                             AuthMsg.Workstation.Buffer.get_data(),
                             AuthMsg.Workstation.len),
                      0);

    // Encrypted Random Session Key
    // LOG(LOG_INFO, "Encrypted Random Session Key ===========\n");
    // hexdump_c(AuthMsg.EncryptedRandomSessionKey.Buffer.get_data(),
    //           AuthMsg.EncryptedRandomSessionKey.Buffer.size());
    uint8_t encryptedrandomsessionkey_match[] =
        "\xb1\xd2\x45\x42\x0f\x37\x9a\x0e\xe0\xce\x77\x40\x10\x8a\xda\xba";
    BOOST_CHECK_EQUAL(memcmp(encryptedrandomsessionkey_match,
                             AuthMsg.EncryptedRandomSessionKey.Buffer.get_data(),
                             AuthMsg.EncryptedRandomSessionKey.len),
                      0);

    BStream tosend;
    AuthMsg.emit(tosend);

    NTLMAuthenticateMessage AuthMsgDuplicate;

    tosend.mark_end();
    tosend.rewind();
    AuthMsgDuplicate.recv(tosend);

    BOOST_CHECK_EQUAL(AuthMsgDuplicate.negoFlags.flags, 0xE2888235);
    // AuthMsgDuplicate.negoFlags.print();

    BOOST_CHECK_EQUAL(AuthMsgDuplicate.LmChallengeResponse.len, 24);
    BOOST_CHECK_EQUAL(AuthMsgDuplicate.LmChallengeResponse.bufferOffset, 72);
    BOOST_CHECK_EQUAL(AuthMsgDuplicate.NtChallengeResponse.len, 112);
    BOOST_CHECK_EQUAL(AuthMsgDuplicate.NtChallengeResponse.bufferOffset, 96);
    BOOST_CHECK_EQUAL(AuthMsgDuplicate.DomainName.len, 8);
    BOOST_CHECK_EQUAL(AuthMsgDuplicate.DomainName.bufferOffset, 208);
    BOOST_CHECK_EQUAL(AuthMsgDuplicate.UserName.len, 16);
    BOOST_CHECK_EQUAL(AuthMsgDuplicate.UserName.bufferOffset, 216);
    BOOST_CHECK_EQUAL(AuthMsgDuplicate.Workstation.len, 10);
    BOOST_CHECK_EQUAL(AuthMsgDuplicate.Workstation.bufferOffset, 232);
    BOOST_CHECK_EQUAL(AuthMsgDuplicate.EncryptedRandomSessionKey.len, 16);

}
