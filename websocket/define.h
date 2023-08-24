/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#ifndef ARKCOMPILER_TOOLCHAIN_WEBSOCKET_DEFINE_H
#define ARKCOMPILER_TOOLCHAIN_WEBSOCKET_DEFINE_H

#include <fstream>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <securec.h>
#include <sstream>
#include <string>
#if defined(WINDOWS_PLATFORM)
#include <winsock2.h>
#include <windows.h>
#ifdef ERROR
#undef ERROR
#endif
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#endif
#include <unistd.h>
#include <vector>

namespace OHOS::ArkCompiler::Toolchain {
std::vector<std::string> ProtocolSplit(const std::string& str, const std::string& input)
{
    std::vector<std::string> result;
    size_t prev = 0;
    size_t len = input.length();
    size_t cur = str.find(input);
    while (cur != std::string::npos) {
        std::string tmp = str.substr(prev, cur - prev);
        result.push_back(tmp);
        prev = cur + len;
        cur = str.find(input, prev);
    }
    if (prev < str.size()) {
        std::string tmp = str.substr(prev);
        result.push_back(tmp);
    }
    return result;
}
} // namespace OHOS::ArkCompiler::Toolchain

#endif // ARKCOMPILER_TOOLCHAIN_WEBSOCKET_DEFINE_H
