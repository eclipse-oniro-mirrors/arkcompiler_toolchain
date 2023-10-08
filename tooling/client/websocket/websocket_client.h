/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_TOOLING_CLIENT_WEBSOCKET_CLIENT_H
#define ECMASCRIPT_TOOLING_CLIENT_WEBSOCKET_CLIENT_H

#include <atomic>
#include <iostream>
#include <memory>
#include <map>

#include "websocket.h"

namespace OHOS::ArkCompiler::Toolchain {
struct ToolchainWebSocketFrame {
    uint8_t fin = 0;
    uint8_t opcode = 0;
    uint8_t mask = 0;
    uint64_t payloadLen = 0;
    char maskingkey[5] = {0};
    std::unique_ptr<char []> payload = nullptr;
};
class WebsocketClient : public WebSocket {
public:
    enum ToolchainSocketState : uint8_t {
        UNINITED,
        INITED,
        CONNECTED,
    };
    WebsocketClient() = default;
    ~WebsocketClient() = default;
    bool InitToolchainWebSocketForPort(int port, uint32_t timeoutLimit = 5);
    bool InitToolchainWebSocketForSockName(const std::string &sockName, uint32_t timeoutLimit = 0);
    bool ClientSendWSUpgradeReq();
    bool ClientRecvWSUpgradeRsp();
    bool ClientSendReq(const std::string &message);
    std::string Decode();
    bool HandleFrame(ToolchainWebSocketFrame& wsFrame);
    bool DecodeMessage(ToolchainWebSocketFrame& wsFrame);
    uint64_t NetToHostLongLong(char* buf, uint32_t len);
    bool Recv(int32_t fd, char* buf, size_t totalLen, int32_t flags) const;
    bool Send(int32_t fd, const char* buf, size_t totalLen, int32_t flags) const;
    void Close();
    bool SetWebSocketTimeOut(int32_t fd, uint32_t timeoutLimit);
    bool IsConnected();

private:
    int32_t client_ {-1};
    std::atomic<ToolchainSocketState> socketState_ {ToolchainSocketState::UNINITED};
    static constexpr int32_t CLIENT_WEBSOCKET_UPGRADE_RSP_LEN = 129;
    static constexpr char CLIENT_WEBSOCKET_UPGRADE_REQ[] =  "GET / HTTP/1.1\r\n"
                                                                "Connection: Upgrade\r\n"
                                                                "Pragma: no-cache\r\n"
                                                                "Cache-Control: no-cache\r\n"
                                                                "Upgrade: websocket\r\n"
                                                                "Sec-WebSocket-Version: 13\r\n"
                                                                "Accept-Encoding: gzip, deflate, br\r\n"
                                                                "Sec-WebSocket-Key: 64b4B+s5JDlgkdg7NekJ+g==\r\n"
                                                                "Sec-WebSocket-Extensions: permessage-deflate\r\n";
    static constexpr int32_t SOCKET_SUCCESS = 0;
    static constexpr int NET_SUCCESS = 1;
    static constexpr int32_t SOCKET_MASK_LEN = 4;
    static constexpr int32_t SOCKET_HEADER_LEN = 2;
    static constexpr int32_t PAYLOAD_LEN = 2;
    static constexpr int32_t EXTEND_PAYLOAD_LEN = 8;
    static constexpr char MASK_KEY[SOCKET_MASK_LEN + 1] = "abcd";
};
} // namespace OHOS::ArkCompiler::Toolchain

#endif