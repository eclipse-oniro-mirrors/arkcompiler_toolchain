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

#include "base/pt_types.h"
#include "ecmascript/tests/test_helper.h"
#include "protocol_handler.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::tooling;

namespace panda::test {
class PtTypesTest : public testing::Test {
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

HWTEST_F_L0(PtTypesTest, BreakpointDetailsToString)
{
    BreakpointDetails input;
    BreakpointId result = BreakpointDetails::ToString(input);
    ASSERT_TRUE(result == "id:0:0:");
}

HWTEST_F_L0(PtTypesTest, BreakpointDetailsParseBreakpointId)
{
    BreakpointId input = "";
    BreakpointDetails detail;
    bool result = BreakpointDetails::ParseBreakpointId(input, &detail);
    ASSERT_TRUE(!result);
    input = "id:0";
    result = BreakpointDetails::ParseBreakpointId(input, &detail);
    ASSERT_TRUE(!result);
    input = "id:0:0";
    result = BreakpointDetails::ParseBreakpointId(input, &detail);
    ASSERT_TRUE(!result);
    input = "id:0:0:url";
    result = BreakpointDetails::ParseBreakpointId(input, &detail);
    ASSERT_TRUE(result);
}

HWTEST_F_L0(PtTypesTest, TypeNameValid)
{
    std::string type = "object";
    bool result = RemoteObject::TypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "function";
    result = RemoteObject::TypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "undefined";
    result = RemoteObject::TypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "string";
    result = RemoteObject::TypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "number";
    result = RemoteObject::TypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "boolean";
    result = RemoteObject::TypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "symbol";
    result = RemoteObject::TypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "bigint";
    result = RemoteObject::TypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "wasm";
    result = RemoteObject::TypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "int";
    result = RemoteObject::TypeName::Valid(type);
    ASSERT_TRUE(!result);
}

HWTEST_F_L0(PtTypesTest, SubTypeNameValid)
{
    std::string type = "array";
    bool result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "null";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "node";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "regexp";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "map";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "set";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "weakmap";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "iterator";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "generator";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "error";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "proxy";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "promise";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "typedarray";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "arraybuffer";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "dataview";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "i32";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "i64";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "f32";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "f64";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "v128";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "externref";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(result);
    type = "int";
    result = RemoteObject::SubTypeName::Valid(type);
    ASSERT_TRUE(!result);
}

HWTEST_F_L0(PtTypesTest, ExceptionDetailsGetException)
{
    ExceptionDetails exception;
    RemoteObject* result = exception.GetException();
    ASSERT_TRUE(result == nullptr);
}

HWTEST_F_L0(PtTypesTest, InternalPropertyDescriptorGetValue)
{
    InternalPropertyDescriptor descriptor;
    RemoteObject* result = descriptor.GetValue();
    ASSERT_TRUE(result == nullptr);
}

HWTEST_F_L0(PtTypesTest, PrivatePropertyDescriptor)
{
    PrivatePropertyDescriptor descriptor;
    ASSERT_TRUE(descriptor.GetName() == "");
}

HWTEST_F_L0(PtTypesTest, PropertyDescriptorGetValue)
{
    panda::ecmascript::tooling::PropertyDescriptor descriptor;
    RemoteObject* result = descriptor.GetValue();
    ASSERT_TRUE(result == nullptr);
    result = descriptor.GetGet();
    ASSERT_TRUE(result == nullptr);
    result = descriptor.GetSet();
    ASSERT_TRUE(result == nullptr);
    result = descriptor.GetSymbol();
    ASSERT_TRUE(result == nullptr);
    bool res = descriptor.HasSymbol();
    ASSERT_TRUE(!res);
}

HWTEST_F_L0(PtTypesTest, BreakLocationTypeValid)
{
    BreakLocation::Type type;
    bool result = type.Valid("debuggerStatement");
    ASSERT_TRUE(result);
    result = type.Valid("call");
    ASSERT_TRUE(result);
    result = type.Valid("return");
    ASSERT_TRUE(result);
    result = type.Valid("test");
    ASSERT_TRUE(!result);
}

HWTEST_F_L0(PtTypesTest, ScopeTypeValid)
{
    Scope::Type type;
    bool result = type.Valid("global");
    ASSERT_TRUE(result);
    result = type.Valid("local");
    ASSERT_TRUE(result);
    result = type.Valid("with");
    ASSERT_TRUE(result);
    result = type.Valid("closure");
    ASSERT_TRUE(result);
    result = type.Valid("catch");
    ASSERT_TRUE(result);
    result = type.Valid("block");
    ASSERT_TRUE(result);
    result = type.Valid("script");
    ASSERT_TRUE(result);
    result = type.Valid("eval");
    ASSERT_TRUE(result);
    result = type.Valid("module");
    ASSERT_TRUE(result);
    result = type.Valid("wasm-expression-stack");
    ASSERT_TRUE(result);
    result = type.Valid("test");
    ASSERT_TRUE(!result);
}

HWTEST_F_L0(PtTypesTest, CallFrameGetFunctionLocation)
{
    CallFrame callFrame;
    Location *location = callFrame.GetFunctionLocation();
    ASSERT_TRUE(location == nullptr);
    RemoteObject *result = callFrame.GetReturnValue();
    ASSERT_TRUE(result == nullptr);
}

HWTEST_F_L0(PtTypesTest, MemoryDumpLevelOfDetailValuesValid)
{
    bool result = MemoryDumpLevelOfDetailValues::Valid("background");
    ASSERT_TRUE(result);
    result = MemoryDumpLevelOfDetailValues::Valid("light");
    ASSERT_TRUE(result);
    result = MemoryDumpLevelOfDetailValues::Valid("detailed");
    ASSERT_TRUE(result);
    result = MemoryDumpLevelOfDetailValues::Valid("test");
    ASSERT_TRUE(!result);
}

HWTEST_F_L0(PtTypesTest, TraceConfigRecordModeValuesValid)
{
    bool result = TraceConfig::RecordModeValues::Valid("recordUntilFull");
    ASSERT_TRUE(result);
    result = TraceConfig::RecordModeValues::Valid("recordContinuously");
    ASSERT_TRUE(result);
    result = TraceConfig::RecordModeValues::Valid("recordAsMuchAsPossible");
    ASSERT_TRUE(result);
    result = TraceConfig::RecordModeValues::Valid("echoToConsole");
    ASSERT_TRUE(result);
    result = TraceConfig::RecordModeValues::Valid("test");
    ASSERT_TRUE(!result);
}

HWTEST_F_L0(PtTypesTest, TracingBackendValues)
{
    bool result = TracingBackendValues::Valid("auto");
    ASSERT_TRUE(result);
    result = TracingBackendValues::Valid("chrome");
    ASSERT_TRUE(result);
    result = TracingBackendValues::Valid("system");
    ASSERT_TRUE(result);
    result = TracingBackendValues::Valid("test");
    ASSERT_TRUE(!result);
}
}  // namespace panda::test