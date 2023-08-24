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

#ifndef ECMASCRIPT_TOOLING_CLIENT_DOMAIN_HEAPPROFILER_CLIENT_H
#define ECMASCRIPT_TOOLING_CLIENT_DOMAIN_HEAPPROFILER_CLIENT_H

#include <iostream>
#include <fstream>
#include <map>

#include "pt_json.h"

using PtJson = panda::ecmascript::tooling::PtJson;

namespace OHOS::ArkCompiler::Toolchain {
enum HeapProfilerEvent {
    DAFAULT_VALUE = 0,
    ALLOCATION = 1,
    ALLOCATION_STOP,
    HEAPDUMP,
    ENABLE,
    DISABLE,
    SAMPLING,
    SAMPLING_STOP,
    COLLECT_GARBAGE
};
class HeapProfilerClient final {
public:
    HeapProfilerClient() = default;
    ~HeapProfilerClient() = default;

    bool DispatcherCmd(int id, const std::string &cmd, const std::string &arg, std::string* reqStr);
    std::string HeapDumpCommand(int id);
    std::string AllocationTrackCommand(int id);
    std::string AllocationTrackStopCommand(int id);
    std::string Enable(int id);
    std::string Disable(int id);
    std::string Samping(int id);
    std::string SampingStop(int id);
    std::string CollectGarbage(int id);
    void RecvReply(std::unique_ptr<PtJson> json);
    bool WriteHeapProfilerForFile(std::string fileName, std::string data);

private:
    std::string fileName_;
    std::map<uint32_t, HeapProfilerEvent> idEventMap_;
    std::string path_;
    bool isAllocationMsg_ {false};
};
} //OHOS::ArkCompiler::Toolchain
#endif