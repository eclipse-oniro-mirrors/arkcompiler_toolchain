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
#include <vector>
#include <pthread.h>
#include <thread>
#include <uv.h>
#include <securec.h>

#include "cli_command.h"

namespace OHOS::ArkCompiler::Toolchain {
uint32_t g_messageId = 0;
uv_async_t* g_socketSignal;
uv_async_t* g_inputSignal;
uv_async_t* g_releaseHandle;
uv_loop_t* g_loop;


bool StrToUInt(const char *content, uint32_t *result)
{
    const int dec = 10;
    char *endPtr = nullptr;
    *result = std::strtoul(content, &endPtr, dec);
    if (endPtr == content || *endPtr != '\0') {
        return false;
    }
    return true;
}

std::vector<std::string> SplitString(const std::string &str, const std::string &delimiter)
{
    std::size_t strIndex = 0;
    std::vector<std::string> value;
    std::size_t pos = str.find_first_of(delimiter, strIndex);
    while ((pos < str.size()) && (pos > strIndex)) {
        std::string subStr = str.substr(strIndex, pos - strIndex);
        value.push_back(std::move(subStr));
        strIndex = pos;
        strIndex = str.find_first_not_of(delimiter, strIndex);
        pos = str.find_first_of(delimiter, strIndex);
    }
    if (pos > strIndex) {
        std::string subStr = str.substr(strIndex, pos - strIndex);
        if (!subStr.empty()) {
            value.push_back(std::move(subStr));
        }
    }
    return value;
}

void ReleaseHandle(uv_async_t *handle)
{
    uv_close(reinterpret_cast<uv_handle_t*>(g_inputSignal), [](uv_handle_t* handle) {
        if (handle != nullptr) {
            delete reinterpret_cast<uv_async_t*>(handle);
            handle = nullptr;
        }
    });

    uv_close(reinterpret_cast<uv_handle_t*>(g_socketSignal), [](uv_handle_t* handle) {
        if (handle != nullptr) {
            delete reinterpret_cast<uv_async_t*>(handle);
            handle = nullptr;
        }
    });

    uv_close(reinterpret_cast<uv_handle_t*>(g_releaseHandle), [](uv_handle_t* handle) {
        if (handle != nullptr) {
            delete reinterpret_cast<uv_async_t*>(handle);
            handle = nullptr;
        }
    });

    if (g_loop != nullptr) {
        uv_stop(g_loop);
    }
}

void InputOnMessage(uv_async_t *handle)
{
    char* msg = static_cast<char*>(handle->data);
    std::string inputStr = std::string(msg);
    std::vector<std::string> cliCmdStr = SplitString(inputStr, " ");
    g_messageId += 1;
    CliCommand cmd(cliCmdStr, g_messageId);
    if (cmd.ExecCommand() == ErrCode::ERR_FAIL) {
        g_messageId -= 1;
    }
    std::cout << ">>> ";
    fflush(stdout);
    if (msg != nullptr) {
        free(msg);
    }
}

void GetInputCommand(void *arg)
{
    std::cout << ">>> ";
    std::string inputStr;
    while (getline(std::cin, inputStr)) {
        if (inputStr.empty()) {
            std::cout << ">>> ";
            continue;
        }
        if ((!strcmp(inputStr.c_str(), "quit")) || (!strcmp(inputStr.c_str(), "q"))) {
            LOGE("arkdb: quit");
            g_cliSocket.Close();
            if (uv_is_active(reinterpret_cast<uv_handle_t*>(g_releaseHandle))) {
                uv_async_send(g_releaseHandle);
            }
            break;
        }
        if (uv_is_active(reinterpret_cast<uv_handle_t*>(g_inputSignal))) {
            uint32_t len = inputStr.length();
            char* msg = (char*)malloc(len + 1);
            if ((msg != nullptr) && uv_is_active(reinterpret_cast<uv_handle_t*>(g_inputSignal))) {
                if (strncpy_s(msg, len + 1, inputStr.c_str(), len) != 0) {
                    g_cliSocket.Close();
                    if (uv_is_active(reinterpret_cast<uv_handle_t*>(g_releaseHandle))) {
                        uv_async_send(g_releaseHandle);
                    }
                    break;
                }
                g_inputSignal->data = std::move(msg);
                uv_async_send(g_inputSignal);
            }
        }
    }
}

void SocketOnMessage(uv_async_t *handle)
{
    char* msg = static_cast<char*>(handle->data);
    g_domainManager.DispatcherReply(msg);
    if (msg != nullptr) {
        free(msg);
    }
}

void GetSocketMessage(void *arg)
{
    while (g_cliSocket.IsConnected()) {
        std::string decMessage = g_cliSocket.Decode();
        uint32_t len = decMessage.length();
        if (len == 0) {
            continue;
        }
        char* msg = (char*)malloc(len + 1);
        if ((msg != nullptr) && uv_is_active(reinterpret_cast<uv_handle_t*>(g_socketSignal))) {
            if (strncpy_s(msg, len + 1, decMessage.c_str(), len) != 0) {
                g_cliSocket.Close();
                if (uv_is_active(reinterpret_cast<uv_handle_t*>(g_releaseHandle))) {
                    uv_async_send(g_releaseHandle);
                }
                break;
            }
            g_socketSignal->data = std::move(msg);
            uv_async_send(g_socketSignal);
        }
    }
}

int Main(const int argc, const char** argv)
{
    uint32_t port = 0;

    if (argc < 2) { // 2: two parameters
        LOGE("arkdb is missing a parameter");
        return -1;
    }
    if (strstr(argv[0], "arkdb") != nullptr) {
        if (StrToUInt(argv[1], &port)) {
            if ((port <= 0) || (port >= 65535)) { // 65535: max port
                LOGE("arkdb:InitToolchainWebSocketForPort the port = %{public}d is wrong.", port);
                return -1;
            }
            if (!g_cliSocket.InitToolchainWebSocketForPort(port, 5)) { // 5: five times
                LOGE("arkdb:InitToolchainWebSocketForPort failed");
                return -1;
            }
        } else {
            if (!g_cliSocket.InitToolchainWebSocketForSockName(argv[1])) {
                LOGE("arkdb:InitToolchainWebSocketForSockName failed");
                return -1;
            }
        }

        if (!g_cliSocket.ClientSendWSUpgradeReq()) {
            LOGE("arkdb:ClientSendWSUpgradeReq failed");
            return -1;
        }
        if (!g_cliSocket.ClientRecvWSUpgradeRsp()) {
            LOGE("arkdb:ClientRecvWSUpgradeRsp failed");
            return -1;
        }

        g_loop = uv_default_loop();

        g_inputSignal = new uv_async_t;
        uv_async_init(g_loop, g_inputSignal, reinterpret_cast<uv_async_cb>(InputOnMessage));

        g_socketSignal = new uv_async_t;
        uv_async_init(g_loop, g_socketSignal, reinterpret_cast<uv_async_cb>(SocketOnMessage));

        g_releaseHandle = new uv_async_t;
        uv_async_init(g_loop, g_releaseHandle, reinterpret_cast<uv_async_cb>(ReleaseHandle));

        uv_thread_t inputTid;
        uv_thread_create(&inputTid, GetInputCommand, nullptr);

        uv_thread_t socketTid;
        uv_thread_create(&socketTid, GetSocketMessage, nullptr);

        uv_run(g_loop, UV_RUN_DEFAULT);
    }
    return 0;
}
} // OHOS::ArkCompiler::Toolchain

int main(int argc, const char **argv)
{
    return OHOS::ArkCompiler::Toolchain::Main(argc, argv);
}
