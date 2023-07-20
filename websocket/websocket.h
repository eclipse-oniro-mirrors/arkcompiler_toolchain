/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ARKCOMPILER_TOOLCHAIN_WEBSOCKET_WEBSOCKET_H
#define ARKCOMPILER_TOOLCHAIN_WEBSOCKET_WEBSOCKET_H

#include <iostream>
#include <memory>
#include <atomic>

namespace panda::test {
class WebSocketTest;
}
namespace OHOS::ArkCompiler::Toolchain {
struct WebSocketFrame {
    uint8_t fin = 0;
    uint8_t opcode = 0;
    uint8_t mask = 0;
    uint64_t payloadLen = 0;
    char maskingkey[5] = {0};
    std::unique_ptr<char []> payload = nullptr;
};

struct HttpProtocol {
    std::string connection;
    std::string upgrade;
    std::string version;
    std::string secWebSocketKey;
};

class WebSocket {
public:
    enum SocketState : uint8_t {
        UNINITED,
        INITED,
        CONNECTED,
    };
    WebSocket() = default;
    ~WebSocket() = default;
    std::string Decode();
    void Close();
    void SendReply(const std::string& message) const;
#if !defined(OHOS_PLATFORM)
    bool InitTcpWebSocket(int port, uint32_t timeoutLimit = 0);
    bool ConnectTcpWebSocket();
#else
    bool InitUnixWebSocket(const std::string& sockName, uint32_t timeoutLimit = 0);
    bool ConnectUnixWebSocket();
#endif
    bool IsConnected();

private:
    friend class panda::test::WebSocketTest;

    bool DecodeMessage(WebSocketFrame& wsFrame);
    bool HttpHandShake();
    bool HttpProtocolDecode(const std::string& request, HttpProtocol& req);
    bool HandleFrame(WebSocketFrame& wsFrame);
    bool ProtocolUpgrade(const HttpProtocol& req);
    uint64_t NetToHostLongLong(char* buf, uint32_t len);
    bool SetWebSocketTimeOut(int32_t fd, uint32_t timeoutLimit);
    bool Recv(int32_t client, char* buf, size_t totalLen, int32_t flags) const;
    bool Send(int32_t client, const char* buf, size_t totalLen, int32_t flags) const;

    int32_t client_ {-1};
    int32_t fd_ {-1};
    std::atomic<SocketState> socketState_ {SocketState::UNINITED};
    static constexpr int32_t ENCODED_KEY_LEN = 128;
    static constexpr char EOL[] = "\r\n";
    static constexpr int32_t SOCKET_HANDSHAKE_LEN = 1024;
    static constexpr int32_t SOCKET_HEADER_LEN = 2;
    static constexpr int32_t SOCKET_MASK_LEN = 4;
    static constexpr int32_t SOCKET_SUCCESS = 0;
    static constexpr int32_t PAYLOAD_LEN = 2;
    static constexpr int32_t EXTEND_PAYLOAD_LEN = 8;
    static constexpr char WEB_SOCKET_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
};
} // namespace OHOS::ArkCompiler::Toolchain

#endif // ARKCOMPILER_TOOLCHAIN_WEBSOCKET_WEBSOCKET_H