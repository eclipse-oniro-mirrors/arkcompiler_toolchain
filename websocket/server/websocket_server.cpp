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

#include <fcntl.h>
#include "common/log_wrapper.h"
#include "frame_builder.h"
#include "handshake_helper.h"
#include "network.h"
#include "server/websocket_server.h"

namespace OHOS::ArkCompiler::Toolchain {
bool WebSocketServer::DecodeMessage(WebSocketFrame& wsFrame) const
{
    uint64_t msgLen = wsFrame.payloadLen;
    if (msgLen == 0) {
        // receiving empty data is OK
        return true;
    }
    auto& buffer = wsFrame.payload;
    buffer.resize(msgLen, 0);

    if (!Recv(connectionFd_, wsFrame.maskingKey, sizeof(wsFrame.maskingKey), 0)) {
        LOGE("DecodeMessage: Recv maskingKey failed");
        return false;
    }

    if (!Recv(connectionFd_, buffer, 0)) {
        LOGE("DecodeMessage: Recv message with mask failed");
        return false;
    }

    for (uint64_t i = 0; i < msgLen; i++) {
        auto j = i % WebSocketFrame::MASK_LEN;
        buffer[i] = static_cast<uint8_t>(buffer[i]) ^ wsFrame.maskingKey[j];
    }

    return true;
}

bool WebSocketServer::ProtocolUpgrade(const HttpRequest& req)
{
    unsigned char encodedKey[WebSocketKeyEncoder::ENCODED_KEY_LEN + 1];
    if (!WebSocketKeyEncoder::EncodeKey(req.secWebSocketKey, encodedKey)) {
        LOGE("ProtocolUpgrade: failed to encode WebSocket-Key");
        return false;
    }

    ProtocolUpgradeBuilder requestBuilder(encodedKey);
    if (!Send(connectionFd_, requestBuilder.GetUpgradeMessage(), requestBuilder.GetLength(), 0)) {
        LOGE("ProtocolUpgrade: Send failed");
        return false;
    }
    return true;
}

bool WebSocketServer::ResponseInvalidHandShake() const
{
    std::string response(BAD_REQUEST_RESPONSE);
    return Send(connectionFd_, response, 0);
}

bool WebSocketServer::HttpHandShake()
{
    std::string msgBuf(HTTP_HANDSHAKE_MAX_LEN, 0);
    ssize_t msgLen = 0;
    while ((msgLen = recv(connectionFd_, msgBuf.data(), HTTP_HANDSHAKE_MAX_LEN, 0)) < 0 &&
           (errno == EINTR || errno == EAGAIN)) {
        LOGW("HttpHandShake recv failed, errno = %{public}d", errno);
    }
    if (msgLen <= 0) {
        LOGE("ReadMsg failed, msgLen = %{public}ld, errno = %{public}d", static_cast<long>(msgLen), errno);
        return false;
    }
    // reduce to received size
    msgBuf.resize(msgLen);

    HttpRequest req;
    if (!HttpRequest::Decode(msgBuf, req)) {
        LOGE("HttpHandShake: Upgrade failed");
        return false;
    }
    if (validateCb_ && !validateCb_(req)) {
        LOGE("HttpHandShake: Validation failed");
        return false;
    }

    if (ValidateHandShakeMessage(req)) {
        return ProtocolUpgrade(req);
    }

    LOGE("HttpHandShake: HTTP upgrade parameters failure");
    if (!ResponseInvalidHandShake()) {
        LOGE("HttpHandShake: failed to send 'bad request' response");
    }
    return false;
}

/* static */
bool WebSocketServer::ValidateHandShakeMessage(const HttpRequest& req)
{
    return req.connection.find("Upgrade") != std::string::npos &&
        req.upgrade.find("websocket") != std::string::npos &&
        req.version.compare("HTTP/1.1") == 0;
}

bool WebSocketServer::AcceptNewConnection()
{
    if (socketState_ == SocketState::UNINITED) {
        LOGE("AcceptNewConnection failed, websocket not inited");
        return false;
    }
    if (socketState_ == SocketState::CONNECTED) {
        LOGI("AcceptNewConnection websocket has connected");
        return true;
    }

    if ((connectionFd_ = accept(serverFd_, nullptr, nullptr)) < SOCKET_SUCCESS) {
        LOGI("AcceptNewConnection accept has exited");
        return false;
    }

    if (!HttpHandShake()) {
        LOGE("AcceptNewConnection HttpHandShake failed");
        CloseConnectionSocket(ConnectionCloseReason::FAIL, SocketState::INITED);
        return false;
    }
    OnNewConnection();
    return true;
}

#if !defined(OHOS_PLATFORM)
bool WebSocketServer::InitTcpWebSocket(int port, uint32_t timeoutLimit)
{
    if (port < 0) {
        LOGE("InitTcpWebSocket invalid port");
        return false;
    }
    if (socketState_ != SocketState::UNINITED) {
        LOGI("InitTcpWebSocket websocket has inited");
        return true;
    }
#if defined(WINDOWS_PLATFORM)
    WORD sockVersion = MAKEWORD(2, 2); // 2: version 2.2
    WSADATA wsaData;
    if (WSAStartup(sockVersion, &wsaData) != 0) {
        LOGE("InitTcpWebSocket WSA init failed");
        return false;
    }
#endif
    serverFd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverFd_ < SOCKET_SUCCESS) {
        LOGE("InitTcpWebSocket socket init failed, errno = %{public}d", errno);
        return false;
    }
    // allow specified port can be used at once and not wait TIME_WAIT status ending
    int sockOptVal = 1;
    if ((setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR,
        reinterpret_cast<char *>(&sockOptVal), sizeof(sockOptVal))) != SOCKET_SUCCESS) {
        LOGE("InitTcpWebSocket setsockopt SO_REUSEADDR failed, errno = %{public}d", errno);
        CloseServerSocket();
        return false;
    }
    if (!SetWebSocketTimeOut(serverFd_, timeoutLimit)) {
        LOGE("InitTcpWebSocket SetWebSocketTimeOut failed");
        CloseServerSocket();
        return false;
    }
    sockaddr_in addrSin = {};
    addrSin.sin_family = AF_INET;
    addrSin.sin_port = htons(port);
    addrSin.sin_addr.s_addr = INADDR_ANY;
    if (bind(serverFd_, reinterpret_cast<struct sockaddr*>(&addrSin), sizeof(addrSin)) != SOCKET_SUCCESS ||
        listen(serverFd_, 1) != SOCKET_SUCCESS) {
        LOGE("InitTcpWebSocket bind/listen failed, errno = %{public}d", errno);
        CloseServerSocket();
        return false;
    }
    socketState_ = SocketState::INITED;
    return true;
}
#else
bool WebSocketServer::InitUnixWebSocket(const std::string& sockName, uint32_t timeoutLimit)
{
    if (socketState_ != SocketState::UNINITED) {
        LOGI("InitUnixWebSocket websocket has inited");
        return true;
    }
    serverFd_ = socket(AF_UNIX, SOCK_STREAM, 0); // 0: default protocol
    if (serverFd_ < SOCKET_SUCCESS) {
        LOGE("InitUnixWebSocket socket init failed, errno = %{public}d", errno);
        return false;
    }
    // set send and recv timeout
    if (!SetWebSocketTimeOut(serverFd_, timeoutLimit)) {
        LOGE("InitUnixWebSocket SetWebSocketTimeOut failed");
        CloseServerSocket();
        return false;
    }

    struct sockaddr_un un;
    if (memset_s(&un, sizeof(un), 0, sizeof(un)) != EOK) {
        LOGE("InitUnixWebSocket memset_s failed");
        CloseServerSocket();
        return false;
    }
    un.sun_family = AF_UNIX;
    if (strcpy_s(un.sun_path + 1, sizeof(un.sun_path) - 1, sockName.c_str()) != EOK) {
        LOGE("InitUnixWebSocket strcpy_s failed");
        CloseServerSocket();
        return false;
    }
    un.sun_path[0] = '\0';
    uint32_t len = offsetof(struct sockaddr_un, sun_path) + strlen(sockName.c_str()) + 1;
    if (bind(serverFd_, reinterpret_cast<struct sockaddr*>(&un), static_cast<int32_t>(len)) != SOCKET_SUCCESS) {
        LOGE("InitUnixWebSocket bind failed, errno = %{public}d", errno);
        CloseServerSocket();
        return false;
    }
    if (listen(serverFd_, 1) != SOCKET_SUCCESS) { // 1: connection num
        LOGE("InitUnixWebSocket listen failed, errno = %{public}d", errno);
        CloseServerSocket();
        return false;
    }
    socketState_ = SocketState::INITED;
    return true;
}

