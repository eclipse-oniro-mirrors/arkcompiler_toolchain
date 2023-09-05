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

#ifndef ECMASCRIPT_TOOLING_CLIENT_MANAGER_VARIABLE_MANAGER_H
#define ECMASCRIPT_TOOLING_CLIENT_MANAGER_VARIABLE_MANAGER_H

#include <iostream>
#include <map>
#include "pt_json.h"
#include "pt_types.h"
namespace OHOS::ArkCompiler::Toolchain {
using PtJson = panda::ecmascript::tooling::PtJson;
using Result = panda::ecmascript::tooling::Result;
using panda::ecmascript::tooling::PropertyDescriptor;
class VariableManager final {
public:
    static VariableManager& getInstance();

    void HandleMessage(const std::unique_ptr<PtJson> json);

    void ShowVariableInfos();

    std::string FindObjectIdByIndex(const int32_t index);

private:
    VariableManager() = default;
    static VariableManager instance;
    std::multimap<int32_t, std::unique_ptr<PropertyDescriptor>> variableInfos_ {};
    VariableManager(const VariableManager&) = delete;
    VariableManager& operator=(const VariableManager&) = delete;
};
} // OHOS::ArkCompiler::Toolchain
#endif