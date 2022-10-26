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

#ifndef ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_VARIABLE_TEST_H
#define ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_VARIABLE_TEST_H

#include "test/utils/test_util.h"

namespace panda::ecmascript::tooling::test {
std::string g_pandaFile = DEBUGGER_ABC_DIR "variable.abc";
std::string g_sourceFile = DEBUGGER_JS_DIR "variable.js";

class JsVariableTest : public TestEvents {
public:
    JsVariableTest()
    {
        breakpoint = [this](const JSPtLocation &location) {
            ASSERT_TRUE(location.GetMethodId().IsValid());
            ASSERT_LOCATION_EQ(location, location_);
            ++breakpointCounter_;
            debugger_->NotifyPaused(location, PauseReason::INSTRUMENTATION);
            TestUtil::SuspendUntilContinue(DebugEvent::BREAKPOINT, location);
            return true;
        };

        loadModule = [this](std::string_view moduleName) {
            static_cast<JsVariableTestChannel *>(channel_)->Initial(vm_, runtime_);
            // 133: breakpointer line
            location_ = TestUtil::GetLocation(g_sourceFile.c_str(), 133, 0, g_pandaFile.c_str());
            ASSERT_TRUE(location_.GetMethodId().IsValid());
            TestUtil::SuspendUntilContinue(DebugEvent::LOAD_MODULE);
            ASSERT_EQ(moduleName, g_pandaFile);
            ASSERT_TRUE(debugger_->NotifyScriptParsed(0, g_pandaFile));
            auto condFuncRef = FunctionRef::Undefined(vm_);
            auto ret = debugInterface_->SetBreakpoint(location_, condFuncRef);
            ASSERT_TRUE(ret);
            return true;
        };

        scenario = [this]() {
            TestUtil::WaitForLoadModule();
            TestUtil::Continue();
            TestUtil::WaitForBreakpoint(location_);
            TestUtil::Continue();
            auto ret = debugInterface_->RemoveBreakpoint(location_);
            ASSERT_TRUE(ret);
            ASSERT_EXITED();
            return true;
        };

        vmDeath = [this]() {
            ASSERT_EQ(breakpointCounter_, 1U);  // 1: break point counter
            return true;
        };

        channel_ = new JsVariableTestChannel();
    }

    std::pair<std::string, std::string> GetEntryPoint() override
    {
        return {g_pandaFile, entryPoint_};
    }
    ~JsVariableTest()
    {
        delete channel_;
        channel_ = nullptr;
    }

private:
    class JsVariableTestChannel : public TestChannel {
    public:
        JsVariableTestChannel() = default;
        ~JsVariableTestChannel() = default;
        void Initial(const EcmaVM *vm, RuntimeImpl *runtime)
        {
            vm_ = vm;
            runtime_ = runtime;
        }

