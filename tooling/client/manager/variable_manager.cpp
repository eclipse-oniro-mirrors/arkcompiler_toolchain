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

#include <cstring>
#include <iostream>

#include "log_wrapper.h"
#include "manager/variable_manager.h"

using PtJson = panda::ecmascript::tooling::PtJson;
namespace OHOS::ArkCompiler::Toolchain {
void VariableManager::HandleMessage(const std::unique_ptr<PtJson> json)
{
    if (json == nullptr) {
        LOGE("toolchain_client: json parse error");
        return;
    }

    if (!json->IsObject()) {
        LOGE("toolchain_client: json parse format error");
        json->ReleaseRoot();
        return;
    }

    std::unique_ptr<PtJson> result;
    Result ret = json->GetObject("result", &result);
    if (ret != Result::SUCCESS) {
        LOGE("toolchain_client: find result error");
        return;
    }

    std::unique_ptr<PtJson> innerResult;
    ret = result->GetArray("result", &innerResult);
    if (ret != Result::SUCCESS) {
        LOGE("toolchain_client: find innerResult error");
        return;
    }

    for (int32_t i = 0; i < innerResult->GetSize(); i++) {
        std::unique_ptr<PropertyDescriptor> variableInfo = PropertyDescriptor::Create(*(innerResult->Get(i)));
        variableInfos_.emplace(i + 5, std::move(variableInfo)); // 5: index start at 5
    }
}

void VariableManager::ShowVariableInfos()
{
    std::cout << std::endl;
    std::cout << "1. LOCAL" << std::endl;
    for (const auto& info : variableInfos_) {
        if (info.second->GetValue()->HasDescription()) {
            std::cout << "   " << info.first << ". " << info.second->GetName() << " = " <<
                         info.second->GetValue()->GetDescription() << std::endl;
        } else {
            std::cout << "   " << info.first << ". " << info.second->GetName() << " = " <<
                         info.second->GetValue()->GetType() << std::endl;
        }
    }
    std::cout << "2. CLOSURE" << std::endl;
    std::cout << "3. MODULE" << std::endl;
    std::cout << "3. GLOBAL" << std::endl;
}

std::string VariableManager::FindObjectIdByIndex(int32_t index)
{
    auto it = variableInfos_.find(index);
    if (it != variableInfos_.end()) {
        return std::to_string(it->second->GetValue()->GetObjectId());
    }
    LOGE("index not found");
    return "";
}
}