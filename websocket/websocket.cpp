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

#include "websocket.h"

#include "define.h"
#include "log_wrapper.h"
#include "securec.h"

namespace OHOS::ArkCompiler::Toolchain {
/**
 *  SendMessage in WebSocket has 3 situations:
 *    1. message's length <= 125
 *    2. message's length >= 126 && messages's length < 65536
 *    3. message's length >= 65536
 */

void WebSocket::SendReply(const std::string& message) const
{
    if (socketState_ != SocketState::CONNECTED) {
        LOGE("SendReply failed, websocket not connected");
        return;
    }
    const size_t msgLen = message.length();
    const uint32_t headerSize = 11; // 11: the maximum expandable length
    std::unique_ptr<char []> msgBuf = std::make_unique<char []>(msgLen + headerSize);
    char* sendBuf = msgBuf.get();
    uint32_t sendMsgLen;
    sendBuf[0] = 0x81; // 0x81: the text message sent by the server should start with '0x81'.

    // Depending on the length of the messages, server will use shift operation to get the res
    // and store them in the buffer.
    if (msgLen <= 125) { // 125: situation 1 when message's length <= 125
        sendBuf[1] = msgLen;
        sendMsgLen = 2; // 2: the length of header frame is 2
    } else if (msgLen < 65536) { // 65536: message's length
        sendBuf[1] = 126; // 126: payloadLen according to the spec
        sendBuf[2] = ((msgLen >> 8) & 0xff); // 8: shift right by 8 bits => res * (256^1)
        sendBuf[3] = (msgLen & 0xff); // 3: store len's data => res * (256^0)
        sendMsgLen = 4; // 4: the length of header frame is 4
    } else {
        sendBuf[1] = 127; // 127: payloadLen according to the spec
        for (int32_t i = 2; i <= 5; i++) { // 2 ~ 5: unused bits
            sendBuf[i] = 0;
        }
        sendBuf[6] = ((msgLen & 0xff000000) >> 24); // 6: shift 24 bits => res * (256^3)
        sendBuf[7] = ((msgLen & 0x00ff0000) >> 16); // 7: shift 16 bits => res * (256^2)
        sendBuf[8] = ((msgLen & 0x0000ff00) >> 8);  // 8: shift 8 bits => res * (256^1)
        sendBuf[9] = (msgLen & 0x000000ff); // 9: res * (256^0)
        sendMsgLen = 10; // 10: the length of header frame is 10
    }
    if (memcpy_s(sendBuf + sendMsgLen, msgLen, message.c_str(), msgLen) != EOK) {
        LOGE("SendReply: memcpy_s failed");
        return;
    }
    sendBuf[sendMsgLen + msgLen] = '\0';
    if (!Send(client_, sendBuf, sendMsgLen + msgLen, 0)) {
        LOGE("SendReply: send failed");
        return;
    }
}

bool WebSocket::HttpProtocolDecode(const std::string& request, HttpProtocol& req)
{
    if (request.find("GET") == std::string::npos) {
        LOGE("Handshake failed: lack of necessary info");
        return false;
    }
    std::vector<std::string> reqStr = ProtocolSplit(request, EOL);
    for (size_t i = 0; i < reqStr.size(); i++) {
        if (i == 0) {
            std::vector<std::string> headers = ProtocolSplit(reqStr.at(i), " ");
            req.version = headers.at(2); // 2: to get the version param
        } else if (i < reqStr.size() - 1) {
            std::vector<std::string> headers = ProtocolSplit(reqStr.at(i), ": ");
            if (reqStr.at(i).find("Connection") != std::string::npos) {
                req.connection = headers.at(1); // 1: to get the connection param
            } else if (reqStr.at(i).find("Upgrade") != std::string::npos) {
                req.upgrade = headers.at(1); // 1: to get the upgrade param
            } else if (reqStr.at(i).find("Sec-WebSocket-Key") != std::string::npos) {
                req.secWebSocketKey = headers.at(1); // 1: to get the secWebSocketKey param
            }
        }
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

bool WebSocket::HandleFrame(WebSocketFrame& wsFrame)
{
    if (wsFrame.payloadLen == 126) { // 126: the payloadLen read from frame
        char recvbuf[PAYLOAD_LEN + 1] = {0};
        if (!Recv(client_, recvbuf, PAYLOAD_LEN, 0)) {
            LOGE("HandleFrame: Recv payloadLen == 126 failed");
            return false;
        }

        uint16_t msgLen = 0;
        if (memcpy_s(&msgLen, sizeof(recvbuf), recvbuf, sizeof(recvbuf) - 1) != EOK) {
            LOGE("HandleFrame: memcpy_s failed");
            return false;
        }
        wsFrame.payloadLen = ntohs(msgLen);
    } else if (wsFrame.payloadLen > 126) { // 126: the payloadLen read from frame
        char recvbuf[EXTEND_PAYLOAD_LEN + 1] = {0};
        if (!Recv(client_, recvbuf, EXTEND_PAYLOAD_LEN, 0)) {
            LOGE("HandleFrame: Recv payloadLen > 127 failed");
            return false;
        }
        wsFrame.payloadLen = NetToHostLongLong(recvbuf, EXTEND_PAYLOAD_LEN);
    }
    return DecodeMessage(wsFrame);
}

bool WebSocket::DecodeMessage(WebSocketFrame& wsFrame)
{
    if (wsFrame.payloadLen == 0 || wsFrame.payloadLen > UINT64_MAX) {
        LOGE("ReadMsg length error, expected greater than zero and less than UINT64_MAX");
        return false;
    }
    uint64_t msgLen = wsFrame.payloadLen;
    wsFrame.payload = std::make_unique<char []>(msgLen + 1);
    if (wsFrame.mask == 1) {
        char buf[msgLen + 1];
        if (!Recv(client_, wsFrame.maskingkey, SOCKET_MASK_LEN, 0)) {
            LOGE("DecodeMessage: Recv maskingkey failed");
            return false;
        }

        if (!Recv(client_, buf, msgLen, 0)) {
            LOGE("DecodeMessage: Recv message with mask failed");
            return false;
        }

        for (uint64_t i = 0; i < msgLen; i++) {
            uint64_t j = i % SOCKET_MASK_LEN;
            wsFrame.payload.get()[i] = buf[i] ^ wsFrame.maskingkey[j];
        }
    } else {
        char buf[msgLen + 1];
        if (!Recv(client_, buf, msgLen, 0)) {
            LOGE("DecodeMessage: Recv message without mask failed");
            return false;
        }

        if (memcpy_s(wsFrame.payload.get(), msgLen, buf, msgLen) != EOK) {
            LOGE("DecodeMessage: memcpy_s failed");
            return false;
        }
    }
    wsFrame.payload.get()[msgLen] = '\0';
    return true;
}

bool WebSocket::ProtocolUpgrade(const HttpProtocol& req)
{
    std::string rawKey = req.secWebSocketKey + WEB_SOCKET_GUID;
    unsigned const char* webSocketKey = reinterpret_cast<unsigned const char*>(std::move(rawKey).c_str());
    unsigned char hash[SHA_DIGEST_LENGTH + 1];
    SHA1(webSocketKey, strlen(reinterpret_cast<const char*>(webSocketKey)), hash);
    hash[SHA_DIGEST_LENGTH] = '\0';
    unsigned char encodedKey[ENCODED_KEY_LEN];
    EVP_EncodeBlock(encodedKey, reinterpret_cast<const unsigned char*>(hash), SHA_DIGEST_LENGTH);
    std::string response;

    std::ostringstream sstream;
    sstream << "HTTP/1.1 101 Switching Protocols" << EOL;
    sstream << "Connection: upgrade" << EOL;
    sstream << "Upgrade: websocket" << EOL;
    sstream << "Sec-WebSocket-Accept: " << encodedKey << EOL;
    sstream << EOL;
    response = sstream.str();
    if (!Send(client_, response.c_str(), response.length(), 0)) {
        LOGE("ProtocolUpgrade: Send failed");
        return false;
    }
    return true;
}

std::string WebSocket::Decode()
{
    if (socketState_ != SocketState::CONNECTED) {
        LOGE("Decode failed, websocket not connected!");
        return "";
    }
    char recvbuf[SOCKET_HEADER_LEN + 1];
    if (!Recv(client_, recvbuf, SOCKET_HEADER_LEN, 0)) {
        LOGE("Decode failed, client websocket disconnect");
        socketState_ = SocketState::INITED;
#if defined(OHOS_PLATFORM)
        shutdown(client_, SHUT_RDWR);
        close(client_);
        client_ = -1;
#else
        close(client_);
        client_ = -1;
#endif
        return "";
    }
    WebSocketFrame wsFrame;
    int32_t index = 0;
    wsFrame.fin = static_cast<uint8_t>(recvbuf[index] >> 7); // 7: shift right by 7 bits to get the fin
    wsFrame.opcode = static_cast<uint8_t>(recvbuf[index] & 0xf);
    if (wsFrame.opcode == 0x1) { // 0x1: 0x1 means a text frame
        index++;
        wsFrame.mask = static_cast<uint8_t>((recvbuf[index] >> 7) & 0x1); // 7: to get the mask
        wsFrame.payloadLen = recvbuf[index] & 0x7f;
        if (HandleFrame(wsFrame)) {
            return wsFrame.payload.get();
        }
        return "";
    } else if (wsFrame.opcode == 0x9) { // 0x9: 0x9 means a ping frame
        // send pong frame
        char pongFrame[SOCKET_HEADER_LEN] = {0};
        pongFrame[0] = 0x8a; // 0x8a: 0x8a means a pong frame
        pongFrame[1] = 0x0;
        if (!Send(client_, pongFrame, SOCKET_HEADER_LEN, 0)) {
            LOGE("Decode: Send pong frame failed");
            return "";
        }
    }
    return "";
}

bool WebSocket::HttpHandShake()
{
    char msgBuf[SOCKET_HANDSHAKE_LEN] = {0};
    ssize_t msgLen = recv(client_, msgBuf, SOCKET_HANDSHAKE_LEN, 0);
    if (msgLen <= 0) {
        LOGE("ReadMsg failed, msgLen = %{public}ld, errno = %{public}d", static_cast<long>(msgLen), errno);
        return false;
    } else {
        msgBuf[msgLen - 1] = '\0';
        HttpProtocol req;
        if (!HttpProtocolDecode(msgBuf, req)) {
            LOGE("HttpHandShake: Upgrade failed");
            return false;
        } else if (req.connection.find("Upgrade") != std::string::npos &&
            req.upgrade.find("websocket") != std::string::npos && req.version.compare("HTTP/1.1") == 0) {
            ProtocolUpgrade(req);
        }
    }
    return true;
}

#if !defined(OHOS_PLATFORM)
bool WebSocket::InitTcpWebSocket(int port, uint32_t timeoutLimit)
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
    fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_ < SOCKET_SUCCESS) {
        LOGE("InitTcpWebSocket socket init failed, errno = %{public}d", errno);
        return false;
    }
    // allow specified port can be used at once and not wait TIME_WAIT status ending
    int sockOptVal = 1;
    if ((setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,
        reinterpret_cast<char *>(&sockOptVal), sizeof(sockOptVal))) != SOCKET_SUCCESS) {
        LOGE("InitTcpWebSocket setsockopt SO_REUSEADDR failed, errno = %{public}d", errno);
        close(fd_);
        fd_ = -1;
        return false;
    }

