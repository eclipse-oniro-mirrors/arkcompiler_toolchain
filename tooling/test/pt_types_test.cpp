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

HWTEST_F_L0(PtTypesTest, TraceConfigToJsonTest)
{
    TraceConfig treceConfig;
    std::vector<std::string> vct {"one", "two"};
    treceConfig.SetIncludedCategories(vct);
    treceConfig.SetExcludedCategories(vct);
    treceConfig.SetSyntheticDelays(vct);
    std::unique_ptr<PtJson> result = treceConfig.ToJson();
    ASSERT_TRUE(result);
}
HWTEST_F_L0(PtTypesTest, TraceConfigCreateTest)
{
    std::unique_ptr<PtJson> includedCategoriesArray = PtJson::CreateArray();
    std::unique_ptr<PtJson> excludedCategoriesArray = PtJson::CreateArray();
    std::unique_ptr<PtJson> syntheticDelaysArray = PtJson::CreateArray();
    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    std::string attribute = "test";
    std::unique_ptr<PtJson> ptJson = PtJson::CreateObject();
    ptJson->Add("recordMode", 0);
    ptJson->Add("enableSampling", attribute.c_str());
    ptJson->Add("enableSystrace", attribute.c_str());
    ptJson->Add("enableArgumentFilter", attribute.c_str());
    ptJson->Add("includedCategories", attribute.c_str());
    ptJson->Add("excludedCategories", attribute.c_str());
    ptJson->Add("syntheticDelays", attribute.c_str());
    ptJson->Add("memoryDumpConfig", attribute.c_str());
    std::unique_ptr<TraceConfig> traceConfig = TraceConfig::Create(*ptJson);
    ASSERT_TRUE(!traceConfig);
    object->Add("test", 0);
    std::unique_ptr<PtJson> ptJson1 = PtJson::CreateObject();
    ptJson1->Add("includedCategories", includedCategoriesArray);
    ptJson1->Add("recordMode", attribute.c_str());
    ptJson1->Add("excludedCategories", excludedCategoriesArray);
    ptJson1->Add("syntheticDelays", syntheticDelaysArray);
    ptJson1->Add("memoryDumpConfig", object);
    std::unique_ptr<TraceConfig> traceConfig1 = TraceConfig::Create(*ptJson1);
    ASSERT_TRUE(!traceConfig1);
    std::unique_ptr<PtJson> ptJson2 = PtJson::CreateObject();
    std::unique_ptr<TraceConfig> traceConfig2 = TraceConfig::Create(*ptJson2);
    ASSERT_TRUE(traceConfig2);
    std::unique_ptr<PtJson> ptJson3 = PtJson::CreateObject();
    std::unique_ptr<PtJson> memoryDumpConfig = PtJson::CreateArray();
    ptJson3->Add("memoryDumpConfig", std::move(memoryDumpConfig));
    std::unique_ptr<TraceConfig> traceConfig3 = TraceConfig::Create(*ptJson3);
    ASSERT_TRUE(!traceConfig3);
}

HWTEST_F_L0(PtTypesTest, ScriptTypeProfileCreateTest)
{
    std::unique_ptr<PtJson> ptJson = PtJson::CreateObject();
    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("offset", "test");
    array->Push(object);
    ptJson->Add("entries", array);
    std::unique_ptr<ScriptTypeProfile> profile = ScriptTypeProfile::Create(*ptJson);
    ASSERT_TRUE(!profile);
}

HWTEST_F_L0(PtTypesTest, TypeProfileEntryCreateTest)
{
    std::unique_ptr<PtJson> ptJson = PtJson::CreateObject();
    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("test", 0);
    array->Push(object);
    ptJson->Add("entries", array);
    std::unique_ptr<TypeProfileEntry> entry = TypeProfileEntry::Create(*ptJson);
    ASSERT_TRUE(!entry);
}

HWTEST_F_L0(PtTypesTest, ScriptCoverageCreateTest)
{
    std::unique_ptr<PtJson> ptJson = PtJson::CreateObject();
    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("functionName", 0);
    array->Push(object);
    ptJson->Add("functions", array);
    std::unique_ptr<ScriptCoverage> coverage = ScriptCoverage::Create(*ptJson);
    ASSERT_TRUE(!coverage);
}

