/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "hybrid_single_stepper.h"

HybridSingleStepper& HybridSingleStepper::GetInstance()
{
    static HybridSingleStepper instance;
    return instance;
}

bool HybridSingleStepper::GetFlagWithLock(std::shared_mutex &mutex, bool &flag)
{
    std::shared_lock<std::shared_mutex> readLock(mutex);
    return flag;
}

void HybridSingleStepper::SetFlagWithLock(std::shared_mutex &mutex, bool &flag, bool value)
{
    std::unique_lock<std::shared_mutex> writeLock(mutex);
    flag = value;
}

bool HybridSingleStepper::GetHybridSingleStepFlag(HybridStepDirection direction)
{
    bool result = false;
    switch (direction) {
        case HybridStepDirection::DYNAMIC_TO_STATIC:
            result = GetFlagWithLock(dynToStatMutex_, dynamicToStatic_);
            break;
        case HybridStepDirection::STATIC_TO_DYNAMIC:
            result = GetFlagWithLock(statToDynMutex_, staticToDynamic_);
            break;
        default:
            break;
    }
    return result;
}

void HybridSingleStepper::SetHybridSingleStepFlag(HybridStepDirection direction, bool value)
{
    switch (direction) {
        case HybridStepDirection::DYNAMIC_TO_STATIC:
            SetFlagWithLock(dynToStatMutex_, dynamicToStatic_, value);
            break;
        case HybridStepDirection::STATIC_TO_DYNAMIC:
            SetFlagWithLock(statToDynMutex_, staticToDynamic_, value);
            break;
        default:
            break;
    }
}
