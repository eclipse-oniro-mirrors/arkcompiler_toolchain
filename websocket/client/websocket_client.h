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

#include "websocket_base.h"
#include "http.h"
#include <array>
#include <atomic>
#include <iostream>
#include <map>
#include <memory>

namespace OHOS::ArkCompiler::Toolchain {
class WebSocketClient final : public WebSocketBase {
public:
    ~WebSocketClient() noexcept override = default;

    void Close();

    bool InitToolchainWebSocketForPort(int port, uint32_t timeoutLimit = 5);
    bool InitToolchainWebSocketForSockName(const std::string &sockName, uint32_t timeoutLimit = 5);
    bool ClientSendWSUpgradeReq();
    bool ClientRecvWSUpgradeRsp();

    std::string GetSocketStateString();

private:
    bool DecodeMessage(WebSocketFrame& wsFrame) const override;

    void CloseOnInitFailure();
    bool ValidateIncomingFrame(const WebSocketFrame& wsFrame) const override;
    std::string CreateFrame(bool isLast, FrameType frameType) const override;
    std::string CreateFrame(bool isLast, FrameType frameType, const std::string& payload) const override;
    std::string CreateFrame(bool isLast, FrameType frameType, std::string&& payload) const override;
    bool ValidateServerHandShake(HttpResponse& response);

private:
    static constexpr std::array<std::string_view, 4> SOCKET_STATE_NAMES = {
        "connecting",
        "open",
        "closing",
        "closed"
    };

    static constexpr char CLIENT_WS_UPGRADE_REQ_BEFORE_KEY[] = "GET / HTTP/1.1\r\n"
                                                               "Connection: Upgrade\r\n"
                                                               "Pragma: no-cache\r\n"
                                                               "Cache-Control: no-cache\r\n"
                                                               "Upgrade: websocket\r\n"
                                                               "Sec-WebSocket-Version: 13\r\n"
                                                               "Accept-Encoding: gzip, deflate, br\r\n"
                                                               "Sec-WebSocket-Key: ";

    static constexpr char CLIENT_WS_UPGRADE_REQ_AFTER_KEY[] = "\r\nSec-WebSocket-Extensions: permessage-deflate\r\n"
                                                              "\r\n";

    static constexpr int NET_SUCCESS = 1;
    static constexpr uint8_t MASK_KEY[] = {0xa, 0xb, 0xc, 0xd};
    std::string secWebSocketKey_;
};
} // namespace OHOS::ArkCompiler::Toolchain

#endif // ARKCOMPILER_TOOLCHAIN_WEBSOCKET_CLIENT_WEBSOCKET_CLIENT_H
