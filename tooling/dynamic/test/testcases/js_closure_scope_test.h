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

#ifndef ECMASCRIPT_TOOLING_TEST_TESTCASES_JS_CLOSURE_SCOPE_TEST_H
#define ECMASCRIPT_TOOLING_TEST_TESTCASES_JS_CLOSURE_SCOPE_TEST_H

#include <map>
#include "tooling/dynamic/test/client_utils/test_util.h"

namespace panda::ecmascript::tooling::test {
class JsClosureScopeTest : public TestActions {
public:
    JsClosureScopeTest()
    {
        testAction = {
            {SocketAction::SEND, "enable"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess},
            {SocketAction::SEND, "runtime-enable"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess},
            {SocketAction::SEND, "run"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess},
            // load closure_scope.js
            {SocketAction::RECV, "Debugger.scriptParsed", ActionRule::STRING_CONTAIN},
            // break on start
            {SocketAction::RECV, "Debugger.paused", ActionRule::STRING_CONTAIN},
            // set first breakpoint
            {SocketAction::SEND, "b " DEBUGGER_JS_DIR "closure_scope.js 30"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess},
            // // set second breakpoint
            {SocketAction::SEND, "b " DEBUGGER_JS_DIR "closure_scope.js 53"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess},

            // hit breakpoint after resume first time
            {SocketAction::SEND, "resume"},
            {SocketAction::RECV, "Debugger.resumed", ActionRule::STRING_CONTAIN},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess},
            {SocketAction::RECV, "Debugger.paused", ActionRule::CUSTOM_RULE, [] (auto recv, auto, auto) -> bool {
                std::unique_ptr<PtJson> json = PtJson::Parse(recv);
                Result ret;
                std::string method;
                ret = json->GetString("method", &method);
                if (ret != Result::SUCCESS || method != "Debugger.paused") {
                    return false;
                }

                DebuggerClient debuggerClient(0);
                debuggerClient.PausedReply(std::move(json));
                return true;
            }},
            {SocketAction::SEND, "print"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, [] (auto recv, auto, auto) -> bool {
                std::unique_ptr<PtJson> json = PtJson::Parse(recv);
                int32_t id = 0;
                Result ret = json->GetInt("id", &id);
                if (ret != Result::SUCCESS) {
                    return false;
                }
                RuntimeClient runtimeClient(0);
                runtimeClient.HandleGetProperties(std::move(json), id);
                return true;
            }},
            {SocketAction::SEND, "print 2"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, [] (auto recv, auto, auto) -> bool {
                std::unique_ptr<PtJson> json = PtJson::Parse(recv);
                Result ret;
                int32_t id = 0;
                ret = json->GetInt("id", &id);
                if (ret != Result::SUCCESS) {
                    return false;
                }

                std::unique_ptr<PtJson> result = nullptr;
                ret = json->GetObject("result", &result);
                if (ret != Result::SUCCESS) {
                    return false;
                }

                std::unique_ptr<PtJson> innerResult;
                ret = result->GetArray("result", &innerResult);
                if (ret != Result::SUCCESS) {
                    return false;
                }

                return CheckScopeVariable(innerResult, "v3", "4");
            }},
            {SocketAction::SEND, "print 3"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, [] (auto recv, auto, auto) -> bool {
                std::unique_ptr<PtJson> json = PtJson::Parse(recv);
                Result ret;
                int32_t id = 0;
                ret = json->GetInt("id", &id);
                if (ret != Result::SUCCESS) {
                    return false;
                }

                std::unique_ptr<PtJson> result = nullptr;
                ret = json->GetObject("result", &result);
                if (ret != Result::SUCCESS) {
                    return false;
                }

                std::unique_ptr<PtJson> innerResult;
                ret = result->GetArray("result", &innerResult);
                if (ret != Result::SUCCESS) {
                    return false;
                }

                return CheckScopeVariable(innerResult, "v2_2", "3") &&
                       CheckScopeVariable(innerResult, "v2_1", "2");
            }},
            {SocketAction::SEND, "print 4"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, [] (auto recv, auto, auto) -> bool {
                std::unique_ptr<PtJson> json = PtJson::Parse(recv);
                Result ret;
                int32_t id = 0;
                ret = json->GetInt("id", &id);
                if (ret != Result::SUCCESS) {
                    return false;
                }

                std::unique_ptr<PtJson> result = nullptr;
                ret = json->GetObject("result", &result);
                if (ret != Result::SUCCESS) {
                    return false;
                }

                std::unique_ptr<PtJson> innerResult;
                ret = result->GetArray("result", &innerResult);
                if (ret != Result::SUCCESS) {
                    return false;
                }

                return CheckScopeVariable(innerResult, "v1", "1");
            }},

            // hit breakpoint after resume second time
            {SocketAction::SEND, "resume"},
            {SocketAction::RECV, "Debugger.resumed", ActionRule::STRING_CONTAIN},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess},
            {SocketAction::RECV, "Debugger.paused", ActionRule::CUSTOM_RULE, [] (auto recv, auto, auto) -> bool {
                std::unique_ptr<PtJson> json = PtJson::Parse(recv);
                Result ret;
                std::string method;
                ret = json->GetString("method", &method);
                if (ret != Result::SUCCESS || method != "Debugger.paused") {
                    return false;
                }
                DebuggerClient debuggerClient(0);
                debuggerClient.PausedReply(std::move(json));
                return true;
            }},
            {SocketAction::SEND, "print"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, [] (auto recv, auto, auto) -> bool {
                std::unique_ptr<PtJson> json = PtJson::Parse(recv);
                int32_t id = 0;
                Result ret = json->GetInt("id", &id);
                if (ret != Result::SUCCESS) {
                    return false;
                }
                RuntimeClient runtimeClient(0);
                runtimeClient.HandleGetProperties(std::move(json), id);
                return true;
            }},
            {SocketAction::SEND, "print 2"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, [] (auto recv, auto, auto) -> bool {
                std::unique_ptr<PtJson> json = PtJson::Parse(recv);
                Result ret;
                int32_t id = 0;
                ret = json->GetInt("id", &id);
                if (ret != Result::SUCCESS) {
                    return false;
                }

                std::unique_ptr<PtJson> result = nullptr;
                ret = json->GetObject("result", &result);
                if (ret != Result::SUCCESS) {
                    return false;
                }

                std::unique_ptr<PtJson> innerResult;
                ret = result->GetArray("result", &innerResult);
                if (ret != Result::SUCCESS) {
                    return false;
                }

                return CheckScopeVariable(innerResult, "i", "5");
            }},
            {SocketAction::SEND, "print 3"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, [] (auto recv, auto, auto) -> bool {
                std::unique_ptr<PtJson> json = PtJson::Parse(recv);
                Result ret;
                int32_t id = 0;
                ret = json->GetInt("id", &id);
                if (ret != Result::SUCCESS) {
                    return false;
                }

                std::unique_ptr<PtJson> result = nullptr;
                ret = json->GetObject("result", &result);
                if (ret != Result::SUCCESS) {
                    return false;
                }

                std::unique_ptr<PtJson> innerResult;
                ret = result->GetArray("result", &innerResult);
                if (ret != Result::SUCCESS) {
                    return false;
                }

                return CheckScopeVariable(innerResult, "a", "10");
            }},
            // reply success and run
            {SocketAction::SEND, "success"},
            {SocketAction::SEND, "resume"},
            {SocketAction::RECV, "Debugger.resumed", ActionRule::STRING_CONTAIN},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess},
        };
    }

    std::pair<std::string, std::string> GetEntryPoint() override
    {
        return {pandaFile_, entryPoint_};
    }
    ~JsClosureScopeTest() = default;