bool WebSocketServer::InitUnixWebSocket(int socketfd)
{
    if (socketState_ != SocketState::UNINITED) {
        LOGI("InitUnixWebSocket websocket has inited");
        return true;
    }
    if (socketfd < SOCKET_SUCCESS) {
        LOGE("InitUnixWebSocket socketfd is invalid");
        socketState_ = SocketState::UNINITED;
        return false;
    }
    connectionFd_ = socketfd;
    int flag = fcntl(connectionFd_, F_GETFL, 0);
    if (flag == -1) {
        LOGE("InitUnixWebSocket get client state is failed");
        return false;
    }
    fcntl(connectionFd_, F_SETFL, static_cast<size_t>(flag) & ~O_NONBLOCK);
    socketState_ = SocketState::INITED;
    return true;
}

bool WebSocketServer::ConnectUnixWebSocketBySocketpair()
{
    if (socketState_ == SocketState::UNINITED) {
        LOGE("ConnectUnixWebSocket failed, websocket not inited");
        return false;
    }
    if (socketState_ == SocketState::CONNECTED) {
        LOGI("ConnectUnixWebSocket websocket has connected");
        return true;
    }

    if (!HttpHandShake()) {
        LOGE("ConnectUnixWebSocket HttpHandShake failed");
        CloseConnectionSocket(ConnectionCloseReason::FAIL, SocketState::UNINITED);
        return false;
    }
    socketState_ = SocketState::CONNECTED;
    return true;
}
#endif

