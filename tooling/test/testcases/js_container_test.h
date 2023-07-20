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

#ifndef ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_CONTAINER_TEST_H
#define ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_CONTAINER_TEST_H

#include "test/utils/test_util.h"

namespace panda::ecmascript::tooling::test {
class JsContainerTest : public TestEvents {
public:
    JsContainerTest()
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
            std::string pandaFile = DEBUGGER_ABC_DIR "container.abc";
            std::string sourceFile = DEBUGGER_JS_DIR "container.js";
            static_cast<JsContainerTestChannel *>(channel_)->Initial(vm_, runtime_);
            runtime_->Enable();
            // 348: breakpointer line
            location_ = TestUtil::GetLocation(sourceFile.c_str(), 348, 0, pandaFile.c_str());
            ASSERT_TRUE(location_.GetMethodId().IsValid());
            TestUtil::SuspendUntilContinue(DebugEvent::LOAD_MODULE);
            ASSERT_EQ(moduleName, pandaFile);
            ASSERT_TRUE(debugger_->NotifyScriptParsed(0, pandaFile));
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

        channel_ = new JsContainerTestChannel();
    }

    std::pair<std::string, std::string> GetEntryPoint() override
    {
        std::string pandaFile = DEBUGGER_ABC_DIR "container.abc";
        return {pandaFile, entryPoint_};
    }
    ~JsContainerTest()
    {
        delete channel_;
        channel_ = nullptr;
    }

private:
    class JsContainerTestChannel : public TestChannel {
    public:
        JsContainerTestChannel() = default;
        ~JsContainerTestChannel() = default;
        void Initial(const EcmaVM *vm, RuntimeImpl *runtime)
        {
            vm_ = vm;
            runtime_ = runtime;
        }

        void SendNotification(const PtBaseEvents &events) override
        {
            const static std::vector<std::function<bool(const PtBaseEvents &events)>> eventList = {
                [](const PtBaseEvents &events) -> bool {
                    std::string sourceFile = DEBUGGER_JS_DIR "container.js";
                    auto parsed = static_cast<const ScriptParsed *>(&events);
                    std::string str = parsed->ToJson()->Stringify();
                    std::cout << "JsModuleVariableTestChannel: SendNotification 0:\n" << str << std::endl;

                    ASSERT_EQ(parsed->GetName(), "Debugger.scriptParsed");
                    ASSERT_EQ(parsed->GetUrl(), sourceFile);
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
                    ASSERT_EQ(scopes->size(), 2U);  // 2: contain local and global
                    for (uint32_t i = 0; i < scopes->size(); i++) {
                        auto scope = scopes->at(i).get();
                        if (scope->GetType() != Scope::Type::Module()) {
                            continue;
                        }
                        GetProperties(scope);
                    }
                    return true;
                }
            };

            ASSERT_TRUE(eventList[index_](events));
            index_++;
        }

