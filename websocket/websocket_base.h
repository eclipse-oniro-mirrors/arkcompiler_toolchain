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

#ifndef ARKCOMPILER_TOOLCHAIN_WEBSOCKET_WEBSOCKET_BASE_H
#define ARKCOMPILER_TOOLCHAIN_WEBSOCKET_WEBSOCKET_BASE_H

#include "web_socket_frame.h"

#include <atomic>
#include <functional>
#include <type_traits>

namespace OHOS::ArkCompiler::Toolchain {
enum CloseStatusCode : uint16_t {
    NO_STATUS_CODE = 0,
    NORMAL = 1000,
    SERVER_GO_AWAY = 1001,
    PROTOCOL_ERROR = 1002,
    UNACCEPTABLE_DATA = 1003,
    INCONSISTENT_DATA = 1007,
    POLICY_VIOLATION = 1008,
    MESSAGE_TOO_BIG = 1009,
    UNEXPECTED_ERROR = 1011,
};

class WebSocketBase {
public:
    using CloseConnectionCallback = std::function<void()>;
    using FailConnectionCallback = std::function<void()>;

public:
    static bool IsDecodeDisconnectMsg(const std::string& message);

    WebSocketBase() = default;
    virtual ~WebSocketBase() noexcept = default;

    // Receive and decode a message.
    // In case of control frames this method handles it accordingly and returns an empty string,
    // otherwise returns the decoded received message.
    std::string Decode();
    // Send message on current connection.
    // Returns success status.
    bool SendReply(const std::string& message, FrameType frameType = FrameType::TEXT, bool isLast = true) const;

    bool IsConnected();

    void SetCloseConnectionCallback(CloseConnectionCallback cb);
    void SetFailConnectionCallback(FailConnectionCallback cb);

    // Close current websocket endpoint and connections (if any).
    virtual void Close() = 0;

protected:
    enum class SocketState : uint8_t {
        UNINITED,
        INITED,
        CONNECTED,
    };

    enum class ConnectionCloseReason: uint8_t {
        FAIL,
        CLOSE,
    };

protected:
    static bool SetWebSocketTimeOut(int32_t fd, uint32_t timeoutLimit);

    bool ReadPayload(WebSocketFrame& wsFrame);
    void SendPongFrame(std::string payload);
    void SendCloseFrame(CloseStatusCode status);
    // Sending close frame and close connection.
    void CloseConnection(CloseStatusCode status, SocketState newSocketState);
    // Close connection socket.
    void CloseConnectionSocket(ConnectionCloseReason status, SocketState newSocketState);

    virtual bool HandleDataFrame(WebSocketFrame& wsFrame);
    virtual bool HandleControlFrame(WebSocketFrame& wsFrame);

    virtual bool ValidateIncomingFrame(const WebSocketFrame& wsFrame) = 0;
    virtual std::string CreateFrame(bool isLast, FrameType frameType) const = 0;
    virtual std::string CreateFrame(bool isLast, FrameType frameType, const std::string& payload) const = 0;
    virtual std::string CreateFrame(bool isLast, FrameType frameType, std::string&& payload) const = 0;
    virtual bool DecodeMessage(WebSocketFrame& wsFrame) const = 0;

protected:
    std::atomic<SocketState> socketState_ {SocketState::UNINITED};

    int connectionFd_ {-1};

    // Callbacks used during different stages of connection lifecycle.
    CloseConnectionCallback closeCb_;
    FailConnectionCallback failCb_;

    static constexpr char WEB_SOCKET_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    static constexpr size_t HTTP_HANDSHAKE_MAX_LEN = 1024;
    static constexpr int SOCKET_SUCCESS = 0;
    static constexpr std::string_view DECODE_DISCONNECT_MSG = "disconnect";
};
} // namespace OHOS::ArkCompiler::Toolchain

#endif // ARKCOMPILER_TOOLCHAIN_WEBSOCKET_WEBSOCKET_BASE_H
