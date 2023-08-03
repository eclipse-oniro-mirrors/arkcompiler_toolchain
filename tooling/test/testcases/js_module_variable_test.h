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

#ifndef ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_MODULE_VARIABLE_TEST_H
#define ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_MODULE_VARIABLE_TEST_H

#include "test/utils/test_util.h"

namespace panda::ecmascript::tooling::test {
std::string gg_pandaFile =  DEBUGGER_ABC_DIR "module_variable.abc";
std::string gg_sourceFile = DEBUGGER_JS_DIR "module_variable.js";
std::string_view gg_entry = "module_variable";

class JsModuleVariableTest : public TestEvents {
public:
    JsModuleVariableTest()
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
            if (moduleName != gg_pandaFile) {
                return false;
            }
            static_cast<JsModuleVariableTestChannel *>(channel_)->Initial(vm_, runtime_);
            runtime_->Enable();
            // 246: breakpointer line
            location_ = TestUtil::GetLocation(gg_sourceFile.c_str(), 246, 0, gg_pandaFile.c_str());
            ASSERT_TRUE(location_.GetMethodId().IsValid());
            TestUtil::SuspendUntilContinue(DebugEvent::LOAD_MODULE);
            ASSERT_EQ(moduleName, gg_pandaFile);
            ASSERT_TRUE(debugger_->NotifyScriptParsed(0, gg_pandaFile, gg_entry));
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

        channel_ = new JsModuleVariableTestChannel();
    }

    std::pair<std::string, std::string> GetEntryPoint() override
    {
        return {gg_pandaFile, entryPoint_};
    }
    ~JsModuleVariableTest()
    {
        delete channel_;
        channel_ = nullptr;
    }

private:
    class JsModuleVariableTestChannel : public TestChannel {
    public:
        JsModuleVariableTestChannel() = default;
        ~JsModuleVariableTestChannel() = default;
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
                    std::cout << "JsModuleVariableTestChannel: SendNotification 0:\n" << str << std::endl;

