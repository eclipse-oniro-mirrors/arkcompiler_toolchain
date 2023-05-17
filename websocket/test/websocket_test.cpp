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

#include <arpa/inet.h>
#include <sys/un.h>

#include "gtest/gtest.h"
#include "websocket.h"
#include "securec.h"

using namespace OHOS::ArkCompiler::Toolchain;

namespace panda::test {
class WebSocketTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

    class ClientWebSocket : public WebSocket {
    public:
        ClientWebSocket() = default;
        ~ClientWebSocket() = default;
#if defined(OHOS_PLATFORM)
        bool ClientConnectUnixWebSocket(const std::string &sockName, uint32_t timeoutLimit = 0)
        {
            if (socketState_ != SocketState::UNINITED) {
                std::cout << "ClientConnectUnixWebSocket::client has inited..." << std::endl;
                return true;
            }

            client_ = socket(AF_UNIX, SOCK_STREAM, 0);
            if (client_ < SOCKET_SUCCESS) {
                std::cerr << "ClientConnectUnixWebSocket::client socket failed, error = "
                          << errno << ", desc = " << strerror(errno) << std::endl;
                return false;
            }

            // set send and recv timeout limit
            if (!SetWebSocketTimeOut(client_, timeoutLimit)) {
                std::cerr << "ClientConnectUnixWebSocket::client SetWebSocketTimeOut failed, error = "
                          << errno << ", desc = " << strerror(errno) << std::endl;
                close(client_);
                client_ = -1;
                return false;
            }

            struct sockaddr_un serverAddr;
            if (memset_s(&serverAddr, sizeof(serverAddr), 0, sizeof(serverAddr)) != EOK) {
                std::cerr << "ClientConnectUnixWebSocket::client memset_s serverAddr failed, error = "
                          << errno << ", desc = " << strerror(errno) << std::endl;
                close(client_);
                client_ = -1;
                return false;
            }
            serverAddr.sun_family = AF_UNIX;
            if (strcpy_s(serverAddr.sun_path + 1, sizeof(serverAddr.sun_path) - 1, sockName.c_str()) != EOK) {
                std::cerr << "ClientConnectUnixWebSocket::client strcpy_s serverAddr.sun_path failed, error = "
                          << errno << ", desc = " << strerror(errno) << std::endl;
                close(client_);
                client_ = -1;
                return false;
            }
            serverAddr.sun_path[0] = '\0';

            uint32_t len = offsetof(struct sockaddr_un, sun_path) + strlen(sockName.c_str()) + 1;
            int ret = connect(client_, reinterpret_cast<struct sockaddr*>(&serverAddr), static_cast<int32_t>(len));
            if (ret != SOCKET_SUCCESS) {
                std::cerr << "ClientConnectUnixWebSocket::client connect failed, error = "
                          << errno << ", desc = " << strerror(errno) << std::endl;
                close(client_);
                client_ = -1;
                return false;
            }
            socketState_ = SocketState::INITED;
            std::cout << "ClientConnectUnixWebSocket::client connect success..." << std::endl;
            return true;
        }
#else
        bool ClientConnectTcpWebSocket(uint32_t timeoutLimit = 0)
        {
            if (socketState_ != SocketState::UNINITED) {
                std::cout << "ClientConnectTcpWebSocket::client has inited..." << std::endl;
                return true;
            }

            client_ = socket(AF_INET, SOCK_STREAM, 0);
            if (client_ < SOCKET_SUCCESS) {
                std::cerr << "ClientConnectTcpWebSocket::client socket failed, error = "
                          << errno << ", desc = " << strerror(errno) << std::endl;
                return false;
            }

            // set send and recv timeout limit
            if (!SetWebSocketTimeOut(client_, timeoutLimit)) {
                std::cerr << "ClientConnectTcpWebSocket::client SetWebSocketTimeOut failed, error = "
                          << errno << ", desc = " << strerror(errno) << std::endl;
                close(client_);
                client_ = -1;
                return false;
            }

            struct sockaddr_in serverAddr;
            if (memset_s(&serverAddr, sizeof(serverAddr), 0, sizeof(serverAddr)) != EOK) {
                std::cerr << "ClientConnectTcpWebSocket::client memset_s serverAddr failed, error = "
                          << errno << ", desc = " << strerror(errno) << std::endl;
                close(client_);
                client_ = -1;
                return false;
            }
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(9230); // 9230: sockName for tcp
            if (int ret = inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) < NET_SUCCESS) {
                std::cerr << "ClientConnectTcpWebSocket::client inet_pton failed, ret = "
                          << ret << ", error = " << errno << ", desc = " << strerror(errno) << std::endl;
                close(client_);
                client_ = -1;
                return false;
            }

