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

#ifndef ECMASCRIPT_TOOLING_TEST_TESTCASES_JS_SIMPLIFIED_MODE_TEST_H
#define ECMASCRIPT_TOOLING_TEST_TESTCASES_JS_SIMPLIFIED_MODE_TEST_H

#include "tooling/dynamic/test/client_utils/test_util.h"

namespace panda::ecmascript::tooling::test {
class JsSimplifiedModeTest : public TestActions {
public:
    JsSimplifiedModeTest()
    {
        testAction = {
            {SocketAction::SEND, "enable-simplified"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess},
            {SocketAction::SEND, "runtime-enable"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess},
            {SocketAction::SEND, "run"},
            {SocketAction::RECV, "", ActionRule::CUSTOM_RULE, MatchRule::replySuccess},
            {SocketAction::RECV, "Debugger.scriptParsed", ActionRule::STRING_CONTAIN},
            {SocketAction::RECV, "Debugger.paused", ActionRule::STRING_CONTAIN},
            {SocketAction::SEND, "backtrack"},
            {SocketAction::RECV, "DropFrame is not supported in simplified mode", ActionRule::STRING_CONTAIN},
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

    ~JsSimplifiedModeTest() = default;

private:
    std::string pandaFile_ = DEBUGGER_ABC_DIR "sample.abc";
    std::string entryPoint_ = "sample";
};

std::unique_ptr<TestActions> GetJsSimplifiedModeTest()
{
    return std::make_unique<JsSimplifiedModeTest>();
}
}  // namespace panda::ecmascript::tooling::test

#endif  // ECMASCRIPT_TOOLING_TEST_TESTCASES_JS_SIMPLIFIED_MODE_TEST_H
