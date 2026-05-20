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

#ifndef PANDA_TOOLING_DYNAMIC_HYBRID_DYNAMIC_FRAME_PROVIDER_H
#define PANDA_TOOLING_DYNAMIC_HYBRID_DYNAMIC_FRAME_PROVIDER_H

#include "tooling/hybrid_step/frame_info.h"
#include "base/pt_types.h"

namespace panda::ecmascript::tooling {

class DebuggerImpl;

/**
 * @brief Frame information provider for dynamic (JS) frames
 *
 * This class implements the IFrameInfoProvider interface for extracting
 * debugging information from dynamic JavaScript frames using DebuggerApi.
 */
class DynamicFrameProvider : public panda::tooling::hybrid_step::IFrameInfoProvider {
public:
    explicit DynamicFrameProvider(DebuggerImpl* debugger) : debugger_(debugger) {}
    ~DynamicFrameProvider() override = default;

    /**
     * @brief Extract frame information from a dynamic (JS) frame
     * @param framePtr Pointer to FrameHandler
     * @param info Output parameter to store the extracted information
     * @return true if extraction was successful, false otherwise
     */
    bool ExtractFrameInfo(const void *framePtr, panda::tooling::hybrid_step::UnifiedFrameInfo &info) override;

private:
    bool ExtractBasicInfo(const void *framePtr, panda::tooling::hybrid_step::UnifiedFrameInfo &info,
                          CallFrame &callFrame);
    void ExtractScopeChain(const CallFrame &callFrame, panda::tooling::hybrid_step::UnifiedFrameInfo &info);
    static panda::tooling::hybrid_step::UnifiedScope::Type ConvertScopeType(const std::string &typeStr);
    static panda::tooling::hybrid_step::UnifiedRemoteObject ConvertToUnifiedRemoteObject(
        const RemoteObject &dynamicObj);

    DebuggerImpl* debugger_;
};

} // namespace panda::ecmascript::tooling

#endif // PANDA_TOOLING_DYNAMIC_HYBRID_DYNAMIC_FRAME_PROVIDER_H
