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

#include <map>

#include "common/log_wrapper.h"
#include "manager/breakpoint_manager.h"
#include "manager/stack_manager.h"
#include "pt_json.h"

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
        { "list", std::bind(&DebuggerClient::ListCommand, this, id)},
        { "next", std::bind(&DebuggerClient::NextCommand, this, id)},
        { "ptype", std::bind(&DebuggerClient::PtypeCommand, this, id)},
        { "run", std::bind(&DebuggerClient::RunCommand, this, id)},
        { "setvar", std::bind(&DebuggerClient::SetvarCommand, this, id)},
        { "step", std::bind(&DebuggerClient::StepCommand, this, id)},
        { "undisplay", std::bind(&DebuggerClient::UndisplayCommand, this, id)},
        { "watch", std::bind(&DebuggerClient::WatchCommand, this, id)},
        { "resume", std::bind(&DebuggerClient::ResumeCommand, this, id)},
        { "step-into", std::bind(&DebuggerClient::StepIntoCommand, this, id)},
        { "step-out", std::bind(&DebuggerClient::StepOutCommand, this, id)},
        { "step-over", std::bind(&DebuggerClient::StepOverCommand, this, id)},
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
    std::string breakpointId = breakPointInfoList_.back().url;
    params->Add("breakpointId", breakpointId.c_str());
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

std::string DebuggerClient::StepIntoCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Debugger.stepInto");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string DebuggerClient::StepOutCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Debugger.stepOut");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string DebuggerClient::StepOverCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Debugger.stepOver");

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

void DebuggerClient::RecvReply(std::unique_ptr<PtJson> json)
{
    if (json == nullptr) {
        LOGE("arkdb: json parse error");
        return;
    }

    if (!json->IsObject()) {
        LOGE("arkdb: json parse format error");
        json->ReleaseRoot();
        return;
    }

    Result ret;
    std::string wholeMethod;
    std::string method;
    ret = json->GetString("method", &wholeMethod);
    if (ret != Result::SUCCESS) {
        LOGE("arkdb: find method error");
    }

    std::string::size_type length = wholeMethod.length();
    std::string::size_type indexPoint = 0;
    indexPoint = wholeMethod.find_first_of('.', 0);
    method = wholeMethod.substr(indexPoint + 1, length);
    if (method == "paused") {
        PausedReply(std::move(json));
        return;
    } else {
        LOGI("arkdb: Debugger reply is: %{public}s", json->Stringify().c_str());
    }

    std::unique_ptr<PtJson> result;
    ret = json->GetObject("result", &result);
    if (ret != Result::SUCCESS) {
        LOGE("arkdb: find result error");
        return;
    }

    std::string breakpointId;
    ret = result->GetString("breakpointId", &breakpointId);
    if (ret == Result::SUCCESS) {
        BreakPointManager &breakpoint = BreakPointManager::GetInstance();
        breakpoint.Createbreaklocation(std::move(json));
    }
}

void DebuggerClient::PausedReply(const std::unique_ptr<PtJson> json)
{
    if (json == nullptr) {
        LOGE("arkdb: json parse error");
        return;
    }

    if (!json->IsObject()) {
        LOGE("arkdb: json parse format error");
        json->ReleaseRoot();
        return;
    }

    std::unique_ptr<PtJson> params;
    Result ret = json->GetObject("params", &params);
    if (ret != Result::SUCCESS) {
        LOGE("arkdb: find params error");
        return;
    }

    std::unique_ptr<PtJson> callFrames;
    ret = params->GetArray("callFrames", &callFrames);
    if (ret != Result::SUCCESS) {
        LOGE("arkdb: find callFrames error");
        return;
    }

    std::map<int32_t, std::unique_ptr<CallFrame>> data;
    for (int32_t i = 0; i < callFrames->GetSize(); i++) {
        std::unique_ptr<CallFrame> callFrameInfo = CallFrame::Create(*(callFrames->Get(i)));
        data.emplace(i + 1, std::move(callFrameInfo));
    }

    StackManager &stackManager = StackManager::GetInstance();
    stackManager.ClearCallFrame();
    stackManager.SetCallFrames(std::move(data));
}
} // OHOS::ArkCompiler::Toolchain