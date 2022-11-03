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
    int32_t msgLen = message.length();
    std::unique_ptr<char []> msgBuf = std::make_unique<char []>(msgLen + 11); // 11: the maximum expandable length
    char* sendBuf = msgBuf.get();
    int32_t sendMsgLen;
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
        char recvbuf[PAYLOAD_LEN + 1];
        int32_t bufLen = recv(client_, recvbuf, PAYLOAD_LEN, 0);
        if (bufLen < PAYLOAD_LEN) {
            LOGE("ReadMsg failed readRet=%{public}d", bufLen);
            return false;
        }
        recvbuf[PAYLOAD_LEN] = '\0';
        uint16_t msgLen;
        if (memcpy_s(&msgLen, sizeof(recvbuf), recvbuf, sizeof(recvbuf) - 1) != EOK) {
            LOGE("HandleFrame: memcpy_s failed");
            return false;
        }
        wsFrame.payloadLen = ntohs(msgLen);
    } else if (wsFrame.payloadLen > 126) { // 126: the payloadLen read from frame
        char recvbuf[EXTEND_PATLOAD_LEN + 1];
        int32_t bufLen = recv(client_, recvbuf, EXTEND_PATLOAD_LEN, 0);
        if (bufLen < EXTEND_PATLOAD_LEN) {
            LOGE("ReadMsg failed readRet=%{public}d", bufLen);
            return false;
        }
        recvbuf[EXTEND_PATLOAD_LEN] = '\0';
        uint64_t msgLen;
        if (memcpy_s(&msgLen, sizeof(recvbuf), recvbuf, sizeof(recvbuf) - 1) != EOK) {
            LOGE("HandleFrame: memcpy_s failed");
            return false;
        }
        wsFrame.payloadLen = ntohl(msgLen);
    }
    return DecodeMessage(wsFrame);
}

bool WebSocket::DecodeMessage(WebSocketFrame& wsFrame)
{
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
        uint64_t msgLen = recv(client_, buf, wsFrame.payloadLen, 0);
        while (msgLen < wsFrame.payloadLen) {
            uint64_t len = recv(client_, buf + msgLen, wsFrame.payloadLen - msgLen, 0);
            msgLen += len;
        }
        buf[wsFrame.payloadLen] = '\0';
        for (uint64_t i = 0; i < wsFrame.payloadLen; i++) {
            uint64_t j = i % SOCKET_MASK_LEN;
            wsFrame.payload.get()[i] = buf[i] ^ wsFrame.maskingkey[j];
        }
    } else {
        char buf[wsFrame.payloadLen + 1];
        uint64_t msgLen = recv(client_, buf, wsFrame.payloadLen, 0);
        if (msgLen != wsFrame.payloadLen) {
            LOGE("ReadMsg failed");
            return false;
        }
        buf[wsFrame.payloadLen] = '\0';
        if (memcpy_s(wsFrame.payload.get(), wsFrame.payloadLen, buf, wsFrame.payloadLen) != EOK) {
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
    char recvbuf[SOCKET_HEADER_LEN + 1];
    int32_t msgLen = recv(client_, recvbuf, SOCKET_HEADER_LEN, 0);
    if (msgLen != SOCKET_HEADER_LEN) {
        return {};
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
        HandleFrame(wsFrame);
        return wsFrame.payload.get();
    }
    return std::string();
}

bool WebSocket::HttpHandShake()
{
    char msgBuf[SOCKET_HANDSHAKE_LEN];
    connectState_ = true;
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
bool WebSocket::StartForSimulator()
{
#if defined(WINDOWS_PLATFORM)
    WORD sockVersion = MAKEWORD(2, 2); // 2: version 2.2
    WSADATA wsaData;
    if (WSAStartup(sockVersion, &wsaData) != 0) {
        LOGE("StartWebSocket WSA init failed");
        return false;
    }
#endif
    fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_ < SOCKET_SUCCESS) {
        LOGE("StartWebSocket socket init failed");
        return false;
    }

    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(9230); // 9230: sockName for tcp
    if (bind(fd_, reinterpret_cast<struct sockaddr*>(&sin), sizeof(sin)) < SOCKET_SUCCESS) {
        LOGE("StartWebSocket bind failed");
        return false;
    }

    if (listen(fd_, 1) < SOCKET_SUCCESS) {
        LOGE("StartWebSocket listen failed");
        return false;
    }

    if ((client_ = accept(fd_, nullptr, nullptr)) < SOCKET_SUCCESS) {
        LOGE("StartWebSocket accept failed");
        return false;
    }

    if (!HttpHandShake()) {
        LOGE("StartWebSocket HttpHandShake failed");
        return false;
    }
    return true;
}
#else
bool WebSocket::StartWebSocket(std::string sockName)
{
    fd_ = socket(AF_UNIX, SOCK_STREAM, 0); // 0: defautlt protocol
    if (fd_ < SOCKET_SUCCESS) {
        LOGE("StartWebSocket socket init failed");
        return false;
    }
    struct sockaddr_un un;
    if (memset_s(&un, sizeof(un), 0, sizeof(un)) != EOK) {
        LOGE("StartWebSocket memset_s failed");
        return false;
    }
    un.sun_family = AF_UNIX;
    if (strcpy_s(un.sun_path + 1, sizeof(un.sun_path) - 1, sockName.c_str()) != EOK) {
        LOGE("StartWebSocket strcpy_s failed");
        return false;
    }
    un.sun_path[0] = '\0';
    int32_t len = offsetof(struct sockaddr_un, sun_path) + strlen(sockName.c_str()) + 1;
    if (bind(fd_, reinterpret_cast<struct sockaddr*>(&un), len) < SOCKET_SUCCESS) {
        LOGE("StartWebSocket bind failed");
        return false;
    }
    if (listen(fd_, 1) < SOCKET_SUCCESS) { // 1: connection num
        LOGE("StartWebSocket listen failed");
        return false;
    }
    if ((client_ = accept(fd_, nullptr, nullptr)) < SOCKET_SUCCESS) {
        LOGD("StartWebSocket accept failed");
        return false;
    }
    if (!HttpHandShake()) {
        LOGE("StartWebSocket HttpHandShake failed");
        return false;
    }
    return true;
}
#endif

void WebSocket::Close()
{
    if (fd_ >= SOCKET_SUCCESS) {
#if defined(OHOS_PLATFORM)
        if (connectState_) {
            shutdown(client_, SHUT_RDWR);
            close(client_);
        }
        shutdown(fd_, SHUT_RDWR);
#endif
        close(fd_);
    }
}
} // namespace OHOS::ArkCompiler::Toolchain
