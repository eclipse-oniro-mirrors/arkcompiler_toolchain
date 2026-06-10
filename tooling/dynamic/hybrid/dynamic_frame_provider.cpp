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

#include "dynamic_frame_provider.h"

#include "agent/debugger_impl.h"
#include "base/pt_types.h"
#include "ecmascript/debugger/debugger_api.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/method.h"
#include "common/log_wrapper.h"

#include <unordered_map>

namespace panda::ecmascript::tooling {

using panda::tooling::hybrid_step::UnifiedFrameInfo;
using panda::tooling::hybrid_step::UnifiedScope;

UnifiedScope::Type DynamicFrameProvider::ConvertScopeType(const std::string &typeStr)
{
    static const std::unordered_map<std::string, UnifiedScope::Type> typeMap = {
        {"global",  UnifiedScope::Type::GLOBAL},
        {"local",   UnifiedScope::Type::LOCAL},
        {"closure", UnifiedScope::Type::CLOSURE},
        {"with",    UnifiedScope::Type::WITH},
        {"catch",   UnifiedScope::Type::CATCH},
        {"block",   UnifiedScope::Type::BLOCK},
        {"module",  UnifiedScope::Type::MODULE},
    };
    auto it = typeMap.find(typeStr);
    return (it != typeMap.end()) ? it->second : UnifiedScope::Type::LOCAL;
}

bool DynamicFrameProvider::ExtractBasicInfo(const void *framePtr, UnifiedFrameInfo &info, CallFrame &callFrame)
{
    if (framePtr == nullptr) {
        LOG_DEBUGGER(ERROR) << "DynamicFrameProvider: invalid input (null framePtr)";
        return false;
    }
    const FrameHandler *frameHandler = static_cast<const FrameHandler *>(framePtr);
    if (!frameHandler->HasFrame()) {
        LOG_DEBUGGER(ERROR) << "DynamicFrameProvider: frame has no frame";
        return false;
    }
    CallFrameId callFrameId = 0;
    if (!debugger_->GenerateCallFrame(&callFrame, frameHandler, callFrameId, true)) {
        LOG_DEBUGGER(ERROR) << "DynamicFrameProvider: failed to generate call frame";
        return false;
    }
    info.methodName = callFrame.GetFunctionName();
    info.sourceFile = callFrame.GetUrl();
    if (callFrame.GetLocation() != nullptr) {
        info.lineNumber = callFrame.GetLocation()->GetLine();
        info.columnNumber = callFrame.GetLocation()->GetColumn();
    }
    return true;
}

void DynamicFrameProvider::ExtractScopeChain(const CallFrame &callFrame, UnifiedFrameInfo &info)
{
    const auto *dynamicScopes = callFrame.GetScopeChain();
    if (dynamicScopes == nullptr) {
        return;
    }
    info.scopeChain.reserve(dynamicScopes->size());
    for (const auto &dynamicScope : *dynamicScopes) {
        UnifiedScope unifiedScope;
        unifiedScope.type = ConvertScopeType(dynamicScope->GetType());
        if (dynamicScope->HasName()) {
            unifiedScope.name = dynamicScope->GetName();
        }
        const RemoteObject *dynamicObj = dynamicScope->GetObject();
        if (dynamicObj != nullptr) {
            unifiedScope.object = ConvertToUnifiedRemoteObject(*dynamicObj);
        }
        info.scopeChain.push_back(std::move(unifiedScope));
    }
}

bool DynamicFrameProvider::ExtractFrameInfo(const void *framePtr, UnifiedFrameInfo &info)
{
    CallFrame callFrame;
    if (!ExtractBasicInfo(framePtr, info, callFrame)) {
        info.hasInfo = false;
        return false;
    }

    ExtractScopeChain(callFrame, info);

    const RemoteObject *dynamicThis = callFrame.GetThis();
    if (dynamicThis != nullptr) {
        info.thisObject = ConvertToUnifiedRemoteObject(*dynamicThis);
    }
    if (callFrame.HasReturnValue()) {
        const RemoteObject *dynamicRet = callFrame.GetReturnValue();
        if (dynamicRet != nullptr) {
            info.returnValue = ConvertToUnifiedRemoteObject(*dynamicRet);
        }
    }

    info.isStaticFrame = false;
    info.hasInfo = true;

    LOG_DEBUGGER(DEBUG) << "DynamicFrameProvider: extracted info - method=" << info.methodName
                        << ", file=" << info.sourceFile
                        << ", line=" << info.lineNumber
                        << ", column=" << info.columnNumber
                        << ", scopes=" << info.scopeChain.size();

    return true;
}

panda::tooling::hybrid_step::UnifiedRemoteObject DynamicFrameProvider::ConvertToUnifiedRemoteObject(
    const RemoteObject &dynamicObj)
{
    panda::tooling::hybrid_step::UnifiedRemoteObject unifiedObj;

    std::string typeStr = dynamicObj.GetType();
    if (typeStr == RemoteObject::TypeName::Undefined) {
        unifiedObj.type = panda::tooling::hybrid_step::UnifiedRemoteObject::Type::UNDEFINED;
    } else if (typeStr == "null") {
        unifiedObj.type = panda::tooling::hybrid_step::UnifiedRemoteObject::Type::NULL_VALUE;
    } else if (typeStr == RemoteObject::TypeName::Boolean) {
        unifiedObj.type = panda::tooling::hybrid_step::UnifiedRemoteObject::Type::BOOLEAN;
    } else if (typeStr == RemoteObject::TypeName::Number) {
        unifiedObj.type = panda::tooling::hybrid_step::UnifiedRemoteObject::Type::NUMBER;
    } else if (typeStr == RemoteObject::TypeName::String) {
        unifiedObj.type = panda::tooling::hybrid_step::UnifiedRemoteObject::Type::STRING;
    } else if (typeStr == RemoteObject::TypeName::Object) {
        unifiedObj.type = panda::tooling::hybrid_step::UnifiedRemoteObject::Type::OBJECT;
    } else if (typeStr == RemoteObject::TypeName::Function) {
        unifiedObj.type = panda::tooling::hybrid_step::UnifiedRemoteObject::Type::FUNCTION;
    } else {
        unifiedObj.type = panda::tooling::hybrid_step::UnifiedRemoteObject::Type::OBJECT;
    }

    if (dynamicObj.HasClassName()) {
        unifiedObj.className = dynamicObj.GetClassName();
    }
    if (dynamicObj.HasObjectId()) {
        unifiedObj.objectId = dynamicObj.GetObjectId();
    }
    if (dynamicObj.HasDescription()) {
        unifiedObj.description = dynamicObj.GetDescription();
    }

    return unifiedObj;
}

} // namespace panda::ecmascript::tooling
