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

namespace OHOS::ArkCompiler::Toolchain {
class HeapProfilerClient final {
public:
    HeapProfilerClient() = default;
    ~HeapProfilerClient() = default;

    bool DispatcherCmd(int id, const std::string cmd, std::string* reqStr);
    std::string HeapDumpCommand(int id);
    std::string AllocationTrackCommand(int id);
    std::string AllocationTrackStopCommand(int id);
    std::string Enable(int id);
    std::string Disable(int id);
    std::string Samping(int id);
    std::string SampingStop(int id);
    std::string CollectGarbage(int id);
private:

};
} //OHOS::ArkCompiler::Toolchain
#endif