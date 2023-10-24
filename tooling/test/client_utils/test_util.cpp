/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "tooling/test/client_utils/test_util.h"

#include "tooling/client/domain/debugger_client.h"
#include "tooling/client/domain/runtime_client.h"
#include "tooling/client/utils/cli_command.h"
#include "tooling/client/session/session.h"

namespace panda::ecmascript::tooling::test {
TestMap TestUtil::testMap_;

MatchFunc MatchRule::replySuccess = [] (auto recv, auto) -> bool {
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

    int32_t code = 0;
    ret = result->GetInt("code", &code);
    if (ret != Result::NOT_EXIST) {
        return false;
    }
    return true;
};

std::ostream &operator<<(std::ostream &out, ActionRule value)
{
    const char *s = nullptr;

#define ADD_CASE(entry) \
    case (entry):       \
        s = #entry;     \
        break

    switch (value) {
        ADD_CASE(ActionRule::STRING_EQUAL);
        ADD_CASE(ActionRule::STRING_CONTAIN);
        ADD_CASE(ActionRule::CUSTOM_RULE);
        default: {
            ASSERT(false && "Unknown ActionRule");
        }
    }
#undef ADD_CASE

    return out << s;
}

void TestUtil::NotifySuccess()
{
    std::vector<std::string> cliCmdStr = { "success" };
    CliCommand cmd(cliCmdStr, 0);
    if (cmd.ExecCommand() == ErrCode::ERR_FAIL) {
        LOG_DEBUGGER(ERROR) << "ExecCommand Test.success fail";
    }
}

void TestUtil::NotifyFail()
{
    std::vector<std::string> cliCmdStr = { "fail" };
    CliCommand cmd(cliCmdStr, 0);
    if (cmd.ExecCommand() == ErrCode::ERR_FAIL) {
        LOG_DEBUGGER(ERROR) << "ExecCommand Test.fail fail";
    }
}

void TestUtil::ForkSocketClient([[maybe_unused]] int port, const std::string &name)
{
#ifdef OHOS_PLATFORM
    auto correntPid = getpid();
#endif
    pid_t pid = fork();
    if (pid < 0) {
        LOG_DEBUGGER(FATAL) << "fork error";
        UNREACHABLE();
    } else if (pid == 0) {
        LOG_DEBUGGER(INFO) << "fork son pid: " << getpid();
        std::this_thread::sleep_for(std::chrono::microseconds(500000));  // 500000: 500ms for wait debugger
#ifdef OHOS_PLATFORM
        std::string pidStr = std::to_string(correntPid);
        std::string sockInfo = pidStr + "PandaDebugger";
#else
        std::string sockInfo = std::to_string(port);
#endif
        int ret = SessionManager::getInstance().CreateTestSession(sockInfo);
        LOG_ECMA_IF(ret, FATAL) << "CreateTestSession fail";

        WebsocketClient &client = SessionManager::getInstance().GetCurrentSession()->GetWebsocketClient();
        auto &testAction = TestUtil::GetTest(name)->testAction;
        for (const auto &action: testAction) {
            LOG_DEBUGGER(INFO) << "message: " << action.message;
            bool success = true;
            if (action.action == SocketAction::SEND) {
                std::vector<std::string> cliCmdStr = Utils::SplitString(action.message, " ");
                CliCommand cmd(cliCmdStr, 0);
                success = (cmd.ExecCommand() == ErrCode::ERR_OK);
            } else {
                ASSERT(action.action == SocketAction::RECV);
                std::string recv = client.Decode();
                switch (action.rule) {
                    case ActionRule::STRING_EQUAL: {
                        success = (recv == action.message);
                        break;
                    }
                    case ActionRule::STRING_CONTAIN: {
                        success = (recv.find(action.message) != std::string::npos);
                        break;
                    }
                    case ActionRule::CUSTOM_RULE: {
                        success = action.matchFunc(recv, action.message);
                        break;
                    }
                }
                LOG_DEBUGGER(INFO) << "recv: " << recv;
                LOG_DEBUGGER(INFO) << "rule: " << action.rule;
            }
            if (!success) {
                LOG_DEBUGGER(ERROR) << "Notify fail";
                NotifyFail();
                SessionManager::getInstance().DelSessionById(0);
                exit(-1);
            }
        }

        NotifySuccess();
        SessionManager::getInstance().DelSessionById(0);
        exit(0);
    }
    LOG_DEBUGGER(INFO) << "ForkSocketClient end";
}
}  // namespace panda::ecmascript::tooling::test
