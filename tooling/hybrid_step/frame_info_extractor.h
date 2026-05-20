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

#ifndef HYBRID_STEP_FRAME_INFO_EXTRACTOR_H
#define HYBRID_STEP_FRAME_INFO_EXTRACTOR_H

#include "frame_info.h"
#include <memory>
#include <mutex>
#include <unordered_map>

namespace panda::tooling::hybrid_step {

class FrameInfoExtractor {
public:

    static FrameInfoExtractor& Get();

    void RegisterProvider(bool isStaticFrame, const void *vm, std::unique_ptr<IFrameInfoProvider> provider);
    void UnregisterProvider(const void *vm);
    bool ExtractFrameInfo(const void *vm, const void *framePtr, bool isStaticFrame, UnifiedFrameInfo &info);
    bool ExtractStaticFrameInfo(const void *framePtr, UnifiedFrameInfo &info);
    bool ExtractDynamicFrameInfo(const void *vm, const void *framePtr, UnifiedFrameInfo &info);

private:
    FrameInfoExtractor() = default;
    ~FrameInfoExtractor() = default;

    FrameInfoExtractor(const FrameInfoExtractor&) = delete;
    FrameInfoExtractor& operator=(const FrameInfoExtractor&) = delete;
    FrameInfoExtractor(FrameInfoExtractor&&) = delete;
    FrameInfoExtractor& operator=(FrameInfoExtractor&&) = delete;

    std::mutex mutex_;
    std::unique_ptr<IFrameInfoProvider> staticProvider_;
    std::unordered_map<const void*, std::unique_ptr<IFrameInfoProvider>> dynamicProviders_;
};

} // namespace panda::tooling::hybrid_step

#endif // HYBRID_STEP_FRAME_INFO_EXTRACTOR_H
