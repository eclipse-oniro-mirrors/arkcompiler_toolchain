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

#include "domain/debugger_client.h"

#include "log_wrapper.h"
#include "pt_json.h"

#include <map>

using PtJson = panda::ecmascript::tooling::PtJson;
namespace OHOS::ArkCompiler::Toolchain {
bool DebuggerClient::DispatcherCmd(int id, const std::string &cmd, std::string* reqStr)
{
    std::map<std::string, std::function<std::string()>> dispatcherTable {
        { "break", std::bind(&DebuggerClient::BreakCommand, this, id)},
        { "backtrack", std::bind(&DebuggerClient::BacktrackCommand, this, id)},
        { "continue", std::bind(&DebuggerClient::ContinueCommand, this, id)},
        { "delete", std::bind(&DebuggerClient::DeleteCommand, this, id)},
        { "jump", std::bind(&DebuggerClient::JumpCommand, this, id)},
        { "disable", std::bind(&DebuggerClient::DisableCommand, this, id)},
        { "display", std::bind(&DebuggerClient::DisplayCommand, this, id)},
        { "enable", std::bind(&DebuggerClient::EnableCommand, this, id)},
        { "finish", std::bind(&DebuggerClient::FinishCommand, this, id)},
        { "frame", std::bind(&DebuggerClient::FrameCommand, this, id)},
        { "ignore", std::bind(&DebuggerClient::IgnoreCommand, this, id)},
        { "infobreakpoints", std::bind(&DebuggerClient::InfobreakpointsCommand, this, id)},
        { "infosource", std::bind(&DebuggerClient::InfosourceCommand, this, id)},
        { "list", std::bind(&DebuggerClient::NextCommand, this, id)},
        { "next", std::bind(&DebuggerClient::ListCommand, this, id)},
        { "print", std::bind(&DebuggerClient::PrintCommand, this, id)},
        { "ptype", std::bind(&DebuggerClient::PtypeCommand, this, id)},
        { "run", std::bind(&DebuggerClient::RunCommand, this, id)},
        { "setvar", std::bind(&DebuggerClient::SetvarCommand, this, id)},
        { "step", std::bind(&DebuggerClient::StepCommand, this, id)},
        { "undisplay", std::bind(&DebuggerClient::UndisplayCommand, this, id)},
        { "watch", std::bind(&DebuggerClient::WatchCommand, this, id)},
        { "resume", std::bind(&DebuggerClient::ResumeCommand, this, id)}
    };

    auto entry = dispatcherTable.find(cmd);
    if (entry != dispatcherTable.end()) {
        *reqStr = entry->second();
        LOGE("DebuggerClient DispatcherCmd reqStr1: %{public}s", reqStr->c_str());
        return true;
    }

    *reqStr = "Unknown commond: " + cmd;
    LOGE("DebuggerClient DispatcherCmd reqStr2: %{public}s", reqStr->c_str());
    return false;
}

std::string DebuggerClient::BreakCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Debugger.setBreakpointByUrl");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    params->Add("columnNumber", breakPointInfoList_.back().columnNumber);
    params->Add("lineNumber", breakPointInfoList_.back().lineNumber);
    params->Add("url", breakPointInfoList_.back().url.c_str());
    request->Add("params", params);
    return request->Stringify();
}

std::string DebuggerClient::BacktrackCommand([[maybe_unused]] int id)
{
    return "backtrack";
}

std::string DebuggerClient::ContinueCommand([[maybe_unused]] int id)
{
    return "continue";
}

std::string DebuggerClient::DeleteCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Debugger.removeBreakpoint");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string DebuggerClient::DisableCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Debugger.disable");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string DebuggerClient::DisplayCommand([[maybe_unused]] int id)
{
    return "display";
}

std::string DebuggerClient::EnableCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Debugger.enable");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string DebuggerClient::FinishCommand([[maybe_unused]] int id)
{
    return "finish";
}

std::string DebuggerClient::FrameCommand([[maybe_unused]] int id)
{
    return "frame";
}

std::string DebuggerClient::IgnoreCommand([[maybe_unused]] int id)
{
    return "ignore";
}

std::string DebuggerClient::InfobreakpointsCommand([[maybe_unused]] int id)
{
    return "infobreakpoint";
}

std::string DebuggerClient::InfosourceCommand([[maybe_unused]] int id)
{
    return "infosource";
}

std::string DebuggerClient::JumpCommand([[maybe_unused]] int id)
{
    return "jump";
}

std::string DebuggerClient::NextCommand([[maybe_unused]] int id)
{
    return "next";
}

std::string DebuggerClient::ListCommand([[maybe_unused]] int id)
{
    return "list";
}

std::string DebuggerClient::PrintCommand([[maybe_unused]] int id)
{
    return "print";
}

std::string DebuggerClient::PtypeCommand([[maybe_unused]] int id)
{
    return "ptype";
}

std::string DebuggerClient::RunCommand([[maybe_unused]] int id)
{
    return "run";
}

std::string DebuggerClient::SetvarCommand([[maybe_unused]] int id)
{
    return "Debugger.setVariableValue";
}

std::string DebuggerClient::StepCommand([[maybe_unused]] int id)
{
    return "step";
}

std::string DebuggerClient::UndisplayCommand([[maybe_unused]] int id)
{
    return "undisplay";
}

std::string DebuggerClient::WatchCommand([[maybe_unused]] int id)
{
    return "Debugger.evaluateOnCallFrame";
}

std::string DebuggerClient::ResumeCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Debugger.resume");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

void DebuggerClient::AddBreakPointInfo(const std::string& url, const int& lineNumber, const int& columnNumber)
{
    BreakPointInfo breakPointInfo;
    breakPointInfo.url = url;
    breakPointInfo.lineNumber = lineNumber;
    breakPointInfo.columnNumber = columnNumber;
    breakPointInfoList_.emplace_back(breakPointInfo);
}
} // OHOS::ArkCompiler::Toolchain