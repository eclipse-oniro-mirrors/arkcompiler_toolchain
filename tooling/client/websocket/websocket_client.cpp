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
#include <securec.h>

#include "common/log_wrapper.h"
#include "websocket_client.h"


namespace OHOS::ArkCompiler::Toolchain {
bool WebsocketClient::InitToolchainWebSocketForPort(int port, uint32_t timeoutLimit)
{
    if (socketState_ != ToolchainSocketState::UNINITED) {
        LOGE("InitToolchainWebSocketForPort::client has inited.");
        return true;
    }

    client_ = socket(AF_INET, SOCK_STREAM, 0);
    if (client_ < SOCKET_SUCCESS) {
        LOGE("InitToolchainWebSocketForPort::client socket failed, error = %{public}d , desc = %{public}s",
            errno, strerror(errno));
        return false;
    }

    // set send and recv timeout limit
    if (!SetWebSocketTimeOut(client_, timeoutLimit)) {
        LOGE("InitToolchainWebSocketForPort::client SetWebSocketTimeOut failed, error = %{public}d , desc = %{public}s",
            errno, strerror(errno));
        close(client_);
        client_ = -1;
        return false;
    }

    sockaddr_in clientAddr;
    if (memset_s(&clientAddr, sizeof(clientAddr), 0, sizeof(clientAddr)) != EOK) {
        LOGE("InitToolchainWebSocketForPort::client memset_s clientAddr failed, error = %{public}d, desc = %{public}s",
            errno, strerror(errno));
        close(client_);
        client_ = -1;
        return false;
    }
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(port);
    if (int ret = inet_pton(AF_INET, "127.0.0.1", &clientAddr.sin_addr) < NET_SUCCESS) {
        LOGE("InitToolchainWebSocketForPort::client inet_pton failed, error = %{public}d, desc = %{public}s",
            errno, strerror(errno));
        close(client_);
        client_ = -1;
        return false;
    }

    int ret = connect(client_, reinterpret_cast<struct sockaddr*>(&clientAddr), sizeof(clientAddr));
    if (ret != SOCKET_SUCCESS) {
        LOGE("InitToolchainWebSocketForPort::client connect failed, error = %{public}d, desc = %{public}s",
            errno, strerror(errno));
        close(client_);
        client_ = -1;
        return false;
    }
    socketState_ = ToolchainSocketState::INITED;
    LOGE("InitToolchainWebSocketForPort::client connect success.");
    return true;
}

bool WebsocketClient::InitToolchainWebSocketForSockName(const std::string &sockName, uint32_t timeoutLimit)
{
    if (socketState_ != ToolchainSocketState::UNINITED) {
        LOGE("InitToolchainWebSocketForSockName::client has inited.");
        return true;
    }

    client_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_ < SOCKET_SUCCESS) {
        LOGE("InitToolchainWebSocketForSockName::client socket failed, error = %{public}d , desc = %{public}s",
            errno, strerror(errno));
        return false;
    }

    // set send and recv timeout limit
    if (!SetWebSocketTimeOut(client_, timeoutLimit)) {
        LOGE("InitToolchainWebSocketForSockName::client SetWebSocketTimeOut failed, error = %{public}d ,\
            desc = %{public}s", errno, strerror(errno));
        close(client_);
        client_ = -1;
        return false;
    }

    struct sockaddr_un serverAddr;
    if (memset_s(&serverAddr, sizeof(serverAddr), 0, sizeof(serverAddr)) != EOK) {
        LOGE("InitToolchainWebSocketForSockName::client memset_s clientAddr failed, error = %{public}d,\
            desc = %{public}s", errno, strerror(errno));
        close(client_);
        client_ = -1;
        return false;
    }
    serverAddr.sun_family = AF_UNIX;
    if (strcpy_s(serverAddr.sun_path + 1, sizeof(serverAddr.sun_path) - 1, sockName.c_str()) != EOK) {
        LOGE("InitToolchainWebSocketForSockName::client strcpy_s serverAddr.sun_path failed, error = %{public}d,\
            desc = %{public}s", errno, strerror(errno));
        close(client_);
        client_ = -1;
        return false;
    }
    serverAddr.sun_path[0] = '\0';

