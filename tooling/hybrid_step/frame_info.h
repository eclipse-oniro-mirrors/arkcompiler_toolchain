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

#ifndef HYBRID_STEP_FRAME_INFO_H
#define HYBRID_STEP_FRAME_INFO_H

#include <string>
#include <cstdint>
#include <vector>
#include <optional>

namespace panda::tooling::hybrid_step {

struct UnifiedRemoteObject {
    enum class Type {
        UNDEFINED,
        NULL_VALUE,
        BOOLEAN,
        NUMBER,
        STRING,
        OBJECT,
        FUNCTION,
        ARRAY,
    };

    Type type {Type::UNDEFINED};
    std::string className;                          // Class name (e.g., "Object", "Array")
    std::optional<uint32_t> objectId;               // Object ID in respective debugger's storage
    std::optional<std::string> description;         // Human-readable description
    std::optional<bool> booleanValue;
    std::optional<double> numberValue;
    std::optional<std::string> stringValue;

    // Helper methods
    bool IsUndefined() const { return type == Type::UNDEFINED; }
    bool HasObjectId() const { return objectId.has_value(); }
};

struct UnifiedScope {
    enum class Type {
        GLOBAL,
        LOCAL,
        CLOSURE,
        WITH,
        CATCH,
        BLOCK,
        MODULE,
    };

    Type type {Type::LOCAL};
    UnifiedRemoteObject object;           // Scope's object containing variables
    std::optional<std::string> name;      // Optional scope name
};

struct UnifiedFrameInfo {
    // Basic Information
    std::string methodName;      // Method/function name
    std::string sourceFile;      // Source file path
    int32_t lineNumber {0};      // Line number in source
    int32_t columnNumber {0};    // Column number in source
    bool isStaticFrame {false};  // true for ArkTS static frames, false for JS dynamic frames
    bool hasInfo {false};        // Whether the info was successfully extracted

    // Complete Information
    std::vector<UnifiedScope> scopeChain;         // Scope chain (always present, may be empty)
    std::optional<UnifiedRemoteObject> thisObject; // 'this' object for this frame
    std::optional<UnifiedRemoteObject> returnValue; // Return value (if present)
};

class IFrameInfoProvider {
public:
    virtual ~IFrameInfoProvider() = default;

    virtual bool ExtractFrameInfo(const void *framePtr, UnifiedFrameInfo &info) = 0;
};

} // namespace panda::tooling::hybrid_step

#endif // HYBRID_STEP_FRAME_INFO_H