        void SendNotification(const PtBaseEvents &events) override
        {
            const static std::vector<std::function<bool(const PtBaseEvents &events)>> eventList = {
                [](const PtBaseEvents &events) -> bool {
                    auto parsed = static_cast<const ScriptParsed *>(&events);
                    std::string str = parsed->ToJson()->Stringify();
                    std::cout << "JsVariableTestChannel: SendNotification 0:\n" << str << std::endl;

                    ASSERT_EQ(parsed->GetName(), "Debugger.scriptParsed");
                    ASSERT_EQ(parsed->GetUrl(), g_sourceFile);
                    return true;
                },
                [this](const PtBaseEvents &events) -> bool {
                    auto paused = static_cast<const Paused *>(&events);
                    std::string str = paused->ToJson()->Stringify();
                    std::cout << "JsVariableTestChannel: SendNotification 1:\n" << str << std::endl;

                    ASSERT_EQ(paused->GetName(), "Debugger.paused");
                    auto frame = paused->GetCallFrames()->at(0).get();
                    ASSERT_EQ(frame->GetFunctionName(), "foo");
                    auto scopes = frame->GetScopeChain();
                    ASSERT_EQ(scopes->size(), 2U);  // 2: contain local and global
                    for (uint32_t i = 0; i < scopes->size(); i++) {
                        auto scope = scopes->at(i).get();
                        if (scope->GetType() != Scope::Type::Local()) {
                            continue;
                        }
                        auto localId = scope->GetObject()->GetObjectId();
                        GetPropertiesParams params;
                        params.SetObjectId(localId).SetOwnProperties(true);
                        std::vector<std::unique_ptr<PropertyDescriptor>> outPropertyDesc;
                        runtime_->GetProperties(params, &outPropertyDesc, {}, {}, {});
                        for (const auto &property : outPropertyDesc) {
                            std::cout << "=====================================" << std::endl;
                            std::cout << property->GetName() << std::endl;
                            auto value = property->GetValue();
                            std::vector<std::string> infos;
                            PushValueInfo(value, infos);
                            if (value->GetType() == RemoteObject::TypeName::Object) {
                                std::vector<std::unique_ptr<PropertyDescriptor>> outPropertyDescInner;
                                ASSERT_TRUE(value->HasObjectId());
                                params.SetObjectId(value->GetObjectId()).SetOwnProperties(true);
                                runtime_->GetProperties(params, &outPropertyDescInner, {}, {}, {});
                                if (outPropertyDescInner.size() == 0) {
                                    infos.push_back("none");
                                }
                                for (const auto &propertyInner : outPropertyDescInner) {
                                    std::cout << "###########################################" << std::endl;
                                    std::cout << propertyInner->GetName() << std::endl;
                                    infos.push_back(propertyInner->GetName());
                                    auto innerValue = propertyInner->GetValue();
                                    PushValueInfo(innerValue, infos);
                                }
                            }
                            ASSERT_EQ(infos.size(), variableMap_.at(property->GetName()).size());
                            for (uint32_t j = 0; j < infos.size(); j++) {
                                ASSERT_EQ(infos[j], variableMap_.at(property->GetName())[j]);
                            }
                        }
                    }
                    return true;
                }
            };

            ASSERT_TRUE(eventList[index_](events));
            index_++;
        }

    private:
        NO_COPY_SEMANTIC(JsVariableTestChannel);
        NO_MOVE_SEMANTIC(JsVariableTestChannel);

        void PushValueInfo(RemoteObject *value, std::vector<std::string> &infos)
        {
            std::cout << "type: " << value->GetType() << std::endl;
            infos.push_back(value->GetType());
            if (value->HasObjectId()) {
                std::cout << "id: " << value->GetObjectId() << std::endl;
            }
            if (value->HasSubType()) {
                std::cout << "sub type: " << value->GetSubType() << std::endl;
                infos.push_back(value->GetSubType());
            }
            if (value->HasClassName()) {
                std::cout << "class name: " << value->GetClassName() << std::endl;
                infos.push_back(value->GetClassName());
            }
            if (value->HasDescription()) {
                std::cout << "desc: " << value->GetDescription() << std::endl;
                infos.push_back(value->GetDescription());
            }
            if (value->HasValue()) {
                std::cout << "type: " <<
                    value->GetValue()->Typeof(vm_)->ToString() << std::endl;
                std::cout << "tostring: " <<
                    value->GetValue()->ToString(vm_)->ToString() << std::endl;
                infos.push_back(value->GetValue()->ToString(vm_)->ToString());
            }
        }

