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

#include "connect_inspector.h"
#include <mutex>
#include "log_wrapper.h"

namespace OHOS::ArkCompiler::Toolchain {
std::mutex g_connectMutex;
std::unique_ptr<ConnectInspector> g_inspector = nullptr;
static constexpr char CONNECTED_MESSAGE[] = "connected";
static constexpr char OPEN_MESSAGE[] = "layoutOpen";
static constexpr char CLOSE_MESSAGE[] = "layoutClose";
static constexpr char REQUEST_MESSAGE[] = "tree";

void* HandleDebugManager(void* const server)
{
    if (server == nullptr) {
        LOGE("HandleDebugManager server nullptr");
        return nullptr;
    }
#if defined(IOS_PLATFORM) || defined(MAC_PLATFORM)
    pthread_setname_np("DebugConnectThread");
#else
    pthread_setname_np(pthread_self(), "DebugConnectThread");
#endif

    static_cast<ConnectServer*>(server)->RunServer();
    return nullptr;
}

void OnMessage(const std::string& message)
{
    std::lock_guard<std::mutex> lock(g_connectMutex);
    if (message.empty()) {
        LOGE("message is empty");
        return;
    }

    LOGI("ConnectServer OnMessage: %{public}s", message.c_str());
    if (g_inspector != nullptr && g_inspector->connectServer_ != nullptr) {
        g_inspector->ideMsgQueue_.push(message);
        if (message.find(CONNECTED_MESSAGE, 0) != std::string::npos) {
            g_inspector->waitingForDebugger_ = false;
            for (auto& info : g_inspector->infoBuffer_) {
                g_inspector->connectServer_->SendMessage(info.second);
            }
        }
        if (message.find(OPEN_MESSAGE, 0) != std::string::npos) {
            if (g_inspector->setSwitchStatus_ != nullptr) {
                LOGI("layoutOpen start");
                g_inspector->setSwitchStatus_(true);
            }
        }
        if (message.find(CLOSE_MESSAGE, 0) != std::string::npos) {
            if (g_inspector->setSwitchStatus_ != nullptr) {
                LOGI("layoutClose start");
                g_inspector->setSwitchStatus_(false);
            }
        }
        if (message.find(REQUEST_MESSAGE, 0) != std::string::npos) {
            if (g_inspector->createLayoutInfo_ != nullptr) {
                LOGI("tree start");
                g_inspector->createLayoutInfo_(g_inspector->instanceId_);
            }
        }
    }
}

void SetSwitchCallBack(const std::function<void(bool)>& setSwitchStatus,
    const std::function<void(int32_t)>& createLayoutInfo, int32_t instanceId)
{
    std::lock_guard<std::mutex> lock(g_connectMutex);
    if (g_inspector != nullptr) {
        g_inspector->setSwitchStatus_ = setSwitchStatus;
        g_inspector->createLayoutInfo_ = createLayoutInfo;
        g_inspector->instanceId_ = instanceId;
    }
}

void ResetService()
{
    if (g_inspector != nullptr && g_inspector->connectServer_ != nullptr) {
        g_inspector->connectServer_->StopServer();
        g_inspector->connectServer_.reset();
    }
}

void StartServer(const std::string& componentName)
{
    g_inspector = std::make_unique<ConnectInspector>();
    g_inspector->connectServer_ = std::make_unique<ConnectServer>(componentName,
        std::bind(&OnMessage, std::placeholders::_1));

    pthread_t tid;
    if (pthread_create(&tid, nullptr, &HandleDebugManager,
        static_cast<void*>(g_inspector->connectServer_.get())) != 0) {
        LOGE("pthread_create fail!");
        ResetService();
        return;
    }
}

void StopServer([[maybe_unused]] const std::string& componentName)
{
    ResetService();
}

void StoreMessage(int32_t instanceId, const std::string& message)
{
    std::lock_guard<std::mutex> lock(g_connectMutex);
    if (g_inspector->infoBuffer_.count(instanceId) == 1) {
        LOGE("The message with the current instance id has existed.");
        return;
    }
    g_inspector->infoBuffer_[instanceId] = message;
}

void StoreInspectorInfo(const std::string& jsonTreeStr, const std::string& jsonSnapshotStr)
{
    std::lock_guard<std::mutex> lock(g_connectMutex);
    g_inspector->layoutInspectorInfo_.tree = jsonTreeStr;
    g_inspector->layoutInspectorInfo_.snapShot = jsonSnapshotStr;
}

void RemoveMessage(int32_t instanceId)
{
    std::lock_guard<std::mutex> lock(g_connectMutex);
    if (g_inspector->infoBuffer_.count(instanceId) != 1) {
        LOGE("The message with the current instance id does not exist.");
        return;
    }
    g_inspector->infoBuffer_.erase(instanceId);
}

void SendLayoutMessage(const std::string& message)
{
    LOGI("SendLayoutMessage start to send message");
    if (g_inspector != nullptr && g_inspector->connectServer_ != nullptr) {
        g_inspector->connectServer_->SendMessage(message);
    }
}

void SendMessage(const std::string& message)
{
    if (g_inspector != nullptr && g_inspector->connectServer_ != nullptr && !g_inspector->waitingForDebugger_) {
        g_inspector->connectServer_->SendMessage(message);
    }
}

bool WaitForConnection()
{
    if (g_inspector == nullptr) {
        return true;
    }
    return g_inspector->waitingForDebugger_;
}
} // OHOS::ArkCompiler::Toolchain
