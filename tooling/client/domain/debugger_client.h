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

#ifndef ECMASCRIPT_TOOLING_CLIENT_DOMAIN_DEBUGGER_CLIENT_H
#define ECMASCRIPT_TOOLING_CLIENT_DOMAIN_DEBUGGER_CLIENT_H

#include <iostream>
#include <vector>

namespace OHOS::ArkCompiler::Toolchain {
struct BreakPointInfo {
    std::string lineNumber;
    std::string url;
};
class DebuggerClient final {
public:
    DebuggerClient() = default;
    ~DebuggerClient() = default;

    bool DispatcherCmd(int id, const std::string &cmd, std::string* reqStr);
    std::string BreakCommand(int id);
    std::string BacktrackCommand(int id);
    std::string ContinueCommand(int id);
    std::string DeleteCommand(int id);
    std::string DisableCommand(int id);
    std::string DisplayCommand(int id);
    std::string EnableCommand(int id);
    std::string FinishCommand(int id);
    std::string FrameCommand(int id);
    std::string IgnoreCommand(int id);
    std::string InfobreakpointsCommand(int id);
    std::string InfosourceCommand(int id);
    std::string JumpCommand(int id);
    std::string NextCommand(int id);
    std::string ListCommand(int id);
    std::string PrintCommand(int id);
    std::string PtypeCommand(int id);
    std::string RunCommand(int id);
    std::string SetvarCommand(int id);
    std::string StepCommand(int id);
    std::string UndisplayCommand(int id);
    std::string WatchCommand(int id);
    std::string ResumeCommand(int id);

    void AddBreakPointInfo(std::string url, std::string lineNumber);

private:
    std::vector<BreakPointInfo> breakPointInfoList_ {};
};
} // OHOS::ArkCompiler::Toolchain
#endif