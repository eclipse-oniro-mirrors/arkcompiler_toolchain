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

#include <cstring>

#include "domain_manager.h"
#include "log_wrapper.h"
#include "pt_json.h"
#include "manager/variable_manager.h"
#include "domain/runtime_client.h"

using PtJson = panda::ecmascript::tooling::PtJson;
using Result = panda::ecmascript::tooling::Result;
namespace OHOS::ArkCompiler::Toolchain {
void DomainManager::DispatcherReply(char* msg)
{
    std::string decMessage = std::string(msg);
    std::unique_ptr<PtJson> json = PtJson::Parse(decMessage);
    if (json == nullptr) {
        LOGE("json parse error");
        return;
    }

    if (!json->IsObject()) {
        LOGE("json parse format error");
        json->ReleaseRoot();
        return;
    }

    std::string domain;
    Result ret;
    int32_t id;
    ret = json->GetInt("id", &id);
    if (ret == Result::SUCCESS) {
        domain = GetDomainById(id);
        RemoveDomainById(id);
    }

    std::string wholeMethod;
    ret = json->GetString("method", &wholeMethod);
    if (ret == Result::SUCCESS) {
        std::string::size_type length = wholeMethod.length();
        std::string::size_type indexPoint;
        indexPoint = wholeMethod.find_first_of('.', 0);
        if (indexPoint == std::string::npos || indexPoint == 0 || indexPoint == length - 1) {
            return;
        }
        domain = wholeMethod.substr(0, indexPoint);
    }

    if (domain == "HeapProfiler") {
        heapProfilerClient_.RecvReply(std::move(json));
    } else if (domain == "Profiler") {
        profilerClient_.RecvProfilerResult(std::move(json));
    } else if (domain == "Runtime") {
        RuntimeClient &runtimeClient = RuntimeClient::getInstance();
        if (id == static_cast<int32_t>(runtimeClient.GetIdByMethod("getProperties"))) {
            VariableManager &variableManager = VariableManager::getInstance();
            variableManager.HandleMessage(std::move(json));
            variableManager.ShowVariableInfos();
        } else {
            LOGI("Runtime replay message is %{public}s", json->Stringify().c_str());
        }
    }else if (domain == "Debugger") {
        LOGI("Debugger replay message is %{public}s", json->Stringify().c_str());
    }
}
}