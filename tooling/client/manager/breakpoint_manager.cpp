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

#include "manager/breakpoint_manager.h"

#include <cstring>
#include <sstream>

#include "ark_cli/cli_command.h"
#include "log_wrapper.h"

using PtJson = panda::ecmascript::tooling::PtJson;
using Result = panda::ecmascript::tooling::Result;
namespace OHOS::ArkCompiler::Toolchain {
BreakPoint BreakPoint::instance_;
BreakPoint& BreakPoint::GetInstance()
{
    return instance_;
}

void BreakPoint::Createbreaklocation(const std::unique_ptr<PtJson> json)
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
    Result ret;
    std::unique_ptr<PtJson> result;
    ret = json->GetObject("result", &result);
    if (ret != Result::SUCCESS) {
        LOGE("arkdb: find result error");
        return;
    }
    std::string breakpointId;
    ret = result->GetString("breakpointId", &breakpointId);
    if (ret == Result::SUCCESS) {
        Breaklocation breaklocation;
        breaklocation.breakpointId = breakpointId;
        std::vector<std::string> breaksplitstring;
        breaksplitstring = SplitString(breakpointId, ':');
        breaklocation.lineNumber = breaksplitstring[1]; // 1: linenumber
        breaklocation.columnNumber = breaksplitstring[2]; // 2: columnnumber
        breaklocation.url = breaksplitstring[3]; // 3: url
        breaklist_.push_back(breaklocation);
    } else {
        LOGE("arkdb: find breakpointId error");
        return;
    }
}

std::vector<std::string> BreakPoint::SplitString(std::string &str, const char delimiter)
{
    int size = str.size();
    std::vector<std::string> value;
    for (int i = 0; i < size; i++) {
        if(str[i] == delimiter) {
           str[i] = ' ';
        }
    }
    std::istringstream out(str);
    std::string sstr;
    while (out >> sstr) {
        value.push_back(sstr);
    }
    return value;
}

void BreakPoint::Show()
{
    int size = breaklist_.size();
    for (int i = 0; i < size; i++) {
        std::cout << i + 1 << ':' << " url:" << breaklist_[i].url;
        std::cout << " lineNumber:" << breaklist_[i].lineNumber << " columnNumber:" << breaklist_[i].columnNumber << std::endl;
    }
}

void BreakPoint::Deletebreaklist(unsigned int num)
{
    std::vector<Breaklocation>::iterator it = breaklist_.begin() + num - 1;
    breaklist_.erase(it);
}

std::vector<Breaklocation> BreakPoint::Getbreaklist() const
{
    return breaklist_;
}
}