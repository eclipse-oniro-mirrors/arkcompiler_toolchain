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

#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <unistd.h>

#include "common/log_wrapper.h"
#include "frame_builder.h"
#include "handshake_helper.h"
#include "network.h"
#include "string_utils.h"
#include "client/websocket_client.h"

namespace OHOS::ArkCompiler::Toolchain {
bool WebSocketClient::InitToolchainWebSocketForPort(int port, uint32_t timeoutLimit)
{
    if (socketState_ != SocketState::UNINITED) {
        LOGE("InitToolchainWebSocketForPort::client has inited.");
        return true;
    }

    connectionFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (connectionFd_ < SOCKET_SUCCESS) {
        LOGE("InitToolchainWebSocketForPort::client socket failed, error = %{public}d , desc = %{public}s",
            errno, strerror(errno));
        return false;
    }

    // set send and recv timeout limit
    if (!SetWebSocketTimeOut(connectionFd_, timeoutLimit)) {
        LOGE("InitToolchainWebSocketForPort::client SetWebSocketTimeOut failed, error = %{public}d , desc = %{public}s",
            errno, strerror(errno));
        CloseConnectionSocketOnFail();
        return false;
    }

    sockaddr_in clientAddr;
    if (memset_s(&clientAddr, sizeof(clientAddr), 0, sizeof(clientAddr)) != EOK) {
        LOGE("InitToolchainWebSocketForPort::client memset_s clientAddr failed, error = %{public}d, desc = %{public}s",
            errno, strerror(errno));
        CloseConnectionSocketOnFail();
        return false;
    }
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(port);
    int ret = inet_pton(AF_INET, "127.0.0.1", &clientAddr.sin_addr);
    if (ret != NET_SUCCESS) {
        LOGE("InitToolchainWebSocketForPort::client inet_pton failed, error = %{public}d, desc = %{public}s",
            errno, strerror(errno));
        CloseConnectionSocketOnFail();
        return false;
    }

    ret = connect(connectionFd_, reinterpret_cast<struct sockaddr*>(&clientAddr), sizeof(clientAddr));
    if (ret != SOCKET_SUCCESS) {
        LOGE("InitToolchainWebSocketForPort::client connect failed, error = %{public}d, desc = %{public}s",
            errno, strerror(errno));
        CloseConnectionSocketOnFail();
        return false;
    }
    socketState_ = SocketState::INITED;
    LOGI("InitToolchainWebSocketForPort::client connect success.");
    return true;
}

bool WebSocketClient::InitToolchainWebSocketForSockName(const std::string &sockName, uint32_t timeoutLimit)
{
    if (socketState_ != SocketState::UNINITED) {
        LOGE("InitToolchainWebSocketForSockName::client has inited.");
        return true;
    }

    connectionFd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (connectionFd_ < SOCKET_SUCCESS) {
        LOGE("InitToolchainWebSocketForSockName::client socket failed, error = %{public}d , desc = %{public}s",
            errno, strerror(errno));
        return false;
    }

    // set send and recv timeout limit
    if (!SetWebSocketTimeOut(connectionFd_, timeoutLimit)) {
        LOGE("InitToolchainWebSocketForSockName::client SetWebSocketTimeOut failed, "
            "error = %{public}d, desc = %{public}s",
            errno, strerror(errno));
        CloseConnectionSocketOnFail();
        return false;
    }

    struct sockaddr_un serverAddr;
    if (memset_s(&serverAddr, sizeof(serverAddr), 0, sizeof(serverAddr)) != EOK) {
        LOGE("InitToolchainWebSocketForSockName::client memset_s clientAddr failed, "
            "error = %{public}d, desc = %{public}s",
            errno, strerror(errno));
        CloseConnectionSocketOnFail();
        return false;
    }
    serverAddr.sun_family = AF_UNIX;
    if (strcpy_s(serverAddr.sun_path + 1, sizeof(serverAddr.sun_path) - 1, sockName.c_str()) != EOK) {
        LOGE("InitToolchainWebSocketForSockName::client strcpy_s serverAddr.sun_path failed, "
            "error = %{public}d, desc = %{public}s",
            errno, strerror(errno));
        CloseConnectionSocketOnFail();
        return false;
    }
    serverAddr.sun_path[0] = '\0';

    uint32_t len = offsetof(struct sockaddr_un, sun_path) + strlen(sockName.c_str()) + 1;
    int ret = connect(connectionFd_, reinterpret_cast<struct sockaddr*>(&serverAddr), static_cast<int32_t>(len));
    if (ret != SOCKET_SUCCESS) {
        LOGE("InitToolchainWebSocketForSockName::client connect failed, error = %{public}d, desc = %{public}s",
            errno, strerror(errno));
        CloseConnectionSocketOnFail();
        return false;
    }
    socketState_ = SocketState::INITED;
    LOGI("InitToolchainWebSocketForSockName::client connect success.");
    return true;
}

bool WebSocketClient::ClientSendWSUpgradeReq()
{
    if (socketState_ == SocketState::UNINITED) {
        LOGE("ClientSendWSUpgradeReq::client has not inited.");
        return false;
    }
    if (socketState_ == SocketState::CONNECTED) {
        LOGE("ClientSendWSUpgradeReq::client has connected.");
        return true;
    }

    // length without null-terminator
    if (!Send(connectionFd_, CLIENT_WEBSOCKET_UPGRADE_REQ, sizeof(CLIENT_WEBSOCKET_UPGRADE_REQ) - 1, 0)) {
        LOGE("ClientSendWSUpgradeReq::client send wsupgrade req failed, error = %{public}d, desc = %{public}sn",
            errno, strerror(errno));
        CloseConnectionSocketOnFail();
        return false;
    }
    LOGI("ClientSendWSUpgradeReq::client send wsupgrade req success.");
    return true;
}

bool WebSocketClient::ClientRecvWSUpgradeRsp()
{
    if (socketState_ == SocketState::UNINITED) {
        LOGE("ClientRecvWSUpgradeRsp::client has not inited.");
        return false;
    }
    if (socketState_ == SocketState::CONNECTED) {
        LOGE("ClientRecvWSUpgradeRsp::client has connected.");
        return true;
    }

    std::string msgBuf(HTTP_HANDSHAKE_MAX_LEN, 0);
    ssize_t msgLen = 0;
    while ((msgLen = recv(connectionFd_, msgBuf.data(), HTTP_HANDSHAKE_MAX_LEN, 0)) < 0 &&
           (errno == EINTR || errno == EAGAIN)) {
        LOGW("ClientRecvWSUpgradeRsp::client recv wsupgrade rsp failed, errno = %{public}d", errno);
    }
    if (msgLen <= 0) {
        LOGE("ClientRecvWSUpgradeRsp::client recv wsupgrade rsp failed, error = %{public}d, desc = %{public}sn",
            errno, strerror(errno));
        CloseConnectionSocketOnFail();
        return false;
    }
    // reduce to received size
    msgBuf.resize(msgLen);

    HttpResponse response;
    if (!HttpResponse::Decode(msgBuf, response) || !ValidateServerHandShake(response)) {
        LOGE("ClientRecvWSUpgradeRsp::client server handshake response is invalid");
        CloseConnectionSocketOnFail();
        return false;
    }

    socketState_ = SocketState::CONNECTED;
    LOGI("ClientRecvWSUpgradeRsp::client recv wsupgrade rsp success.");
    return true;
}

std::string WebSocketClient::GetSocketStateString()
{
    return std::string(SOCKET_STATE_NAMES[EnumToNumber(socketState_.load())]);
}

/* static */
bool WebSocketClient::ValidateServerHandShake(HttpResponse& response)
{
    // in accordance to https://www.rfc-editor.org/rfc/rfc6455#section-4.1
    if (response.status != HTTP_SWITCHING_PROTOCOLS_STATUS_CODE) {
        return false;
    }
    ToLowerCase(response.upgrade);
    if (response.upgrade != HTTP_RESPONSE_REQUIRED_UPGRADE) {
        return false;
    }
    ToLowerCase(response.connection);
    if (response.connection != HTTP_RESPONSE_REQUIRED_CONNECTION) {
        return false;
    }

    // The same WebSocket-Key is used for all connections
    // - must either use a randomly-selected, as required by spec or do this calculation statically.
    unsigned char expectedSecWebSocketAccept_[WebSocketKeyEncoder::ENCODED_KEY_LEN + 1];
    if (!WebSocketKeyEncoder::EncodeKey(DEFAULT_WEB_SOCKET_KEY, expectedSecWebSocketAccept_)) {
        LOGE("ValidateServerHandShake::client failed to generate expected Sec-WebSocket-Accept token");
        return false;
    }

    Trim(response.secWebSocketAccept);
    if (response.secWebSocketAccept.size() != WebSocketKeyEncoder::ENCODED_KEY_LEN ||
        response.secWebSocketAccept.compare(reinterpret_cast<const char *>(expectedSecWebSocketAccept_)) != 0) {
        return false;
    }

    // may support two remaining checks
    return true;
}

bool WebSocketClient::DecodeMessage(WebSocketFrame& wsFrame) const
{
    uint64_t msgLen = wsFrame.payloadLen;
    if (msgLen == 0) {
        // receiving empty data is OK
        return true;
    }
    auto& buffer = wsFrame.payload;
    buffer.resize(msgLen, 0);

    if (!Recv(connectionFd_, buffer, 0)) {
        LOGE("DecodeMessage: Recv message without mask failed");
        return false;
    }

    return true;
}

void WebSocketClient::Close()
{
    if (socketState_ == SocketState::CONNECTED) {
        CloseConnection(CloseStatusCode::SERVER_GO_AWAY, SocketState::UNINITED);
    }
}

void WebSocketClient::CloseConnectionSocketOnFail()
{
    CloseConnectionSocket(ConnectionCloseReason::FAIL, SocketState::UNINITED);
}

bool WebSocketClient::ValidateIncomingFrame(const WebSocketFrame& wsFrame)
{
    // "A server MUST NOT mask any frames that it sends to the client."
    // https://www.rfc-editor.org/rfc/rfc6455#section-5.1
    return wsFrame.mask == 0;
}

std::string WebSocketClient::CreateFrame(bool isLast, FrameType frameType) const
{
    ClientFrameBuilder builder(isLast, frameType, MASK_KEY);
    return builder.Build();
}

std::string WebSocketClient::CreateFrame(bool isLast, FrameType frameType, const std::string& payload) const
{
    ClientFrameBuilder builder(isLast, frameType, MASK_KEY);
    return builder.SetPayload(payload).Build();
}

std::string WebSocketClient::CreateFrame(bool isLast, FrameType frameType, std::string&& payload) const
{
    ClientFrameBuilder builder(isLast, frameType, MASK_KEY);
    return builder.SetPayload(std::move(payload)).Build();
}
}  // namespace OHOS::ArkCompiler::Toolchain
