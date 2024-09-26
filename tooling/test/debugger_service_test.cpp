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

#include "ecmascript/tests/test_helper.h"
#include "ecmascript/ecma_vm.h"
#include "debugger_service.h"
#include "protocol_handler.h"
#include "ecmascript/debugger/js_debugger_manager.h"
using namespace panda::ecmascript;
using namespace panda::ecmascript::tooling;

namespace panda::test {
class DebuggerServiceTest : public testing::Test {
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

HWTEST_F_L0(DebuggerServiceTest, InitializeDebuggerTest)
{
    ProtocolHandler *handler = ecmaVm->GetJsDebuggerManager()->GetDebuggerHandler();
    ASSERT_TRUE(handler == nullptr);
    InitializeDebugger(ecmaVm, nullptr);
    handler = ecmaVm->GetJsDebuggerManager()->GetDebuggerHandler();
    ASSERT_TRUE(handler != nullptr);
    InitializeDebugger(ecmaVm, nullptr);
    ASSERT_TRUE(handler != nullptr);
    EcmaVM *vm = nullptr;
    InitializeDebugger(vm, nullptr);
    ASSERT_TRUE(handler != nullptr);
}

HWTEST_F_L0(DebuggerServiceTest, UninitializeDebuggerTest)
{
    UninitializeDebugger(ecmaVm);
    InitializeDebugger(ecmaVm, nullptr);
    ProtocolHandler *handler = ecmaVm->GetJsDebuggerManager()->GetDebuggerHandler();
    ASSERT_TRUE(handler != nullptr);
    UninitializeDebugger(ecmaVm);
    handler = ecmaVm->GetJsDebuggerManager()->GetDebuggerHandler();
    ASSERT_TRUE(handler == nullptr);
    EcmaVM *vm = nullptr;
    UninitializeDebugger(vm);
    ASSERT_TRUE(handler == nullptr);
}

HWTEST_F_L0(DebuggerServiceTest, OnMessageTest)
{
    ProtocolHandler *handler = ecmaVm->GetJsDebuggerManager()->GetDebuggerHandler();
    OnMessage(ecmaVm, "");
    ASSERT_TRUE(handler == nullptr);
    std::string result = "";
    std::function<void(const void*, const std::string &)> callback =
        [&result]([[maybe_unused]] const void *ptr, const std::string &temp) { result = temp; };
    InitializeDebugger(ecmaVm, callback);
    std::string msg = std::string() + R"({"id":0,"method":"Tracing.Test","params":{}})";
    OnMessage(ecmaVm, msg + "");
    ProcessMessage(ecmaVm);
    ASSERT_TRUE(result.find("Unknown method: Test") != std::string::npos);
    EcmaVM *vm = nullptr;
    OnMessage(vm, "");
    ASSERT_TRUE(result.find("Unknown method: Test") != std::string::npos);
}

HWTEST_F_L0(DebuggerServiceTest, WaitForDebuggerTest)
{
    ProtocolHandler *handler = ecmaVm->GetJsDebuggerManager()->GetDebuggerHandler();
    WaitForDebugger(ecmaVm);
    ASSERT_TRUE(handler == nullptr);
}

HWTEST_F_L0(DebuggerServiceTest, ProcessMessageTest)
{
    ProtocolHandler *handler = ecmaVm->GetJsDebuggerManager()->GetDebuggerHandler();
    ProcessMessage(ecmaVm);
    ASSERT_TRUE(handler == nullptr);
    std::string result = "";
    std::function<void(const void*, const std::string &)> callback =
        [&result]([[maybe_unused]] const void *ptr, const std::string &temp) { result = temp; };
    InitializeDebugger(ecmaVm, callback);
    std::string msg = std::string() + R"({"id":0,"method":"Tracing.end","params":{}})";
    OnMessage(ecmaVm, msg + "");
    ProcessMessage(ecmaVm);
    ASSERT_TRUE(result.find("\"id\":0,") != std::string::npos);
    EcmaVM *vm = nullptr;
    ProcessMessage(vm);
    ASSERT_TRUE(result.find("Stop is failure") != std::string::npos);
}

HWTEST_F_L0(DebuggerServiceTest, GetDispatchStatusTest)
{
    ProtocolHandler::DispatchStatus status = ProtocolHandler::DispatchStatus(GetDispatchStatus(ecmaVm));
    ASSERT_TRUE(status == ProtocolHandler::DispatchStatus::UNKNOWN);
    std::string result = "";
    std::function<void(const void*, const std::string &)> callback =
        [&result]([[maybe_unused]] const void *ptr, const std::string &temp) { result = temp; };
    InitializeDebugger(ecmaVm, callback);
    status = ProtocolHandler::DispatchStatus(GetDispatchStatus(ecmaVm));
    ASSERT_TRUE(status == ProtocolHandler::DispatchStatus::DISPATCHED);
    EcmaVM *vm = nullptr;
    int32_t GetDispatchStatusTest = GetDispatchStatus(vm);
    ASSERT_TRUE(GetDispatchStatusTest == 0);
}
}  // namespace panda::test