HWTEST_F_L0(PtTypesTest, FunctionCoverageCreateTest)
{
    std::unique_ptr<PtJson> ptJson = PtJson::CreateObject();
    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("functionName", 0);
    array->Push(object);
    ptJson->Add("ranges", array);
    std::unique_ptr<FunctionCoverage> coverage = FunctionCoverage::Create(*ptJson);
    ASSERT_TRUE(!coverage);
}

HWTEST_F_L0(PtTypesTest, SymbolRemoteObjectTest)
{
    size_t size = 10;
    const char *data = "SymbolRemoteObjectTest";
    SymbolRemoteObject obj(ecmaVm, SymbolRef::New(ecmaVm, StringRef::NewFromUtf8(ecmaVm, (const char*)data, size)));
}

HWTEST_F_L0(PtTypesTest, DescriptionForObjectForDate)
{
    double input = 123456789;
    Local<DateRef> date = DateRef::New(ecmaVm, input);
    std::string description = ObjectRemoteObject::DescriptionForObject(ecmaVm, date);
    ASSERT_TRUE(description == "Fri Jan 02 1970 18:17:36 GMT+0800");
}

HWTEST_F_L0(PtTypesTest, DescriptionForObjectForPromise)
{
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(ecmaVm);
    Local<PromiseRef> promise = capability->GetPromise(ecmaVm);
    std::string description = ObjectRemoteObject::DescriptionForObject(ecmaVm, promise);
    ASSERT_TRUE(description == "Promise");
}

HWTEST_F_L0(PtTypesTest, PrivatePropertyDescriptorCreate)
{
    std::unique_ptr<PtJson> ptJson = PtJson::CreateObject();
    std::unique_ptr<PtJson> ptJson1 = PtJson::CreateObject();
    bool value = true;
    ptJson->Add("userCode", value);
    std::unique_ptr<PrivatePropertyDescriptor> result = PrivatePropertyDescriptor::Create(*ptJson);
    ASSERT_TRUE(result == nullptr);
    value = false;
    ptJson->Add("userCode1", value);
    result = PrivatePropertyDescriptor::Create(*ptJson);
    ASSERT_TRUE(result == nullptr);
    int32_t value1 = 100;
    ptJson1->Add("test", value1);
    std::unique_ptr<PrivatePropertyDescriptor> result1 = PrivatePropertyDescriptor::Create(*ptJson1);
    ASSERT_TRUE(result == nullptr);
    int64_t value2 = 0;
    ptJson1->Add("", value2);
    result1 = PrivatePropertyDescriptor::Create(*ptJson1);
    ASSERT_TRUE(result == nullptr);
    std::unique_ptr<PtJson> ptJson2 = PtJson::CreateObject();
    ptJson2->Add("name", value);
    std::unique_ptr<PrivatePropertyDescriptor> result2 = PrivatePropertyDescriptor::Create(*ptJson2);
    ASSERT_TRUE(result == nullptr);
    ptJson2->Add("value", value1);
    result2 = PrivatePropertyDescriptor::Create(*ptJson2);
    ASSERT_TRUE(result == nullptr);
    ptJson2->Add("get", value2);
    result2 = PrivatePropertyDescriptor::Create(*ptJson2);
    ASSERT_TRUE(result == nullptr);
    value2 = 10;
    ptJson2->Add("set", value2);
    result2 = PrivatePropertyDescriptor::Create(*ptJson2);
    ASSERT_TRUE(result == nullptr);
    std::unique_ptr<PtJson> ptJson3 = PtJson::CreateObject();
    ptJson3->Add("name", "test");
    ptJson3->Add("value", ptJson);
    ptJson3->Add("get", ptJson1);
    ptJson3->Add("set", ptJson2);
    std::unique_ptr<PrivatePropertyDescriptor> result3 = PrivatePropertyDescriptor::Create(*ptJson3);
    ASSERT_TRUE(result == nullptr);
}

HWTEST_F_L0(PtTypesTest, CallArgumentToJson)
{
    CallArgument callArgument;
    Local<JSValueRef> name = StringRef::NewFromUtf8(ecmaVm, "name");
    callArgument.SetValue(name);
    RemoteObjectId objectId(10);
    callArgument.SetObjectId(objectId);
    UnserializableValue value("test");
    callArgument.SetUnserializableValue(value);
    std::unique_ptr<PtJson> ret = callArgument.ToJson();
    ASSERT_TRUE(ret != nullptr);
    CallArgument callArgument1;
    RemoteObjectId objectId1(20);
    callArgument1.SetObjectId(objectId1);
    ret = callArgument1.ToJson();
    ASSERT_TRUE(ret != nullptr);
    CallArgument callArgument2;
    UnserializableValue value1("CallArgumentToJson");
    callArgument2.ToJson();
    ASSERT_TRUE(ret != nullptr);
}

