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
namespace panda::ecmascript::tooling {
class DebuggerImplFriendTest {
public:
    explicit DebuggerImplFriendTest(std::unique_ptr<DebuggerImpl> &debuggerImpl)
    {
        debuggerImpl_ = std::move(debuggerImpl);
    }

    void CheckAndGenerateCondFunc(const std::optional<std::string> condition)
    {
        debuggerImpl_->CheckAndGenerateCondFunc(condition);
    }

    Local<JSValueRef> ConvertToLocal(const std::string &varValue)
    {
        return debuggerImpl_->ConvertToLocal(varValue);
    }

    bool GetMixStackEnabled()
    {
        return debuggerImpl_->mixStackEnabled_;
    }

    bool DecodeAndCheckBase64(const std::string &src, std::vector<uint8_t> &dest)
    {
        return debuggerImpl_->DecodeAndCheckBase64(src, dest);
    }
private:
    std::unique_ptr<DebuggerImpl> debuggerImpl_;
};
}

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
    EXPECT_FALSE(debuggerImpl->NotifyScriptParsed(strFilename, ""));

    // DebuggerImpl::NotifyScriptParsed -- fileName.substr(0, DATA_APP_PATH.length()) != DATA_APP_PATH
    strFilename = "/filename";
    EXPECT_FALSE(debuggerImpl->NotifyScriptParsed(strFilename, ""));

    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, NotifyScriptParsed__002)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    ecmaVm->GetJsDebuggerManager()->SetIsDebugApp(true);
    ecmaVm->GetJsDebuggerManager()->SetDebugMode(true);
    // DebuggerImpl::NotifyScriptParsed -- fileName.substr(0, DATA_APP_PATH.length()) != DATA_APP_PATH
    std::string strFilename = "";
    EXPECT_FALSE(debuggerImpl->NotifyScriptParsed(strFilename, ""));
    ecmaVm->GetJsDebuggerManager()->SetIsDebugApp(false);
    ecmaVm->GetJsDebuggerManager()->SetDebugMode(false);
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

    bool condition = outStrForCallbackCheck.find("protocols") != std::string::npos &&
        outStrForCallbackCheck.find("debuggerId") != std::string::npos;
    EXPECT_TRUE(condition);
    EXPECT_TRUE(ecmaVm->GetJsDebuggerManager()->IsDebugMode());
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_Enable_Accelerate_Launch_Mode)
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
                "maxScriptsCacheSize":1024,
                "options":[
                    "enableLaunchAccelerate"
                ]
            }
        })";
    DispatchRequest request(msg);
    dispatcherImpl->Dispatch(request);

    bool condition = outStrForCallbackCheck.find("protocols") != std::string::npos &&
        outStrForCallbackCheck.find("debuggerId") != std::string::npos &&
        outStrForCallbackCheck.find("saveAllPossibleBreakpoints") != std::string::npos;
    EXPECT_TRUE(condition);
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

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_EvaluateOnCallFrame__004)
{
    ProtocolChannel *protocolChannel = new ProtocolHandler(nullptr, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));
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
    std::unique_ptr<PtBaseReturns> resultPtr;
    DispatchResponse response = dispatcherImpl->EvaluateOnCallFrame(request, resultPtr);
    EXPECT_STREQ(response.GetMessage().c_str(), "Invalid callFrameId.");
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

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_RemoveBreakpointsByUrl__001)
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
            "method":"Debugger.removeBreakpointsByUrl",
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

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_RemoveBreakpointsByUrl__002)
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
            "method":"Debugger.removeBreakpointsByUrl",
            "params":{
                "url":"1"
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{"code":1,"message":"Unknown url"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_RemoveBreakpointsByUrl__003)
{
    ProtocolChannel *protocolChannel = new ProtocolHandler(nullptr, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));
    std::string msg = std::string() +
        R"({
            "id": 0,
            "method": "Debugger.removeBreakpointsByUrl",
            "params": {
                "url": "entry|entry|1.0.0|src/main/ets/pages/Index.ts"
            }
        })";
    DispatchRequest request(msg);
    DispatchResponse response = dispatcherImpl->RemoveBreakpointsByUrl(request);
    EXPECT_STREQ(response.GetMessage().c_str(), "Unknown url");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SaveAllPossibleBreakpoints__001)
{
    ProtocolChannel *protocolChannel = new ProtocolHandler(nullptr, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));
    std::string msg = std::string() +
        R"({
            "id": 0,
            "method": "Debugger.saveAllPossibleBreakpoints",
            "params": {
                "locations": {
                    "entry|entry|1.0.0|src/main/ets/pages/Index.ts": [{
                        "lineNumber": 59,
                        "columnNumber": 16
                    }]
                }
            }
        })";
    DispatchRequest request(msg);
    DispatchResponse response = dispatcherImpl->SaveAllPossibleBreakpoints(request);
    EXPECT_STREQ(response.GetMessage().c_str(), "SaveAllPossibleBreakpoints: debugger agent is not enabled");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetSymbolBreakpoints_001)
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
            "method":"Debugger.setSymbolicBreakpoints",
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

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetSymbolBreakpoints_002) 
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
            "method":"Debugger.setSymbolicBreakpoints",
            "params":{
                "symbolicBreakpoints":[
                    {
                        "condition":"testDebug"
                    }
                ]
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

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetSymbolBreakpoints_003)
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
            "method":"Debugger.setSymbolicBreakpoints",
            "params":{
                "symbolicBreakpoints":[
                    {
                        "functionName":"testDebug"
                    }
                ]
            }
        })";
    DispatchRequest request(msg);
    std::optional<std::string> result = dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"({"id":0,"result":{}})");
    EXPECT_FALSE(result.has_value());
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetSymbolBreakpoints_004)
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
            "method":"Debugger.setSymbolicBreakpoints",
            "params":{
                "symbolicBreakpoints":[
                    {
                        "functionName":"testDebug"
                    }
                ]
            }
        })";
    DispatchRequest request(msg);
    std::optional<std::string> result = dispatcherImpl->Dispatch(request, true);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), "");
    EXPECT_STREQ(result.value().c_str(), R"({"id":0,"result":{}})");
    EXPECT_TRUE(result.has_value());
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
            "method":"Debugger.setAsyncCallStackDepth",
            "params":{
                "maxDepth":32
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{}})");
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
                "enabled":true,
                "mixedStackEnabled":true
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

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_ContinueToLocation__001)
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
            "method":"Debugger.continueToLocation",
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

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_ContinueToLocation__002)
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
            "method":"Debugger.continueToLocation",
            "params":{
                "location":{
                    "scriptId":"2",
                    "lineNumber":3,
                    "columnNumber":20
                },
                "targetCallFrames":"testTargetCallFrames"
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

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetBreakpointsActive__001)
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
            "method":"Debugger.setBreakpointsActive",
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

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetBreakpointsActive__002)
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
            "method":"Debugger.setBreakpointsActive",
            "params":{
                "active":true
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

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetSkipAllPauses__001)
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
            "method":"Debugger.setSkipAllPauses",
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

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetSkipAllPauses__002)
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
            "method":"Debugger.setSkipAllPauses",
            "params":{
                "skip":true
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

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_GetPossibleAndSetBreakpoint__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    ecmaVm->GetJsDebuggerManager()->SetDebugMode(true);
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.getPossibleAndSetBreakpointByUrl",
            "params":{}
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{"code":1,"message":"GetPossibleAndSetBreakpointByUrl: no pennding breakpoint exists"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_GetPossibleAndSetBreakpoint__002)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    ecmaVm->GetJsDebuggerManager()->SetDebugMode(true);
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    std::string msg = std::string() +
        R"({
            "id":0,
            "method":"Debugger.getPossibleAndSetBreakpointByUrl",
            "params":{
                "locations":[
                    {
                        "lineNumber":3,
                        "columnNumber":20,
                        "url":"Index.ets"
                    }]
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{"locations":[{"lineNumber":3,"columnNumber":20,"id":"invalid","scriptId":0}]}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_GetPossibleAndSetBreakpointByUrl__003)
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
            "id": 0,
            "method": "Debugger.getPossibleAndSetBreakpointByUrl",
            "params": {
                "locations": [{
                    "url": "entry|entry|1.0.0|src/main/ets/pages/Index.ts",
                    "lineNumber": 59,
                    "columnNumber": 16
                }]
            }
        })";
    DispatchRequest request(msg);
    std::optional<std::string> response =  dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{"code":1,"message":"GetPossibleAndSetBreakpointByUrl: debugger agent is not enabled"}})");
    EXPECT_FALSE(response.has_value());
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_GetPossibleAndSetBreakpointByUrl__004)
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

    std::string msg1 = std::string() +
        R"({
            "id":0,
            "method":"Debugger.enable",
            "params":{
                "maxScriptsCacheSize":1024,
                "options":[
                    "enableLaunchAccelerate"
                ]
            }
        })";
    DispatchRequest request1(msg1);
    dispatcherImpl->Dispatch(request1);
    EXPECT_TRUE(ecmaVm->GetJsDebuggerManager()->IsDebugMode());
    
    outStrForCallbackCheck = "";
    std::string msg2 = std::string() +
        R"({
            "id":0,
            "method":"Debugger.getPossibleAndSetBreakpointByUrl",
            "params":{
                "locations":[
                    {
                        "lineNumber":3,
                        "columnNumber":20,
                        "url":"Index.ets"
                    }]
            }
        })";
    DispatchRequest request2(msg2);
    std::optional<std::string> response =  dispatcherImpl->Dispatch(request2, true);
    EXPECT_TRUE(response.has_value());
    EXPECT_STREQ(response.value().c_str(),
        R"({"id":0,"result":{"locations":[{"lineNumber":3,"columnNumber":20,"id":"invalid","scriptId":0}]}})");
    EXPECT_TRUE(outStrForCallbackCheck.empty());
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_DropFrame__001)
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
            "method":"Debugger.dropFrame",
            "params":{
                "droppedDepth":3
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{"code":1,"message":"Not yet support dropping multiple frames"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, DispatcherImplCallFunctionOn__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));

    std::string msg = std::string() + R"({"id":0,"method":"Debugger.callFunctionOn","params":{
        "callFrameId":"0", "functionDeclaration":0}})";
    DispatchRequest request(msg);
    std::unique_ptr<PtBaseReturns> result = nullptr;
    DispatchResponse response = dispatcherImpl->CallFunctionOn(request, result);
    protocolChannel->SendResponse(request, response, *result);
    ASSERT_TRUE(outStrForCallbackCheck.find("wrong params") != std::string::npos);

    msg = std::string() + R"({"id":0,"method":"Debugger.callFunctionOn","params":{
        "callFrameId":"0", "functionDeclaration":"test"}})";
    DispatchRequest request1(msg);
    response = dispatcherImpl->CallFunctionOn(request1, result);
    protocolChannel->SendResponse(request1, response, *result);
    ASSERT_TRUE(outStrForCallbackCheck.find("Unsupport eval now") == std::string::npos);
    if (protocolChannel != nullptr) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, DispatcherImplCallFunctionOn__002)
{
    ProtocolChannel *protocolChannel = new ProtocolHandler(nullptr, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto dispatcherImpl = std::make_unique<DebuggerImpl::DispatcherImpl>(protocolChannel, std::move(debuggerImpl));
    std::string msg = std::string() + R"({"id":0,"method":"Debugger.callFunctionOn","params":{
        "callFrameId":"0", "functionDeclaration":"test"}})";
    DispatchRequest request(msg);
    std::unique_ptr<PtBaseReturns> resultPtr;
    auto response = dispatcherImpl->CallFunctionOn(request, resultPtr);
    EXPECT_STREQ(response.GetMessage().c_str(), "Invalid callFrameId.");
    if (protocolChannel != nullptr) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SmartStepInto__001)
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
            "method":"Debugger.smartStepInto",
            "params":{}
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{"code":1,"message":"wrong params"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SmartStepInto__002)
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
            "method":"Debugger.smartStepInto",
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
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{"code":1,"message":"Can only perform operation while paused"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SmartStepInto__003)
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
            "method":"Debugger.smartStepInto",
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
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{"code":1,"message":"SetBreakpointByUrl: debugger agent is not enabled"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetNativeRange__001)
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
            "method":"Debugger.setNativeRange",
            "params":{
                "nativeRange":""
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{"code":1,"message":"wrong params"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_SetNativeRange__002)
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
            "method":"Debugger.setNativeRange",
            "params":{}
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_ResetSingleStepper__001)
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
            "method":"Debugger.resetSingleStepper",
            "params":{
                "resetSingleStepper":"test"
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{"code":1,"message":"wrong params"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_ResetSingleStepper__002)
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
            "method":"Debugger.resetSingleStepper",
            "params":{
                "resetSingleStepper":true
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_ClientDisconnect)
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
            "method":"Debugger.clientDisconnect",
            "params":{}
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(), R"()");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_CallFunctionOn__001)
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
            "method":"Debugger.callFunctionOn",
            "params":{
                "callFrameId":"1",
                "functionDeclaration":"test"
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{"code":1,"message":"Invalid callFrameId."}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, Dispatcher_Dispatch_CallFunctionOn__002)
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
            "method":"Debugger.callFunctionOn",
            "params":{
                "callFrameId":"0",
                "functionDeclaration":0
            }
        })";
    DispatchRequest request(msg);

    dispatcherImpl->Dispatch(request);
    EXPECT_STREQ(outStrForCallbackCheck.c_str(),
        R"({"id":0,"result":{"code":1,"message":"wrong params"}})");
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, NativeOutTest)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    std::unique_ptr<JSPtHooks> jspthooks = std::make_unique<JSPtHooks>(debuggerImpl.get());
    bool result1 = jspthooks->NativeOut();
    ASSERT_TRUE(!result1);
    bool value = true;
    debuggerImpl->SetNativeOutPause(value);
    bool result2 = jspthooks->NativeOut();
    ASSERT_TRUE(result2);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(DebuggerImplTest, NotifyNativeReturn__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);

    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    std::unique_ptr<PtJson> json = PtJson::CreateObject();
    json->Add("enabled", false);
    json->Add("mixedStackEnabled", false);
    std::unique_ptr<SetMixedDebugParams> params = SetMixedDebugParams::Create(*json);
    debuggerImpl->SetMixedDebugEnabled(*params);
    debuggerImpl->NotifyNativeReturn(nullptr);
    auto debugger = std::make_unique<DebuggerImplFriendTest>(debuggerImpl);
    ASSERT_TRUE(!debugger->GetMixStackEnabled());
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, NotifyNativeReturn__002)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);

    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    std::unique_ptr<PtJson> json = PtJson::CreateObject();
    json->Add("enabled", false);
    json->Add("mixedStackEnabled", true);
    std::unique_ptr<SetMixedDebugParams> params = SetMixedDebugParams::Create(*json);
    debuggerImpl->SetMixedDebugEnabled(*params);
    debuggerImpl->NotifyNativeReturn(nullptr);
    auto debugger = std::make_unique<DebuggerImplFriendTest>(debuggerImpl);
    ASSERT_TRUE(debugger->GetMixStackEnabled());
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, NotifyReturnNative__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);

    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    std::unique_ptr<PtJson> json = PtJson::CreateObject();
    json->Add("enabled", false);
    json->Add("mixedStackEnabled", true);
    std::unique_ptr<SetMixedDebugParams> params = SetMixedDebugParams::Create(*json);
    debuggerImpl->SetMixedDebugEnabled(*params);
    debuggerImpl->NotifyReturnNative();
    auto debugger = std::make_unique<DebuggerImplFriendTest>(debuggerImpl);
    ASSERT_TRUE(debugger->GetMixStackEnabled());
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, NotifyReturnNative__002)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);

    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    std::unique_ptr<PtJson> json = PtJson::CreateObject();
    json->Add("enabled", false);
    json->Add("mixedStackEnabled", false);
    std::unique_ptr<SetMixedDebugParams> params = SetMixedDebugParams::Create(*json);
    debuggerImpl->SetMixedDebugEnabled(*params);
    debuggerImpl->NotifyReturnNative();
    auto debugger = std::make_unique<DebuggerImplFriendTest>(debuggerImpl);
    ASSERT_TRUE(!debugger->GetMixStackEnabled());
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, NotifyNativeCalling__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);

    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    std::unique_ptr<PtJson> json = PtJson::CreateObject();
    json->Add("enabled", false);
    json->Add("mixedStackEnabled", false);
    std::unique_ptr<SetMixedDebugParams> params = SetMixedDebugParams::Create(*json);
    debuggerImpl->SetMixedDebugEnabled(*params);
    debuggerImpl->NotifyNativeCalling(nullptr);
    auto debugger = std::make_unique<DebuggerImplFriendTest>(debuggerImpl);
    ASSERT_TRUE(!debugger->GetMixStackEnabled());
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, NotifyNativeCalling__002)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);

    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    std::unique_ptr<PtJson> json = PtJson::CreateObject();
    json->Add("enabled", false);
    json->Add("mixedStackEnabled", true);
    std::unique_ptr<SetMixedDebugParams> params = SetMixedDebugParams::Create(*json);
    debuggerImpl->SetMixedDebugEnabled(*params);
    debuggerImpl->NotifyNativeCalling(nullptr);
    auto debugger = std::make_unique<DebuggerImplFriendTest>(debuggerImpl);
    ASSERT_TRUE(debugger->GetMixStackEnabled());
    if (protocolChannel) {
        delete protocolChannel;
        protocolChannel = nullptr;
    }
}