    uint32_t len = offsetof(struct sockaddr_un, sun_path) + strlen(sockName.c_str()) + 1;
    int ret = connect(client_, reinterpret_cast<struct sockaddr*>(&serverAddr), static_cast<int32_t>(len));
    if (ret != SOCKET_SUCCESS) {
        LOGE("InitToolchainWebSocketForSockName::client connect failed, error = %{public}d, desc = %{public}s",
            errno, strerror(errno));
        close(client_);
        client_ = -1;
        return false;
    }
    socketState_ = ToolchainSocketState::INITED;
    LOGE("InitToolchainWebSocketForSockName::client connect success.");
    return true;
}

bool WebsocketClient::ClientSendWSUpgradeReq()
{
    if (socketState_ == ToolchainSocketState::UNINITED) {
        LOGE("ClientSendWSUpgradeReq::client has not inited.");
        return false;
    }
    if (socketState_ == ToolchainSocketState::CONNECTED) {
        LOGE("ClientSendWSUpgradeReq::client has connected.");
        return true;
    }

    int msgLen = strlen(CLIENT_WEBSOCKET_UPGRADE_REQ);
    int32_t sendLen = send(client_, CLIENT_WEBSOCKET_UPGRADE_REQ, msgLen, 0);
    if (sendLen != msgLen) {
        LOGE("ClientSendWSUpgradeReq::client send wsupgrade req failed, error = %{public}d, desc = %{public}sn",
            errno, strerror(errno));
        socketState_ = ToolchainSocketState::UNINITED;
        close(client_);
        client_ = -1;
        return false;
    }
    LOGE("ClientSendWSUpgradeReq::client send wsupgrade req success.");
    return true;
}

bool WebsocketClient::ClientRecvWSUpgradeRsp()
{
    if (socketState_ == ToolchainSocketState::UNINITED) {
        LOGE("ClientRecvWSUpgradeRsp::client has not inited.");
        return false;
    }
    if (socketState_ == ToolchainSocketState::CONNECTED) {
        LOGE("ClientRecvWSUpgradeRsp::client has connected.");
        return true;
    }

    char recvBuf[CLIENT_WEBSOCKET_UPGRADE_RSP_LEN + 1] = {0};
    int32_t bufLen = recv(client_, recvBuf, CLIENT_WEBSOCKET_UPGRADE_RSP_LEN, 0);
    if (bufLen != CLIENT_WEBSOCKET_UPGRADE_RSP_LEN) {
        LOGE("ClientRecvWSUpgradeRsp::client recv wsupgrade rsp failed, error = %{public}d, desc = %{public}sn",
            errno, strerror(errno));
        socketState_ = ToolchainSocketState::UNINITED;
        close(client_);
        client_ = -1;
        return false;
    }
    socketState_ = ToolchainSocketState::CONNECTED;
    LOGE("ClientRecvWSUpgradeRsp::client recv wsupgrade rsp success.");
    return true;
}

