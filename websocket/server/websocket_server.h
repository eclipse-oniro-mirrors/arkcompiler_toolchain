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

#ifndef ARKCOMPILER_TOOLCHAIN_WEBSOCKET_SERVER_WEBSOCKET_SERVER_H
#define ARKCOMPILER_TOOLCHAIN_WEBSOCKET_SERVER_WEBSOCKET_SERVER_H

#include "http.h"
#include "websocket_base.h"

#include <functional>

namespace OHOS::ArkCompiler::Toolchain {
class WebSocketServer final : public WebSocketBase {
public:
    using ValidateConnectionCallback = std::function<bool(const HttpRequest&)>;
    using OpenConnectionCallback = std::function<void()>;

public:
    ~WebSocketServer() noexcept override = default;

    bool AcceptNewConnection();

#if !defined(OHOS_PLATFORM)
    // Initialize server socket, transition to `INITED` state.
    bool InitTcpWebSocket(int port, uint32_t timeoutLimit = 0);
#else
    // Initialize server socket, transition to `INITED` state.
    bool InitUnixWebSocket(const std::string& sockName, uint32_t timeoutLimit = 0);
    bool InitUnixWebSocket(int socketfd);
    bool ConnectUnixWebSocketBySocketpair();
#endif

    void SetValidateConnectionCallback(ValidateConnectionCallback cb);
    void SetOpenConnectionCallback(OpenConnectionCallback cb);

    void Close() override;

private:
    static bool ValidateHandShakeMessage(const HttpRequest& req);

    bool ValidateIncomingFrame(const WebSocketFrame& wsFrame) override;
    std::string CreateFrame(bool isLast, FrameType frameType) const override;
    std::string CreateFrame(bool isLast, FrameType frameType, const std::string& payload) const override;
    std::string CreateFrame(bool isLast, FrameType frameType, std::string&& payload) const override;
    bool DecodeMessage(WebSocketFrame& wsFrame) const override;

    bool HttpHandShake();
    bool ProtocolUpgrade(const HttpRequest& req);
    bool ResponseInvalidHandShake() const;
    // Run `openCb_`, transition to `OnNewConnection` state.
    void OnNewConnection();
    // Close server socket, transition to `UNINITED` state.
    void CloseServerSocket();

private:
    int32_t serverFd_ {-1};

    // Callbacks used during different stages of connection lifecycle.
    // E.g. validation callback - it is executed during handshake
    // and used to indicate whether the incoming connection should be accepted.
    ValidateConnectionCallback validateCb_;
    OpenConnectionCallback openCb_;

    static constexpr std::string_view BAD_REQUEST_RESPONSE = "HTTP/1.1 400 Bad Request\r\n\r\n";
};
} // namespace OHOS::ArkCompiler::Toolchain

#endif // ARKCOMPILER_TOOLCHAIN_WEBSOCKET_SERVER_WEBSOCKET_SERVER_H
