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

#ifndef ECMASCRIPT_TOOLING_CLIENT_TOOLCHAIN_CLI_COMMAND_H
#define ECMASCRIPT_TOOLING_CLIENT_TOOLCHAIN_CLI_COMMAND_H

#include <vector>
#include <map>
#include <cstdlib>
#include <functional>
#include "../websocket/websocket_client.h"
#include "log_wrapper.h"

namespace OHOS::ArkCompiler::Toolchain{
using ErrCode = int;
using StrPair = std::pair<std::string, std::string>;
using VecStr = std::vector<std::string>;
enum {
    ERR_OK                = 0,
    ERR_FAIL              = 1
};

class CliCommand {
public:
    CliCommand(ToolchainWebsocket* cliSocket, std::vector<std::string> cliCmdStr)
    {
        cliSocket_ = std::move(cliSocket);
        cmd_ = cliCmdStr[0];
        for (unsigned int i = 1; i < cliCmdStr.size(); i++) {
            argList_.push_back(cliCmdStr[i]);
        }
    }
    
    ~CliCommand()
    {
    }

    ErrCode OnCommand();
    std::string ExecCommand();
    std::string GetCommand(const std::string &str);
    ErrCode CreateCommandMap();
    ErrCode ExecAllocationTrackCommand();
    ErrCode ExecBreakCommand();
    ErrCode ExecBackTrackCommand();
    ErrCode ExecContinueCommand();
    ErrCode ExecCpuProfileCommand();
    ErrCode ExecDeleteCommand();
    ErrCode ExecDisableCommand();
    ErrCode ExecDisplayCommand();
    ErrCode ExecEnableCommand();
    ErrCode ExecFinishCommand();
    ErrCode ExecFrameCommand();
    ErrCode ExecHeapDumpCommand();
    ErrCode ExecHelpCommand();
    ErrCode ExecIgnoreCommand();
    ErrCode ExecInfoBCommand();
    ErrCode ExecInfoSCommand();
    ErrCode ExecJumpCommand();
    ErrCode ExecListCommand();
    ErrCode ExecNextCommand();
    ErrCode ExecPrintCommand();
    ErrCode ExecPtypeCommand();
    ErrCode ExecRunCommand();
    ErrCode ExecSetValueCommand();
    ErrCode ExecStepCommand();
    ErrCode ExecUndisplayCommand();
    ErrCode ExecWatchCommand();
    
private:
    std::string cmd_;
    VecStr argList_;
    std::map<StrPair, std::function<int()>> commandMap_;
    std::string resultReceiver_ = "";
    ToolchainWebsocket* cliSocket_ {nullptr};
};
} // namespace OHOS::ArkCompiler::Toolchain

#endif //ECMASCRIPT_TOOLING_CLIENT_TOOLCHAIN_CLI_COMMAND_H