bool WebsocketClient::ClientSendReq(const std::string &message)
{
    if (socketState_ != ToolchainSocketState::CONNECTED) {
        LOGE("ClientSendReq::client has not connected.");
        return false;
    }

    uint32_t msgLen = message.length();
    std::unique_ptr<char []> msgBuf = std::make_unique<char []>(msgLen + 15); // 15: the maximum expand length
    char *sendBuf = msgBuf.get();
    uint32_t sendMsgLen = 0;
    sendBuf[0] = 0x81; // 0x81: the text message sent by the server should start with '0x81'.
    uint32_t mask = 1;
    // Depending on the length of the messages, client will use shift operation to get the res
    // and store them in the buffer.
    if (msgLen <= 125) { // 125: situation 1 when message's length <= 125
        sendBuf[1] = msgLen | (mask << 7); // 7: mask need shift left by 7 bits
        sendMsgLen = 2; // 2: the length of header frame is 2;
    } else if (msgLen < 65536) { // 65536: message's length
        sendBuf[1] = 126 | (mask << 7); // 126: payloadLen according to the spec; 7: mask shift left by 7 bits
        sendBuf[2] = ((msgLen >> 8) & 0xff); // 8: shift right by 8 bits => res * (256^1)
        sendBuf[3] = (msgLen & 0xff); // 3: store len's data => res * (256^0)
        sendMsgLen = 4; // 4: the length of header frame is 4
    } else {
        sendBuf[1] = 127 | (mask << 7); // 127: payloadLen according to the spec; 7: mask shift left by 7 bits
        for (int32_t i = 2; i <= 5; i++) { // 2 ~ 5: unused bits
            sendBuf[i] = 0;
        }
        sendBuf[6] = ((msgLen & 0xff000000) >> 24); // 6: shift 24 bits => res * (256^3)
        sendBuf[7] = ((msgLen & 0x00ff0000) >> 16); // 7: shift 16 bits => res * (256^2)
        sendBuf[8] = ((msgLen & 0x0000ff00) >> 8);  // 8: shift 8 bits => res * (256^1)
        sendBuf[9] = (msgLen & 0x000000ff); // 9: res * (256^0)
        sendMsgLen = 10; // 10: the length of header frame is 10
    }

    if (memcpy_s(sendBuf + sendMsgLen, SOCKET_MASK_LEN, MASK_KEY, SOCKET_MASK_LEN) != EOK) {
        LOGE("ClientSendReq::client memcpy_s MASK_KEY failed, error = %{public}d, desc = %{public}s",
            errno, strerror(errno));
        return false;
    }
    sendMsgLen += SOCKET_MASK_LEN;

    std::string maskMessage;
    for (uint64_t i = 0; i < msgLen; i++) {
        uint64_t j = i % SOCKET_MASK_LEN;
        maskMessage.push_back(message[i] ^ MASK_KEY[j]);
    }
    if (memcpy_s(sendBuf + sendMsgLen, msgLen, maskMessage.c_str(), msgLen) != EOK) {
        LOGE("ClientSendReq::client memcpy_s maskMessage failed, error = %{public}d, desc = %{public}s",
            errno, strerror(errno));
        return false;
    }
    msgBuf[sendMsgLen + msgLen] = '\0';

    if (send(client_, sendBuf, sendMsgLen + msgLen, 0) != static_cast<int>(sendMsgLen + msgLen)) {
        LOGE("ClientSendReq::client send msg req failed, error = %{public}d, desc = %{public}s",
            errno, strerror(errno));
        return false;
    }
    LOGE("ClientRecvWSUpgradeRsp::client send msg req success.");
    return true;
}

std::string WebsocketClient::Decode()
{
    if (socketState_ != ToolchainSocketState::CONNECTED) {
        LOGE("WebsocketClient:Decode failed, websocket not connected!");
        return "";
    }
    char recvbuf[SOCKET_HEADER_LEN + 1];
    errno = 0;
    if (!Recv(client_, recvbuf, SOCKET_HEADER_LEN, 0)) {
        if (errno != EAGAIN) {
            LOGE("WebsocketClient:Decode failed, client websocket disconnect");
            socketState_ = ToolchainSocketState::INITED;
            close(client_);
            client_ = -1;
            return "";
        }
        return "try again";
    }
    ToolchainWebSocketFrame wsFrame;
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
            LOGE("WebsocketClient Decode: Send pong frame failed");
            return "";
        }
    }
    return "";
}

bool WebsocketClient::HandleFrame(ToolchainWebSocketFrame& wsFrame)
{
    if (wsFrame.payloadLen == 126) { // 126: the payloadLen read from frame
        char recvbuf[PAYLOAD_LEN + 1] = {0};
        if (!Recv(client_, recvbuf, PAYLOAD_LEN, 0)) {
            LOGE("WebsocketClient HandleFrame: Recv payloadLen == 126 failed");
            return false;
        }

        uint16_t msgLen = 0;
        if (memcpy_s(&msgLen, sizeof(recvbuf), recvbuf, sizeof(recvbuf) - 1) != EOK) {
            LOGE("WebsocketClient HandleFrame: memcpy_s failed");
            return false;
        }
        wsFrame.payloadLen = ntohs(msgLen);
    } else if (wsFrame.payloadLen > 126) { // 126: the payloadLen read from frame
        char recvbuf[EXTEND_PAYLOAD_LEN + 1] = {0};
        if (!Recv(client_, recvbuf, EXTEND_PAYLOAD_LEN, 0)) {
            LOGE("WebsocketClient HandleFrame: Recv payloadLen > 127 failed");
            return false;
        }
        wsFrame.payloadLen = NetToHostLongLong(recvbuf, EXTEND_PAYLOAD_LEN);
    }
    return DecodeMessage(wsFrame);
}

