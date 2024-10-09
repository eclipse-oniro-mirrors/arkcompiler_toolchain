/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "inspector/connect_inspector.h"
#include "inspector/connect_server.h"
#include "websocket/client/websocket_client.h"
#include "websocket/server/websocket_server.h"

using namespace OHOS::ArkCompiler::Toolchain;

namespace panda::test {
class ConnectServerTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "ConnectServerTest::SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "ConnectServerTest::TearDownTestCase";
    }

    void SetUp() override {}

    void TearDown() override {}
    void ReturnValueVerification();
#if defined(OHOS_PLATFORM)
    static constexpr char CONNECTED_MESSAGE_TEST[] = "connected";
    static constexpr char OPEN_MESSAGE_TEST[] = "layoutOpen";
    static constexpr char CLOSE_MESSAGE_TEST[] = "layoutClose";
    static constexpr char REQUEST_MESSAGE_TEST[] = "tree";
    static constexpr char STOPDEBUGGER_MESSAGE_TEST[] = "stopDebugger";
    static constexpr char OPEN_ARKUI_STATE_PROFILER_TEST[] = "ArkUIStateProfilerOpen";
    static constexpr char CLOSE_ARKUI_STATE_PROFILER_TEST[] = "ArkUIStateProfilerClose";
    static constexpr char START_RECORD_MESSAGE_TEST[] = "rsNodeStartRecord";
    static constexpr char STOP_RECORD_MESSAGE_TEST[] = "rsNodeStopRecord";

    static constexpr char HELLO_INSPECTOR_CLIENT[] = "hello inspector client";
    static constexpr char INSPECTOR_SERVER_OK[]    = "inspector server ok";
    static constexpr char INSPECTOR_RUN[]          = "inspector run";
    static constexpr char INSPECTOR_QUIT[]         = "inspector quit";
    static constexpr int32_t WAIT_TIME = 2;
#endif
};

bool g_profilerFlag = false;
bool g_connectFlag = false;
bool g_switchStatus = false;
int32_t g_createInfoId = 0;
int32_t g_instanceId = 1;

void CallbackInit()
{
    auto profilerCb = [](bool flag) -> void {
    g_profilerFlag = flag;
    };
    SetProfilerCallback(profilerCb);

    auto connectCb = [](bool flag) -> void {
        g_connectFlag = flag;
    };
    SetConnectCallback(connectCb);

    auto debugModeCb = []() -> void {
        GTEST_LOG_(INFO) << "Execute DebugModeCallBack.";
    };
    SetDebugModeCallBack(debugModeCb);

    auto switchStatusCb = [](bool flag) -> void {
        g_switchStatus = flag;
    };
    auto createInfoCb = [](int32_t id) -> void {
        g_createInfoId = id;
    };
    SetSwitchCallBack(switchStatusCb, createInfoCb, g_instanceId);

    auto startRecordFunc = []() -> void {};
    auto stopRecordFunc = []() -> void {};
    SetRecordCallback(startRecordFunc, stopRecordFunc);
    GTEST_LOG_(INFO) << "ConnectServerTest::CallbackInit finished";
}

void ConnectServerTest::ReturnValueVerification()
{
#if defined(OHOS_PLATFORM)
    // Waiting for executing the message instruction sent by the client
    sleep(WAIT_TIME);
    ASSERT_TRUE(g_profilerFlag);
    ASSERT_EQ(g_createInfoId, g_instanceId);
    ASSERT_TRUE(g_switchStatus);
    ASSERT_TRUE(g_connectFlag);
    ASSERT_FALSE(WaitForConnection());

    SendMessage(INSPECTOR_SERVER_OK);
    SendLayoutMessage(INSPECTOR_RUN);

    // Waiting for executing the message instruction sent by the client
    sleep(WAIT_TIME);
    ASSERT_FALSE(g_profilerFlag);
    ASSERT_FALSE(g_switchStatus);
    ASSERT_FALSE(g_connectFlag);

    SendProfilerMessage(INSPECTOR_QUIT);

    StopServer("InspectorConnectTest");
#endif
}

HWTEST_F(ConnectServerTest, InspectorBasicTest, testing::ext::TestSize.Level0)
{
    ASSERT_TRUE(WaitForConnection());
}

HWTEST_F(ConnectServerTest, InspectorConnectTest, testing::ext::TestSize.Level0)
{
    CallbackInit();
#if defined(OHOS_PLATFORM)
    int appPid = getprocpid();
    int oldProcessfd = -2;
    ASSERT_TRUE(StartServerForSocketPair(oldProcessfd));
    // test ConnectServer is not nullptr
    ASSERT_TRUE(StartServerForSocketPair(oldProcessfd));
    StoreMessage(g_instanceId, HELLO_INSPECTOR_CLIENT);
    pid_t pid = fork();
    if (pid == 0) {
        WebSocketClient clientSocket;
        ASSERT_TRUE(clientSocket.InitToolchainWebSocketForSockName(std::to_string(appPid)));
        ASSERT_TRUE(clientSocket.ClientSendWSUpgradeReq());
        ASSERT_TRUE(clientSocket.ClientRecvWSUpgradeRsp());
        EXPECT_TRUE(clientSocket.SendReply(OPEN_ARKUI_STATE_PROFILER_TEST));
        EXPECT_TRUE(clientSocket.SendReply(REQUEST_MESSAGE_TEST));
        EXPECT_TRUE(clientSocket.SendReply(OPEN_MESSAGE_TEST));
        EXPECT_TRUE(clientSocket.SendReply(CONNECTED_MESSAGE_TEST));
        EXPECT_TRUE(clientSocket.SendReply(START_RECORD_MESSAGE_TEST));
        EXPECT_TRUE(clientSocket.SendReply(STOP_RECORD_MESSAGE_TEST));

        EXPECT_STREQ(clientSocket.Decode().c_str(), HELLO_INSPECTOR_CLIENT);

        std::string recv = clientSocket.Decode();
        EXPECT_STREQ(recv.c_str(), INSPECTOR_SERVER_OK);
        if (strcmp(recv.c_str(), INSPECTOR_SERVER_OK) == 0) {
            EXPECT_TRUE(clientSocket.SendReply(CLOSE_MESSAGE_TEST));
            EXPECT_TRUE(clientSocket.SendReply(STOPDEBUGGER_MESSAGE_TEST));
            EXPECT_TRUE(clientSocket.SendReply(CLOSE_ARKUI_STATE_PROFILER_TEST));
        }

        EXPECT_STREQ(clientSocket.Decode().c_str(), INSPECTOR_RUN);
        EXPECT_STREQ(clientSocket.Decode().c_str(), INSPECTOR_QUIT);
        clientSocket.Close();
        exit(0);
    } else if (pid > 0) {
        ReturnValueVerification();
    }
    else {
        std::cerr << "InspectorConnectTest::fork failed" << std::endl;
    }
    RemoveMessage(g_instanceId);
#endif
}

} // namespace panda::test