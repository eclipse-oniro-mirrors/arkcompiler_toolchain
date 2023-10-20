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

#include "tooling/client/domain/runtime_client.h"

#include "common/log_wrapper.h"
#include "tooling/client/manager/variable_manager.h"
#include "tooling/base/pt_json.h"

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
        LOGI("RuntimeClient DispatcherCmd reqStr1: %{public}s", reqStr->c_str());
        return true;
    } else {
        *reqStr = "Unknown commond: " + cmd;
        LOGI("RuntimeClient DispatcherCmd reqStr2: %{public}s", reqStr->c_str());
        return false;
    }
}

std::string RuntimeClient::HeapusageCommand(int id)
{
    idMethodMap_[id] = std::make_tuple("getHeapUsage", "");
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Runtime.getHeapUsage");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string RuntimeClient::RuntimeEnableCommand(int id)
{
    idMethodMap_[id] = std::make_tuple("enable", "");
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Runtime.enable");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string RuntimeClient::RuntimeDisableCommand(int id)
{
    idMethodMap_[id] = std::make_tuple("disable", "");
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Runtime.disable");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string RuntimeClient::RunIfWaitingForDebuggerCommand(int id)
{
    idMethodMap_[id] = std::make_tuple("runIfWaitingForDebugger", "");
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Runtime.runIfWaitingForDebugger");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string RuntimeClient::GetPropertiesCommand(int id)
{
    idMethodMap_[id] = std::make_tuple("getProperties", objectId_);
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
    idMethodMap_[id] = std::make_tuple("getProperties", objectId_);
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

void RuntimeClient::RecvReply(std::unique_ptr<PtJson> json)
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

    int replyId;
    Result ret = json->GetInt("id", &replyId);
    if (ret != Result::SUCCESS) {
        LOGE("arkdb: find id error");
        return;
    }

    if (GetMethodById(replyId) == "getHeapUsage") {
        HandleHeapUsage(std::move(json));
    } else if (GetMethodById(replyId) == "getProperties") {
        HandleGetProperties(std::move(json), replyId);
    } else {
        LOGI("arkdb: Runtime replay message is %{public}s", json->Stringify().c_str());
    }
}

std::string RuntimeClient::GetMethodById(const int &id)
{
    auto it = idMethodMap_.find(id);
    if (it != idMethodMap_.end()) {
        return std::get<0>(it->second);
    }
    return "";
}

std::string RuntimeClient::GetRequestObjectIdById(const int &id)
{
    auto it = idMethodMap_.find(id);
    if (it != idMethodMap_.end()) {
        return std::get<1>(it->second);
    }
    return "";
}

void RuntimeClient::HandleGetProperties(std::unique_ptr<PtJson> json, const int &id)
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

    std::unique_ptr<PtJson> result;
    Result ret = json->GetObject("result", &result);
    if (ret != Result::SUCCESS) {
        LOGE("arkdb: find result error");
        return;
    }

    std::unique_ptr<PtJson> innerResult;
    ret = result->GetArray("result", &innerResult);
    if (ret != Result::SUCCESS) {
        LOGE("arkdb: find innerResult error");
        return;
    }

    StackManager &stackManager = StackManager::GetInstance();
    VariableManager &variableManager = VariableManager::GetInstance();
    std::map<int32_t, std::map<int32_t, std::string>> treeInfo = stackManager.GetScopeChainInfo();
    if (isInitializeTree_) {
        variableManager.ClearVariableInfo();
        variableManager.InitializeTree(treeInfo);
    }
    std::string requestObjectId = GetRequestObjectIdById(id);
    TreeNode *node = nullptr;
    if (!isInitializeTree_) {
        node = variableManager.FindNodeWithObjectId(std::stoi(requestObjectId));
    } else {
        node = variableManager.FindNodeObjectZero();
    }

    for (int32_t i = 0; i < innerResult->GetSize(); i++) {
        std::unique_ptr<PropertyDescriptor> variableInfo = PropertyDescriptor::Create(*(innerResult->Get(i)));
        variableManager.AddVariableInfo(node, std::move(variableInfo));
    }

    std::cout << std::endl;
    variableManager.PrintVariableInfo();
}

void RuntimeClient::HandleHeapUsage(std::unique_ptr<PtJson> json)
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

    std::unique_ptr<PtJson> result;
    Result ret = json->GetObject("result", &result);
    if (ret != Result::SUCCESS) {
        LOGE("arkdb: find result error");
        return;
    }

    VariableManager &variableManager = VariableManager::GetInstance();
    std::unique_ptr<GetHeapUsageReturns> heapUsageReturns = GetHeapUsageReturns::Create(*result);
    variableManager.SetHeapUsageInfo(std::move(heapUsageReturns));
    variableManager.ShowHeapUsageInfo();
}
} // OHOS::ArkCompiler::Toolchain