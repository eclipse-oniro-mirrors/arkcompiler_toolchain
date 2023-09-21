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

#include "pt_types.h"

using PtJson = panda::ecmascript::tooling::PtJson;
using Result = panda::ecmascript::tooling::Result;
namespace OHOS::ArkCompiler::Toolchain {
class RuntimeClient final {
public:
    static RuntimeClient& GetInstance();

    bool DispatcherCmd(int id, const std::string &cmd, std::string *reqStr);
    std::string HeapusageCommand(int id);
    std::string RuntimeEnableCommand(int id);
    std::string RuntimeDisableCommand(int id);
    std::string RunIfWaitingForDebuggerCommand(int id);
    std::string GetPropertiesCommand(int id);
    std::string GetPropertiesCommand2(int id);
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

    bool GetIsInitializeTree() const {
        return isInitializeTree_;
    }

private:
    RuntimeClient() = default;
    static RuntimeClient instance_;
    std::map<int, std::tuple<std::string, std::string>> idMethodMap_ {};
    std::string objectId_ {"0"};
    bool isInitializeTree_ {true};
    RuntimeClient(const RuntimeClient&) = delete;
    RuntimeClient& operator=(const RuntimeClient&) = delete;
};
} // OHOS::ArkCompiler::Toolchain
#endif