            int ret = connect(client_, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr));
            if (ret != SOCKET_SUCCESS) {
                std::cerr << "ClientConnectTcpWebSocket::client connect failed, error = "
                          << errno << ", desc = " << strerror(errno) << std::endl;
                close(client_);
                client_ = -1;
                return false;
            }
            socketState_ = SocketState::INITED;
            std::cout << "ClientConnectTcpWebSocket::client connect success..." << std::endl;
            return true;
        }
#endif

        bool ClientSendWSUpgradeReq()
        {
            if (socketState_ == SocketState::UNINITED) {
                std::cerr << "ClientSendWSUpgradeReq::client has not inited..." << std::endl;
                return false;
            }
            if (socketState_ == SocketState::CONNECTED) {
                std::cout << "ClientSendWSUpgradeReq::client has connected..." << std::endl;
                return true;
            }

            int msgLen = strlen(CLIENT_WEBSOCKET_UPGRADE_REQ);
            int32_t sendLen = send(client_, CLIENT_WEBSOCKET_UPGRADE_REQ, msgLen, 0);
            if (sendLen != msgLen) {
                std::cerr << "ClientSendWSUpgradeReq::client send wsupgrade req failed, error = "
                          << errno << ", desc = " << strerror(errno) << std::endl;
                socketState_ = SocketState::UNINITED;
#if defined(OHOS_PLATFORM)
                shutdown(client_, SHUT_RDWR);
#endif
                close(client_);
                client_ = -1;
                return false;
            }
            std::cout << "ClientSendWSUpgradeReq::client send wsupgrade req success..." << std::endl;
            return true;
        }

        bool ClientRecvWSUpgradeRsp()
        {
            if (socketState_ == SocketState::UNINITED) {
                std::cerr << "ClientRecvWSUpgradeRsp::client has not inited..." << std::endl;
                return false;
            }
            if (socketState_ == SocketState::CONNECTED) {
                std::cout << "ClientRecvWSUpgradeRsp::client has connected..." << std::endl;
                return true;
            }

            char recvBuf[CLIENT_WEBSOCKET_UPGRADE_RSP_LEN + 1] = {0};
            int32_t bufLen = recv(client_, recvBuf, CLIENT_WEBSOCKET_UPGRADE_RSP_LEN, 0);
            if (bufLen != CLIENT_WEBSOCKET_UPGRADE_RSP_LEN) {
                std::cerr << "ClientRecvWSUpgradeRsp::client recv wsupgrade rsp failed, error = "
                          << errno << ", desc = " << strerror(errno) << std::endl;
                socketState_ = SocketState::UNINITED;
#if defined(OHOS_PLATFORM)
                shutdown(client_, SHUT_RDWR);
#endif
                close(client_);
                client_ = -1;
                return false;
            }
            socketState_ = SocketState::CONNECTED;
            std::cout << "ClientRecvWSUpgradeRsp::client recv wsupgrade rsp success..." << std::endl;
            return true;
        }

        bool ClientSendReq(const std::string &message)
        {
            if (socketState_ != SocketState::CONNECTED) {
                std::cerr << "ClientSendReq::client has not connected..." << std::endl;
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
                std::cerr << "ClientSendReq::client memcpy_s MASK_KEY failed, error = "
                          << errno << ", desc = " << strerror(errno) << std::endl;
                return false;
            }
            sendMsgLen += SOCKET_MASK_LEN;

            std::string maskMessage;
            for (uint64_t i = 0; i < msgLen; i++) {
                uint64_t j = i % SOCKET_MASK_LEN;
                maskMessage.push_back(message[i] ^ MASK_KEY[j]);
            }
            if (memcpy_s(sendBuf + sendMsgLen, msgLen, maskMessage.c_str(), msgLen) != EOK) {
                std::cerr << "ClientSendReq::client memcpy_s maskMessage failed, error = "
                          << errno << ", desc = " << strerror(errno) << std::endl;
                return false;
            }
            msgBuf[sendMsgLen + msgLen] = '\0';

            if (send(client_, sendBuf, sendMsgLen + msgLen, 0) != static_cast<int>(sendMsgLen + msgLen)) {
                std::cerr << "ClientSendReq::client send msg req failed, error = "
                          << errno << ", desc = " << strerror(errno) << std::endl;
                return false;
            }
            std::cout << "ClientRecvWSUpgradeRsp::client send msg req success..." << std::endl;
            return true;
        }

        void Close()
        {
            if (socketState_ == SocketState::UNINITED) {
                return;
            }
#if defined(OHOS_PLATFORM)
            shutdown(client_, SHUT_RDWR);
#endif
            close(client_);
            client_ = -1;
            socketState_ = SocketState::UNINITED;
        }

    private:
        static constexpr char CLIENT_WEBSOCKET_UPGRADE_REQ[] =  "GET / HTTP/1.1\r\n"
                                                                "Connection: Upgrade\r\n"
                                                                "Pragma: no-cache\r\n"
                                                                "Cache-Control: no-cache\r\n"
                                                                "Upgrade: websocket\r\n"
                                                                "Sec-WebSocket-Version: 13\r\n"
                                                                "Accept-Encoding: gzip, deflate, br\r\n"
                                                                "Sec-WebSocket-Key: 64b4B+s5JDlgkdg7NekJ+g==\r\n"
                                                                "Sec-WebSocket-Extensions: permessage-deflate\r\n";
        static constexpr int32_t CLIENT_WEBSOCKET_UPGRADE_RSP_LEN = 129;
        static constexpr char MASK_KEY[SOCKET_MASK_LEN + 1] = "abcd";
        static constexpr int NET_SUCCESS = 1;
    };

