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

#ifndef ARKCOMPILER_TOOLCHAIN_CONNECT_SERVER_CONNECT_SERVER_H
#define ARKCOMPILER_TOOLCHAIN_CONNECT_SERVER_CONNECT_SERVER_H

#include <functional>
#include <iostream>
#include "websocket/websocket.h"
#ifdef WINDOWS_PLATFORM
#include <pthread.h>
#endif

namespace OHOS::ArkCompiler::Toolchain {
class ConnectServer {
public:
    ConnectServer(const std::string& bundleName, std::function<void(std::string&&)> onMessage)
        : bundleName_(bundleName), wsOnMessage_(std::move(onMessage))
    {}
    ~ConnectServer() = default;
    void RunServer();
    void StopServer();
    void SendMessage(const std::string& message) const;

private:
    std::atomic<bool> terminateExecution_ = false;
    std::string bundleName_;
    pthread_t tid_ {0};
    std::function<void(std::string&&)> wsOnMessage_ {};
    std::unique_ptr<WebSocket> webSocket_ { nullptr };
};
} // namespace OHOS::ArkCompiler::Toolchain

#endif // ARKCOMPILER_TOOLCHAIN_CONNECT_SERVER_CONNECT_INSPECTOR_H