HWTEST_F_L0(PtTypesTest, ScriptPositionToJson)
{
    ScriptPosition position;
    int32_t line = 10;
    position.SetLine(line);
    int32_t column = 8;
    position.SetColumn(column);
    std::unique_ptr<PtJson> ret = position.ToJson();
    ASSERT_TRUE(ret != nullptr);
}

HWTEST_F_L0(PtTypesTest, SearchMatchCreate)
{
    std::unique_ptr<PtJson> ptJson = PtJson::CreateObject();
    int32_t lineNumber = 10;
    ptJson->Add("lineNumber", lineNumber);
    std::string lineContent = "12";
    ptJson->Add("lineContent", lineContent.c_str());
    std::unique_ptr<SearchMatch> ret = SearchMatch::Create(*ptJson);
    ASSERT_TRUE(ret != nullptr);

    std::unique_ptr<PtJson> ptJson1 = PtJson::CreateObject();
    int32_t lineNumber1 = 20;
    ptJson1->Add("lineNumber", lineNumber1);
    ret = SearchMatch::Create(*ptJson1);
    ASSERT_TRUE(ret == nullptr);

    std::unique_ptr<PtJson> ptJson2 = PtJson::CreateObject();
    int32_t lineNumber2 = 14;
    std::string lineContent2 = "12";
    ptJson1->Add("lineNumber1", lineNumber2);
    ptJson1->Add("lineContent2", lineContent2.c_str());
    ret = SearchMatch::Create(*ptJson1);
    ASSERT_TRUE(ret == nullptr);
}

HWTEST_F_L0(PtTypesTest, LocationRangeCreate)
{
    std::unique_ptr<PtJson> ptJson = PtJson::CreateObject();
    std::unique_ptr<PtJson> start = PtJson::CreateObject();
    std::unique_ptr<PtJson> end = PtJson::CreateObject();
    std::string scriptId = "12";
    int32_t lineNumber = 1;
    int32_t columnNumber = 2;
    start->Add("lineNumber", lineNumber);
    start->Add("columnNumber", columnNumber);
    end->Add("lineNumber", lineNumber + 1);
    end->Add("columnNumber", columnNumber + 2);
    ptJson->Add("scriptId", scriptId.c_str());
    ptJson->Add("start", start);
    ptJson->Add("end", end);
    std::unique_ptr<LocationRange> ret = LocationRange::Create(*ptJson);
    ASSERT_TRUE(ret != nullptr);
}

HWTEST_F_L0(PtTypesTest, LocationRangeCreateSwitch)
{
    std::unique_ptr<PtJson> ptJson = PtJson::CreateObject();
    std::unique_ptr<PtJson> start = PtJson::CreateObject();
    std::unique_ptr<PtJson> end = PtJson::CreateObject();
    std::string scriptId = "12";
    int32_t lineNumber = 1;
    int32_t columnNumber = 2;
    start->Add("lineNumber1", lineNumber);
    start->Add("columnNumber1", columnNumber);
    end->Add("lineNumber1", lineNumber + 1);
    end->Add("columnNumber1", columnNumber + 2);
    ptJson->Add("scriptId1", scriptId.c_str());
    ptJson->Add("start", start);
    ptJson->Add("end", end);
    std::unique_ptr<LocationRange> ret = LocationRange::Create(*ptJson);
    ASSERT_TRUE(ret == nullptr);
}

HWTEST_F_L0(PtTypesTest, LocationRangeCreateSwitchFail)
{
    std::unique_ptr<PtJson> ptJson = PtJson::CreateObject();
    std::unique_ptr<PtJson> start = PtJson::CreateObject();
    std::unique_ptr<PtJson> end = PtJson::CreateObject();
    std::string scriptId = "12";
    int32_t lineNumber = 1;
    int32_t columnNumber = 2;
    start->Add("lineNumber1", lineNumber);
    start->Add("columnNumber1", columnNumber);
    end->Add("lineNumber1", lineNumber + 1);
    end->Add("columnNumber1", columnNumber + 2);
    ptJson->Add("scriptId1", scriptId.c_str());
    ptJson->Add("start1", start);
    ptJson->Add("end1", end);
    std::unique_ptr<LocationRange> ret = LocationRange::Create(*ptJson);
    ASSERT_TRUE(ret == nullptr);
}