bool WebsocketClient::DecodeMessage(ToolchainWebSocketFrame& wsFrame)
{
    if (wsFrame.payloadLen == 0 || wsFrame.payloadLen > UINT64_MAX) {
        LOGE("WebsocketClient:ReadMsg length error, expected greater than zero and less than UINT64_MAX");
        return false;
    }
    uint64_t msgLen = wsFrame.payloadLen;
    wsFrame.payload = std::make_unique<char []>(msgLen + 1);
    if (wsFrame.mask == 1) {
        char buf[msgLen + 1];
        if (!Recv(client_, wsFrame.maskingkey, SOCKET_MASK_LEN, 0)) {
            LOGE("WebsocketClient DecodeMessage: Recv maskingkey failed");
            return false;
        }

        if (!Recv(client_, buf, msgLen, 0)) {
            LOGE("WebsocketClient DecodeMessage: Recv message with mask failed");
            return false;
        }

        for (uint64_t i = 0; i < msgLen; i++) {
            uint64_t j = i % SOCKET_MASK_LEN;
            wsFrame.payload.get()[i] = buf[i] ^ wsFrame.maskingkey[j];
        }
    } else {
        char buf[msgLen + 1];
        if (!Recv(client_, buf, msgLen, 0)) {
            LOGE("WebsocketClient DecodeMessage: Recv message without mask failed");
            return false;
        }

        if (memcpy_s(wsFrame.payload.get(), msgLen, buf, msgLen) != EOK) {
            LOGE("WebsocketClient DecodeMessage: memcpy_s failed");
            return false;
        }
    }
    wsFrame.payload.get()[msgLen] = '\0';
    return true;
}

uint64_t WebsocketClient::NetToHostLongLong(char* buf, uint32_t len)
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

bool WebsocketClient::Send(int32_t fd, const char* buf, size_t totalLen, int32_t flags) const
{
    size_t sendLen = 0;
    while (sendLen < totalLen) {
        ssize_t len = send(fd, buf + sendLen, totalLen - sendLen, flags);
        if (len <= 0) {
            LOGE("WebsocketClient Send Message in while failed, WebsocketClient disconnect");
            return false;
        }
        sendLen += static_cast<size_t>(len);
    }
    return true;
}

bool WebsocketClient::Recv(int32_t fd, char* buf, size_t totalLen, int32_t flags) const
{
    size_t recvLen = 0;
    while (recvLen < totalLen) {
        ssize_t len = recv(fd, buf + recvLen, totalLen - recvLen, flags);
        if (len <= 0) {
            LOGE("WebsocketClient Recv payload in while failed, WebsocketClient disconnect");
            return false;
        }
        recvLen += static_cast<size_t>(len);
    }
    buf[totalLen] = '\0';
    return true;
}

void WebsocketClient::Close()
{
    if (socketState_ == ToolchainSocketState::UNINITED) {
        return;
    }
    socketState_ = ToolchainSocketState::UNINITED;
    close(client_);
    client_ = -1;
}

bool WebsocketClient::SetWebSocketTimeOut(int32_t fd, uint32_t timeoutLimit)
{
    if (timeoutLimit > 0) {
        struct timeval timeout = {timeoutLimit, 0};
        if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO,
            reinterpret_cast<char *>(&timeout), sizeof(timeout)) != SOCKET_SUCCESS) {
            LOGE("WebsocketClient:SetWebSocketTimeOut setsockopt SO_SNDTIMEO failed");
            return false;
        }
        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
            reinterpret_cast<char *>(&timeout), sizeof(timeout)) != SOCKET_SUCCESS) {
            LOGE("WebsocketClient:SetWebSocketTimeOut setsockopt SO_RCVTIMEO failed");
            return false;
        }
    }
    return true;
}

bool WebsocketClient::IsConnected()
{
    return socketState_ == ToolchainSocketState::CONNECTED;
}

std::string WebsocketClient::GetSocketStateString()
{
    std::vector<std::string> stateStr = {
        "uninited",
        "inited",
        "connected"
    };

    return stateStr[socketState_];
}
}  // namespace OHOS::ArkCompiler::Toolchain