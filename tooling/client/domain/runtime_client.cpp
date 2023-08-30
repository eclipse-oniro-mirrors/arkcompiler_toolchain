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

#include "domain/runtime_client.h"
#include "pt_json.h"
#include "log_wrapper.h"

using PtJson = panda::ecmascript::tooling::PtJson;
namespace OHOS::ArkCompiler::Toolchain {
bool RuntimeClient::DispatcherCmd(int id, const std::string &cmd, std::string* reqStr)
{
    std::map<std::string, std::function<std::string()>> dispatcherTable {
        { "heapusage", std::bind(&RuntimeClient::HeapusageCommand, this, id)},
        { "runtime-enable", std::bind(&RuntimeClient::RuntimeEnableCommand, this, id)},
        { "runtime-disable", std::bind(&RuntimeClient::RuntimeDisableCommand, this, id)},
        { "print", std::bind(&RuntimeClient::GetPropertiesCommand, this, id)},
        { "print2", std::bind(&RuntimeClient::GetPropertiesCommand2, this, id)},
        { "run", std::bind(&RuntimeClient::RunIfWaitingForDebuggerCommand, this, id)},
    };

    auto entry = dispatcherTable.find(cmd);
    if (entry != dispatcherTable.end()) {
        *reqStr = entry->second();
        LOGE("RuntimeClient DispatcherCmd reqStr1: %{public}s", reqStr->c_str());
        return true;
    } else {
        *reqStr = "Unknown commond: " + cmd;
        LOGE("RuntimeClient DispatcherCmd reqStr2: %{public}s", reqStr->c_str());
        return false;
    }
}

std::string RuntimeClient::HeapusageCommand(int id)
{
    idMethodMap_.emplace("getHeapUsage", id);
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Runtime.getHeapUsage");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string RuntimeClient::RuntimeEnableCommand(int id)
{
    idMethodMap_.emplace("enable", id);
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Runtime.enable");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string RuntimeClient::RuntimeDisableCommand(int id)
{
    idMethodMap_.emplace("disable", id);
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Runtime.disable");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string RuntimeClient::RunIfWaitingForDebuggerCommand(int id)
{
    idMethodMap_.emplace("runIfWaitingForDebugger", id);
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Runtime.runIfWaitingForDebugger");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string RuntimeClient::GetPropertiesCommand(int id)
{
    idMethodMap_.emplace("getProperties", id);
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Runtime.getProperties");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    params->Add("accessorPropertiesOnly", false);
    params->Add("generatePreview", true);
    params->Add("objectId", objectId_.c_str());
    params->Add("ownProperties", true);
    request->Add("params", params);
    return request->Stringify();
}

std::string RuntimeClient::GetPropertiesCommand2(int id)
{
    idMethodMap_.emplace("getProperties", id);
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Runtime.getProperties");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    params->Add("accessorPropertiesOnly", true);
    params->Add("generatePreview", true);
    params->Add("objectId", "0");
    params->Add("ownProperties", false);
    request->Add("params", params);
    return request->Stringify();
}

int RuntimeClient::GetIdByMethod(const std::string method)
{
    auto it = idMethodMap_.find(method);
    if (it != idMethodMap_.end()) {
        return it->second;
    }
    return 0;
}
} // OHOS::ArkCompiler::Toolchain