    // set send and recv timeout
    if (!SetWebSocketTimeOut(fd_, timeoutLimit)) {
        LOGE("InitTcpWebSocket SetWebSocketTimeOut failed");
        close(fd_);
        fd_ = -1;
        return false;
    }

    sockaddr_in addr_sin = {0};
    addr_sin.sin_family = AF_INET;
    addr_sin.sin_port = htons(port);
    addr_sin.sin_addr.s_addr = INADDR_ANY;
    if (bind(fd_, reinterpret_cast<struct sockaddr*>(&addr_sin), sizeof(addr_sin)) < SOCKET_SUCCESS) {
        LOGE("InitTcpWebSocket bind failed, errno = %{public}d", errno);
        close(fd_);
        fd_ = -1;
        return false;
    }
    if (listen(fd_, 1) < SOCKET_SUCCESS) {
        LOGE("InitTcpWebSocket listen failed, errno = %{public}d", errno);
        close(fd_);
        fd_ = -1;
        return false;
    }
    socketState_ = SocketState::INITED;
    return true;
}

bool WebSocket::ConnectTcpWebSocket()
{
    if (socketState_ == SocketState::UNINITED) {
        LOGE("ConnectTcpWebSocket failed, websocket not inited");
        return false;
    }
    if (socketState_ == SocketState::CONNECTED) {
        LOGI("ConnectTcpWebSocket websocket has connected");
        return true;
    }

    if ((client_ = accept(fd_, nullptr, nullptr)) < SOCKET_SUCCESS) {
        LOGI("ConnectTcpWebSocket accept has exited");
        socketState_ = SocketState::UNINITED;
        close(fd_);
        fd_ = -1;
        return false;
    }

    if (!HttpHandShake()) {
        LOGE("ConnectTcpWebSocket HttpHandShake failed");
        socketState_ = SocketState::UNINITED;
        close(client_);
        client_ = -1;
        close(fd_);
        fd_ = -1;
        return false;
    }
    socketState_ = SocketState::CONNECTED;
    return true;
}
#else
bool WebSocket::InitUnixWebSocket(const std::string& sockName, uint32_t timeoutLimit)
{
    if (socketState_ != SocketState::UNINITED) {
        LOGI("InitUnixWebSocket websocket has inited");
        return true;
    }
    fd_ = socket(AF_UNIX, SOCK_STREAM, 0); // 0: defautlt protocol
    if (fd_ < SOCKET_SUCCESS) {
        LOGE("InitUnixWebSocket socket init failed, errno = %{public}d", errno);
        return false;
    }
    // set send and recv timeout
    if (!SetWebSocketTimeOut(fd_, timeoutLimit)) {
        LOGE("InitUnixWebSocket SetWebSocketTimeOut failed");
        close(fd_);
        fd_ = -1;
        return false;
    }

    struct sockaddr_un un;
    if (memset_s(&un, sizeof(un), 0, sizeof(un)) != EOK) {
        LOGE("InitUnixWebSocket memset_s failed");
        close(fd_);
        fd_ = -1;
        return false;
    }
    un.sun_family = AF_UNIX;
    if (strcpy_s(un.sun_path + 1, sizeof(un.sun_path) - 1, sockName.c_str()) != EOK) {
        LOGE("InitUnixWebSocket strcpy_s failed");
        close(fd_);
        fd_ = -1;
        return false;
    }
    un.sun_path[0] = '\0';
    uint32_t len = offsetof(struct sockaddr_un, sun_path) + strlen(sockName.c_str()) + 1;
    if (bind(fd_, reinterpret_cast<struct sockaddr*>(&un), static_cast<int32_t>(len)) < SOCKET_SUCCESS) {
        LOGE("InitUnixWebSocket bind failed, errno = %{public}d", errno);
        close(fd_);
        fd_ = -1;
        return false;
    }
    if (listen(fd_, 1) < SOCKET_SUCCESS) { // 1: connection num
        LOGE("InitUnixWebSocket listen failed, errno = %{public}d", errno);
        close(fd_);
        fd_ = -1;
        return false;
    }
    socketState_ = SocketState::INITED;
    return true;
}

