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

#ifndef HYBRID_SINGLE_STEPPER_H
#define HYBRID_SINGLE_STEPPER_H

#include <mutex>
#include <shared_mutex>

enum class HybridStepDirection {
    DYNAMIC_TO_STATIC,
    STATIC_TO_DYNAMIC
};

class HybridSingleStepper {
public:
    // Get HybridSingleStepper instance
    static HybridSingleStepper& GetInstance();

    // Retrieve stepper flag based on step type
    bool GetHybridSingleStepFlag(HybridStepDirection type);

    // Set stepper flag based on step type
    void SetHybridSingleStepFlag(HybridStepDirection type, bool value);

private:
    bool GetFlagWithLock(std::shared_mutex &mutex, bool &flag);

    void SetFlagWithLock(std::shared_mutex &mutex, bool &flag, bool value);

    // Mutex for dynamic to static stepper flag
    std::shared_mutex dynToStatMutex_;
    // Mutex for static to dynamic stepper flag
    std::shared_mutex statToDynMutex_;

    bool dynamicToStatic_ { false };
    bool staticToDynamic_ { false };
};
#endif // HYBRID_SINGLE_STEPPER_H