HWTEST_F_L0(PtTypesTest, RuntimeCallFrameCreate)
{
    std::unique_ptr<PtJson> ptJson = PtJson::CreateObject();
    std::string functionName = "RuntimeCallFrameCreate";
    std::string scriptId = "scriptId";
    std::string url = "url";
    int32_t lineNumber = 2;
    int32_t columnNumber = 10;
    ptJson->Add("functionName", functionName.c_str());
    ptJson->Add("scriptId", scriptId.c_str());
    ptJson->Add("url", url.c_str());
    ptJson->Add("lineNumber", lineNumber);
    ptJson->Add("columnNumber", columnNumber);
    std::unique_ptr<RuntimeCallFrame> ret = RuntimeCallFrame::Create(*ptJson);
    ASSERT_TRUE(ret != nullptr);
}

HWTEST_F_L0(PtTypesTest, RuntimeCallFrameCreateFailSwitch)
{
    std::unique_ptr<PtJson> ptJson = PtJson::CreateObject();
    std::string functionName = "RuntimeCallFrameCreate";
    std::string scriptId = "scriptId";
    std::string url = "url";
    int32_t lineNumber = 2;
    int32_t columnNumber = 10;
    ptJson->Add("functionName1", functionName.c_str());
    ptJson->Add("scriptId1", scriptId.c_str());
    ptJson->Add("url1", url.c_str());
    ptJson->Add("lineNumber1", lineNumber);
    ptJson->Add("columnNumber1", columnNumber);
    std::unique_ptr<RuntimeCallFrame> ret = RuntimeCallFrame::Create(*ptJson);
    ASSERT_TRUE(ret == nullptr);
}

HWTEST_F_L0(PtTypesTest, SamplingHeapProfileNodeCreate)
{
    std::unique_ptr<PtJson> ptJson = PtJson::CreateObject();
    std::unique_ptr<PtJson> callFrame = PtJson::CreateObject();
    callFrame->Add("callFrame", "test");
    ptJson->Add("callFrame", callFrame);
    std::unique_ptr<SamplingHeapProfileNode> ret = SamplingHeapProfileNode::Create(*ptJson);
    ASSERT_TRUE(ret == nullptr);
}

HWTEST_F_L0(PtTypesTest, SamplingHeapProfileCreate)
{
    std::unique_ptr<PtJson> ptJson = PtJson::CreateObject();
    std::unique_ptr<PtJson> head = PtJson::CreateObject();
    head->Add("head", "headTest");
    ptJson->Add("head", head);
    std::unique_ptr<SamplingHeapProfile> ret = SamplingHeapProfile::Create(*ptJson);
    ASSERT_TRUE(ret == nullptr);
}

HWTEST_F_L0(PtTypesTest, ExceptionDetailsCreate)
{
    std::unique_ptr<PtJson> ptJson = PtJson::CreateObject();
    std::unique_ptr<PtJson> exception = PtJson::CreateObject();
    exception->Add("exception", "test");
    ptJson->Add("exception", exception);
    std::unique_ptr<ExceptionDetails> ret = ExceptionDetails::Create(*ptJson);
    ASSERT_TRUE(ret == nullptr);
}


HWTEST_F_L0(PtTypesTest, ProfileToJsonTest)
{
    Profile profile;
    std::vector<int32_t> samples {1, 2, 3}; // 1:element 2:element 3:element
    std::vector<int32_t> timeDeltas {1, 2, 3}; // 1:element 2:element 3:element
    profile.SetSamples(samples);
    profile.SetTimeDeltas(timeDeltas);
    std::unique_ptr<PtJson> result = profile.ToJson();
    ASSERT_TRUE(result);
}

HWTEST_F_L0(PtTypesTest, ProfileFromProfileInfoTest)
{
    ProfileInfo info;
    info.samples.push_back(1);
    info.samples.push_back(2);
    info.timeDeltas.push_back(1);
    info.timeDeltas.push_back(2);
    std::unique_ptr<Profile> result = Profile::FromProfileInfo(info);
    ASSERT_TRUE(result);
}

