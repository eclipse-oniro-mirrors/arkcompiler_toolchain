/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "dispatcherdispatch_fuzzer.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "tooling/dispatcher.h"
#include "tooling/protocol_handler.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::tooling;

namespace OHOS {
    void DispatcherDispatchFuzzTest([[maybe_unused]]const uint8_t* data, [[maybe_unused]]size_t size)
    {
        if (size <= 0) {
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        std::string message(data, data + size);
        DispatchRequest dispatchRequest(message);
        auto vm = JSNApi::CreateJSVM(option);
        std::string result = "";
        std::function<void(const void*, const std::string &)> callback =
            [&result]([[maybe_unused]] const void *ptr, const std::string &temp) { result = temp; };
        ProtocolChannel *channel =  new ProtocolHandler(callback, vm);
        auto dispatcher = std::make_unique<Dispatcher>(vm, channel);
        dispatcher->Dispatch(dispatchRequest);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::DispatcherDispatchFuzzTest(data, size);
    return 0;
}