                    ASSERT_EQ(parsed->GetName(), "Debugger.scriptParsed");
                    ASSERT_EQ(parsed->GetUrl(), gg_sourceFile);
                    return true;
                },
                [this](const PtBaseEvents &events) -> bool {
                    auto paused = static_cast<const Paused *>(&events);
                    std::string str = paused->ToJson()->Stringify();
                    std::cout << "JsModuleVariableTestChannel: SendNotification 1:\n" << str << std::endl;

                    ASSERT_EQ(paused->GetName(), "Debugger.paused");
                    auto frame = paused->GetCallFrames()->at(0).get();
                    ASSERT_EQ(frame->GetFunctionName(), "foo");
                    auto scopes = frame->GetScopeChain();
                    for (uint32_t i = 0; i < scopes->size(); i++) {
                        auto scope = scopes->at(i).get();
                        if (scope->GetType() != Scope::Type::Module()) {
                            continue;
                        }
                        GetModuleProperties(scope);
                    }
                    return true;
                }
            };

            ASSERT_TRUE(eventList[index_](events));
            index_++;
        }

        void GetModuleProperties(panda::ecmascript::tooling::Scope *scope)
        {
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
                ASSERT_EQ(infos.size(), moduleVariableMap_.at(property->GetName()).size());
                for (uint32_t j = 0; j < infos.size(); j++) {
                    ASSERT_EQ(infos[j], moduleVariableMap_.at(property->GetName())[j]);
                }
            }
        }

    private:
        NO_COPY_SEMANTIC(JsModuleVariableTestChannel);
        NO_MOVE_SEMANTIC(JsModuleVariableTestChannel);

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
        const std::map<std::string, std::vector<std::string>> moduleVariableMap_ = {
            { "string0", { "string", "helloworld", "helloworld" } },
            { "boolean0", { "object", "Object", "Boolean{[[PrimitiveValue]]: false}", "false", "[[PrimitiveValue]]",
                            "boolean", "false", "false" } },
            { "number0", { "number", "1", "1" } },
            { "obj0", { "object", "Object", "Object", "[object Object]",
                        "key0", "string", "value0", "value0",
                        "key1", "number", "100", "100" } },
            { "arraybuffer0", { "object", "arraybuffer", "Arraybuffer", "ArrayBuffer(24)", "[object ArrayBuffer]",
                                "[[Int8Array]]", "object", "Object", "Int8Array(24)",
                                "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "[[Uint8Array]]", "object",
                                "Object", "Uint8Array(24)", "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
                                "[[Uint8ClampedArray]]", "object", "Object", "Object",
                                "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "[[Int16Array]]", "object",
                                "Object", "Int16Array(12)", "0,0,0,0,0,0,0,0,0,0,0,0", "[[Uint16Array]]", "object",
                                "Object", "Object", "0,0,0,0,0,0,0,0,0,0,0,0", "[[Int32Array]]", "object", "Object",
                                "Int32Array(6)", "0,0,0,0,0,0", "[[Uint32Array]]", "object", "Object", "Object",
                                "0,0,0,0,0,0", "[[Float32Array]]", "object", "Object", "Object", "0,0,0,0,0,0",
                                "[[Float64Array]]", "object", "Object", "Object", "0,0,0", "[[BigInt64Array]]",
                                "object", "Object", "Object", "0,0,0", "[[BigUint64Array]]", "object", "Object",
                                "Object", "0,0,0" } },
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
            { "regexp0", { "object", "regexp", "RegExp", "/^\\d+\\.\\d+$/i", "/^\\d+\\.\\d+$/i", "global", "boolean",
                           "false", "false", "ignoreCase", "boolean", "true", "true", "multiline", "boolean", "false",
                           "false", "dotAll", "boolean", "false", "false", "hasIndices", "boolean", "false", "false",
                           "unicode", "boolean", "false", "false", "sticky", "boolean", "false", "false", "flags",
                           "string", "i", "i", "source", "string", "^\\d+\\.\\d+$", "^\\d+\\.\\d+$", "lastIndex",
                           "number", "0", "0" } },
            { "uint8array0", { "object", "Object", "Uint8Array(24)",
                               "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "         0", "number", "0", "0",
                               "         1", "number", "0", "0", "         2", "number", "0", "0", "         3",
                               "number", "0", "0", "         4", "number", "0", "0", "         5", "number", "0",
                               "0", "         6", "number", "0", "0", "         7", "number", "0", "0", "         8",
                               "number", "0", "0", "         9", "number", "0", "0", "        10", "number", "0",
                               "0", "        11", "number", "0", "0", "        12", "number", "0", "0", "        13",
                               "number", "0", "0", "        14", "number", "0", "0", "        15", "number", "0",
                               "0", "        16", "number", "0", "0", "        17", "number", "0", "0", "        18",
                               "number", "0", "0", "        19", "number", "0", "0", "        20", "number", "0",
                               "0", "        21", "number", "0", "0", "        22", "number", "0", "0", "        23",
                               "number", "0", "0" } },
            { "dataview0", { "object", "dataview", "Dataview", "DataView(24)", "[object DataView]", "buffer",
                             "object", "arraybuffer", "Arraybuffer", "ArrayBuffer(24)", "[object ArrayBuffer]",
                             "byteLength", "number", "24", "24", "byteOffset", "number", "0", "0" } },
            { "bigint0", { "bigint", "999n", "999" } },
            { "typedarray0", { "object", "Object", "Uint8Array(0)", "", "none" } },
            { "sharedarraybuffer0", { "object", "Object", "SharedArrayBuffer(32)", "[object SharedArrayBuffer]",
                                      "[[Int8Array]]", "object", "Object", "Int8Array(32)",
                                      "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
                                      "[[Uint8Array]]", "object", "Object", "Uint8Array(32)",
                                      "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
                                      "[[Int16Array]]", "object", "Object", "Int16Array(16)",
                                      "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "[[Int32Array]]", "object", "Object",
                                      "Int32Array(8)", "0,0,0,0,0,0,0,0", "[[ArrayBufferByteLength]]", "number", "32",
                                      "32", "byteLength", "number", "32", "32" } },
            { "weakref0", { "object", "Object", "WeakRef {}", "[object WeakRef]", "none" } },
            { "iterator0", { "function", "Function", "function [Symbol.iterator]( { [native code] }",
                             "function [Symbol.iterator]() { [native code] }" } },
            { "string1", { "string", "helloworld1", "helloworld1" } },
            { "boolean1", { "object", "Object", "Boolean{[[PrimitiveValue]]: false}", "false", "[[PrimitiveValue]]",
                            "boolean", "false", "false" } },
            { "number1", { "number", "11", "11" } },
            { "obj1", { "object", "Object", "Object", "[object Object]",
                        "key2", "string", "value1", "value1",
                        "key3", "number", "1000", "1000" } },
            { "arraybuffer1", { "object", "arraybuffer", "Arraybuffer", "ArrayBuffer(24)", "[object ArrayBuffer]",
                                "[[Int8Array]]", "object", "Object", "Int8Array(24)",
                                "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "[[Uint8Array]]", "object",
                                "Object", "Uint8Array(24)", "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
                                "[[Uint8ClampedArray]]", "object", "Object", "Object",
                                "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "[[Int16Array]]", "object",
                                "Object", "Int16Array(12)", "0,0,0,0,0,0,0,0,0,0,0,0", "[[Uint16Array]]", "object",
                                "Object", "Object", "0,0,0,0,0,0,0,0,0,0,0,0", "[[Int32Array]]", "object", "Object",
                                "Int32Array(6)", "0,0,0,0,0,0", "[[Uint32Array]]", "object", "Object", "Object",
                                "0,0,0,0,0,0", "[[Float32Array]]", "object", "Object", "Object", "0,0,0,0,0,0",
                                "[[Float64Array]]", "object", "Object", "Object", "0,0,0", "[[BigInt64Array]]",
                                "object", "Object", "Object", "0,0,0", "[[BigUint64Array]]", "object", "Object",
                                "Object", "0,0,0" } },
            { "function1", { "function", "Function", "function function1( { [js code] }",
                             "Cannot get source code of funtion" } },
            { "generator1", { "function", "Generator", "function* generator1( { [js code] }",
                              "Cannot get source code of funtion" } },
            { "map1", { "object", "map", "Map", "Map(0)", "[object Map]", "size", "number", "0", "0", "[[Entries]]",
                        "object", "array", "Array", "Array(0)", "" } },
            { "set1", { "object", "set", "Set", "Set(0)", "[object Set]", "size", "number", "0", "0", "[[Entries]]",
                        "object", "array", "Array", "Array(0)", "" } },
            { "undefined1", { "undefined" } },
            { "array1", { "object", "array", "Array", "Array(2)", "Cherry, Pear", "0", "string", "Cherry", "Cherry",
                          "1", "string", "Pear", "Pear", "length", "number", "2", "2" } },
            { "regexp1", { "object", "regexp", "RegExp", "/^\\d+\\.\\d+$/i", "/^\\d+\\.\\d+$/i", "global", "boolean",
                           "false", "false", "ignoreCase", "boolean", "true", "true", "multiline", "boolean", "false",
                           "false", "dotAll", "boolean", "false", "false", "hasIndices", "boolean", "false", "false",
                           "unicode", "boolean", "false", "false", "sticky", "boolean", "false", "false", "flags",
                           "string", "i", "i", "source", "string", "^\\d+\\.\\d+$", "^\\d+\\.\\d+$", "lastIndex",
                           "number", "0", "0" } },
            { "uint8array1", { "object", "Object", "Uint8Array(24)",
                               "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "         0", "number", "0", "0",
                               "         1", "number", "0", "0", "         2", "number", "0", "0", "         3",
                               "number", "0", "0", "         4", "number", "0", "0", "         5", "number", "0",
                               "0", "         6", "number", "0", "0", "         7", "number", "0", "0", "         8",
                               "number", "0", "0", "         9", "number", "0", "0", "        10", "number", "0",
                               "0", "        11", "number", "0", "0", "        12", "number", "0", "0", "        13",
                               "number", "0", "0", "        14", "number", "0", "0", "        15", "number", "0",
                               "0", "        16", "number", "0", "0", "        17", "number", "0", "0", "        18",
                               "number", "0", "0", "        19", "number", "0", "0", "        20", "number", "0",
                               "0", "        21", "number", "0", "0", "        22", "number", "0", "0", "        23",
                               "number", "0", "0" } },
            { "dataview1", { "object", "dataview", "Dataview", "DataView(24)", "[object DataView]", "buffer",
                             "object", "arraybuffer", "Arraybuffer", "ArrayBuffer(24)", "[object ArrayBuffer]",
                             "byteLength", "number", "24", "24", "byteOffset", "number", "0", "0" } },
            { "bigint1", { "bigint", "9999n", "9999" } },
            { "typedarray1", { "object", "Object", "Uint8Array(0)", "", "none" } },
            { "sharedarraybuffer1", { "object", "Object", "SharedArrayBuffer(32)", "[object SharedArrayBuffer]",
                                      "[[Int8Array]]", "object", "Object", "Int8Array(32)",
                                      "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
                                      "[[Uint8Array]]", "object", "Object", "Uint8Array(32)",
                                      "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
                                      "[[Int16Array]]", "object", "Object", "Int16Array(16)",
                                      "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "[[Int32Array]]", "object", "Object",
                                      "Int32Array(8)", "0,0,0,0,0,0,0,0", "[[ArrayBufferByteLength]]", "number", "32",
                                      "32", "byteLength", "number", "32", "32" } },
            { "weakref1", { "object", "Object", "WeakRef {}", "[object WeakRef]", "none" } },
            { "iterator1", { "function", "Function", "function [Symbol.iterator]( { [native code] }",
                             "function [Symbol.iterator]() { [native code] }" } },
            { "string2", { "string", "helloworld11", "helloworld11" } },
            { "boolean2", { "object", "Object", "Boolean{[[PrimitiveValue]]: false}", "false", "[[PrimitiveValue]]",
                            "boolean", "false", "false" } },
            { "number2", { "number", "111", "111" } },
            { "obj2", { "object", "Object", "Object", "[object Object]",
                        "key4", "string", "value2", "value2",
                        "key5", "number", "10000", "10000" } },
            { "arraybuffer2", { "object", "arraybuffer", "Arraybuffer", "ArrayBuffer(24)", "[object ArrayBuffer]",
                                "[[Int8Array]]", "object", "Object", "Int8Array(24)",
                                "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "[[Uint8Array]]", "object",
                                "Object", "Uint8Array(24)", "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
                                "[[Uint8ClampedArray]]", "object", "Object", "Object",
                                "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "[[Int16Array]]", "object",
                                "Object", "Int16Array(12)", "0,0,0,0,0,0,0,0,0,0,0,0", "[[Uint16Array]]", "object",
                                "Object", "Object", "0,0,0,0,0,0,0,0,0,0,0,0", "[[Int32Array]]", "object", "Object",
                                "Int32Array(6)", "0,0,0,0,0,0", "[[Uint32Array]]", "object", "Object", "Object",
                                "0,0,0,0,0,0", "[[Float32Array]]", "object", "Object", "Object", "0,0,0,0,0,0",
                                "[[Float64Array]]", "object", "Object", "Object", "0,0,0", "[[BigInt64Array]]",
                                "object", "Object", "Object", "0,0,0", "[[BigUint64Array]]", "object", "Object",
                                "Object", "0,0,0" } },
            { "function2", { "function", "Function", "function function2( { [js code] }",
                             "Cannot get source code of funtion" } },
            { "generator2", { "function", "Generator", "function* generator2( { [js code] }",
                              "Cannot get source code of funtion" } },
            { "map2", { "object", "map", "Map", "Map(0)", "[object Map]", "size", "number", "0", "0", "[[Entries]]",
                        "object", "array", "Array", "Array(0)", "" } },
            { "set2", { "object", "set", "Set", "Set(0)", "[object Set]", "size", "number", "0", "0", "[[Entries]]",
                        "object", "array", "Array", "Array(0)", "" } },
            { "undefined2", { "undefined" } },
            { "array2", { "object", "array", "Array", "Array(2)", "Lemon, Durian", "0", "string", "Lemon", "Lemon",
                          "1", "string", "Durian", "Durian", "length", "number", "2", "2" } },
            { "regexp2", { "object", "regexp", "RegExp", "/^\\d+\\.\\d+$/i", "/^\\d+\\.\\d+$/i", "global", "boolean",
                           "false", "false", "ignoreCase", "boolean", "true", "true", "multiline", "boolean", "false",
                           "false", "dotAll", "boolean", "false", "false", "hasIndices", "boolean", "false", "false",
                           "unicode", "boolean", "false", "false", "sticky", "boolean", "false", "false", "flags",
                           "string", "i", "i", "source", "string", "^\\d+\\.\\d+$", "^\\d+\\.\\d+$", "lastIndex",
                           "number", "0", "0" } },
            { "uint8array2", { "object", "Object", "Uint8Array(24)",
                               "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "         0", "number", "0", "0",
                               "         1", "number", "0", "0", "         2", "number", "0", "0", "         3",
                               "number", "0", "0", "         4", "number", "0", "0", "         5", "number", "0",
                               "0", "         6", "number", "0", "0", "         7", "number", "0", "0", "         8",
                               "number", "0", "0", "         9", "number", "0", "0", "        10", "number", "0",
                               "0", "        11", "number", "0", "0", "        12", "number", "0", "0", "        13",
                               "number", "0", "0", "        14", "number", "0", "0", "        15", "number", "0",
                               "0", "        16", "number", "0", "0", "        17", "number", "0", "0", "        18",
                               "number", "0", "0", "        19", "number", "0", "0", "        20", "number", "0",
                               "0", "        21", "number", "0", "0", "        22", "number", "0", "0", "        23",
                               "number", "0", "0" } },
            { "dataview2", { "object", "dataview", "Dataview", "DataView(24)", "[object DataView]", "buffer",
                             "object", "arraybuffer", "Arraybuffer", "ArrayBuffer(24)", "[object ArrayBuffer]",
                             "byteLength", "number", "24", "24", "byteOffset", "number", "0", "0" } },
            { "bigint2", { "bigint", "9999n", "9999" } },
            { "typedarray2", { "object", "Object", "Uint8Array(0)", "", "none" } },
            { "sharedarraybuffer2", { "object", "Object", "SharedArrayBuffer(32)", "[object SharedArrayBuffer]",
                                      "[[Int8Array]]", "object", "Object", "Int8Array(32)",
                                      "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
                                      "[[Uint8Array]]", "object", "Object", "Uint8Array(32)",
                                      "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
                                      "[[Int16Array]]", "object", "Object", "Int16Array(16)",
                                      "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "[[Int32Array]]", "object", "Object",
                                      "Int32Array(8)", "0,0,0,0,0,0,0,0", "[[ArrayBufferByteLength]]", "number", "32",
                                      "32", "byteLength", "number", "32", "32" } },
            { "weakref2", { "object", "Object", "WeakRef {}", "[object WeakRef]", "none" } },
            { "iterator2", { "function", "Function", "function [Symbol.iterator]( { [native code] }",
                             "function [Symbol.iterator]() { [native code] }" } },
            { "string3", { "string", "helloworld12", "helloworld12" } },
            { "boolean3", { "object", "Object", "Boolean{[[PrimitiveValue]]: true}", "true", "[[PrimitiveValue]]",
                            "boolean", "true", "true" } },
            { "number3", { "number", "3", "3" } },
            { "string4", { "string", "helloworld123", "helloworld123" } },
            { "boolean4", { "object", "Object", "Boolean{[[PrimitiveValue]]: true}", "true", "[[PrimitiveValue]]",
                            "boolean", "true", "true" } },
            { "number4", { "number", "4", "4" } },
            { "string5", { "string", "helloworld123", "helloworld123" } },
            { "boolean5", { "object", "Object", "Boolean{[[PrimitiveValue]]: true}", "true", "[[PrimitiveValue]]",
                            "boolean", "true", "true" } },
            { "number5", { "number", "4", "4" } },
            { "foo", { "function", "Function", "function foo( { [js code] }", "Cannot get source code of funtion"} },
            { "string6", { "string", "helloworld12345", "helloworld12345" } },
            { "boolean6", { "object", "Object", "Boolean{[[PrimitiveValue]]: true}", "true", "[[PrimitiveValue]]",
                            "boolean", "true", "true" } },
            { "number6", { "number", "6", "6" } },
            { "obj6", { "object", "Object", "Object", "[object Object]",
                        "key6", "string", "value3", "value3",
                        "key7", "number", "10000", "10000" } },
            { "arraybuffer6", { "object", "arraybuffer", "Arraybuffer", "ArrayBuffer(24)", "[object ArrayBuffer]",
                                "[[Int8Array]]", "object", "Object", "Int8Array(24)",
                                "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "[[Uint8Array]]", "object",
                                "Object", "Uint8Array(24)", "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
                                "[[Uint8ClampedArray]]", "object", "Object", "Object",
                                "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "[[Int16Array]]", "object",
                                "Object", "Int16Array(12)", "0,0,0,0,0,0,0,0,0,0,0,0", "[[Uint16Array]]", "object",
                                "Object", "Object", "0,0,0,0,0,0,0,0,0,0,0,0", "[[Int32Array]]", "object", "Object",
                                "Int32Array(6)", "0,0,0,0,0,0", "[[Uint32Array]]", "object", "Object", "Object",
                                "0,0,0,0,0,0", "[[Float32Array]]", "object", "Object", "Object", "0,0,0,0,0,0",
                                "[[Float64Array]]", "object", "Object", "Object", "0,0,0", "[[BigInt64Array]]",
                                "object", "Object", "Object", "0,0,0", "[[BigUint64Array]]", "object", "Object",
                                "Object", "0,0,0" } },
            { "function6", { "function", "Function", "function function6( { [js code] }",
                             "Cannot get source code of funtion" } },
            { "generator6", { "function", "Generator", "function* generator6( { [js code] }",
                              "Cannot get source code of funtion" } },
            { "map6", { "object", "map", "Map", "Map(0)", "[object Map]", "size", "number", "0", "0", "[[Entries]]",
                        "object", "array", "Array", "Array(0)", "" } },
            { "set6", { "object", "set", "Set", "Set(0)", "[object Set]", "size", "number", "0", "0", "[[Entries]]",
                        "object", "array", "Array", "Array(0)", "" } },
            { "undefined6", { "undefined" } },
            { "array6", { "object", "array", "Array", "Array(2)", "orange,Grape", "0", "string", "orange", "orange",
                          "1", "string", "Grape", "Grape", "length", "number", "2", "2" } },
            { "regexp6", { "object", "regexp", "RegExp", "/^\\d+\\.\\d+$/i", "/^\\d+\\.\\d+$/i", "global", "boolean",
                           "false", "false", "ignoreCase", "boolean", "true", "true", "multiline", "boolean", "false",
                           "false", "dotAll", "boolean", "false", "false", "hasIndices", "boolean", "false", "false",
                           "unicode", "boolean", "false", "false", "sticky", "boolean", "false", "false", "flags",
                           "string", "i", "i", "source", "string", "^\\d+\\.\\d+$", "^\\d+\\.\\d+$", "lastIndex",
                           "number", "0", "0" } },
            { "uint8array6", { "object", "Object", "Uint8Array(24)",
                               "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "         0", "number", "0", "0",
                               "         1", "number", "0", "0", "         2", "number", "0", "0", "         3",
                               "number", "0", "0", "         4", "number", "0", "0", "         5", "number", "0",
                               "0", "         6", "number", "0", "0", "         7", "number", "0", "0", "         8",
                               "number", "0", "0", "         9", "number", "0", "0", "        10", "number", "0",
                               "0", "        11", "number", "0", "0", "        12", "number", "0", "0", "        13",
                               "number", "0", "0", "        14", "number", "0", "0", "        15", "number", "0",
                               "0", "        16", "number", "0", "0", "        17", "number", "0", "0", "        18",
                               "number", "0", "0", "        19", "number", "0", "0", "        20", "number", "0",
                               "0", "        21", "number", "0", "0", "        22", "number", "0", "0", "        23",
                               "number", "0", "0" } },
            { "dataview6", { "object", "dataview", "Dataview", "DataView(23)", "[object DataView]", "buffer",
                             "object", "arraybuffer", "Arraybuffer", "ArrayBuffer(24)", "[object ArrayBuffer]",
                             "byteLength", "number", "23", "23", "byteOffset", "number", "1", "1" } },
            { "bigint6", { "bigint", "99999n", "99999" } },
            { "string7", { "string", "helloworld12345", "helloworld12345" } },
            { "boolean7", { "object", "Object", "Boolean{[[PrimitiveValue]]: true}", "true", "[[PrimitiveValue]]",
                            "boolean", "true", "true" } },
            { "number7", { "number", "6", "6" } },
            { "obj7", { "object", "Object", "Object", "[object Object]",
                        "key6", "string", "value3", "value3",
                        "key7", "number", "10000", "10000" } },
            { "arraybuffer7", { "object", "arraybuffer", "Arraybuffer", "ArrayBuffer(24)", "[object ArrayBuffer]",
                                "[[Int8Array]]", "object", "Object", "Int8Array(24)",
                                "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "[[Uint8Array]]", "object",
                                "Object", "Uint8Array(24)", "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
                                "[[Uint8ClampedArray]]", "object", "Object", "Object",
                                "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "[[Int16Array]]", "object",
                                "Object", "Int16Array(12)", "0,0,0,0,0,0,0,0,0,0,0,0", "[[Uint16Array]]", "object",
                                "Object", "Object", "0,0,0,0,0,0,0,0,0,0,0,0", "[[Int32Array]]", "object", "Object",
                                "Int32Array(6)", "0,0,0,0,0,0", "[[Uint32Array]]", "object", "Object", "Object",
                                "0,0,0,0,0,0", "[[Float32Array]]", "object", "Object", "Object", "0,0,0,0,0,0",
                                "[[Float64Array]]", "object", "Object", "Object", "0,0,0", "[[BigInt64Array]]",
                                "object", "Object", "Object", "0,0,0", "[[BigUint64Array]]", "object", "Object",
                                "Object", "0,0,0" } },
            { "function7", { "function", "Function", "function function6( { [js code] }",
                             "Cannot get source code of funtion" } },
            { "generator7", { "function", "Generator", "function* generator6( { [js code] }",
                              "Cannot get source code of funtion" } },
            { "map7", { "object", "map", "Map", "Map(0)", "[object Map]", "size", "number", "0", "0", "[[Entries]]",
                        "object", "array", "Array", "Array(0)", "" } },
            { "set7", { "object", "set", "Set", "Set(0)", "[object Set]", "size", "number", "0", "0", "[[Entries]]",
                        "object", "array", "Array", "Array(0)", "" } },
            { "undefined7", { "undefined" } },
            { "array7", { "object", "array", "Array", "Array(2)", "orange,Grape", "0", "string", "orange", "orange",
                          "1", "string", "Grape", "Grape", "length", "number", "2", "2" } },
            { "regexp7", { "object", "regexp", "RegExp", "/^\\d+\\.\\d+$/i", "/^\\d+\\.\\d+$/i", "global", "boolean",
                           "false", "false", "ignoreCase", "boolean", "true", "true", "multiline", "boolean", "false",
                           "false", "dotAll", "boolean", "false", "false", "hasIndices", "boolean", "false", "false",
                           "unicode", "boolean", "false", "false", "sticky", "boolean", "false", "false", "flags",
                           "string", "i", "i", "source", "string", "^\\d+\\.\\d+$", "^\\d+\\.\\d+$", "lastIndex",
                           "number", "0", "0" } },
            { "uint8array7", { "object", "Object", "Uint8Array(24)",
                               "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "         0", "number", "0", "0",
                               "         1", "number", "0", "0", "         2", "number", "0", "0", "         3",
                               "number", "0", "0", "         4", "number", "0", "0", "         5", "number", "0",
                               "0", "         6", "number", "0", "0", "         7", "number", "0", "0", "         8",
                               "number", "0", "0", "         9", "number", "0", "0", "        10", "number", "0",
                               "0", "        11", "number", "0", "0", "        12", "number", "0", "0", "        13",
                               "number", "0", "0", "        14", "number", "0", "0", "        15", "number", "0",
                               "0", "        16", "number", "0", "0", "        17", "number", "0", "0", "        18",
                               "number", "0", "0", "        19", "number", "0", "0", "        20", "number", "0",
                               "0", "        21", "number", "0", "0", "        22", "number", "0", "0", "        23",
                               "number", "0", "0" } },
            { "dataview7", { "object", "dataview", "Dataview", "DataView(23)", "[object DataView]", "buffer",
                             "object", "arraybuffer", "Arraybuffer", "ArrayBuffer(24)", "[object ArrayBuffer]",
                             "byteLength", "number", "23", "23", "byteOffset", "number", "1", "1" } },
            { "bigint7", { "bigint", "99999n", "99999" } },
            { "string9", { "string", "helloworld11", "helloworld11" } },
            { "boolean9", { "object", "Object", "Boolean{[[PrimitiveValue]]: false}", "false", "[[PrimitiveValue]]",
                            "boolean", "false", "false" } },
            { "number9", { "number", "111", "111" } },
            { "obj9", { "object", "Object", "Object", "[object Object]",
                        "key8", "string", "value4", "value4",
                        "key9", "number", "10000", "10000" } },
            { "arraybuffer9", { "object", "arraybuffer", "Arraybuffer", "ArrayBuffer(24)", "[object ArrayBuffer]",
                                "[[Int8Array]]", "object", "Object", "Int8Array(24)",
                                "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "[[Uint8Array]]", "object",
                                "Object", "Uint8Array(24)", "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
                                "[[Uint8ClampedArray]]", "object", "Object", "Object",
                                "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "[[Int16Array]]", "object",
                                "Object", "Int16Array(12)", "0,0,0,0,0,0,0,0,0,0,0,0", "[[Uint16Array]]", "object",
                                "Object", "Object", "0,0,0,0,0,0,0,0,0,0,0,0", "[[Int32Array]]", "object", "Object",
                                "Int32Array(6)", "0,0,0,0,0,0", "[[Uint32Array]]", "object", "Object", "Object",
                                "0,0,0,0,0,0", "[[Float32Array]]", "object", "Object", "Object", "0,0,0,0,0,0",
                                "[[Float64Array]]", "object", "Object", "Object", "0,0,0", "[[BigInt64Array]]",
                                "object", "Object", "Object", "0,0,0", "[[BigUint64Array]]", "object", "Object",
                                "Object", "0,0,0" } },
            { "function9", { "function", "Function", "function function8( { [js code] }",
                             "Cannot get source code of funtion" } },
            { "generator9", { "function", "Generator", "function* generator8( { [js code] }",
                              "Cannot get source code of funtion" } },
            { "map9", { "object", "map", "Map", "Map(0)", "[object Map]", "size", "number", "0", "0", "[[Entries]]",
                        "object", "array", "Array", "Array(0)", "" } },
            { "set9", { "object", "set", "Set", "Set(0)", "[object Set]", "size", "number", "0", "0", "[[Entries]]",
                        "object", "array", "Array", "Array(0)", "" } },
            { "undefined9", { "undefined" } },
            { "array9", { "object", "array", "Array", "Array(2)", "Lemon,Durian", "0", "string", "Lemon", "Lemon",
                          "1", "string", "Durian", "Durian", "length", "number", "2", "2" } },
            { "regexp9", { "object", "regexp", "RegExp", "/^\\d+\\.\\d+$/i", "/^\\d+\\.\\d+$/i", "global", "boolean",
                           "false", "false", "ignoreCase", "boolean", "true", "true", "multiline", "boolean", "false",
                           "false", "dotAll", "boolean", "false", "false", "hasIndices", "boolean", "false", "false",
                           "unicode", "boolean", "false", "false", "sticky", "boolean", "false", "false", "flags",
                           "string", "i", "i", "source", "string", "^\\d+\\.\\d+$", "^\\d+\\.\\d+$", "lastIndex",
                           "number", "0", "0" } },
            { "uint8array9", { "object", "Object", "Uint8Array(24)",
                               "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "         0", "number", "0", "0",
                               "         1", "number", "0", "0", "         2", "number", "0", "0", "         3",
                               "number", "0", "0", "         4", "number", "0", "0", "         5", "number", "0",
                               "0", "         6", "number", "0", "0", "         7", "number", "0", "0", "         8",
                               "number", "0", "0", "         9", "number", "0", "0", "        10", "number", "0",
                               "0", "        11", "number", "0", "0", "        12", "number", "0", "0", "        13",
                               "number", "0", "0", "        14", "number", "0", "0", "        15", "number", "0",
                               "0", "        16", "number", "0", "0", "        17", "number", "0", "0", "        18",
                               "number", "0", "0", "        19", "number", "0", "0", "        20", "number", "0",
                               "0", "        21", "number", "0", "0", "        22", "number", "0", "0", "        23",
                               "number", "0", "0" } },
            { "dataview9", { "object", "dataview", "Dataview", "DataView(24)", "[object DataView]", "buffer",
                             "object", "arraybuffer", "Arraybuffer", "ArrayBuffer(24)", "[object ArrayBuffer]",
                             "byteLength", "number", "24", "24", "byteOffset", "number", "0", "0" } },
            { "bigint9", { "bigint", "9999n", "9999" } }, { "string2", { "string", "helloworld11", "helloworld11" } },
            { "boolean10", { "object", "Object", "Boolean{[[PrimitiveValue]]: false}", "false", "[[PrimitiveValue]]",
                             "boolean", "false", "false" } },
            { "string10", { "string", "helloworld11", "helloworld11" } },
            { "number10", { "number", "111", "111" } },
            { "obj10", { "object", "Object", "Object", "[object Object]",
                         "key8", "string", "value4", "value4",
                         "key9", "number", "10000", "10000" } },
            { "arraybuffer10", { "object", "arraybuffer", "Arraybuffer", "ArrayBuffer(24)", "[object ArrayBuffer]",
                                 "[[Int8Array]]", "object", "Object", "Int8Array(24)",
                                 "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "[[Uint8Array]]", "object",
                                 "Object", "Uint8Array(24)", "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
                                 "[[Uint8ClampedArray]]", "object", "Object", "Object",
                                 "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "[[Int16Array]]", "object",
                                 "Object", "Int16Array(12)", "0,0,0,0,0,0,0,0,0,0,0,0", "[[Uint16Array]]", "object",
                                 "Object", "Object", "0,0,0,0,0,0,0,0,0,0,0,0", "[[Int32Array]]", "object", "Object",
                                 "Int32Array(6)", "0,0,0,0,0,0", "[[Uint32Array]]", "object", "Object", "Object",
                                 "0,0,0,0,0,0", "[[Float32Array]]", "object", "Object", "Object", "0,0,0,0,0,0",
                                 "[[Float64Array]]", "object", "Object", "Object", "0,0,0", "[[BigInt64Array]]",
                                 "object", "Object", "Object", "0,0,0", "[[BigUint64Array]]", "object", "Object",
                                 "Object", "0,0,0" } },
            { "function10", { "function", "Function", "function function8( { [js code] }",
                              "Cannot get source code of funtion" } },
            { "generator10", { "function", "Generator", "function* generator8( { [js code] }",
                               "Cannot get source code of funtion" } },
            { "map10", { "object", "map", "Map", "Map(0)", "[object Map]", "size", "number", "0", "0", "[[Entries]]",
                         "object", "array", "Array", "Array(0)", "" } },
            { "set10", { "object", "set", "Set", "Set(0)", "[object Set]", "size", "number", "0", "0", "[[Entries]]",
                         "object", "array", "Array", "Array(0)", "" } },
            { "undefined10", { "undefined" } },
            { "array10", { "object", "array", "Array", "Array(2)", "Lemon,Durian", "0", "string", "Lemon", "Lemon",
                           "1", "string", "Durian", "Durian", "length", "number", "2", "2" } },
            { "regexp10", { "object", "regexp", "RegExp", "/^\\d+\\.\\d+$/i", "/^\\d+\\.\\d+$/i", "global", "boolean",
                            "false", "false", "ignoreCase", "boolean", "true", "true", "multiline", "boolean", "false",
                            "false", "dotAll", "boolean", "false", "false", "hasIndices", "boolean", "false", "false",
                            "unicode", "boolean", "false", "false", "sticky", "boolean", "false", "false", "flags",
                            "string", "i", "i", "source", "string", "^\\d+\\.\\d+$", "^\\d+\\.\\d+$", "lastIndex",
                            "number", "0", "0" } },
            { "uint8array10", { "object", "Object", "Uint8Array(24)",
                                "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", "         0", "number", "0", "0",
                                "         1", "number", "0", "0", "         2", "number", "0", "0", "         3",
                                "number", "0", "0", "         4", "number", "0", "0", "         5", "number", "0",
                                "0", "         6", "number", "0", "0", "         7", "number", "0", "0", "         8",
                                "number", "0", "0", "         9", "number", "0", "0", "        10", "number", "0",
                                "0", "        11", "number", "0", "0", "        12", "number", "0", "0", "        13",
                                "number", "0", "0", "        14", "number", "0", "0", "        15", "number", "0",
                                "0", "        16", "number", "0", "0", "        17", "number", "0", "0", "        18",
                                "number", "0", "0", "        19", "number", "0", "0", "        20", "number", "0",
                                "0", "        21", "number", "0", "0", "        22", "number", "0", "0", "        23",
                                "number", "0", "0" } },
            { "dataview10", { "object", "dataview", "Dataview", "DataView(24)", "[object DataView]", "buffer",
                              "object", "arraybuffer", "Arraybuffer", "ArrayBuffer(24)", "[object ArrayBuffer]",
                              "byteLength", "number", "24", "24", "byteOffset", "number", "0", "0" } },
            { "bigint10", { "bigint", "9999n", "9999" } },
            { "transit1", { "number", "1", "1" } },
            { "transit2", { "number", "2", "2" } },
            { "transit4", { "number", "3", "3" } },
            { "transit5", { "number", "1", "1" } },
            { "transit6", { "number", "2", "2" } },
            { "transit7", { "number", "3", "3" } },
            { "multipleOut1", { "number", "1", "1" } },
            { "multipleOut2", { "number", "2", "2" } },
            { "multipleOut3", { "number", "3", "3" } },
            { "multipleOut4", { "number", "4", "4" } },
            { "multipleOut5", { "number", "5", "5" } },
            { "multipleOut6", { "number", "6", "6" } },
            { "multipleOut10", { "number", "10", "10" } },
            { "multipleOut11", { "number", "11", "11" } },
            { "multipleOut12", { "number", "12", "12" } },
            { "*default*", { "function", "Function", "function default( { [js code] }",
                             "Cannot get source code of funtion" } },
            {"obj", { "object", "Object", "Object",  "[object Object]",
                      "boolean3", "object", "Object", "Boolean{[[PrimitiveValue]]: true}", "true",
                      "boolean4", "object", "Object", "Boolean{[[PrimitiveValue]]: true}", "true",
                      "number3", "number", "3", "3",
                      "number4", "number", "4", "4",
                      "string3", "string", "helloworld12", "helloworld12",
                      "string4", "string", "helloworld123", "helloworld123",
            }}
        };

        int32_t index_ {0};
        const EcmaVM *vm_ {nullptr};
        RuntimeImpl *runtime_ {nullptr};
    };

    std::string entryPoint_ = "module_variable";
    JSPtLocation location_ {nullptr, JSPtLocation::EntityId(0), 0};
    size_t breakpointCounter_ = 0;
};

std::unique_ptr<TestEvents> GetJsModuleVariableTest()
{
    return std::make_unique<JsModuleVariableTest>();
}
}  // namespace panda::ecmascript::tooling::test

#endif  // ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_VARIABLE_TEST_H
