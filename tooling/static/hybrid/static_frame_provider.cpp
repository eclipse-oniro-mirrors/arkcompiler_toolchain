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

#include "static_frame_provider.h"

#include "debugger/debug_info_cache.h"
#include "debugger/object_repository.h"
#include "types/scope.h"
#include "libarkbase/utils/logger.h"

namespace ark::tooling::inspector {

StaticFrameProvider::StaticFrameProvider(DebugInfoCache &debugInfoCache)
    : debugInfoCache_(debugInfoCache)
{
}

bool StaticFrameProvider::ExtractFrameInfo(const void *framePtr,
                                           panda::tooling::hybrid_step::UnifiedFrameInfo &info)
{
    if (framePtr == nullptr) {
        LOG(ERROR, DEBUGGER) << "StaticFrameProvider: invalid input (null framePtr)";
        info.hasInfo = false;
        return false;
    }

    const PtFrame *ptFrame = static_cast<const PtFrame *>(framePtr);

    std::string_view sourceFile;
    std::string_view methodName;
    int32_t lineNumber;
    debugInfoCache_.GetSourceLocation(*ptFrame, sourceFile, methodName, lineNumber);

    if (sourceFile.empty()) {
        info.hasInfo = false;
        return false;
    }

    // Fill basic information
    info.methodName = std::string(methodName);
    info.sourceFile = std::string(sourceFile);
    info.lineNumber = lineNumber;
    info.columnNumber = 0;
    info.isStaticFrame = true;
    info.hasInfo = true;

    ObjectRepository objectRepository;
    std::optional<RemoteObject> objThis;
    auto frameObject = objectRepository.CreateFrameObject(*ptFrame, debugInfoCache_.GetLocals(*ptFrame), objThis);
    auto globalObject = objectRepository.CreateGlobalObject();

    panda::tooling::hybrid_step::UnifiedScope localScope;
    localScope.type = panda::tooling::hybrid_step::UnifiedScope::Type::LOCAL;
    localScope.object = ConvertToUnifiedRemoteObject(frameObject);

    panda::tooling::hybrid_step::UnifiedScope globalScope;
    globalScope.type = panda::tooling::hybrid_step::UnifiedScope::Type::GLOBAL;
    globalScope.object = ConvertToUnifiedRemoteObject(globalObject);

    info.scopeChain.push_back(std::move(localScope));
    info.scopeChain.push_back(std::move(globalScope));

    if (objThis.has_value()) {
        info.thisObject = ConvertToUnifiedRemoteObject(objThis.value());
    }

    LOG(DEBUG, DEBUGGER) << "StaticFrameProvider: extracted info - method=" << info.methodName
                         << ", file=" << info.sourceFile
                         << ", line=" << info.lineNumber
                         << ", scopes=" << info.scopeChain.size();

    return true;
}

panda::tooling::hybrid_step::UnifiedRemoteObject StaticFrameProvider::ConvertToUnifiedRemoteObject(
    const RemoteObject &staticObj)
{
    auto objType = staticObj.GetType();
    panda::tooling::hybrid_step::UnifiedRemoteObject unifiedObj;
    if (objType == RemoteObjectType("undefined")) {
        unifiedObj.type = panda::tooling::hybrid_step::UnifiedRemoteObject::Type::UNDEFINED;
    } else if (objType == RemoteObjectType("object", "null")) {
        unifiedObj.type = panda::tooling::hybrid_step::UnifiedRemoteObject::Type::NULL_VALUE;
    } else if (objType == RemoteObjectType("boolean")) {
        unifiedObj.type = panda::tooling::hybrid_step::UnifiedRemoteObject::Type::BOOLEAN;
    } else if (objType == RemoteObjectType("number")) {
        unifiedObj.type = panda::tooling::hybrid_step::UnifiedRemoteObject::Type::NUMBER;
    } else if (objType == RemoteObjectType("string")) {
        unifiedObj.type = panda::tooling::hybrid_step::UnifiedRemoteObject::Type::STRING;
    } else if (objType == RemoteObjectType("object")) {
        unifiedObj.type = panda::tooling::hybrid_step::UnifiedRemoteObject::Type::OBJECT;
        auto object = std::get_if<RemoteObjectType::ObjectT>(&(staticObj.GetValue()));
        unifiedObj.description = RemoteObject::GetDescription(*object);
    } else if (objType == RemoteObjectType("function")) {
        unifiedObj.type = panda::tooling::hybrid_step::UnifiedRemoteObject::Type::FUNCTION;
    } else {
        unifiedObj.type = panda::tooling::hybrid_step::UnifiedRemoteObject::Type::OBJECT;
    }

    unifiedObj.className = staticObj.GetClassName();

    return unifiedObj;
}

}  // namespace ark::tooling::inspector