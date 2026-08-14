/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TOOLING_TEST_TESTCASES_JS_MODULE_REEXPORT_TEST_H
#define ECMASCRIPT_TOOLING_TEST_TESTCASES_JS_MODULE_REEXPORT_TEST_H

#include "tooling/dynamic/test/client_utils/test_util.h"

namespace panda::ecmascript::tooling::test {
class JsModuleReexportTest : public TestActions {
public:
    JsModuleReexportTest()
    {
        testAction = {
            {SocketAction::SEND, "enable"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess},
            {SocketAction::SEND, "runtime-enable"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess},
            {SocketAction::SEND, "run"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess},
        };
        for (const auto &name : {"reexport_source", "reexport_middle"}) {
            testAction.push_back({SocketAction::RECV, "Debugger.scriptParsed", ActionRule::STRING_CONTAIN});
            testAction.push_back({SocketAction::RECV, "Debugger.paused", ActionRule::STRING_CONTAIN});
            testAction.push_back({SocketAction::SEND, "resume"});
            testAction.push_back({SocketAction::RECV, "Debugger.resumed", ActionRule::STRING_CONTAIN});
            testAction.push_back({SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess});
        }
        testAction.push_back({SocketAction::RECV, "Debugger.scriptParsed", ActionRule::STRING_CONTAIN});
        testAction.push_back({SocketAction::RECV, "Debugger.paused", ActionRule::STRING_CONTAIN});
        testAction.push_back({SocketAction::SEND, "b " DEBUGGER_JS_DIR "module_reexport.js 22"});
        testAction.push_back({SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess});
        testAction.push_back({SocketAction::SEND, "resume"});
        testAction.push_back({SocketAction::RECV, "Debugger.resumed", ActionRule::STRING_CONTAIN});
        testAction.push_back({SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess});
        testAction.push_back({SocketAction::RECV, "Debugger.paused", ActionRule::CUSTOM_RULE,
            [this](auto recv, auto, auto) -> bool { return RecvPausedReply(std::move(recv)); }});
        testAction.push_back({SocketAction::SEND, "watch foo"});
        testAction.push_back({SocketAction::RECV, "", ActionRule::CUSTOM_RULE,
            [this](auto recv, auto, auto) -> bool { return RecvWatchFunctionInfo(recv, "foo"); }});
        testAction.push_back({SocketAction::SEND, "watch reexport_num"});
        testAction.push_back({SocketAction::RECV, "", ActionRule::CUSTOM_RULE,
            [this](auto recv, auto, auto) -> bool { return RecvWatchVaribleInfo(recv, "number", "42"); }});
        testAction.push_back({SocketAction::SEND, "watch a"});
        testAction.push_back({SocketAction::RECV, "", ActionRule::CUSTOM_RULE,
            [this](auto recv, auto, auto) -> bool { return RecvWatchVaribleInfo(recv, "number", "1"); }});
        testAction.push_back({SocketAction::SEND, "watch b"});
        testAction.push_back({SocketAction::RECV, "", ActionRule::CUSTOM_RULE,
            [this](auto recv, auto, auto) -> bool { return RecvWatchVaribleInfo(recv, "number", "42"); }});
        testAction.push_back({SocketAction::SEND, "success"});
        testAction.push_back({SocketAction::SEND, "resume"});
        testAction.push_back({SocketAction::RECV, "Debugger.resumed", ActionRule::STRING_CONTAIN});
        testAction.push_back({SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess});
    }

    bool RecvPausedReply(std::string recv)
    {
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
    }

    bool RecvWatchVaribleInfo(std::string recv, std::string var_type, std::string var_value = "undefined")
    {
        std::unique_ptr<PtJson> json = PtJson::Parse(recv);
        Result ret;
        int id = 0;
        ret = json->GetInt("id", &id);
        if (ret != Result::SUCCESS) {
            return false;
        }

        std::unique_ptr<PtJson> result = nullptr;
        ret = json->GetObject("result", &result);
        if (ret != Result::SUCCESS) {
            return false;
        }

        std::unique_ptr<PtJson> watchResult = nullptr;
        ret = result->GetObject("result", &watchResult);
        if (ret != Result::SUCCESS) {
            return false;
        }

        std::string type = "";
        ret = watchResult->GetString("type", &type);
        if (ret != Result::SUCCESS || type != var_type) {
            return false;
        }
        if (type == "undefined") {
            return true;
        }

        std::string value = "";
        ret = watchResult->GetString("unserializableValue", &value);
        if (ret != Result::SUCCESS || value != var_value) {
            return false;
        }

        std::string description = "";
        ret = watchResult->GetString("description", &description);
        if (ret != Result::SUCCESS || description != var_value) {
            return false;
        }

        DebuggerClient debuggerClient(0);
        debuggerClient.RecvReply(std::move(json));
        return true;
    }

    bool RecvWatchFunctionInfo(std::string recv, std::string funcName)
    {
        std::unique_ptr<PtJson> json = PtJson::Parse(recv);
        Result ret;
        int id = 0;
        ret = json->GetInt("id", &id);
        if (ret != Result::SUCCESS) {
            return false;
        }

        std::unique_ptr<PtJson> result = nullptr;
        ret = json->GetObject("result", &result);
        if (ret != Result::SUCCESS) {
            return false;
        }

        std::unique_ptr<PtJson> watchResult = nullptr;
        ret = result->GetObject("result", &watchResult);
        if (ret != Result::SUCCESS) {
            return false;
        }

        std::string type = "";
        ret = watchResult->GetString("type", &type);
        if (ret != Result::SUCCESS || type != "function") {
            return false;
        }

        std::string description = "";
        ret = watchResult->GetString("description", &description);
        if (ret != Result::SUCCESS || description.find(funcName) == std::string::npos) {
            return false;
        }

        DebuggerClient debuggerClient(0);
        debuggerClient.RecvReply(std::move(json));
        return true;
    }

    std::pair<std::string, std::string> GetEntryPoint() override
    {
        return {pandaFile_, entryPoint_};
    }
    ~JsModuleReexportTest() = default;

private:
    std::string pandaFile_ = DEBUGGER_ABC_DIR "module_reexport.abc";
    std::string sourceFile_ = DEBUGGER_JS_DIR "module_reexport.js";
    std::string entryPoint_ = "module_reexport";
};

std::unique_ptr<TestActions> GetJsModuleReexportTest()
{
    return std::make_unique<JsModuleReexportTest>();
}
} // namespace panda::ecmascript::tooling::test

#endif // ECMASCRIPT_TOOLING_TEST_TESTCASES_JS_MODULE_REEXPORT_TEST_H