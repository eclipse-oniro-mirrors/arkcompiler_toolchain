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

#include "agent/debugger_impl.h"
#include "ecmascript/tests/test_helper.h"
#include "protocol_channel.h"
#include "protocol_handler.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::tooling;

namespace panda::test {
class DebuggerImplTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(ecmaVm, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm, scope);
    }

protected:
    EcmaVM *ecmaVm {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(DebuggerImplTest, NotifyScriptParsed__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());

    // DebuggerImpl::NotifyScriptParsed -- fileName.substr(0, DATA_APP_PATH.length()) != DATA_APP_PATH
    std::string strFilename = "filename";
    EXPECT_FALSE(debuggerImpl->NotifyScriptParsed(0, strFilename, ""));

    // DebuggerImpl::NotifyScriptParsed -- fileName.substr(0, DATA_APP_PATH.length()) != DATA_APP_PATH
    strFilename = "/filename";
    EXPECT_FALSE(debuggerImpl->NotifyScriptParsed(0, strFilename, ""));

    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, GenerateCallFrames__001)
{
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_Enable__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DebuggerImpl::DispatcherImpl::Enable -- params == nullptr
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.enable",
            "params":{
                "maxScriptsCacheSize":"NotIntValue"
            }
        })";
    DispatchRequest request(msg);
    dispatcherImpl->Dispatch(request);

    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"code":1,"message":"wrong params"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_Enable__002)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));
    EXPECT_FALSE(ecmaVm->GetJsDebuggerManager()->IsDebugMode());

    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.enable",
            "params":{
                "maxScriptsCacheSize":1024
            }
        })";
    DispatchRequest request(msg);
    dispatcherImpl->Dispatch(request);

    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"debuggerId":"0"}})");
    EXPECT_TRUE(ecmaVm->GetJsDebuggerManager()->IsDebugMode());
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_Disable__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));
    ecmaVm->GetJsDebuggerManager()->SetDebugMode(true);

    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.disable"
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{}})");
    EXPECT_FALSE(ecmaVm->GetJsDebuggerManager()->IsDebugMode());
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_EvaluateOnCallFrame__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DebuggerImpl::DispatcherImpl::EvaluateOnCallFrame -- params == nullptr
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.evaluateOnCallFrame",
            "params":{}
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"code":1,"message":"wrong params"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_EvaluateOnCallFrame__002)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DebuggerImpl::EvaluateOnCallFrame -- callFrameId < 0
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.evaluateOnCallFrame",
            "params":{
                "callFrameId":"-1",
                "expression":"the expression"
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"code":1,"message":"Invalid callFrameId."}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_EvaluateOnCallFrame__003)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DebuggerImpl::EvaluateOnCallFrame -- callFrameId >= static_cast<CallFrameId>(callFrameHandlers_.size())
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.evaluateOnCallFrame",
            "params":{
                "callFrameId":"0",
                "expression":"the expression"
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"code":1,"message":"Invalid callFrameId."}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_GetPossibleBreakpoints__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DebuggerImpl::DispatcherImpl::GetPossibleBreakpoints -- params == nullptr
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.getPossibleBreakpoints",
            "params":{}
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"code":1,"message":"wrong params"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_GetPossibleBreakpoints__002)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DebuggerImpl::GetPossibleBreakpoints -- iter == scripts_.end()
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.getPossibleBreakpoints",
            "params":{
                "start":{
                    "scriptId":"0",
                    "lineNumber":1,
                    "columnNumber":0
                },
                "end":{
                    "scriptId":"0",
                    "lineNumber":1,
                    "columnNumber":0
                },
                "restrictToFunction":true
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"code":1,"message":"Unknown file name."}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_GetScriptSource__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DebuggerImpl::DispatcherImpl::GetScriptSource -- params == nullptr
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.getScriptSource",
            "params":{}
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"code":1,"message":"wrong params"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_GetScriptSource__002)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DebuggerImpl::GetScriptSource -- iter == scripts_.end()
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.getScriptSource",
            "params":{
                "scriptId":"0"
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"code":1,"message":"unknown script id: 0"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_Pause__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DDebuggerImpl::DispatcherImpl::GetScriptSource -- params == nullptr
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.pause"
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_RemoveBreakpoint__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DDebuggerImpl::DispatcherImpl::RemoveBreakpoint -- params == nullptr
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.removeBreakpoint",
            "params":{}
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"code":1,"message":"wrong params"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_RemoveBreakpoint__002)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DDebuggerImpl::RemoveBreakpoint -- !BreakpointDetails::ParseBreakpointId(id, &metaData)
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.removeBreakpoint",
            "params":{
                "breakpointId":"id:0:0"
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{"code":1,"message":"Parse breakpoint id failed"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_RemoveBreakpoint__003)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DDebuggerImpl::RemoveBreakpoint -- !MatchScripts(scriptFunc, metaData.url_, ScriptMatchType::URL)
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.removeBreakpoint",
            "params":{
                "breakpointId":"id:0:0:url"
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{"code":1,"message":"Unknown file name."}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_Resume__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    debuggerImpl->SetDebuggerState(DebuggerState::PAUSED);
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DebuggerImpl::DispatcherImpl::Resume -- params == nullptr
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.resume",
            "params":{
                "terminateOnResume":"NotBool"
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"code":1,"message":"wrong params"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_Resume__002)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    debuggerImpl->SetDebuggerState(DebuggerState::PAUSED);
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.resume",
            "params":{
                "terminateOnResume":true
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetAsyncCallStackDepth__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.setAsyncCallStackDepth"
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{"code":1,"message":"SetAsyncCallStackDepth not support now"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetBreakpointByUrl__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DebuggerImpl::DispatcherImpl::SetBreakpointByUrl -- params == nullptr
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.setBreakpointByUrl",
            "params":{}
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"code":1,"message":"wrong params"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetBreakpointByUrl__002)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));
    ecmaVm->GetJsDebuggerManager()->SetDebugMode(true);

    // DebuggerImpl::SetBreakpointByUrl -- extractor == nullptr
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.setBreakpointByUrl",
            "params":{
                "lineNumber":0,
                "url":"url_str",
                "urlRegex":"urlRegex_str",
                "scriptHash":"scriptHash_str",
                "columnNumber":0,
                "condition":"condition_str"
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"code":1,"message":"Unknown file name."}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetPauseOnExceptions__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DebuggerImpl::DispatcherImpl::SetPauseOnExceptions -- params == nullptr
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.setPauseOnExceptions",
            "params":{}
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"code":1,"message":"wrong params"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetPauseOnExceptions__002)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.setPauseOnExceptions",
            "params":{
                "state":"all"
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_StepInto__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DebuggerImpl::DispatcherImpl::StepInto -- params == nullptr
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.stepInto",
            "params":{
                "breakOnAsyncCall":"Str"
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"code":1,"message":"wrong params"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetMixedDebugEnabled__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DebuggerImpl::DispatcherImpl::SetMixedDebugEnabled -- Params == nullptr
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.setMixedDebugEnabled",
            "params":{
                "enabled":"NotBoolValue"
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"code":1,"message":"wrong params"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetMixedDebugEnabled__002)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));
    EXPECT_FALSE(ecmaVm->GetJsDebuggerManager()->IsMixedDebugEnabled());

    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.setMixedDebugEnabled",
            "params":{
                "enabled":true
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_TRUE(ecmaVm->GetJsDebuggerManager()->IsMixedDebugEnabled());
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetBlackboxPatterns__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.setBlackboxPatterns"
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{"code":1,"message":"SetBlackboxPatterns not support now"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_ReplyNativeCalling__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    // DebuggerImpl::DispatcherImpl::ReplyNativeCalling -- params == nullptr
    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.replyNativeCalling",
            "params":{
                "userCode":"NotBoolValue"
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"code":1,"message":"wrong params"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_ReplyNativeCalling__002)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.replyNativeCalling",
            "params":{
                "userCode":true
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}
}  // namespace panda::test
