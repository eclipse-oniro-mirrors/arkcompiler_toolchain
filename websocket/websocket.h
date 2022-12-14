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

namespace OHOS::ArkCompiler::Toolchain {
struct WebSocketFrame {
    uint8_t fin;
    uint8_t opcode;
    uint8_t mask;
    uint64_t payloadLen;
    char maskingkey[5];
    std::unique_ptr<char []> payload;
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
    bool InitTcpWebSocket();
    bool ConnectTcpWebSocket();
#else
    bool InitUnixWebSocket(const std::string& sockName);
    bool ConnectUnixWebSocket();
#endif
    bool IsConnected();

private:
    bool DecodeMessage(WebSocketFrame& wsFrame);
    bool HttpHandShake();
    bool HttpProtocolDecode(const std::string& request, HttpProtocol& req);
    bool HandleFrame(WebSocketFrame& wsFrame);
    bool ProtocolUpgrade(const HttpProtocol& req);

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
    static constexpr int32_t EXTEND_PATLOAD_LEN = 8;
    static constexpr char WEB_SOCKET_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
};
} // namespace OHOS::ArkCompiler::Toolchain

#endif // ARKCOMPILER_TOOLCHAIN_WEBSOCKET_WEBSOCKET_H