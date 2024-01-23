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

#ifndef ARKCOMPILER_TOOLCHAIN_WEBSOCKET_HTTP_H
#define ARKCOMPILER_TOOLCHAIN_WEBSOCKET_HTTP_H

#include <string>

namespace OHOS::ArkCompiler::Toolchain {
struct HttpBase {
    static constexpr std::string_view EOL = "\r\n";
    static constexpr std::string_view GET = "GET";
    static constexpr std::string_view CONNECTION = "Connection: ";
    static constexpr std::string_view UPGRADE = "Upgrade: ";
    static constexpr std::string_view SEC_WEBSOCKET_ACCEPT = "Sec-WebSocket-Accept: ";
    static constexpr std::string_view SEC_WEBSOCKET_KEY = "Sec-WebSocket-Key: ";

    static std::string DecodeHeader(const std::string& headersText, std::string_view headerName);
};


struct HttpRequest : private HttpBase {
    std::string version;
    std::string connection;
    std::string upgrade;
    std::string secWebSocketKey;

    static bool Decode(const std::string& request, HttpRequest& parsed);

private:
    static std::string DecodeVersion(const std::string& request, std::string::size_type methodStartPos);
};


struct HttpResponse : private HttpBase {
    std::string version;
    std::string status;
    std::string connection;
    std::string upgrade;
    std::string secWebSocketAccept;

    static bool Decode(const std::string& response, HttpResponse& parsed);

private:
    static std::string DecodeVersion(const std::string& response, std::string::size_type versionStartPos);
    static std::string DecodeStatus(const std::string& response, std::string::size_type versionEndPos);
};
} // namespace OHOS::ArkCompiler::Toolchain

#endif // ARKCOMPILER_TOOLCHAIN_WEBSOCKET_HTTP_H
