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

namespace OHOS::ArkCompiler::Toolchain {
class RuntimeClient final {
public:
    static RuntimeClient& getInstance();

    bool DispatcherCmd(int id, const std::string &cmd, std::string *reqStr);
    std::string HeapusageCommand(int id);
    std::string RuntimeEnableCommand(int id);
    std::string RuntimeDisableCommand(int id);
    std::string RunIfWaitingForDebuggerCommand(int id);
    std::string GetPropertiesCommand(int id);
    std::string GetPropertiesCommand2(int id);

    const std::map<std::string, int>& GetIdMethodMap() const
    {
        return idMethodMap_;
    }

    void SetObjectId(const std::string &objectId)
    {
        objectId_ = objectId;
    }

    int GetIdByMethod(const std::string method);

private:
    RuntimeClient() = default;
    std::map<std::string, int> idMethodMap_ {};
    std::string objectId_ {"0"};
    static RuntimeClient instance;
    RuntimeClient(const RuntimeClient&) = delete;
    RuntimeClient& operator=(const RuntimeClient&) = delete;
};
} // OHOS::ArkCompiler::Toolchain
#endif