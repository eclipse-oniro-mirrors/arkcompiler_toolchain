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
#include "pt_json.h"
#include "log_wrapper.h"

#include <map>
#include <functional>
#include <cstring>

using PtJson = panda::ecmascript::tooling::PtJson;
namespace OHOS::ArkCompiler::Toolchain {
bool DebugerClient::DispatcherCmd(int id, const std::string &cmd, std::string* reqStr)
{
    std::map<std::string, std::function<std::string()>> dispatcherTable {
        { "break", std::bind(&DebugerClient::BreakCommand, this, id)},
        { "backtrack", std::bind(&DebugerClient::BacktrackCommand, this, id)},
        { "continue", std::bind(&DebugerClient::ContinueCommand, this, id)},
        { "delete", std::bind(&DebugerClient::DeleteCommand, this, id)},
        { "jump", std::bind(&DebugerClient::JumpCommand, this, id)},
        { "disable", std::bind(&DebugerClient::DisableCommand, this, id)},
        { "display", std::bind(&DebugerClient::DisplayCommand, this, id)},
        { "enable", std::bind(&DebugerClient::EnableCommand, this, id)},
        { "finish", std::bind(&DebugerClient::FinishCommand, this, id)},
        { "frame", std::bind(&DebugerClient::FrameCommand, this, id)},
        { "ignore", std::bind(&DebugerClient::IgnoreCommand, this, id)},
        { "infobreakpoints", std::bind(&DebugerClient::InfobreakpointsCommand, this, id)},
        { "infosource", std::bind(&DebugerClient::InfosourceCommand, this, id)},
        { "list", std::bind(&DebugerClient::NextCommand, this, id)},
        { "next", std::bind(&DebugerClient::ListCommand, this, id)},
        { "print", std::bind(&DebugerClient::PrintCommand, this, id)},
        { "ptype", std::bind(&DebugerClient::PtypeCommand, this, id)},
        { "run", std::bind(&DebugerClient::RunCommand, this, id)},
        { "setvar", std::bind(&DebugerClient::SetvarCommand, this, id)},
        { "step", std::bind(&DebugerClient::StepCommand, this, id)},
        { "undisplay", std::bind(&DebugerClient::UndisplayCommand, this, id)},
        { "watch", std::bind(&DebugerClient::WatchCommand, this, id)},
        { "resume", std::bind(&DebugerClient::ResumeCommand, this, id)}
    };

    auto entry = dispatcherTable.find(cmd);

    if (entry != dispatcherTable.end()) {
        *reqStr = entry->second();
        LOGE("DebugerClient DispatcherCmd reqStr1: %{public}s", reqStr->c_str());
        return true;
    } else {
        *reqStr = "Unknown commond: " + cmd;
        LOGE("DebugerClient DispatcherCmd reqStr2: %{public}s", reqStr->c_str());
        return false;
    }
}

std::string DebugerClient::BreakCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Debugger.setBreakpointByUrl");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    params->Add("columnNumber", 0);
    params->Add("lineNumber", std::stoi(breakPointInfoList_.back().lineNumber));
    params->Add("url", breakPointInfoList_.back().url.c_str());
    request->Add("params", params);
    return request->Stringify();
}

std::string DebugerClient::BacktrackCommand([[maybe_unused]] int id)
{
    return "backtrack";
}

std::string DebugerClient::ContinueCommand([[maybe_unused]] int id)
{
    return "continue";
}

std::string DebugerClient::DeleteCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Debugger.removeBreakpoint");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string DebugerClient::DisableCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Debugger.disable");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string DebugerClient::DisplayCommand([[maybe_unused]] int id)
{
    return "display";
}

std::string DebugerClient::EnableCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Debugger.enable");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    // params->Add("maxScriptsCacheSize", 1.0E7);
    request->Add("params", params);
    return request->Stringify();
}

std::string DebugerClient::FinishCommand([[maybe_unused]] int id)
{
    return "finish";
}

std::string DebugerClient::FrameCommand([[maybe_unused]] int id)
{
    return "frame";
}

std::string DebugerClient::IgnoreCommand([[maybe_unused]] int id)
{
    return "ignore";
}

std::string DebugerClient::InfobreakpointsCommand([[maybe_unused]] int id)
{
    return "infobreakpoint";
}

std::string DebugerClient::InfosourceCommand([[maybe_unused]] int id)
{
    return "infosource";
}

std::string DebugerClient::JumpCommand([[maybe_unused]] int id)
{
    return "jump";
}

std::string DebugerClient::NextCommand([[maybe_unused]] int id)
{
    return "next";
}

std::string DebugerClient::ListCommand([[maybe_unused]] int id)
{
    return "list";
}

std::string DebugerClient::PrintCommand([[maybe_unused]] int id)
{
    return "print";
}

std::string DebugerClient::PtypeCommand([[maybe_unused]] int id)
{
    return "ptype";
}

std::string DebugerClient::RunCommand([[maybe_unused]] int id)
{
    return "run";
}

std::string DebugerClient::SetvarCommand([[maybe_unused]] int id)
{
    return "Debugger.setVariableValue";
}

std::string DebugerClient::StepCommand([[maybe_unused]] int id)
{
    return "step";
}

std::string DebugerClient::UndisplayCommand([[maybe_unused]] int id)
{
    return "undisplay";
}

std::string DebugerClient::WatchCommand([[maybe_unused]] int id)
{
    return "Debugger.evaluateOnCallFrame";
}

std::string DebugerClient::ResumeCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Debugger.resume");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

void DebugerClient::AddBreakPointInfo(std::string url, std::string lineNumber)
{
    BreakPointInfo breakPointInfo;
    breakPointInfo.url = url;
    breakPointInfo.lineNumber = lineNumber;
    breakPointInfoList_.emplace_back(breakPointInfo);
}
} //OHOS::ArkCompiler::Toolchain