private:
    static bool CheckScopeVariable(const std::unique_ptr<PtJson> &innerResult,
                                   const std::string &expectName, const std::string &expectValue)
    {
        for (int32_t i = 0; i < innerResult->GetSize(); i++) {
            std::string name;
            if (innerResult->Get(i)->GetString("name", &name) != Result::SUCCESS) {
                return false;
            }
            if (name != expectName) {
                continue;
            }
            std::unique_ptr<PtJson> value;
            if (innerResult->Get(i)->GetObject("value", &value) != Result::SUCCESS) {
                return false;
            }
            std::string valueDes;
            if (value->GetString("description", &valueDes) != Result::SUCCESS) {
                return false;
            }
            return valueDes == expectValue;
        }
        return false;
    }

    std::string pandaFile_ = DEBUGGER_ABC_DIR "closure_scope.abc";
    std::string sourceFile_ = DEBUGGER_JS_DIR "closure_scope.js";
    std::string entryPoint_ = "closure_scope";
};

std::unique_ptr<TestActions> GetJsClosureScopeTest()
{
    return std::make_unique<JsClosureScopeTest>();
}
}  // namespace panda::ecmascript::tooling::test

#endif  // ECMASCRIPT_TOOLING_TEST_TESTCASES_JS_CLOSURE_SCOPE_TEST_H
