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

#ifndef ARKCOMPILER_TOOLCHAIN_WEBSOCKET_CLIENT_WEBSOCKET_CLIENT_H
#define ARKCOMPILER_TOOLCHAIN_WEBSOCKET_CLIENT_WEBSOCKET_CLIENT_H

#include "http.h"
#include "websocket_base.h"

#include <array>
#include <atomic>
#include <iostream>
#include <map>
#include <memory>

namespace OHOS::ArkCompiler::Toolchain {
class WebSocketClient final : public WebSocketBase {
public:
    ~WebSocketClient() noexcept override = default;

    bool DecodeMessage(WebSocketFrame& wsFrame) const override;
    void Close() override;

    bool InitToolchainWebSocketForPort(int port, uint32_t timeoutLimit = 5);
    bool InitToolchainWebSocketForSockName(const std::string &sockName, uint32_t timeoutLimit = 5);
    bool ClientSendWSUpgradeReq();
    bool ClientRecvWSUpgradeRsp();

    std::string GetSocketStateString();

private:
    static bool ValidateServerHandShake(HttpResponse& response);

    void CloseConnectionSocketOnFail();
    bool ValidateIncomingFrame(const WebSocketFrame& wsFrame) override;
    std::string CreateFrame(bool isLast, FrameType frameType) const override;
    std::string CreateFrame(bool isLast, FrameType frameType, const std::string& payload) const override;
    std::string CreateFrame(bool isLast, FrameType frameType, std::string&& payload) const override;

private:
    static constexpr std::array<std::string_view, 3> SOCKET_STATE_NAMES = {
        "uninited",
        "inited",
        "connected"
    };

    static constexpr std::string_view HTTP_SWITCHING_PROTOCOLS_STATUS_CODE = "101";
    static constexpr std::string_view HTTP_RESPONSE_REQUIRED_UPGRADE = "websocket";
    static constexpr std::string_view HTTP_RESPONSE_REQUIRED_CONNECTION = "upgrade";

    // may replace default websocket-key with randomly generated, as required by spec
    static constexpr unsigned char DEFAULT_WEB_SOCKET_KEY[] = "64b4B+s5JDlgkdg7NekJ+g==";
    static constexpr char CLIENT_WEBSOCKET_UPGRADE_REQ[] = "GET / HTTP/1.1\r\n"
                                                                "Connection: Upgrade\r\n"
                                                                "Pragma: no-cache\r\n"
                                                                "Cache-Control: no-cache\r\n"
                                                                "Upgrade: websocket\r\n"
                                                                "Sec-WebSocket-Version: 13\r\n"
                                                                "Accept-Encoding: gzip, deflate, br\r\n"
                                                                "Sec-WebSocket-Key: 64b4B+s5JDlgkdg7NekJ+g==\r\n"
                                                                "Sec-WebSocket-Extensions: permessage-deflate\r\n"
                                                                "\r\n";

    static constexpr int NET_SUCCESS = 1;
    static constexpr uint8_t MASK_KEY[] = {0xa, 0xb, 0xc, 0xd};
};
} // namespace OHOS::ArkCompiler::Toolchain

#endif // ARKCOMPILER_TOOLCHAIN_WEBSOCKET_CLIENT_WEBSOCKET_CLIENT_H
