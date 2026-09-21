/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "types/async_stack_trace.h"

#include "libarkbase/utils/json_builder.h"

namespace ark::tooling::inspector {
void AsyncStackTrace::Serialize(JsonObjectBuilder &builder) const
{
    builder.AddProperty("description", description_);
    builder.AddProperty("callFrames", [this](JsonArrayBuilder &callFrames) {
        for (const auto &callFrame : callFrames_) {
            callFrames.Add([&callFrame](JsonObjectBuilder &frame) {
                frame.AddProperty("functionName", callFrame.functionName);
                frame.AddProperty("scriptId", std::to_string(callFrame.scriptId));
                frame.AddProperty("url", callFrame.url);
                frame.AddProperty("lineNumber", callFrame.lineNumber - 1);
                frame.AddProperty("columnNumber", 0);
            });
        }
    });

    if (parent_ != nullptr) {
        builder.AddProperty("parent", *parent_);
    }
}
}  // namespace ark::tooling::inspector