void WebSocketServer::CloseServerSocket()
{
    close(serverFd_);
    serverFd_ = -1;
    socketState_ = SocketState::UNINITED;
}

void WebSocketServer::OnNewConnection()
{
    LOGI("New client connected");
    socketState_ = SocketState::CONNECTED;
    if (openCb_) {
        openCb_();
    }
}

void WebSocketServer::SetValidateConnectionCallback(ValidateConnectionCallback cb)
{
    validateCb_ = std::move(cb);
}

void WebSocketServer::SetOpenConnectionCallback(OpenConnectionCallback cb)
{
    openCb_ = std::move(cb);
}

bool WebSocketServer::ValidateIncomingFrame(const WebSocketFrame& wsFrame)
{
    // "The server MUST close the connection upon receiving a frame that is not masked."
    // https://www.rfc-editor.org/rfc/rfc6455#section-5.1
    return wsFrame.mask == 1;
}

std::string WebSocketServer::CreateFrame(bool isLast, FrameType frameType) const
{
    ServerFrameBuilder builder(isLast, frameType);
    return builder.Build();
}

std::string WebSocketServer::CreateFrame(bool isLast, FrameType frameType, const std::string& payload) const
{
    ServerFrameBuilder builder(isLast, frameType);
    return builder.SetPayload(payload).Build();
}

std::string WebSocketServer::CreateFrame(bool isLast, FrameType frameType, std::string&& payload) const
{
    ServerFrameBuilder builder(isLast, frameType);
    return builder.SetPayload(std::move(payload)).Build();
}

void WebSocketServer::Close()
{
    if (socketState_ == SocketState::UNINITED) {
        return;
    }
    if (socketState_ == SocketState::CONNECTED) {
        CloseConnection(CloseStatusCode::SERVER_GO_AWAY, SocketState::INITED);
    }
    usleep(10000); // 10000: time for websocket to enter the accept
#if defined(OHOS_PLATFORM)
    shutdown(serverFd_, SHUT_RDWR);
#endif
    CloseServerSocket();
}
} // namespace OHOS::ArkCompiler::Toolchain
