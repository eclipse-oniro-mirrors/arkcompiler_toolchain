/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "log_wrapper.h"

#ifdef ANDROID_PLATFORM
#include <android/log.h>
#include <string>
#endif

namespace OHOS::ArkCompiler::Toolchain {
#ifdef ANDROID_PLATFORM
const char* tag = "ArkCompiler";

void StripString(const std::string& prefix, std::string& str)
{
    for (auto pos = str.find(prefix, 0); pos != std::string::npos; pos = str.find(prefix, pos)) {
        str.erase(pos, prefix.size());
    }
}

std::string StripFormatString(const char* fmt)
{
    std::string newFmt(fmt);
    StripString("{public}", newFmt);
    StripString("{private}", newFmt);
    return newFmt;
}

void AndroidLog::PrintLog(LogLevel level, const char* fmt, ...)
{
    std::string formatted = StripFormatString(fmt);
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(static_cast<int>(level), tag, formatted.c_str(), args);
    va_end(args);
}
#endif
} // namespace OHOS::ArkCompiler::Toolchain