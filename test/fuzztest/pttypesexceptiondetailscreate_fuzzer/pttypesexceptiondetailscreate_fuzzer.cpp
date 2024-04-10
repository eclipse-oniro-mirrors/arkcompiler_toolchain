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

#include "pttypesexceptiondetailscreate_fuzzer.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "tooling/base/pt_types.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::tooling;

namespace OHOS {
    void PtTypesExceptionDetailsCreateFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        auto vm = JSNApi::CreateJSVM(option);
        if (size <= 0 || data == NULL) {
            return;
        }
        // if data: "{\"\":1,"
        // cJSON_ParseWithLength will heap-buffer-overflow
        // https://github.com/DaveGamble/cJSON/issues/804
        cJSON* cjson = NULL;
        if (cjson != NULL) {
            PtJson pjson(cjson);
            auto details = ExceptionDetails::Create(pjson);
            cJSON_Delete(cjson);
        }

        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::PtTypesExceptionDetailsCreateFuzzTest(data, size);
    return 0;
}
