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
    uint32_t msgLen = message.length();
    std::unique_ptr<char []> msgBuf = std::make_unique<char []>(msgLen + 11); // 11: the maximum expandable length
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
    msgBuf[sendMsgLen + msgLen] = '\0';
    send(client_, sendBuf, sendMsgLen + msgLen, 0);
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
        int32_t bufLen = recv(client_, recvbuf, PAYLOAD_LEN, 0);
        if (bufLen < PAYLOAD_LEN) {
            LOGE("ReadMsg failed readRet=%{public}d", bufLen);
            return false;
        }
        recvbuf[PAYLOAD_LEN] = '\0';
        uint16_t msgLen = 0;
        if (memcpy_s(&msgLen, sizeof(recvbuf), recvbuf, sizeof(recvbuf) - 1) != EOK) {
            LOGE("HandleFrame: memcpy_s failed");
            return false;
        }
        wsFrame.payloadLen = ntohs(msgLen);
    } else if (wsFrame.payloadLen > 126) { // 126: the payloadLen read from frame
        char recvbuf[EXTEND_PATLOAD_LEN + 1] = {0};
        int32_t bufLen = recv(client_, recvbuf, EXTEND_PATLOAD_LEN, 0);
        if (bufLen < EXTEND_PATLOAD_LEN) {
            LOGE("ReadMsg failed readRet=%{public}d", bufLen);
            return false;
        }
        recvbuf[EXTEND_PATLOAD_LEN] = '\0';
        wsFrame.payloadLen = NetToHostLongLong(recvbuf, EXTEND_PATLOAD_LEN);
    }
    return DecodeMessage(wsFrame);
}

bool WebSocket::DecodeMessage(WebSocketFrame& wsFrame)
{
    if (wsFrame.payloadLen == 0 || wsFrame.payloadLen > UINT64_MAX) {
        LOGE("ReadMsg length error, expected greater than zero and less than UINT64_MAX");
        return false;
    }
    wsFrame.payload = std::make_unique<char []>(wsFrame.payloadLen + 1);
    if (wsFrame.mask == 1) {
        char buf[wsFrame.payloadLen + 1];
        char mask[SOCKET_MASK_LEN + 1];
        int32_t bufLen = recv(client_, mask, SOCKET_MASK_LEN, 0);
        if (bufLen != SOCKET_MASK_LEN) {
            LOGE("ReadMsg failed readRet=%{public}d", bufLen);
            return false;
        }
        mask[SOCKET_MASK_LEN] = '\0';
        if (memcpy_s(wsFrame.maskingkey, SOCKET_MASK_LEN, mask, sizeof(mask) - 1) != EOK) {
            LOGE("DecodeMessage: memcpy_s failed");
            return false;
        }
        uint64_t msgLen = static_cast<uint64_t>(recv(static_cast<uint32_t>(client_), buf,
                                                     static_cast<int64_t>(wsFrame.payloadLen), 0));
        if (msgLen == 0) {
            LOGE("DecodeMessage recv payload failed, client disconnect");
            return false;
        }
        while (msgLen < wsFrame.payloadLen) {
            uint64_t len = static_cast<uint64_t>(recv(static_cast<uint32_t>(client_), buf + msgLen,
                                                      static_cast<int64_t>(wsFrame.payloadLen - msgLen), 0));
            if (len == 0) {
                LOGE("DecodeMessage recv payload in while failed, client disconnect");
                return false;
            }
            msgLen += len;
        }
        buf[wsFrame.payloadLen] = '\0';
        for (uint64_t i = 0; i < msgLen; i++) {
            uint64_t j = i % SOCKET_MASK_LEN;
            wsFrame.payload.get()[i] = buf[i] ^ wsFrame.maskingkey[j];
        }
    } else {
        char buf[wsFrame.payloadLen + 1];
        uint64_t msgLen = static_cast<uint64_t>(recv(static_cast<uint32_t>(client_), buf,
                                                     static_cast<int64_t>(wsFrame.payloadLen), 0));
        if (msgLen != wsFrame.payloadLen) {
            LOGE("ReadMsg failed");
            return false;
        }
        buf[wsFrame.payloadLen] = '\0';
        if (memcpy_s(wsFrame.payload.get(), wsFrame.payloadLen, buf, msgLen) != EOK) {
            LOGE("DecodeMessage: memcpy_s failed");
            return false;
        }
    }
    wsFrame.payload.get()[wsFrame.payloadLen] = '\0';
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
    int32_t sendLen = send(client_, response.c_str(), response.length(), 0);
    if (sendLen <= 0) {
        LOGE("ProtocolUpgrade: Send failed");
        return false;
    }
    return true;
}

