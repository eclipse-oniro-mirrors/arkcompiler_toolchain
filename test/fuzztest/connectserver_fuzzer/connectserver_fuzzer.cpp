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

#include "connectserver_fuzzer.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "inspector/connect_server.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace OHOS::ArkCompiler::Toolchain;

namespace OHOS {
    void TestFun([[maybe_unused]]std::string &&message)
    {
        return;
    }
    void ConnectServerFuzzTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        if (size <= 0) {
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        std::function<void(std::string&&)> fun = TestFun;
        ConnectServer connectServer("com.example.myapplication", fun);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::ConnectServerFuzzTest(data, size);
    return 0;
}