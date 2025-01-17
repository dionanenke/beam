// Copyright 2018 The Beam Team
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

#include "http/http_connection.h"
#include "http/http_msg_creator.h"
#include "utility/io/tcpserver.h"
#include "utility/io/coarsetimer.h"
#include "utility/helpers.h"
#include <string_view>
#include <set>
#include "nlohmann/json.hpp"

#define ExplorerNodeDirs(macro) \
    macro(status) \
    macro(block) \
    macro(blocks) \
    macro(peers) \
    macro(swap_offers) \
    macro(swap_totals) \
    macro(contracts) \
    macro(contract) \
    macro(asset)

namespace beam { namespace explorer {

struct IAdapter;

class Server {
public:
    Server(IAdapter& adapter, io::Reactor& reactor, io::Address bindAddress, const std::string& keysFileName, const std::vector<uint32_t>& whitelist);

private:
    class IPAccessControl {
    public:
        explicit IPAccessControl(const std::string& ipsFileName);

        bool check(io::Address peerAddress);

        void refresh();
    private:
        bool _enabled;
        std::string _ipsFileName;
        time_t _lastModified;
        std::set<uint32_t> _ips;
    };

    void start_server();
    void refresh_acl();

    void on_stream_accepted(io::TcpStream::Ptr&& newStream, io::ErrorCode errorCode);

    bool on_request(uint64_t id, const HttpMsgReader::Message& msg);
    bool send(const HttpConnection::Ptr& conn, int code, const char* message, bool isHtml = false);

#define THE_MACRO(dir) nlohmann::json on_request_##dir(const HttpConnection::Ptr& conn);
    ExplorerNodeDirs(THE_MACRO)
#undef THE_MACRO

    HttpMsgCreator _msgCreator;
    IAdapter& _backend;
    io::Reactor& _reactor;
    io::MultipleTimers _timers;
    io::Address _bindAddress;
    io::TcpServer::Ptr _server;
    std::map<uint64_t, HttpConnection::Ptr> _connections;
    HttpUrl _currentUrl;
    io::SerializedMsg _headers;
    io::SerializedMsg _body;
    //AccessControl _acl;
    IPAccessControl _acl;
    std::vector<uint32_t> _whitelist;
    std::map<std::string_view, int> m_Dirs;
};

}} //namespaces