        /*
        * Expect map type: map<name, value list>
        * value list (optional):
        *    type
        *    subType
        *    className
        *    description
        *    value tostring
        *
        * if is object value, will push back key and value.
        *
        * for example:
        * var abc = 1
        *     { "abc", { "number", "1", "1" } }
        * var obj = { "key": "2" }
        *     { "obj0", { "object", "Object", "Object", "[object Object]", "key", "string", "2", "2" } }
        */
        const std::map<std::string, std::vector<std::string>> variableMap_ = {
            { "nop", { "undefined" } },
            { "foo", { "function", "Function", "function foo( { [js code] }",
                        "Cannot get source code of funtion"} },
            { "string0", { "string", "helloworld", "helloworld" } },
            { "boolean0", { "object", "Object", "Boolean{[[PrimitiveValue]]: false}", "false", "[[PrimitiveValue]]",
                            "boolean", "false", "false" } },
            { "number0", { "number", "1", "1" } },
            { "obj0", { "object", "Object", "Object", "[object Object]",
                        "key0", "string", "value0", "value0",
                        "key1", "number", "100", "100" } },
            { "arraybuffer0", { "object", "arraybuffer", "Arraybuffer", "ArrayBuffer(10)", "[object ArrayBuffer]",
                                "[[Int8Array]]", "object", "Object", "Int8Array(10)", "0,0,0,0,0,0,0,0,0,0",
                                "[[Uint8Array]]", "object", "Object", "Uint8Array(10)", "0,0,0,0,0,0,0,0,0,0",
                                "[[Uint8ClampedArray]]", "object", "Object", "Object", "0,0,0,0,0,0,0,0,0,0",
                                "[[Int16Array]]", "object", "Object", "Int16Array(5)", "0,0,0,0,0", "[[Uint16Array]]",
                                "object", "Object", "Object", "0,0,0,0,0" } },
            { "function0", { "function", "Function", "function function0( { [js code] }",
                             "Cannot get source code of funtion" } },
            { "generator0", { "function", "Generator", "function* generator0( { [js code] }",
                              "Cannot get source code of funtion" } },
            { "map0", { "object", "map", "Map", "Map(0)", "[object Map]", "size", "number", "0", "0", "[[Entries]]",
                        "object", "array", "Array", "Array(0)", "" } },
            { "set0", { "object", "set", "Set", "Set(0)", "[object Set]", "size", "number", "0", "0", "[[Entries]]",
                        "object", "array", "Array", "Array(0)", "" } },
            { "undefined0", { "undefined" } },
            { "array0", { "object", "array", "Array", "Array(2)", "Apple,Banana", "0", "string", "Apple", "Apple",
                          "1", "string", "Banana", "Banana", "length", "number", "2", "2" } },
            { "date0", { "object", "date", "Date", "Sun Dec 17 1995 03:24:00 GMT+0800",
                         "Sun Dec 17 1995 03:24:00 GMT+0800", "none" } },
            { "regexp0", { "object", "regexp", "RegExp", "/^\\d+\\.\\d+$/i", "/^\\d+\\.\\d+$/i", "global", "boolean",
                           "false", "false", "ignoreCase", "boolean", "true", "true", "multiline", "boolean", "false",
                           "false", "dotAll", "boolean", "false", "false", "hasIndices", "boolean", "false", "false",
                           "unicode", "boolean", "false", "false", "sticky", "boolean", "false", "false", "flags",
                           "string", "i", "i", "source", "string", "^\\d+\\.\\d+$", "^\\d+\\.\\d+$", "lastIndex",
                           "number", "0", "0" } },
            { "uint8array0", { "object", "Object", "Uint8Array(10)", "0,0,0,0,0,0,0,0,0,0", "         0", "number",
                               "0", "0", "         1", "number", "0", "0", "         2", "number", "0", "0",
                               "         3", "number", "0", "0", "         4", "number", "0", "0", "         5",
                               "number", "0", "0", "         6", "number", "0", "0", "         7", "number", "0", "0",
                               "         8", "number", "0", "0", "         9", "number", "0", "0" } },
            { "dataview0", { "object", "Object", "Object", "[object DataView]", "none" } },
            { "bigint0", { "bigint", "999n", "999" } },
            { "set1", { "object", "set", "Set", "Set(1) {1}", "[object Set]", "size", "number", "1", "1",
                        "[[Entries]]", "object", "array", "Array", "Array(1)", "1" } },
            { "set2", { "object", "set", "Set", "Set(7) {'h', 'e', 'l', 'o', 'w', ...}", "[object Set]", "size",
                        "number", "7", "7", "[[Entries]]", "object", "array", "Array", "Array(7)",
                        "h,e,l,o,w,r,d" } },
            { "set3", { "object", "set", "Set", "Set(1) {Object}", "[object Set]", "size", "number", "1", "1",
                        "[[Entries]]", "object", "array", "Array", "Array(1)", "[object Object]" } },
            { "set4", { "object", "set", "Set", "Set(0)", "[object Set]", "size", "number", "0", "0", "[[Entries]]",
                        "object", "array", "Array", "Array(0)", "" } },
            { "set5", { "object", "set", "Set", "Set(2) {'Apple', 'Banana'}", "[object Set]", "size", "number", "2",
                        "2", "[[Entries]]", "object", "array", "Array", "Array(2)", "Apple,Banana" } },
            { "set6", { "object", "set", "Set", "Set(0)", "[object Set]", "size", "number", "0", "0", "[[Entries]]",
                        "object", "array", "Array", "Array(0)", "" } },
            { "set7", { "object", "set", "Set", "Set(0)", "[object Set]", "size", "number", "0", "0", "[[Entries]]",
                        "object", "array", "Array", "Array(0)", "" } },
            { "set8", { "object", "set", "Set", "Set(1) {Object}", "[object Set]", "size", "number", "1", "1",
                        "[[Entries]]", "object", "array", "Array", "Array(1)", "[object Object]" } },
            { "set9", { "object", "set", "Set", "Set(1) {Object}", "[object Set]", "size", "number", "1", "1",
                        "[[Entries]]", "object", "array", "Array", "Array(1)", "[object Object]" } },
            { "set10", { "object", "set", "Set", "Set(1) {Object}", "[object Set]", "size", "number", "1", "1",
                         "[[Entries]]", "object", "array", "Array", "Array(1)", "[object Object]" } },
            { "set11", { "object", "set", "Set", "Set(1) {Object}", "[object Set]", "size", "number", "1", "1",
                         "[[Entries]]", "object", "array", "Array", "Array(1)", "[object Object]" } },
            { "set12", { "object", "set", "Set", "Set(1) {Object}", "[object Set]", "size", "number", "1", "1",
                         "[[Entries]]", "object", "array", "Array", "Array(1)", "[object Object]" } },
            { "set13", { "object", "set", "Set", "Set(1) {Object}", "[object Set]", "size", "number", "1", "1",
                         "[[Entries]]", "object", "array", "Array", "Array(1)", "[object Object]" } },
            { "set14", { "object", "set", "Set", "Set(1) {Object}", "[object Set]", "size", "number", "1", "1",
                         "[[Entries]]", "object", "array", "Array", "Array(1)", "[object Object]" } },
            { "set15", { "object", "set", "Set", "Set(0)", "[object Set]", "size", "number", "0", "0", "[[Entries]]",
                         "object", "array", "Array", "Array(0)", "" } },
            { "set16", { "object", "set", "Set", "Set(4) {0, 'hello', Object, Object}", "[object Set]", "size",
                         "number", "4", "4", "[[Entries]]", "object", "array", "Array", "Array(4)",
                         "0,hello,[object Object],[object Object]" } },
            { "set17", { "object", "set", "Set", "Set(1) {Object}", "[object Set]", "size", "number", "1", "1",
                         "[[Entries]]", "object", "array", "Array", "Array(1)", "[object Object]" } },
            { "set18", { "object", "set", "Set", "Set(1) {999}", "[object Set]", "size", "number", "1", "1",
                         "[[Entries]]", "object", "array", "Array", "Array(1)", "999" } },
            { "set19", { "object", "set", "Set", "Set(1) {Object}", "[object Set]", "size", "number", "1", "1",
                         "[[Entries]]", "object", "array", "Array", "Array(1)", "[object Object]" } },
            { "set20", { "object", "set", "Set", "Set(0)", "[object Set]", "size", "number", "0", "0",
                         "[[Entries]]", "object", "array", "Array", "Array(0)", "" } },
            { "number1", { "number", "65535", "65535" } },
            { "number2", { "number", "5e-324", "5e-324" } },
            { "number3", { "number", "10000000000", "10000000000" } },
            { "number4", { "number", "2199023255551", "2199023255551" } },
            { "number5", { "number", "16383", "16383" } },
            { "number6", { "object", "Object", "Number{[[PrimitiveValue]]: 999}", "999", "[[PrimitiveValue]]",
                           "number", "999", "999" } },
            { "number7", { "number", "1.23e+47", "1.23e+47" } },
            { "number8", { "number", "1", "1" } },
            { "number9", { "number", "65536", "65536" } },
            { "number10", { "number", "-65534", "-65534" } },
            { "number11", { "number", "65535", "65535" } },
            { "number12", { "number", "0.000015259021896696422", "0.000015259021896696422" } },
            { "number13", { "number", "1", "1" } },
            { "number14", { "object", "Object", "Number{[[PrimitiveValue]]: 0}", "0", "[[PrimitiveValue]]", "number",
                            "0", "0" } },
            { "number15", { "object", "Object", "Number{[[PrimitiveValue]]: 1.7976931348623157e+308}",
                            "1.7976931348623157e+308", "[[PrimitiveValue]]", "number", "1.7976931348623157e+308",
                            "1.7976931348623157e+308" } },
            { "number16", { "object", "Object", "Number{[[PrimitiveValue]]: 5e-324}", "5e-324", "[[PrimitiveValue]]",
                            "number", "5e-324", "5e-324" } },
            { "number17", { "object", "Object", "Number{[[PrimitiveValue]]: 10000000000}", "10000000000",
                            "[[PrimitiveValue]]", "number", "10000000000", "10000000000" } },
            { "number18", { "object", "Object", "Number{[[PrimitiveValue]]: 2199023255551}", "2199023255551",
                            "[[PrimitiveValue]]", "number", "2199023255551", "2199023255551" } },
            { "number19", { "object", "Object", "Number{[[PrimitiveValue]]: 16383}", "16383", "[[PrimitiveValue]]",
                            "number", "16383", "16383" } },
            { "number20", { "object", "Object", "Number{[[PrimitiveValue]]: 1.23e+47}", "1.23e+47",
                            "[[PrimitiveValue]]", "number", "1.23e+47", "1.23e+47" } },
            { "number21", { "object", "Object", "Number{[[PrimitiveValue]]: 1}", "1", "[[PrimitiveValue]]", "number",
                            "1", "1" } },
            { "number22", { "object", "Object", "Number{[[PrimitiveValue]]: 65536}", "65536", "[[PrimitiveValue]]",
                            "number", "65536", "65536" } },
            { "number23", { "object", "Object", "Number{[[PrimitiveValue]]: -65534}", "-65534", "[[PrimitiveValue]]",
                            "number", "-65534", "-65534" } },
            { "number24", { "object", "Object", "Number{[[PrimitiveValue]]: 65535}", "65535", "[[PrimitiveValue]]",
                            "number", "65535", "65535" } },
            { "number25", { "object", "Object", "Number{[[PrimitiveValue]]: 0.000015259021896696422}",
                            "0.000015259021896696422", "[[PrimitiveValue]]", "number", "0.000015259021896696422",
                            "0.000015259021896696422" } },
            { "number26", { "object", "Object", "Number{[[PrimitiveValue]]: 1}", "1", "[[PrimitiveValue]]", "number",
                            "1", "1" } },
            { "number27", { "number", "1.7976931348623157e+308", "1.7976931348623157e+308" } },
            { "string1", { "string", "", "" } },
            { "string2", { "string", "", "" } },
            { "string3", { "string", "world", "world" } },
            { "string4", { "string", "helloworld", "helloworld" } },
            { "string5", { "string", "e", "e" } },
            { "string6", { "string", "1", "1" } },
            { "string7", { "object", "Object", "String{[[PrimitiveValue]]: }", "", "[[PrimitiveValue]]", "string", "",
                           "", "length", "number", "0", "0" } },
            { "string8", {"object", "Object", "String{[[PrimitiveValue]]: [object Set]}", "[object Set]",
                          "[[PrimitiveValue]]", "string", "[object Set]", "[object Set]", "0", "string", "[", "[",
                          "1", "string", "o", "o", "2", "string", "b", "b", "3", "string", "j", "j", "4", "string",
                          "e", "e", "5", "string", "c", "c", "6", "string", "t", "t", "7", "string", " ", " ", "8",
                          "string", "S", "S", "9", "string", "e", "e", "10", "string", "t", "t", "11", "string", "]",
                          "]", "length", "number", "12", "12"}},
            { "string9", {"object", "Object", "String{[[PrimitiveValue]]: 1}", "1", "[[PrimitiveValue]]", "string",
                          "1", "1", "0", "string", "1", "1", "length", "number", "1", "1"}},
            { "string10", {"object", "Object", "String{[[PrimitiveValue]]: helloworld}", "helloworld",
                           "[[PrimitiveValue]]", "string", "helloworld", "helloworld", "0", "string", "h", "h", "1",
                           "string", "e", "e", "2", "string", "l", "l", "3", "string", "l", "l", "4", "string", "o",
                           "o", "5", "string", "w", "w", "6", "string", "o", "o", "7", "string", "r", "r", "8",
                           "string", "l", "l", "9", "string", "d", "d", "length", "number", "10", "10"}},
            { "string11", {"object", "Object", "String{[[PrimitiveValue]]: [object Object]}", "[object Object]",
                           "[[PrimitiveValue]]", "string", "[object Object]", "[object Object]", "0", "string", "[",
                           "[", "1", "string", "o", "o", "2", "string", "b", "b", "3", "string", "j", "j", "4",
                           "string", "e", "e", "5", "string", "c", "c", "6", "string", "t", "t", "7", "string", " ",
                           " ", "8", "string", "O", "O", "9", "string", "b", "b", "10", "string", "j", "j", "11",
                           "string", "e", "e", "12", "string", "c", "c", "13", "string", "t", "t", "14", "string",
                           "]", "]", "length", "number", "15", "15"}},
            { "string12", { "object", "Object", "String{[[PrimitiveValue]]: undefined}", "undefined",
                            "[[PrimitiveValue]]", "string", "undefined", "undefined", "0", "string", "u", "u", "1",
                            "string", "n", "n", "2", "string", "d", "d", "3", "string", "e", "e", "4", "string", "f",
                            "f", "5", "string", "i", "i", "6", "string", "n", "n", "7", "string", "e", "e", "8",
                            "string", "d", "d", "length", "number", "9", "9" } },
            { "string13", { "object", "Object", "String{[[PrimitiveValue]]: Apple,Banana}", "Apple,Banana",
                            "[[PrimitiveValue]]", "string", "Apple,Banana", "Apple,Banana", "0", "string", "A", "A",
                            "1", "string", "p", "p", "2", "string", "p", "p", "3", "string", "l", "l", "4", "string",
                            "e", "e", "5", "string", ",", ",", "6", "string", "B", "B", "7", "string", "a", "a", "8",
                            "string", "n", "n", "9", "string", "a", "a", "10", "string", "n", "n", "11", "string",
                            "a", "a", "length", "number", "12", "12" } },
            { "string14", {"object", "Object", "String{[[PrimitiveValue]]: Sun Dec 17 1995 03:24:00 GMT+0800}",
                           "Sun Dec 17 1995 03:24:00 GMT+0800", "[[PrimitiveValue]]", "string",
                           "Sun Dec 17 1995 03:24:00 GMT+0800", "Sun Dec 17 1995 03:24:00 GMT+0800", "0", "string",
                           "S", "S", "1", "string", "u", "u", "2", "string", "n", "n", "3", "string", " ", " ", "4",
                           "string", "D", "D", "5", "string", "e", "e", "6", "string", "c", "c", "7", "string", " ",
                           " ", "8", "string", "1", "1", "9", "string", "7", "7", "10", "string", " ", " ", "11",
                           "string", "1", "1", "12", "string", "9", "9", "13", "string", "9", "9", "14", "string",
                           "5", "5", "15", "string", " ", " ", "16", "string", "0", "0", "17", "string", "3", "3",
                           "18", "string", ":", ":", "19", "string", "2", "2", "20", "string", "4", "4", "21",
                           "string", ":", ":", "22", "string", "0", "0", "23", "string", "0", "0", "24", "string",
                           " ", " ", "25", "string", "G", "G", "26", "string", "M", "M", "27", "string", "T", "T",
                           "28", "string", "+", "+", "29", "string", "0", "0", "30", "string", "8", "8", "31",
                           "string", "0", "0", "32", "string", "0", "0", "length", "number", "33", "33"}},
            { "string15", { "object", "Object", "String{[[PrimitiveValue]]: 999}", "999", "[[PrimitiveValue]]",
                            "string", "999", "999", "0", "string", "9", "9", "1", "string", "9", "9", "2", "string",
                            "9", "9", "length", "number", "3", "3" } },
            { "string16", {"object", "Object", "String{[[PrimitiveValue]]: Cannot get source code of funtion}",
                           "Cannot get source code of funtion", "[[PrimitiveValue]]", "string",
                           "Cannot get source code of funtion", "Cannot get source code of funtion", "0", "string",
                           "C", "C", "1", "string", "a", "a", "2", "string", "n", "n", "3", "string", "n", "n", "4",
                           "string", "o", "o", "5", "string", "t", "t", "6", "string", " ", " ", "7", "string", "g",
                           "g", "8", "string", "e", "e", "9", "string", "t", "t", "10", "string", " ", " ", "11",
                           "string", "s", "s", "12", "string", "o", "o", "13", "string", "u", "u", "14", "string",
                           "r", "r", "15", "string", "c", "c", "16", "string", "e", "e", "17", "string", " ", " ",
                           "18", "string", "c", "c", "19", "string", "o", "o", "20", "string", "d", "d", "21",
                           "string", "e", "e", "22", "string", " ", " ", "23", "string", "o", "o", "24", "string",
                           "f", "f", "25", "string", " ", " ", "26", "string", "f", "f", "27", "string", "u", "u",
                           "28", "string", "n", "n", "29", "string", "t", "t", "30", "string", "i", "i", "31",
                           "string", "o", "o", "32", "string", "n", "n", "length", "number", "33", "33"}},
            { "string17", {"object", "Object", "String{[[PrimitiveValue]]: /^\\d+\\.\\d+$/i}", "/^\\d+\\.\\d+$/i",
                           "[[PrimitiveValue]]", "string", "/^\\d+\\.\\d+$/i", "/^\\d+\\.\\d+$/i", "0", "string", "/",
                           "/", "1", "string", "^", "^", "2", "string", "\\", "\\", "3", "string", "d", "d", "4",
                           "string", "+", "+", "5", "string", "\\", "\\", "6", "string", ".", ".", "7", "string",
                           "\\", "\\", "8", "string", "d", "d", "9", "string", "+", "+", "10", "string", "$", "$",
                           "11", "string", "/", "/", "12", "string", "i", "i", "length", "number", "13", "13"} },
            { "string18", {"object", "Object", "String{[[PrimitiveValue]]: [object ArrayBuffer]}",
                           "[object ArrayBuffer]", "[[PrimitiveValue]]", "string", "[object ArrayBuffer]",
                           "[object ArrayBuffer]", "0", "string", "[", "[", "1", "string", "o", "o", "2", "string",
                           "b", "b", "3", "string", "j", "j", "4", "string", "e", "e", "5", "string", "c", "c", "6",
                           "string", "t", "t", "7", "string", " ", " ", "8", "string", "A", "A", "9", "string", "r",
                           "r", "10", "string", "r", "r", "11", "string", "a", "a", "12", "string", "y", "y", "13",
                           "string", "B", "B", "14", "string", "u", "u", "15", "string", "f", "f", "16", "string",
                           "f", "f", "17", "string", "e", "e", "18", "string", "r", "r", "19", "string", "]", "]",
                           "length", "number", "20", "20"} },
            { "string19", {"object", "Object", "String{[[PrimitiveValue]]: 0,0,0,0,0,0,0,0,0,0}",
                           "0,0,0,0,0,0,0,0,0,0", "[[PrimitiveValue]]", "string", "0,0,0,0,0,0,0,0,0,0",
                           "0,0,0,0,0,0,0,0,0,0", "0", "string", "0", "0", "1", "string", ",", ",", "2", "string",
                           "0", "0", "3", "string", ",", ",", "4", "string", "0", "0", "5", "string", ",", ",", "6",
                           "string", "0", "0", "7", "string", ",", ",", "8", "string", "0", "0", "9", "string", ",",
                           ",", "10", "string", "0", "0", "11", "string", ",", ",", "12", "string", "0", "0", "13",
                           "string", ",", ",", "14", "string", "0", "0", "15", "string", ",", ",", "16", "string",
                           "0", "0", "17", "string", ",", ",", "18", "string", "0", "0", "length", "number", "19",
                           "19"} },
            { "string20", {"object", "Object", "String{[[PrimitiveValue]]: [object DataView]}", "[object DataView]",
                           "[[PrimitiveValue]]", "string", "[object DataView]", "[object DataView]",
                           "0", "string", "[", "[", "1", "string", "o", "o", "2", "string", "b", "b", "3", "string",
                           "j", "j", "4", "string", "e", "e", "5", "string", "c", "c", "6", "string", "t", "t", "7",
                           "string", " ", " ", "8", "string", "D", "D", "9", "string", "a", "a", "10", "string", "t",
                           "t", "11", "string", "a", "a", "12", "string", "V", "V", "13", "string", "i", "i", "14",
                           "string", "e", "e", "15", "string", "w", "w", "16", "string", "]", "]", "length", "number",
                           "17", "17"} },
            { "string21", { "object", "Object", "String{[[PrimitiveValue]]: [object Map]}", "[object Map]",
                            "[[PrimitiveValue]]", "string", "[object Map]", "[object Map]", "0", "string", "[", "[",
                            "1", "string", "o", "o", "2", "string", "b", "b", "3", "string", "j", "j", "4", "string",
                            "e", "e", "5", "string", "c", "c", "6", "string", "t", "t", "7", "string", " ", " ", "8",
                            "string", "M", "M", "9", "string", "a", "a", "10", "string", "p", "p", "11", "string",
                            "]", "]", "length", "number", "12", "12"} },
            { "string22", {"object", "Object", "String{[[PrimitiveValue]]: Cannot get source code of funtion}",
                           "Cannot get source code of funtion", "[[PrimitiveValue]]", "string",
                           "Cannot get source code of funtion", "Cannot get source code of funtion", "0", "string",
                           "C", "C", "1", "string", "a", "a", "2", "string", "n", "n", "3", "string", "n", "n", "4",
                           "string", "o", "o", "5", "string", "t", "t", "6", "string", " ", " ", "7", "string", "g",
                           "g", "8", "string", "e", "e", "9", "string", "t", "t", "10", "string", " ", " ", "11",
                           "string", "s", "s", "12", "string", "o", "o", "13", "string", "u", "u", "14", "string",
                           "r", "r", "15", "string", "c", "c", "16", "string", "e", "e", "17", "string", " ", " ",
                           "18", "string", "c", "c", "19", "string", "o", "o", "20", "string", "d", "d", "21",
                           "string", "e", "e", "22", "string", " ", " ", "23", "string", "o", "o", "24", "string",
                           "f", "f", "25", "string", " ", " ", "26", "string", "f", "f", "27", "string", "u", "u",
                           "28", "string", "n", "n", "29", "string", "t", "t", "30", "string", "i", "i", "31",
                           "string", "o", "o", "32", "string", "n", "n", "length", "number", "33", "33"} }
        };

        int32_t index_ {0};
        const EcmaVM *vm_ {nullptr};
        RuntimeImpl *runtime_ {nullptr};
    };

    std::string entryPoint_ = "_GLOBAL::func_main_0";
    JSPtLocation location_ {nullptr, JSPtLocation::EntityId(0), 0};
    size_t breakpointCounter_ = 0;
};

std::unique_ptr<TestEvents> GetJsVariableTest()
{
    return std::make_unique<JsVariableTest>();
}
}  // namespace panda::ecmascript::tooling::test

#endif  // ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_VARIABLE_TEST_H
