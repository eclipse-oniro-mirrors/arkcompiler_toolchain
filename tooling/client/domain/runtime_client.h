/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TOOLING_CLIENT_DOMAIN_RUNTIME_CLIENT_H
#define ECMASCRIPT_TOOLING_CLIENT_DOMAIN_RUNTIME_CLIENT_H

#include <iostream>
#include <map>

#include "tooling/base/pt_types.h"

using PtJson = panda::ecmascript::tooling::PtJson;
using Result = panda::ecmascript::tooling::Result;
namespace OHOS::ArkCompiler::Toolchain {
class RuntimeClient final {
public:
    RuntimeClient(int32_t sessionId) : sessionId_(sessionId) {}
    ~RuntimeClient() = default;

    bool DispatcherCmd(const std::string &cmd);
    int HeapusageCommand();
    int RuntimeEnableCommand();
    int RuntimeDisableCommand();
    int RunIfWaitingForDebuggerCommand();
    int GetPropertiesCommand();
    int GetPropertiesCommand2();
    std::string GetMethodById(const int &id);
    std::string GetRequestObjectIdById(const int &id);
    void RecvReply(std::unique_ptr<PtJson> json);
    void HandleHeapUsage(std::unique_ptr<PtJson> json);
    void HandleGetProperties(std::unique_ptr<PtJson> json, const int &id);

    void SetObjectId(const std::string &objectId)
    {
        objectId_ = objectId;
    }

    void SetIsInitializeTree(const bool &isInitializeTree)
    {
        isInitializeTree_ = isInitializeTree;
    }

    bool GetIsInitializeTree() const
    {
        return isInitializeTree_;
    }

private:
    std::map<int, std::tuple<std::string, std::string>> idMethodMap_ {};
    std::string objectId_ {"0"};
    bool isInitializeTree_ {true};
    int32_t sessionId_;
};
} // OHOS::ArkCompiler::Toolchain
#endif