bool WebSocket::ConnectUnixWebSocket()
{
    if (socketState_ == SocketState::UNINITED) {
        LOGE("ConnectUnixWebSocket failed, websocket not inited");
        return false;
    }
    if (socketState_ == SocketState::CONNECTED) {
        LOGI("ConnectUnixWebSocket websocket has connected");
        return true;
    }

    if ((client_ = accept(fd_, nullptr, nullptr)) < SOCKET_SUCCESS) {
        LOGI("ConnectUnixWebSocket accept has exited");
        socketState_ = SocketState::UNINITED;
        close(fd_);
        fd_ = -1;
        return false;
    }
    if (!HttpHandShake()) {
        LOGE("ConnectUnixWebSocket HttpHandShake failed");
        socketState_ = SocketState::UNINITED;
        shutdown(client_, SHUT_RDWR);
        close(client_);
        client_ = -1;
        shutdown(fd_, SHUT_RDWR);
        close(fd_);
        fd_ = -1;
        return false;
    }
    socketState_ = SocketState::CONNECTED;
    return true;
}
#endif

bool WebSocket::IsConnected()
{
    return socketState_ == SocketState::CONNECTED;
}

void WebSocket::Close()
{
    if (socketState_ == SocketState::UNINITED) {
        return;
    }
    if (socketState_ == SocketState::CONNECTED) {
#if defined(OHOS_PLATFORM)
        shutdown(client_, SHUT_RDWR);
#endif
        close(client_);
        client_ = -1;
    }
    socketState_ = SocketState::UNINITED;
    usleep(10000); // 10000: time for websocket to enter the accept
#if defined(OHOS_PLATFORM)
    shutdown(fd_, SHUT_RDWR);
#endif
    close(fd_);
    fd_ = -1;
}

