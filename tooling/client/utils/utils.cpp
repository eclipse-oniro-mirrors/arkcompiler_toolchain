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

#include "utils/utils.h"
#include "log_wrapper.h"

#include <ctime>

namespace OHOS::ArkCompiler::Toolchain {
bool Utils::GetCurrentTime(char *date, char *tim, size_t size)
{
    time_t timep = time(nullptr);
    if (timep == -1) {
        LOGE("get timep failed");
        return false;
    }

    tm* currentDate = localtime(&timep);
    if (currentDate == nullptr) {
        LOGE("get currentDate failed");
        return false;
    }

    size_t timeResult = 0;
    timeResult = strftime(date, size, "%Y%m%d", currentDate);
    if (timeResult == 0) {
        LOGE("format date failed");
        return false;
    }

    tm* currentTime = localtime(&timep);
    if (currentTime == nullptr) {
        LOGE("get currentTime failed");
        return false;
    }

    timeResult = strftime(tim, size, "%H%M%S", currentTime);
    if (timeResult == 0) {
        LOGE("format time failed");
        return false;
    }
    return true;
}
} // OHOS::ArkCompiler::Toolchain