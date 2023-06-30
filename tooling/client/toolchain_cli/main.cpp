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

#include "cli_command.h"

namespace OHOS::ArkCompiler::Toolchain{
bool StrToUInt(const char *content, uint32_t *result)
{
    const int DEC = 10;
    char *endPtr = nullptr;
    *result = std::strtoul(content, &endPtr, DEC);
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

int Main(const int argc, const char** argv)
{
    uint32_t port = 0;
    OHOS::ArkCompiler::Toolchain::ToolchainWebsocket cliSocket;
    if(argc < 2) {
        LOGE("toolchain_cli is missing a parameter");
        return -1;
    }
    if(strstr(argv[0], "toolchain_cli") != nullptr) {
        if(StrToUInt(argv[1], &port)) {
            if((port <= 0) || (port >= 65535)) {
                LOGE("toolchain_cli:InitToolchainWebSocketForPort the port = %{public}d is wrong.", port);
                return -1;
            }
            if(!cliSocket.InitToolchainWebSocketForPort(port, 5)) {
                LOGE("toolchain_cli:InitToolchainWebSocketForPort failed");
                return -1;
            }
        } else {
            if(!cliSocket.InitToolchainWebSocketForSockName(argv[1])) {
                LOGE("toolchain_cli:InitToolchainWebSocketForSockName failed");
                return -1;
            }
        }

        if (!cliSocket.ClientSendWSUpgradeReq()) {
            LOGE("toolchain_cli:ClientSendWSUpgradeReq failed");
            return -1;
        }
        if (!cliSocket.ClientRecvWSUpgradeRsp()) {
            LOGE("toolchain_cli:ClientRecvWSUpgradeRsp failed");
            return -1;
        }

        std::cout << ">>> ";
        std::string inputStr;
        while(getline(std::cin, inputStr)) {
            if((!strcmp(inputStr.c_str(), "quit"))||(!strcmp(inputStr.c_str(), "q"))) {
                LOGE("toolchain_cli: quit");
                cliSocket.Close();
                break;
            }
            std::vector<std::string> cliCmdStr = SplitString(inputStr, " ");
            OHOS::ArkCompiler::Toolchain::CliCommand cmd(&cliSocket, cliCmdStr);
            std::cout << cmd.ExecCommand();
            std::cout << ">>> ";
        }
    }
    return 0;
}
} //OHOS::ArkCompiler::Toolchain

int main(int argc, const char **argv)
{
    return OHOS::ArkCompiler::Toolchain::Main(argc, argv);
}
