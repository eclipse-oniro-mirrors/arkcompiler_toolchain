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
            // 29: breakpointer line
            location_ = TestUtil::GetLocation(g_sourceFile.c_str(), 29, 0, g_pandaFile.c_str());
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
            ASSERT_EQ(breakpointCounter_, 1U);  // 2: break point counter
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
            { "string1", { "string", "", "" } },
            { "number0", { "number", "1", "1" } },
            { "number1", { "number", "65535", "65535" } },
            { "obj0", { "object", "Object", "Object", "[object Object]",
                        "key0", "string", "value0", "value0",
                        "key1", "number", "100", "100" } },
            { "obj1", { "object", "Object", "Object", "[object Object]", "none" } },
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