HWTEST_F_L0(PtTypesTest, ProfileNodeToJsonTest)
{
    std::unique_ptr<PtJson> ptJson = PtJson::CreateObject();
    ProfileNode node;
    std::vector<int32_t> children {0, 1, 2, 3}; // 1: element 2:element, 3:element
    std::unique_ptr<PositionTickInfo> positionTickInfo = std::make_unique<PositionTickInfo>();
    std::unique_ptr<RuntimeCallFrame> callFrame = std::make_unique<RuntimeCallFrame>();
    positionTickInfo->SetLine(123);
    positionTickInfo->SetTicks(123);
    std::vector<std::unique_ptr<PositionTickInfo>> positionTicks;
    positionTicks.push_back(std::move(positionTickInfo));
    node.SetChildren(children);
    node.SetPositionTicks(std::move(positionTicks));
    node.SetCallFrame(std::move(callFrame));
    std::unique_ptr<PtJson> result = node.ToJson();
    ASSERT_TRUE(result);
    ProfileNode node1;
    std::unique_ptr<RuntimeCallFrame> callFrame1 = std::make_unique<RuntimeCallFrame>();
    node1.SetCallFrame(std::move(callFrame1));
    std::unique_ptr<PtJson> result1 = node1.ToJson();
    ASSERT_TRUE(result1);
}

HWTEST_F_L0(PtTypesTest, ProfileNodeFromCpuProfileNodeTest)
{
    CpuProfileNode cpuProfileNode;
    cpuProfileNode.children.push_back(0);
    cpuProfileNode.children.push_back(1); // 1: element
    std::unique_ptr<ProfileNode> result = ProfileNode::FromCpuProfileNode(cpuProfileNode);
    ASSERT_TRUE(result);
}

HWTEST_F_L0(PtTypesTest, PrivatePropertyDescriptorToJsonTest)
{
    PrivatePropertyDescriptor descriptor;
    std::unique_ptr<RemoteObject> value = std::make_unique<RemoteObject>();
    std::unique_ptr<RemoteObject> get = std::make_unique<RemoteObject>();
    std::unique_ptr<RemoteObject> set = std::make_unique<RemoteObject>();
    descriptor.SetValue(std::move(value));
    descriptor.SetGet(std::move(get));
    descriptor.SetSet(std::move(set));
    std::unique_ptr<PtJson> result = descriptor.ToJson();
    ASSERT_TRUE(result);
}

HWTEST_F_L0(PtTypesTest, PropertyDescriptorFromPropertyTest)
{
    Local<SymbolRef> symbol = SymbolRef::New(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "test"));
    PropertyAttribute property;
    property.SetGetter(StringRef::NewFromUtf8(ecmaVm, "test"));
    property.SetSetter(StringRef::NewFromUtf8(ecmaVm, "test"));
    std::unique_ptr<panda::ecmascript::tooling::PropertyDescriptor> result =
        panda::ecmascript::tooling::PropertyDescriptor::FromProperty(ecmaVm, symbol, property);
    ASSERT_TRUE(result);
}

HWTEST_F_L0(PtTypesTest, PtEventsGetReasonString)
{
    PauseReason reason = static_cast<PauseReason>(50);
    std::string ret = Paused::GetReasonString(reason);
    ASSERT_TRUE(ret.empty());
}

HWTEST_F_L0(PtTypesTest, LocationRangeToJsonTest)
{
    LocationRange range;
    std::unique_ptr<ScriptPosition> start = std::make_unique<ScriptPosition>();
    std::unique_ptr<ScriptPosition> end = std::make_unique<ScriptPosition>();
    range.SetStart(std::move(start));
    range.SetEnd(std::move(end));
    std::unique_ptr<PtJson> result = range.ToJson();
    ASSERT_TRUE(result);
}

HWTEST_F_L0(PtTypesTest, BreakLocationToJsonTest)
{
    BreakLocation location;
    location.SetColumn(0);
    location.SetType("test");
    std::unique_ptr<PtJson> result = location.ToJson();
    ASSERT_TRUE(result);
}

HWTEST_F_L0(PtTypesTest, CallFrameCreateTest)
{
    std::unique_ptr<PtJson> ptJson = PtJson::CreateObject();
    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("type", 0);
    array->Push(object);
    ptJson->Add("scopeChain", array);
    std::unique_ptr<CallFrame> result = CallFrame::Create(*ptJson);
    ASSERT_TRUE(!result);
}
}  // namespace panda::test