        void GetProperties(panda::ecmascript::tooling::Scope *scope)
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
                ASSERT_EQ(infos.size(), variableMap_.at(property->GetName()).size());
                for (uint32_t j = 0; j < infos.size(); j++) {
                    ASSERT_EQ(infos[j], variableMap_.at(property->GetName())[j]);
                }
            }
        }

    private:
        NO_COPY_SEMANTIC(JsContainerTestChannel);
        NO_MOVE_SEMANTIC(JsContainerTestChannel);

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
                       "Cannot get source code of funtion" } },
            { "ArrayList", { "function", "Function", "function ArrayList( { [native code] }",
                             "function ArrayList() { [native code] }" } },
            { "Deque", { "function", "Function", "function Deque( { [native code] }",
                         "function Deque() { [native code] }" } },
            { "HashMap", { "function", "Function", "function HashMap( { [native code] }",
                           "function HashMap() { [native code] }" } },
            { "HashSet", { "function", "Function", "function HashSet( { [native code] }",
                           "function HashSet() { [native code] }" } },
            { "LightWeightMap", { "function", "Function", "function LightWeightMap( { [native code] }",
                                  "function LightWeightMap() { [native code] }" } },
            { "LightWeightSet", { "function", "Function", "function LightWeightSet( { [native code] }",
                                  "function LightWeightSet() { [native code] }" } },
            { "LinkedList", { "function", "Function", "function LinkedList( { [native code] }",
                              "function LinkedList() { [native code] }" } },
            { "List", { "function", "Function", "function List( { [native code] }",
                        "function List() { [native code] }" } },
            { "PlainArray", { "function", "Function", "function PlainArray( { [native code] }",
                              "function PlainArray() { [native code] }" } },
            { "Queue", { "function", "Function", "function Queue( { [native code] }",
                         "function Queue() { [native code] }" } },
            { "Stack", { "function", "Function", "function Stack( { [native code] }",
                         "function Stack() { [native code] }" } },
            { "TreeMap", { "function", "Function", "function TreeMap( { [native code] }",
                           "function TreeMap() { [native code] }" } },
            { "TreeSet", { "function", "Function", "function TreeSet( { [native code] }",
                           "function TreeSet() { [native code] }" } },
            { "Vector", { "function", "Function", "function Vector( { [native code] }",
                          "function Vector() { [native code] }" } },
            { "arrayList", { "object", "Object", "ArrayList", "[object ArrayList]", "size", "number", "6", "6",
                             "[[ArrayList]]", "object", "array", "Array", "Array(6)", "8,15,3,10,288,188" } },
            { "deque", { "object", "Object", "Deque", "[object Deque]", "size", "number", "10", "10", "[[Deque]]",
                         "object", "array", "Array", "Array(10)", "1888,5,3,13,1888,17,888,387,666,177" } },
            { "hashMap", { "object", "Object", "HashMap", "[object HashMap]", "size", "number", "1", "1",
                           "[[HashMap]]", "object", "array", "Array", "Array(1)", "[object Object]" } },
            { "hashSet", { "object", "Object", "HashSet", "[object HashSet]", "size", "number", "14",
                           "14", "[[HashSet]]", "object", "array", "Array", "Array(14)",
                           "17,1888,0,99,one,four,433,5,18,112,8,38,537,three" } },
            { "lightWeightMap", { "object", "Object", "LightWeightMap", "0:0", "size", "number", "1", "1",
                                  "[[LightWeightMap]]", "object", "array", "Array", "Array(1)", "[object Object]" } },
            { "lightWeightSet", { "object", "Object", "LightWeightSet", "4,66,388,566,578,593,855,100,one,four,three",
                                  "size", "number", "11", "11", "[[LightWeightSet]]", "object", "array", "Array",
                                  "Array(11)", "4,66,388,566,578,593,855,100,one,four,three" } },
            { "linkedList", { "object", "Object", "LinkedList", "[object Object]", "size", "number",
                              "9", "9", "[[LinkedList]]", "object", "array", "Array", "Array(9)",
                              "588,388,one,88,3989,1888,888,187,two" } },
            { "list", { "object", "Object", "List", "[object Object]", "size", "number",
                        "12", "12", "[[List]]", "object", "array", "Array", "Array(12)",
                        "18,changge,2,527,0,changge,three,88,changge,18,100,322" } },
            { "plainArray", { "object", "Object", "PlainArray", "8:8", "size", "number", "1", "1",
                              "[[PlainArray]]", "object", "array", "Array", "Array(1)", "[object Object]" } },
            { "queue", { "object", "Object", "Queue", "[object Queue]", "size", "number", "8", "8", "[[Queue]]",
                         "object", "array", "Array", "Array(8)", "3,2888,1,555,857,eleven,999,male" } },
            { "stack", { "object", "Object", "Stack", "[object Stack]", "size", "number", "7", "7", "[[Stack]]",
                         "object", "array", "Array", "Array(7)", "5,18,3,28,1,one,888" } },
            { "treeMap", { "object", "Object", "TreeMap", "[object TreeMap]", "size", "number", "1", "1",
                           "[[TreeMap]]", "object", "array", "Array", "Array(1)", "[object Object]" } },
            { "treeSet", { "object", "Object", "TreeSet", "[object TreeSet]", "size", "number",
                           "9", "9", "[[TreeSet]]", "object", "array", "Array", "Array(9)",
                           "5,18,99,377,588,888,1388,11199,array" } },
            { "vector", { "object", "Object", "Vector", "857,33,change,male,999,change,999,one", "size",
                          "number", "8", "8", "[[Vector]]", "object", "array", "Array", "Array(8)",
                          "857,33,change,male,999,change,999,one" } },
            { "boolean0", { "boolean", "false", "false" } },
            { "boolean1", { "boolean", "true", "true" } },
            { "boolean2", { "boolean", "false", "false" } },
            { "boolean3", { "boolean", "true", "true" } },
            { "number0", { "number", "1888", "1888" } },
            { "number1", { "number", "177", "177" } },
            { "number2", { "number", "-1", "-1" } },
            { "number3", { "number", "-1", "-1" } },
            { "number4", { "number", "222", "222" } },
            { "number5", { "number", "-2", "-2" } },
            { "number6", { "number", "566", "566" } },
            { "number7", { "number", "588", "588" } },
            { "number8", { "number", "588", "588" } },
            { "number9", { "number", "0", "0" } },
            { "number10", { "number", "-1", "-1" } },
            { "number11", { "number", "88", "88" } },
            { "number12", { "number", "18", "18" } },
            { "number13", { "number", "322", "322" } },
            { "number14", { "number", "8", "8" } },
            { "number15", { "number", "4", "4" } },
            { "number16", { "number", "8", "8" } },
            { "number17", { "number", "5", "5" } },
            { "number18", { "number", "188", "188" } },
            { "number19", { "number", "8", "8" } },
            { "number20", { "number", "388", "388" } },
            { "number21", { "number", "0", "0" } },
            { "number22", { "number", "5", "5" } },
            { "number23", { "number", "18", "18" } },
            { "number24", { "number", "1388", "1388" } },
            { "number25", { "number", "6", "6" } },
            { "number26", { "number", "0", "0" } },
            { "number27", { "number", "857", "857" } },
            { "string0", { "string", "two", "two" } },
            { "string1", { "string", "change", "change" } },
            { "string2", { "string", "three", "three" } },
            { "string3", { "string", "array", "array" } },
            { "string4", { "string", "male", "male" } },
            { "string5", { "string", "one", "one" } },
        };

        int32_t index_ {0};
        const EcmaVM *vm_ {nullptr};
        RuntimeImpl *runtime_ {nullptr};
    };

    std::string entryPoint_ = "_GLOBAL::func_main_0";
    JSPtLocation location_ {nullptr, JSPtLocation::EntityId(0), 0};
    size_t breakpointCounter_ = 0;
};

std::unique_ptr<TestEvents> GetJsContainerTest()
{
    return std::make_unique<JsContainerTest>();
}
}  // namespace panda::ecmascript::tooling::test

#endif  // ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_CONTAINER_TEST_H
