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
#ifndef PANDA_TOOLING_INSPECTOR_TYPES_ASYNC_STACK_TRACE_H
#define PANDA_TOOLING_INSPECTOR_TYPES_ASYNC_STACK_TRACE_H

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "json_serialization/serializable.h"
#include "types/numeric_id.h"

namespace ark {
class JsonObjectBuilder;
}  // namespace ark

namespace ark::tooling::inspector {

struct AsyncCallFrame {
    std::string functionName;
    std::string url;
    ScriptId scriptId = 0U;
    int32_t lineNumber = 1;
};

struct AsyncFrameSourceLocation {
    std::string sourceFile;
    int32_t lineNumber = 0;
};

class AsyncStackTrace final : public JsonSerializable {
public:
    AsyncStackTrace(std::string description, std::vector<AsyncCallFrame> callFrames)
        : description_(std::move(description)), callFrames_(std::move(callFrames))
    {
    }

    void SetParent(std::unique_ptr<AsyncStackTrace> parent)
    {
        parent_ = std::move(parent);
    }

    void Serialize(JsonObjectBuilder &builder) const override;

private:
    std::string description_;
    std::vector<AsyncCallFrame> callFrames_;
    std::unique_ptr<AsyncStackTrace> parent_;
};

}  // namespace ark::tooling::inspector

#endif  // PANDA_TOOLING_INSPECTOR_TYPES_ASYNC_STACK_TRACE_H
