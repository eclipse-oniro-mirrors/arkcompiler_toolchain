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

#ifndef ECMASCRIPT_TOOLING_CLIENT_DOMAIN_PROFILER_CLIENT_H
#define ECMASCRIPT_TOOLING_CLIENT_DOMAIN_PROFILER_CLIENT_H

#include <iostream>
#include <map>

#include <vector>

#include "pt_json.h"

namespace OHOS::ArkCompiler::Toolchain {
using PtJson = panda::ecmascript::tooling::PtJson;

class ProfilerSingleton {
public:
    ProfilerSingleton(const ProfilerSingleton&) = delete;
    ProfilerSingleton& operator=(const ProfilerSingleton&) = delete;

    static ProfilerSingleton& getInstance()
    {
        static ProfilerSingleton instance;
        return instance;
    }

    std::vector<std::string> SaveCpuName(const std::string &data)
    {
        cpulist_.emplace_back(data);
        return cpulist_;
    }

    void ShowCpuFile()
    {
        size_t size = cpulist_.size();
        for (size_t i = 0;i < size;i++) {
            std::cout << cpulist_[i] << std::endl;
        }
    }

    void SetAddress(std::string address)
    {
        address_ = address;
    }

    std::string GetAddress()
    {
        return address_;
    }

private:
    std::vector<std::string> cpulist_;
    std::string address_ = "";
    ProfilerSingleton() {}
    ~ProfilerSingleton() {}
};

class ProfilerClient final {
public:
    ProfilerClient() = default;
    ~ProfilerClient() = default;

    bool DispatcherCmd(int id, const std::string &cmd, std::string* reqStr);
    std::string CpuprofileCommand(int id);
    std::string CpuprofileStopCommand(int id);
    std::string SetSamplingIntervalCommand(int id);
    std::string CpuprofileEnableCommand(int id);
    std::string CpuprofileDisableCommand(int id);
    bool WriteCpuProfileForFile(const std::string &fileName, const std::string &data);
    void RecvProfilerResult(std::unique_ptr<PtJson> json);
    void SetSamplingInterval(int interval);

private:
    int32_t interval_ = 0;
    std::map<uint32_t, std::string> idEventMap_ {};
};
} //OHOS::ArkCompiler::Toolchain
#endif