HWTEST_F_L0(DebuggerImplTest, CheckAndGenerateCondFunc__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto debugger = std::make_unique<DebuggerImplFriendTest>(debuggerImpl);
    std::vector<uint8_t> dest;
    debugger->CheckAndGenerateCondFunc("");
    ASSERT_TRUE(!debugger->DecodeAndCheckBase64("", dest));
    std::string msg = "UEFOREEAAAAAAAAADAACAEgBAAAAAAAAAAAAAAIAAAA8AAAAAQAA";
    msg += "AEQBAAAAAAARAAAAAEAAABEAAAAkQAAAMQAAAB8AAAASAEAAAIAAABsAAAAAgAAAHQAAAD//////////";
    debugger->CheckAndGenerateCondFunc(msg);
    ASSERT_TRUE(debugger->DecodeAndCheckBase64(msg, dest));
}

HWTEST_F_L0(DebuggerImplTest, ConvertToLocal__001)
{
    std::string outStrForCallbackCheck = "";
    std::function<void(const void*, const std::string &)> callback =
        [&outStrForCallbackCheck]([[maybe_unused]] const void *ptr, const std::string &inStrOfReply) {
            outStrForCallbackCheck = inStrOfReply;};
    ProtocolChannel *protocolChannel = new ProtocolHandler(callback, ecmaVm);
    auto runtimeImpl = std::make_unique<RuntimeImpl>(ecmaVm, protocolChannel);
    auto debuggerImpl = std::make_unique<DebuggerImpl>(ecmaVm, protocolChannel, runtimeImpl.get());
    auto debugger = std::make_unique<DebuggerImplFriendTest>(debuggerImpl);
    Local<JSValueRef> taggedValue = debugger->ConvertToLocal("");
    ASSERT_TRUE(!taggedValue.IsEmpty());
    taggedValue = debugger->ConvertToLocal("false");
    ASSERT_TRUE(!taggedValue.IsEmpty());
    taggedValue = debugger->ConvertToLocal("true");
    ASSERT_TRUE(!taggedValue.IsEmpty());
    taggedValue = debugger->ConvertToLocal("undefined");
    ASSERT_TRUE(!taggedValue.IsEmpty());
    taggedValue = debugger->ConvertToLocal("\"test\"");
    ASSERT_TRUE(!taggedValue.IsEmpty());
    taggedValue = debugger->ConvertToLocal("test");
    ASSERT_TRUE(taggedValue.IsEmpty());
    taggedValue = debugger->ConvertToLocal("1");
    ASSERT_TRUE(!taggedValue.IsEmpty());
}
}  // namespace panda::test