uint64_t WebSocket::NetToHostLongLong(char* buf, uint32_t len)
{
    uint64_t result = 0;
    for (uint32_t i = 0; i < len; i++) {
        result |= static_cast<unsigned char>(buf[i]);
        if ((i + 1) < len) {
            result <<= 8; // 8: result need shift left 8 bits in order to big endian convert to int
        }
    }
    return result;
}

bool WebSocket::Recv(int32_t client, char* buf, size_t totalLen, int32_t flags) const
{
    size_t recvLen = 0;
    while (recvLen < totalLen) {
        ssize_t len = recv(client, buf + recvLen, totalLen - recvLen, flags);
        if (len <= 0) {
            LOGE("Recv payload in while failed, websocket disconnect, len = %{public}ld, errno = %{public}d",
                 static_cast<long>(len), errno);
            return false;
        }
        recvLen += static_cast<size_t>(len);
    }
    buf[totalLen] = '\0';
    return true;
}

bool WebSocket::Send(int32_t client, const char* buf, size_t totalLen, int32_t flags) const
{
    size_t sendLen = 0;
    while (sendLen < totalLen) {
        ssize_t len = send(client, buf + sendLen, totalLen - sendLen, flags);
        if (len <= 0) {
            LOGE("Send Message in while failed, websocket disconnect, len = %{public}ld, errno = %{public}d",
                 static_cast<long>(len), errno);
            return false;
        }
        sendLen += static_cast<size_t>(len);
    }
    return true;
}

#if !defined(OHOS_PLATFORM)
bool WebSocket::SetWebSocketTimeOut(int32_t fd, uint32_t timeoutLimit)
{
    if (timeoutLimit > 0) {
        struct timeval timeout = {timeoutLimit, 0};
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
bool WebSocket::SetWebSocketTimeOut(int32_t fd, uint32_t timeoutLimit)
{
    if (timeoutLimit > 0) {
        struct timeval timeout = {timeoutLimit, 0};
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
