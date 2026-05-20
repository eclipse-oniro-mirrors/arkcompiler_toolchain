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

#ifndef PANDA_TOOLING_INSPECTOR_HYBRID_STATIC_FRAME_PROVIDER_H
#define PANDA_TOOLING_INSPECTOR_HYBRID_STATIC_FRAME_PROVIDER_H

#include "tooling/hybrid_step/frame_info.h"
#include "types/remote_object.h"

namespace ark::tooling::inspector {

class DebugInfoCache;

/**
 * @brief Frame information provider for static (ArkTS) frames
 *
 * This class implements the IFrameInfoProvider interface for extracting
 * debugging information from static ArkTS frames.
 */
class StaticFrameProvider : public panda::tooling::hybrid_step::IFrameInfoProvider {
public:
    explicit StaticFrameProvider(DebugInfoCache &debugInfoCache);
    ~StaticFrameProvider() override = default;

    /**
     * @brief Extract frame information from a static (ArkTS) frame
     * @param framePtr Pointer to PtFrame
     * @param info Output parameter to store the extracted information
     * @return true if extraction was successful, false otherwise
     */
    bool ExtractFrameInfo(const void *framePtr, panda::tooling::hybrid_step::UnifiedFrameInfo &info) override;

private:
    /**
     * @brief Convert static debugger's RemoteObject to UnifiedRemoteObject
     */
    static panda::tooling::hybrid_step::UnifiedRemoteObject ConvertToUnifiedRemoteObject(
        const RemoteObject &staticObj);

    DebugInfoCache &debugInfoCache_;
};

} // namespace ark::tooling::inspector

#endif // PANDA_TOOLING_INSPECTOR_HYBRID_STATIC_FRAME_PROVIDER_H
