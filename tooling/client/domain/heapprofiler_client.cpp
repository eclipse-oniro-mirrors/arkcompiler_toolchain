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

#include "domain/heapprofiler_client.h"
#include "pt_json.h"
#include "log_wrapper.h"

#include <map>
#include <functional>
#include <cstring>

using PtJson = panda::ecmascript::tooling::PtJson;
namespace OHOS::ArkCompiler::Toolchain {
bool HeapProfilerClient::DispatcherCmd(int id, const std::string cmd, std::string* reqStr)
{
    std::map<std::string, std::function<std::string()>> dispatcherTable {
        { "allocationtrack", std::bind(&HeapProfilerClient::AllocationTrackCommand, this, id)},
        { "allocationtrack-stop", std::bind(&HeapProfilerClient::AllocationTrackStopCommand, this, id)},
        { "heapdump", std::bind(&HeapProfilerClient::HeapDumpCommand, this, id)},
        { "heapprofiler-enable", std::bind(&HeapProfilerClient::Enable, this, id)},
        { "heapprofiler-disable", std::bind(&HeapProfilerClient::Disable, this, id)},
        { "sampling", std::bind(&HeapProfilerClient::Samping, this, id)},
        { "sampling-stop", std::bind(&HeapProfilerClient::SampingStop, this, id)},
        { "collectgarbage", std::bind(&HeapProfilerClient::CollectGarbage, this, id)}
    };

    auto entry = dispatcherTable.find(cmd);

    if (entry != dispatcherTable.end() && entry->second != nullptr) {
        *reqStr = entry->second();
        LOGE("DispatcherCmd reqStr1: %{public}s", reqStr->c_str());
        return true;
    } else {
        *reqStr = "Unknown commond: " + cmd;
        LOGE("DispatcherCmd reqStr2: %{public}s", reqStr->c_str());
        return false;
    }
}

std::string HeapProfilerClient::HeapDumpCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "HeapProfiler.takeHeapSnapshot");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    params->Add("reportProgress", true);
    params->Add("captureNumericValue", true);
    params->Add("exposeInternals", false);
    request->Add("params", params);
    return request->Stringify();
}

std::string HeapProfilerClient::AllocationTrackCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "HeapProfiler.startTrackingHeapObjects");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    params->Add("trackAllocations", true);
    request->Add("params", params);
    return request->Stringify();
}

std::string HeapProfilerClient::AllocationTrackStopCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "HeapProfiler.stopTrackingHeapObjects");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    params->Add("reportProgress", true);
    request->Add("params", params);
    return request->Stringify();
}

std::string HeapProfilerClient::Enable(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "HeapProfiler.enable");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string HeapProfilerClient::Disable(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "HeapProfiler.disable");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string HeapProfilerClient::Samping([[maybe_unused]] int id)
{
    return "Samping";
}

std::string HeapProfilerClient::SampingStop([[maybe_unused]] int id)
{
    return "SampingStop";
}

std::string HeapProfilerClient::CollectGarbage([[maybe_unused]] int id)
{
    return "CollectGarbage";
}
} //OHOS::ArkCompiler::Toolchain