/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "frame_info_extractor.h"

namespace panda::tooling::hybrid_step {

FrameInfoExtractor &FrameInfoExtractor::Get()
{
    static FrameInfoExtractor instance;
    return instance;
}

void FrameInfoExtractor::RegisterProvider(
    bool isStaticFrame, const void *vm, std::unique_ptr<IFrameInfoProvider> provider)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (isStaticFrame) {
        staticProvider_ = std::move(provider);
    } else {
        dynamicProviders_[vm] = std::move(provider);
    }
}

void FrameInfoExtractor::UnregisterProvider(const void *vm)
{
    std::lock_guard<std::mutex> lock(mutex_);
    dynamicProviders_.erase(vm);
}

bool FrameInfoExtractor::ExtractFrameInfo(
    const void *vm, const void *framePtr, bool isStaticFrame, UnifiedFrameInfo &info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    info.isStaticFrame = isStaticFrame;

    if (isStaticFrame) {
        if (staticProvider_ != nullptr) {
            return staticProvider_->ExtractFrameInfo(framePtr, info);
        }
        info.hasInfo = false;
        return false;
    }
    auto it = dynamicProviders_.find(vm);
    if (it != dynamicProviders_.end() && it->second != nullptr) {
        return it->second->ExtractFrameInfo(framePtr, info);
    }
    info.hasInfo = false;
    return false;
}

bool FrameInfoExtractor::ExtractStaticFrameInfo(const void *framePtr, UnifiedFrameInfo &info)
{
    return ExtractFrameInfo(nullptr, framePtr, true, info);
}

bool FrameInfoExtractor::ExtractDynamicFrameInfo(const void *vm, const void *framePtr, UnifiedFrameInfo &info)
{
    return ExtractFrameInfo(vm, framePtr, false, info);
}

} // namespace panda::tooling::hybrid_step
