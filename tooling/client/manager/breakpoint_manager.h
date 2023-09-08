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
#ifndef ECMASCRIPT_TOOLING_CLIENT_MANAGER_BREAKPOINT_MANAGER_H
#define ECMASCRIPT_TOOLING_CLIENT_MANAGER_BREAKPOINT_MANAGER_H

#include <iostream>
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <string>
#include "pt_json.h"
#include "pt_types.h"
namespace OHOS::ArkCompiler::Toolchain {
using PtJson = panda::ecmascript::tooling::PtJson;
using Result = panda::ecmascript::tooling::Result;
struct Breaklocation{
    std::string breakpointId;
    std::string url;
    std::string lineNumber;
    std::string columnNumber;
};
class BreakPoint{
public:
    BreakPoint(const BreakPoint&) = delete;
    BreakPoint& operator=(const BreakPoint&) = delete;

    static BreakPoint& getInstance() {
        static BreakPoint instance;
        return instance;
    }
    std::vector<Breaklocation> breaklist_;
    std::vector<std::string> SplitString(std::string &str, const char delimiter);
    void Createbreaklocation(const std::unique_ptr<PtJson> json);
    void Show();
    void Deletebreaklist(unsigned int num);
private:
    BreakPoint() = default;
    ~BreakPoint() = default;
};
}
#endif