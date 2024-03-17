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

#include "common/log_wrapper.h"
#include "define.h"
#include "frame_builder.h"
#include "network.h"
#include "websocket_base.h"

namespace OHOS::ArkCompiler::Toolchain {
// if the data is too large, it will be split into multiple frames, the first frame will be marked as 0x0
// and the last frame will be marked as 0x1.
// we just add the 'isLast' parameter to indicate whether it is the last frame.
bool WebSocketBase::SendReply(const std::string& message, FrameType frameType, bool isLast) const
{
    if (socketState_ != SocketState::CONNECTED) {
        LOGE("SendReply failed, websocket not connected");
        return false;
    }

    auto frame = CreateFrame(isLast, frameType, message);
    if (!Send(connectionFd_, frame, 0)) {
        LOGE("SendReply: send failed");
        return false;
    }
    return true;
}

/**
  *  The wired format of this data transmission section is described in detail through ABNFRFC5234.
  *  When receive the message, we should decode it according the spec. The structure is as follows:
  *     0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
  *    +-+-+-+-+-------+-+-------------+-------------------------------+
  *    |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
  *    |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
  *    |N|V|V|V|       |S|             |   (if payload len==126/127)   |
  *    | |1|2|3|       |K|             |                               |
  *    +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
  *    |     Extended payload length continued, if payload len == 127  |
  *    + - - - - - - - - - - - - - - - +-------------------------------+
  *    |                               |Masking-key, if MASK set to 1  |
  *    +-------------------------------+-------------------------------+
  *    | Masking-key (continued)       |          Payload Data         |
  *    +-------------------------------- - - - - - - - - - - - - - - - +
  *    :                     Payload Data continued ...                :
  *    + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
  *    |                     Payload Data continued ...                |
  *    +---------------------------------------------------------------+
  */

bool WebSocketBase::ReadPayload(WebSocketFrame& wsFrame)
{
    if (wsFrame.payloadLen == WebSocketFrame::TWO_BYTES_LENTH_ENC) {
        uint8_t recvbuf[WebSocketFrame::TWO_BYTES_LENTH] = {0};
        if (!Recv(connectionFd_, recvbuf, WebSocketFrame::TWO_BYTES_LENTH, 0)) {
            LOGE("ReadPayload: Recv payloadLen == 126 failed");
            return false;
        }
        wsFrame.payloadLen = NetToHostLongLong(recvbuf, WebSocketFrame::TWO_BYTES_LENTH);
    } else if (wsFrame.payloadLen == WebSocketFrame::EIGHT_BYTES_LENTH_ENC) {
        uint8_t recvbuf[WebSocketFrame::EIGHT_BYTES_LENTH] = {0};
        if (!Recv(connectionFd_, recvbuf, WebSocketFrame::EIGHT_BYTES_LENTH, 0)) {
            LOGE("ReadPayload: Recv payloadLen == 127 failed");
            return false;
        }
        wsFrame.payloadLen = NetToHostLongLong(recvbuf, WebSocketFrame::EIGHT_BYTES_LENTH);
    }
    return DecodeMessage(wsFrame);
}

bool WebSocketBase::HandleDataFrame(WebSocketFrame& wsFrame)
{
    if (wsFrame.opcode == EnumToNumber(FrameType::TEXT)) {
        return ReadPayload(wsFrame);
    } else {
        LOGW("Received unsupported data frame, opcode = %{public}d", wsFrame.opcode);
    }
    return true;
}

bool WebSocketBase::HandleControlFrame(WebSocketFrame& wsFrame)
{
    if (wsFrame.opcode == EnumToNumber(FrameType::PING)) {
        // A Pong frame sent in response to a Ping frame must have identical
        // "Application data" as found in the message body of the Ping frame
        // being replied to.
        // https://www.rfc-editor.org/rfc/rfc6455#section-5.5.3
        if (!ReadPayload(wsFrame)) {
            LOGE("Failed to read ping frame payload");
            return false;
        }
        SendPongFrame(wsFrame.payload);
    } else if (wsFrame.opcode == EnumToNumber(FrameType::CLOSE)) {
        // might read payload to response by echoing the status code
        CloseConnection(CloseStatusCode::NO_STATUS_CODE, SocketState::INITED);
    }
    return true;
}

std::string WebSocketBase::Decode()
{
    if (socketState_ != SocketState::CONNECTED) {
        LOGE("Decode failed, websocket not connected!");
        return "";
    }

    uint8_t recvbuf[WebSocketFrame::HEADER_LEN] = {0};
    if (!Recv(connectionFd_, recvbuf, WebSocketFrame::HEADER_LEN, 0)) {
        LOGE("Decode failed, client websocket disconnect");
        CloseConnection(CloseStatusCode::UNEXPECTED_ERROR, SocketState::INITED);
        return std::string(DECODE_DISCONNECT_MSG);
    }
    WebSocketFrame wsFrame(recvbuf);
    if (!ValidateIncomingFrame(wsFrame)) {
        LOGE("Received websocket frame is invalid - header is %02x%02x", recvbuf[0], recvbuf[1]);
        CloseConnection(CloseStatusCode::PROTOCOL_ERROR, SocketState::INITED);
    }

    if (IsControlFrame(wsFrame.opcode)) {
        if (HandleControlFrame(wsFrame)) {
            return wsFrame.payload;
        }
    } else if (HandleDataFrame(wsFrame)) {
        return wsFrame.payload;
    }
    return std::string(DECODE_DISCONNECT_MSG);
}

bool WebSocketBase::IsConnected()
{
    return socketState_ == SocketState::CONNECTED;
}

void WebSocketBase::SetCloseConnectionCallback(CloseConnectionCallback cb)
{
    closeCb_ = std::move(cb);
}

void WebSocketBase::SetFailConnectionCallback(FailConnectionCallback cb)
{
    failCb_ = std::move(cb);
}

void WebSocketBase::CloseConnectionSocket(ConnectionCloseReason status, SocketState newSocketState)
{
#if defined(OHOS_PLATFORM)
    shutdown(connectionFd_, SHUT_RDWR);
#endif
    close(connectionFd_);
    connectionFd_ = -1;
    socketState_ = newSocketState;

    if (status == ConnectionCloseReason::FAIL) {
        if (failCb_) {
            failCb_();
        }
    } else if (status == ConnectionCloseReason::CLOSE) {
        if (closeCb_) {
            closeCb_();
        }
    }
}

void WebSocketBase::SendPongFrame(std::string payload)
{
    auto frame = CreateFrame(true, FrameType::PONG, std::move(payload));
    if (!Send(connectionFd_, frame, 0)) {
        LOGE("Decode: Send pong frame failed");
    }
}

static std::string ToString(CloseStatusCode status)
{
    if (status == CloseStatusCode::NO_STATUS_CODE) {
        return "";
    }
    std::string result;
    PushNumberPerByte(result, EnumToNumber(status));
    return result;
}

void WebSocketBase::SendCloseFrame(CloseStatusCode status)
{
    auto frame = CreateFrame(true, FrameType::CLOSE, ToString(status));
    if (!Send(connectionFd_, frame, 0)) {
        LOGE("SendCloseFrame: Send close frame failed");
    }
}

void WebSocketBase::CloseConnection(CloseStatusCode status, SocketState newSocketState)
{
    LOGI("Close connection, status = %{public}d", static_cast<int>(status));
    // can close connection right after sending back close frame.
    CloseConnectionSocket(ConnectionCloseReason::CLOSE, newSocketState);
}

/* static */
bool WebSocketBase::IsDecodeDisconnectMsg(const std::string& message)
{
    return message == DECODE_DISCONNECT_MSG;
}

#if !defined(OHOS_PLATFORM)
/* static */
bool WebSocketBase::SetWebSocketTimeOut(int32_t fd, uint32_t timeoutLimit)
{
    if (timeoutLimit > 0) {
        struct timeval timeout = {static_cast<time_t>(timeoutLimit), 0};
        if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO,
            reinterpret_cast<char *>(&timeout), sizeof(timeout)) != SOCKET_SUCCESS) {
            LOGE("SetWebSocketTimeOut setsockopt SO_SNDTIMEO failed, errno = %{public}d", errno);
            return false;
        }
        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
            reinterpret_cast<char *>(&timeout), sizeof(timeout)) != SOCKET_SUCCESS) {
            LOGE("SetWebSocketTimeOut setsockopt SO_RCVTIMEO failed, errno = %{public}d", errno);
            return false;
        }
    }
    return true;
}
#else
/* static */
bool WebSocketBase::SetWebSocketTimeOut(int32_t fd, uint32_t timeoutLimit)
{
    if (timeoutLimit > 0) {
        struct timeval timeout = {static_cast<time_t>(timeoutLimit), 0};
        if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) != SOCKET_SUCCESS) {
            LOGE("SetWebSocketTimeOut setsockopt SO_SNDTIMEO failed, errno = %{public}d", errno);
            return false;
        }
        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != SOCKET_SUCCESS) {
            LOGE("SetWebSocketTimeOut setsockopt SO_RCVTIMEO failed, errno = %{public}d", errno);
            return false;
        }
    }
    return true;
}
#endif
} // namespace OHOS::ArkCompiler::Toolchain