#if defined(OHOS_PLATFORM)
    static constexpr char UNIX_DOMAIN_PATH[] = "server.sock";
#endif
    static constexpr char HELLO_SERVER[]   = "hello server";
    static constexpr char HELLO_CLIENT[]   = "hello client";
    static constexpr char SERVER_OK[]      = "server ok";
    static constexpr char CLIENT_OK[]      = "client ok";
    static constexpr char QUIT[]           = "quit";
    static const std::string LONG_MSG;
    static const std::string LONG_LONG_MSG;
};

const std::string WebSocketTest::LONG_MSG       = std::string(1000, 'f');
const std::string WebSocketTest::LONG_LONG_MSG  = std::string(0xfffff, 'f');

HWTEST_F(WebSocketTest, ConnectWebSocketTest, testing::ext::TestSize.Level0)
{
    WebSocket serverSocket;
    bool ret = false;
#if defined(OHOS_PLATFORM)
    int appPid = getpid();
    ret = serverSocket.InitUnixWebSocket(UNIX_DOMAIN_PATH + std::to_string(appPid), 5);
#else
    ret = serverSocket.InitTcpWebSocket(9230, 5);
#endif
    ASSERT_TRUE(ret);
    pid_t pid = fork();
    if (pid == 0) {
        // subprocess, handle client connect and recv/send message
        // note: EXPECT/ASSERT produce errors in subprocess that can not lead to failure of testcase in mainprocess,
        //       so testcase still success finally.
        ClientWebSocket clientSocket;
        bool retClient = false;
#if defined(OHOS_PLATFORM)
        retClient = clientSocket.ClientConnectUnixWebSocket(UNIX_DOMAIN_PATH + std::to_string(appPid), 5);
#else
        retClient = clientSocket.ClientConnectTcpWebSocket(5);
#endif
        ASSERT_TRUE(retClient);
        retClient = clientSocket.ClientSendWSUpgradeReq();
        ASSERT_TRUE(retClient);
        retClient = clientSocket.ClientRecvWSUpgradeRsp();
        ASSERT_TRUE(retClient);
        retClient = clientSocket.ClientSendReq(HELLO_SERVER);
        EXPECT_TRUE(retClient);
        std::string recv = clientSocket.Decode();
        EXPECT_EQ(strcmp(recv.c_str(), HELLO_CLIENT), 0);
        if (strcmp(recv.c_str(), HELLO_CLIENT) == 0) {
            retClient = clientSocket.ClientSendReq(CLIENT_OK);
            EXPECT_TRUE(retClient);
        }
        retClient = clientSocket.ClientSendReq(LONG_MSG);
        EXPECT_TRUE(retClient);
        recv = clientSocket.Decode();
        EXPECT_EQ(strcmp(recv.c_str(), SERVER_OK), 0);
        if (strcmp(recv.c_str(), SERVER_OK) == 0) {
            retClient = clientSocket.ClientSendReq(CLIENT_OK);
            EXPECT_TRUE(retClient);
        }
        retClient = clientSocket.ClientSendReq(LONG_LONG_MSG);
        EXPECT_TRUE(retClient);
        recv = clientSocket.Decode();
        EXPECT_EQ(strcmp(recv.c_str(), SERVER_OK), 0);
        if (strcmp(recv.c_str(), SERVER_OK) == 0) {
            retClient = clientSocket.ClientSendReq(CLIENT_OK);
            EXPECT_TRUE(retClient);
        }
        retClient = clientSocket.ClientSendReq(QUIT);
        EXPECT_TRUE(retClient);
        clientSocket.Close();
        exit(0);
    } else if (pid > 0) {
        // mainprocess, handle server connect and recv/send message
#if defined(OHOS_PLATFORM)
        ret = serverSocket.ConnectUnixWebSocket();
#else
        ret = serverSocket.ConnectTcpWebSocket();
#endif
        ASSERT_TRUE(ret);
        std::string recv = serverSocket.Decode();
        EXPECT_EQ(strcmp(recv.c_str(), HELLO_SERVER), 0);
        serverSocket.SendReply(HELLO_CLIENT);
        recv = serverSocket.Decode();
        EXPECT_EQ(strcmp(recv.c_str(), CLIENT_OK), 0);
        recv = serverSocket.Decode();
        EXPECT_EQ(strcmp(recv.c_str(), LONG_MSG.c_str()), 0);
        serverSocket.SendReply(SERVER_OK);
        recv = serverSocket.Decode();
        EXPECT_EQ(strcmp(recv.c_str(), CLIENT_OK), 0);
        recv = serverSocket.Decode();
        EXPECT_EQ(strcmp(recv.c_str(), LONG_LONG_MSG.c_str()), 0);
        serverSocket.SendReply(SERVER_OK);
        recv = serverSocket.Decode();
        EXPECT_EQ(strcmp(recv.c_str(), CLIENT_OK), 0);
        recv = serverSocket.Decode();
        EXPECT_EQ(strcmp(recv.c_str(), QUIT), 0);
        serverSocket.Close();
        // sleep ensure that linux os core can really release resource
        sleep(3);
    } else {
        std::cerr << "ConnectWebSocketTest::fork failed, error = "
                  << errno << ", desc = " << strerror(errno) << std::endl;
    }
}

