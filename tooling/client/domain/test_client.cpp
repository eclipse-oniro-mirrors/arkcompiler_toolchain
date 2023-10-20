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

#include "tooling/client/domain/test_client.h"

#include "common/log_wrapper.h"
#include "tooling/client/manager/variable_manager.h"
#include "tooling/base/pt_json.h"

using PtJson = panda::ecmascript::tooling::PtJson;
namespace OHOS::ArkCompiler::Toolchain {
bool TestClient::DispatcherCmd(int id, const std::string &cmd, std::string* reqStr)
{
    std::map<std::string, std::function<std::string()>> dispatcherTable {
        { "success", std::bind(&TestClient::SuccessCommand, this, id)},
        { "fail", std::bind(&TestClient::FailCommand, this, id)},
    };

    auto entry = dispatcherTable.find(cmd);
    if (entry != dispatcherTable.end()) {
        *reqStr = entry->second();
        LOGI("TestClient DispatcherCmd reqStr1: %{public}s", reqStr->c_str());
        return true;
    } else {
        *reqStr = "Unknown commond: " + cmd;
        LOGI("TestClient DispatcherCmd reqStr2: %{public}s", reqStr->c_str());
        return false;
    }
}

std::string TestClient::SuccessCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Test.success");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}

std::string TestClient::FailCommand(int id)
{
    std::unique_ptr<PtJson> request = PtJson::CreateObject();
    request->Add("id", id);
    request->Add("method", "Test.fail");

    std::unique_ptr<PtJson> params = PtJson::CreateObject();
    request->Add("params", params);
    return request->Stringify();
}
} // OHOS::ArkCompiler::Toolchain