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

#include "agent/page_impl.h"

#include "base/pt_events.h"
#include "protocol_channel.h"

namespace panda::ecmascript::tooling {
void PageImpl::DispatcherImpl::Dispatch(const DispatchRequest &request)
{
    Method method = GetMethodEnum(request.GetMethod());
    LOG_DEBUGGER(INFO) << "dispatch [" << request.GetMethod() << "] to PageImpl";
    switch (method) {
        case Method::GET_NAVIGATION_HISTORY:
            GetNavigationHistory(request);
            break;
        default:
            SendResponse(request, DispatchResponse::Fail("Unknown method: " + request.GetMethod()));
            break;
    }
}

PageImpl::DispatcherImpl::Method PageImpl::DispatcherImpl::GetMethodEnum(const std::string& method)
{
    if (method == "getNavigationHistory") {
        return Method::GET_NAVIGATION_HISTORY;
    } else {
        return Method::UNKNOWN;
    }
}

void PageImpl::DispatcherImpl::GetNavigationHistory(const DispatchRequest &request)
{
    DispatchResponse response = DispatchResponse::Ok();
    GetNavigationHistoryReturns result;
    SendResponse(request, response, result);
}
}  // namespace panda::ecmascript::tooling