HWTEST_F(WebSocketTest, ReConnectWebSocketTest, testing::ext::TestSize.Level0)
{
    WebSocket serverSocket;
    bool ret = false;
#if defined(OHOS_PLATFORM)
    int appPid = getpid();
    ret = serverSocket.InitUnixWebSocket(UNIX_DOMAIN_PATH + std::to_string(appPid), 5);
#else
    ret = serverSocket.InitTcpWebSocket(9230, 5);
#endif
    ASSERT_TRUE(ret);
    for (int i = 0; i < 5; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // subprocess, handle client connect and recv/send message
            // note: EXPECT/ASSERT produce errors in subprocess that can not lead to failure of testcase in mainprocess,
            //       so testcase still success finally.
            ClientWebSocket clientSocket;
            bool retClient = false;
#if defined(OHOS_PLATFORM)
            retClient = clientSocket.ClientConnectUnixWebSocket(UNIX_DOMAIN_PATH + std::to_string(appPid), 5);
#else
            retClient = clientSocket.ClientConnectTcpWebSocket(5);
#endif
            ASSERT_TRUE(retClient);
            retClient = clientSocket.ClientSendWSUpgradeReq();
            ASSERT_TRUE(retClient);
            retClient = clientSocket.ClientRecvWSUpgradeRsp();
            ASSERT_TRUE(retClient);
            retClient = clientSocket.ClientSendReq(HELLO_SERVER + std::to_string(i));
            EXPECT_TRUE(retClient);
            std::string recv = clientSocket.Decode();
            EXPECT_EQ(strcmp(recv.c_str(), (HELLO_CLIENT + std::to_string(i)).c_str()), 0);
            if (strcmp(recv.c_str(), (HELLO_CLIENT + std::to_string(i)).c_str()) == 0) {
                retClient = clientSocket.ClientSendReq(CLIENT_OK + std::to_string(i));
                EXPECT_TRUE(retClient);
            }
            clientSocket.Close();
            exit(0);
        } else if (pid > 0) {
            // mainprocess, handle server connect and recv/send message
#if defined(OHOS_PLATFORM)
            ret = serverSocket.ConnectUnixWebSocket();
#else
            ret = serverSocket.ConnectTcpWebSocket();
#endif
            ASSERT_TRUE(ret);
            std::string recv = serverSocket.Decode();
            EXPECT_EQ(strcmp(recv.c_str(), (HELLO_SERVER + std::to_string(i)).c_str()), 0);
            serverSocket.SendReply(HELLO_CLIENT + std::to_string(i));
            recv = serverSocket.Decode();
            EXPECT_EQ(strcmp(recv.c_str(), (CLIENT_OK + std::to_string(i)).c_str()), 0);
            while (serverSocket.IsConnected()) {
                serverSocket.Decode();
            }
        } else {
            std::cerr << "ReConnectWebSocketTest::fork failed, error = "
                      << errno << ", desc = " << strerror(errno) << std::endl;
        }
    }
    serverSocket.Close();
}
}  // namespace panda::test