std::string WebSocket::Decode()
{
    if (socketState_ != SocketState::CONNECTED) {
        LOGE("Decode failed, websocket not connected");
        return "";
    }
    char recvbuf[SOCKET_HEADER_LEN + 1];
    int32_t msgLen = recv(client_, recvbuf, SOCKET_HEADER_LEN, 0);
    if (msgLen != SOCKET_HEADER_LEN) {
        // msgLen 0 means client disconnect socket.
        if (msgLen == 0) {
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
        }
        return "";
    }
    recvbuf[SOCKET_HEADER_LEN] = '\0';
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
    }
    return "";
}

bool WebSocket::HttpHandShake()
{
    char msgBuf[SOCKET_HANDSHAKE_LEN];
    int32_t msgLen = recv(client_, msgBuf, SOCKET_HANDSHAKE_LEN, 0);
    if (msgLen <= 0) {
        LOGE("ReadMsg failed readRet=%{public}d", msgLen);
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
bool WebSocket::InitTcpWebSocket(uint32_t timeoutLimit)
{
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
        LOGE("InitTcpWebSocket socket init failed");
        return false;
    }
    // allow specified port can be used at once and not wait TIME_WAIT status ending
    int sockOptVal = 1;
    if ((setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &sockOptVal, sizeof(sockOptVal))) != SOCKET_SUCCESS) {
        LOGE("InitTcpWebSocket setsockopt SO_REUSEADDR failed");
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
    addr_sin.sin_port = htons(9230); // 9230: sockName for tcp
    addr_sin.sin_addr.s_addr = INADDR_ANY;
    if (bind(fd_, reinterpret_cast<struct sockaddr*>(&addr_sin), sizeof(addr_sin)) < SOCKET_SUCCESS) {
        LOGE("InitTcpWebSocket bind failed");
        close(fd_);
        fd_ = -1;
        return false;
    }
    if (listen(fd_, 1) < SOCKET_SUCCESS) {
        LOGE("InitTcpWebSocket listen failed");
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
        LOGE("ConnectTcpWebSocket accept failed");
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
        LOGE("InitUnixWebSocket socket init failed");
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
        LOGE("InitUnixWebSocket bind failed");
        close(fd_);
        fd_ = -1;
        return false;
    }
    if (listen(fd_, 1) < SOCKET_SUCCESS) { // 1: connection num
        LOGE("InitUnixWebSocket listen failed");
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
        LOGE("ConnectUnixWebSocket accept failed");
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
#if defined(OHOS_PLATFORM)
    shutdown(fd_, SHUT_RDWR);
#endif
    close(fd_);
    fd_ = -1;
    socketState_ = SocketState::UNINITED;
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

bool WebSocket::SetWebSocketTimeOut(int32_t fd, uint32_t timeoutLimit)
{
    if (timeoutLimit > 0) {
        struct timeval timeout = {timeoutLimit, 0};
        if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) != SOCKET_SUCCESS) {
            LOGE("SetWebSocketTimeOut setsockopt SO_SNDTIMEO failed");
            return false;
        }
        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != SOCKET_SUCCESS) {
            LOGE("SetWebSocketTimeOut setsockopt SO_RCVTIMEO failed");
            return false;
        }
    }
    return true;
}
} // namespace OHOS::ArkCompiler::Toolchain
