// Copyright 2019 The Beam Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "utility/io/reactor.h"
#include "utility/io/tcpstream.h"

#include <array>

namespace beam {
namespace io {

class ProxyConnector {
public:
    using OnConnect = Reactor::ConnectCallback;
    using OnResponse = std::function<bool(  ProxyConnector& instance,   // implicit this pass
                                            uint64_t tag,
                                            ErrorCode errorCode,
                                            void *data,
                                            size_t size)>;

    ProxyConnector(Reactor&);
    ~ProxyConnector(); // TODO: proxy

    OnConnect create_connection(uint64_t tag,   // unique identifier
                                Address destination,
                                const OnConnect& onProxyEstablish,
                                int timeoutMsec,
                                bool tlsConnect);

    void cancel_all() {}; // TODO: proxy
    void destroy_connect_timer_if_needed() {};
    void cancel_tcp_connect(uint64_t tag) {};

private:
    struct ProxyConnectRequest {
        ~ProxyConnectRequest();
        // using Ptr = std::unique_ptr<ProxyConnectRequest>;

        uint64_t tag;
        beam::io::Address destination;
        OnConnect on_connection_establish;
        OnResponse on_proxy_response;
        int timeoutMsec;
        bool tlsConnect;
        TcpStream::Ptr stream;
    };

    void release_connection(uint64_t tag, Result res);

    void on_tcp_connect(uint64_t tag, std::unique_ptr<TcpStream>&& new_stream, ErrorCode errorCode);
    void send_auth_methods(uint64_t tag);
    bool on_auth_method_resp(uint64_t tag, ErrorCode errorCode, void* data, size_t size);
    void send_connect_request(uint64_t tag);
    bool on_connect_resp(uint64_t tag, ErrorCode errorCode, void* data, size_t size);
    void on_connection_established(uint64_t tag);

    Reactor& _reactor;
    MemPool<ProxyConnectRequest, sizeof(ProxyConnectRequest)> _connectRequestsPool;
    std::unordered_map<uint64_t, ProxyConnectRequest*> _connectRequests;
};

struct Socks5_Protocol {

    static constexpr uint8_t ProtoVersion = 0x05;
    static constexpr uint8_t Reserved = 0x00;

    enum class AuthMethod : uint8_t {
        NO_AUTHENTICATION_REQUIRED = 0x00,
        GSSAPI = 0x01,
        USERNAME_PASSWORD = 0x02,
        // 0x03 to X'7F' IANA ASSIGNED
        // 0x80 to X'FE' RESERVED FOR PRIVATE METHODS
        NO_ACCEPTABLE_METHODS = 0xFF
    };

    enum class Command : uint8_t {
        CONNECT = 0x01,
        BIND = 0x02,
        UDP_ASSOCIATE = 0x03
    };

    enum class AddrType : uint8_t {
        IP_V4 = 0x01,
        DOMAINNAME = 0x03,
        IP_V6 = 0x04
    };

    enum class Reply : uint8_t {
        OK = 0x00,              /// succeeded
        SOCKS_FAIL = 0x01,      /// general SOCKS server failure
        NOT_ALLOWED = 0x02,     /// connection not allowed by ruleset
        NET_UNREACH = 0x03,     /// Network unreachable
        HOST_UNREACH = 0x04,    /// Host unreachable
        REFUSED = 0x05,         /// Connection refused
        TTL_EXP = 0x06,         /// TTL expired
        CMD_UNSUPP = 0x07,      /// Command not supported
        ADDR_UNSUPP = 0x08,     /// Address type not supported
        // X'09' to X'FF' unassigned
    };

    static std::array<uint8_t,3> makeAuthRequest(AuthMethod method);
    static bool parseAuthResp(void* data, AuthMethod& outMethod);
    static std::array<uint8_t,10> makeRequest(const Address& addr, Command cmd);
    static Reply parseReply(void* data);

};

}   // namespace beam
}   // namespace io