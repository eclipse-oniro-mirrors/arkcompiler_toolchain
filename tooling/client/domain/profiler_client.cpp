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

#include "domain/profiler_client.h"
#include "pt_types.h"
#include "log_wrapper.h"
#include "utils/utils.h"

#include <map>
#include <functional>
#include <cstring>
#include <fstream>

using Result = panda::ecmascript::tooling::Result;
using Profile = panda::ecmascript::tooling::Profile;
namespace OHOS::ArkCompiler::Toolchain {
ProfilerSingleton ProfilerSingleton::instance;
bool ProfilerClient::DispatcherCmd(int id, const std::string &cmd, std::string* reqStr)
{
    std::map<std::string, std::function<std::string()>> dispatcherTable {
        { "cpuprofile", std::bind(&ProfilerClient::CpuprofileCommand, this, id)},
        { "cpuprofile-stop", std::bind(&ProfilerClient::CpuprofileStopCommand, this, id)},
        { "cpuprofile-setSamplingInterval", std::bind(&ProfilerClient::SetSamplingIntervalCommand, this, id)},
        { "cpuprofile-enable", std::bind(&ProfilerClient::CpuprofileEnableCommand, this, id)},
        { "cpuprofile-disable", std::bind(&ProfilerClient::CpuprofileDisableCommand, this, id)},
    };

    auto entry = dispatcherTable.find(cmd);
    if (entry == dispatcherTable.end()) {
        *reqStr = "Unknown commond: " + cmd;
        LOGE("DispatcherCmd reqStr2: %{public}s", reqStr->c_str());
        return false;
    }
    *reqStr = entry->second();
    LOGE("DispatcherCmd reqStr1: %{public}s", reqStr->c_str());
    return true;
}

std::string ProfilerClient::CpuprofileEnableCommand(int id)
{
    idEventMap_.emplace(id, "cpuprofileenable");
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Profiler.enable");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string ProfilerClient::CpuprofileDisableCommand(int id)
{
    idEventMap_.emplace(id, "cpuprofiledisable");
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Profiler.disable");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string ProfilerClient::CpuprofileCommand(int id)
{
    idEventMap_.emplace(id, "cpuprofile");
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Profiler.start");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string ProfilerClient::CpuprofileStopCommand(int id)
{
    idEventMap_.emplace(id, "cpuprofilestop");
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Profiler.stop");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string ProfilerClient::SetSamplingIntervalCommand(int id)
{
    idEventMap_.emplace(id, "setsamplinginterval");
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Profiler.setSamplingInterval");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    params->Add("interval", interval_);
    request->Add("params", params);
    return request->Stringify();
}

ProfilerSingleton& ProfilerSingleton::getInstance()
{
    return instance;
}

void ProfilerClient::RecvProfilerResult(std::unique_ptr<PtJson> json)
{
    if (json == nullptr) {
        LOGE("toolchain_client: json parse error");
        return;
    }

    if (!json->IsObject()) {
        LOGE("toolchain_client: json parse format error");
        json->ReleaseRoot();
        return;
    }

    std::unique_ptr<PtJson> result;
    Result ret = json->GetObject("result", &result);
    if (ret != Result::SUCCESS) {
        LOGE("toolchain_client: find result error");
        return;
    }

    std::unique_ptr<PtJson> profile;
    ret = result->GetObject("profile", &profile);
    if (ret != Result::SUCCESS) {
        LOGE("toolchain_client: the cmd is not cp-stop!");
        return;
    }

    char date[16];
    char time[16];
    bool res = Utils::GetCurrentTime(date, time, sizeof(date));
    if (!res) {
        LOGE("arkdb: get time failed");
        return;
    }

    ProfilerSingleton& pro = ProfilerSingleton::getInstance();
    std::string fileName = "CPU-" + std::string(date) + "T" + std::string(time) + ".cpuprofile";
    std::string cpufile = pro.GetAddress() + fileName;
    std::cout << "toolchain_client: cpuprofile file name is " << cpufile << std::endl;
    std::cout << ">>> ";
    fflush(stdout);
    WriteCpuProfileForFile(cpufile, profile->Stringify());
    pro.AddCpuName(fileName);
}

bool ProfilerClient::WriteCpuProfileForFile(const std::string &fileName, const std::string &data)
{
    std::ofstream ofs;
    ofs.open(fileName.c_str(), std::ios::out);
    if (!ofs.is_open()) {
        LOGE("toolchain_client: file open error!");
        return false;
    }
    int strSize = data.size();
    ofs.write(data.c_str(), strSize);
    ofs.close();
    ofs.clear();
    ProfilerSingleton& pro = ProfilerSingleton::getInstance();
    pro.SetAddress("");
    return true;
}

void ProfilerClient::SetSamplingInterval(int interval)
{
    this->interval_ = interval;
}
} // OHOS::ArkCompiler::Toolchain
