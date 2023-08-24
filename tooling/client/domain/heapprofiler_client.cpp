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
#include "log_wrapper.h"

#include <map>
#include <functional>
#include <cstring>

using Result = panda::ecmascript::tooling::Result;
namespace OHOS::ArkCompiler::Toolchain {
bool HeapProfilerClient::DispatcherCmd(int id, const std::string &cmd, const std::string &arg, std::string* reqStr)
{
    if (reqStr == nullptr) {
        return false;
    }
    path_ = arg;

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
    idEventMap_.emplace(id, HEAPDUMP);
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
    idEventMap_.emplace(id, ALLOCATION);
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
    idEventMap_.emplace(id, ALLOCATION_STOP);
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
    idEventMap_.emplace(id, ENABLE);
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "HeapProfiler.enable");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string HeapProfilerClient::Disable(int id)
{
    idEventMap_.emplace(id, DISABLE);
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "HeapProfiler.disable");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string HeapProfilerClient::Samping(int id)
{
    idEventMap_.emplace(id, SAMPLING);
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "HeapProfiler.startSampling");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    params->Add("samplingInterval", 16384);
    request->Add("params", params);
    return request->Stringify();
}

std::string HeapProfilerClient::SampingStop(int id)
{
    idEventMap_.emplace(id, SAMPLING_STOP);
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "HeapProfiler.stopSampling");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string HeapProfilerClient::CollectGarbage(int id)
{
    idEventMap_.emplace(id, COLLECT_GARBAGE);
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "HeapProfiler.collectGarbage");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

void HeapProfilerClient::RecvReply(std::unique_ptr<PtJson> json)
{
    if (json == nullptr) {
        LOGE("json parse error");
        return;
    }

    if (!json->IsObject()) {
        LOGE("json parse format error");
        json->ReleaseRoot();
        return;
    }

    Result ret;
    std::unique_ptr<PtJson> params;
    ret = json->GetObject("params", &params);
    if (ret != Result::SUCCESS) {
        LOGE("find params error");
        return;
    }
    
    std::string wholeMethod;
    std::string method;
    ret = json->GetString("method", &wholeMethod);
    if (ret == Result::SUCCESS) {
        std::string::size_type length = wholeMethod.length();
        std::string::size_type indexPoint;
        indexPoint = wholeMethod.find_first_of('.', 0);
        if (indexPoint == std::string::npos || indexPoint == 0 || indexPoint == length - 1) {
            return;
        }
        method = wholeMethod.substr(indexPoint + 1, length);
        if (method == "lastSeenObjectId") {
            isAllocationMsg_ = true;
        }
    }

    std::string chunk;
    ret = params->GetString("chunk", &chunk);
    if (ret != Result::SUCCESS) {
        LOGE("find chunk error");
        return;
    }

    std::string head = "{\"snapshot\":\n";
    if (!strncmp(chunk.c_str(), head.c_str(), head.length())) {
        time_t timep;
        time(&timep);
        char tmp1[16];
        char tmp2[16];
        strftime(tmp1, sizeof(tmp1), "%Y%m%d", localtime(&timep));
        strftime(tmp2, sizeof(tmp2), "%H%M%S", localtime(&timep));
        if (isAllocationMsg_) {
            fileName_ = "Heap-" + std::string(tmp1) + "T" + std::string(tmp2) + ".heaptimeline";
        } else {
            fileName_ = "Heap-" + std::string(tmp1) + "T" + std::string(tmp2) + ".heapsnapshot";
        }
        std::cout << "file name is " << fileName_ << std::endl;
        std::cout << ">>> ";
        fflush(stdout);
    }

    std::string tail = "]\n}\n";
    std::string subStr = chunk.substr(chunk.length() - tail.length(), chunk.length());
    if (!strncmp(subStr.c_str(), tail.c_str(), tail.length())) {
        isAllocationMsg_ = false;
    }
    WriteHeapProfilerForFile(fileName_, chunk);
}

bool HeapProfilerClient::WriteHeapProfilerForFile(std::string fileName, std::string data)
{
    std::ofstream ofs;
    std::string pathname = path_ + fileName;
    ofs.open(pathname.c_str(), std::ios::app);
    if (!ofs.is_open()) {
        LOGE("toolchain_client: file open error!");
        return false;
    }
    int strSize = data.size();
    ofs.write(data.c_str(), strSize);
    ofs.close();
    ofs.clear();
    return true;
}
} //OHOS::ArkCompiler::Toolchain