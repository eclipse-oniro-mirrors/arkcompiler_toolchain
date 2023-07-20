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

#include "ws_server.h"

#include <fstream>
#include <iostream>
#include <shared_mutex>
#include <sys/types.h>
#include <unistd.h>

#include "log_wrapper.h"

namespace OHOS::ArkCompiler::Toolchain {
std::shared_mutex g_mutex;

void WsServer::RunServer()
{
    {
        std::lock_guard<std::mutex> lock(wsMutex_);
        if (terminateExecution_) {
            LOGE("WsServer has been terminated unexpectedly");
            return;
        }
        webSocket_ = std::make_unique<WebSocket>();
#if !defined(OHOS_PLATFORM)
        LOGI("WsSever Runsever: Init tcp websocket %{public}d", port_);
        if (!webSocket_->InitTcpWebSocket(port_)) {
            return;
        }
#else
        int appPid = getpid();
        std::string pidStr = std::to_string(appPid);
        std::string instanceIdStr("");

        if (instanceId_ != 0) {
            instanceIdStr = std::to_string(instanceId_);
        }
        std::string sockName = pidStr + instanceIdStr + componentName_;
        LOGI("WsServer RunServer: %{public}d%{public}s%{public}s", appPid, instanceIdStr.c_str(),
            componentName_.c_str());
        if (!webSocket_->InitUnixWebSocket(sockName)) {
            return;
        }
#endif
    }
    while (!terminateExecution_) {
#if !defined(OHOS_PLATFORM)
        if (!webSocket_->ConnectTcpWebSocket()) {
            return;
        }
#else
        if (!webSocket_->ConnectUnixWebSocket()) {
            return;
        }
#endif
        while (webSocket_->IsConnected()) {
            std::string message = webSocket_->Decode();
            if (!message.empty()) {
                LOGI("WsServer OnMessage: %{public}s", message.c_str());
                wsOnMessage_(std::move(message));
            }
        }
    }
}

void WsServer::StopServer()
{
    LOGI("WsServer StopServer");
    {
        std::lock_guard<std::mutex> lock(wsMutex_);
        terminateExecution_ = true;
        if (webSocket_ != nullptr) {
            webSocket_->Close();
        }
    }
#if defined(OHOS_PLATFORM)
    pthread_join(tid_, nullptr);
#endif
    if (webSocket_ != nullptr) {
        webSocket_.reset();
    }
}

void WsServer::SendReply(const std::string& message) const
{
    std::unique_lock<std::shared_mutex> lock(g_mutex);
    if (webSocket_ == nullptr) {
        LOGE("WsServer SendReply websocket has been closed unexpectedly");
        return;
    }
    LOGI("WsServer SendReply: %{public}s", message.c_str());
    webSocket_->SendReply(message);
}
} // namespace OHOS::ArkCompiler::Toolchain
