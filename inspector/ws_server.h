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

#ifndef ARKCOMPILER_TOOLCHAIN_INSPECTOR_WS_SERVER_H
#define ARKCOMPILER_TOOLCHAIN_INSPECTOR_WS_SERVER_H

#include <functional>
#include <iostream>
#include <mutex>
#ifdef WINDOWS_PLATFORM
#include <pthread.h>
#endif

#include "websocket/websocket.h"

namespace OHOS::ArkCompiler::Toolchain {
class WsServer {
public:
    WsServer(const std::string& component, const std::function<void(std::string&&)>& onMessage, int32_t instanceId,
        int port) : instanceId_(instanceId), componentName_(component), wsOnMessage_(onMessage), port_(port)
    {}
    ~WsServer() = default;
    void RunServer();
    void StopServer();
    void SendReply(const std::string& message) const;

    pthread_t tid_ {0};

private:
    std::atomic<bool> terminateExecution_ { false };
    [[maybe_unused]] int32_t instanceId_ {0};
    std::mutex wsMutex_;
    std::string componentName_ {};
    std::function<void(std::string&&)> wsOnMessage_ {};
    std::unique_ptr<WebSocket> webSocket_ { nullptr };
    [[maybe_unused]] int port_ = -1;
};
} // namespace OHOS::ArkCompiler::Toolchain

#endif // ARKCOMPILER_TOOLCHAIN_INSPECTOR_WS_SERVER_H
