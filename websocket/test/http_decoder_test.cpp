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

#include <string_view>

#include "gtest/gtest.h"
#include "http.h"

using namespace OHOS::ArkCompiler::Toolchain;

namespace panda::test {
class HttpDecoderTest : public testing::Test {
public:
    static constexpr std::string_view REQUEST_HEADERS = "GET / HTTP/1.1\r\n"
        "Host: 127.0.0.1:19015\r\n"
        "Connection: Upgrade\r\n"
        "Pragma: no-cache\r\n"
        "Cache-Control: no-cache\r\n"
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64) Chrome/117.0.0.0 Safari/537.36\r\n"
        "Upgrade: websocket\r\n"
        "Origin: dvtls://dvtls\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Accept-Encoding: gzip, deflate, br\r\n"
        "Accept-Language: en-US,en;q=0.9\r\n"
        "Sec-WebSocket-Key: AyuTxzyBTJJdViDskomT0Q==\r\n"
        "Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits\r\n\r\n";
    std::string requestHeaders = std::string(REQUEST_HEADERS);

    static constexpr std::string_view ERR_REQUEST_HEADERS = "GEY\r\n";
    std::string errRequestHeaders = std::string(ERR_REQUEST_HEADERS);

    static constexpr std::string_view NO_INFO_REQUEST_HEADERS = "GET\r\n";
    std::string noInfoRequestHeaders = std::string(NO_INFO_REQUEST_HEADERS);

    static constexpr std::string_view RESPONSE_HEADERS = "HTTP/1.1 101 Switching Protocols\r\n"
        "Connection: Upgrade\r\n"
        "Upgrade: websocket\r\n"
        "Sec-WebSocket-Accept: AyuTxzyBTJJdViDskomT0Q==\r\n\r\n";
    std::string responseHeaders = std::string(RESPONSE_HEADERS);

    static constexpr std::string_view ERR_RESPONSE_HEADERS = "HTTB\r\n";
    std::string errResponseHeaders = std::string(ERR_RESPONSE_HEADERS);

    static constexpr std::string_view NO_INFO_RESPONSE_HEADERS = "HTTP\r\n";
    std::string noInfoResponseHeaders = std::string(NO_INFO_RESPONSE_HEADERS);

    static constexpr std::string_view EXPECTED_VERSION              = "HTTP/1.1";
    static constexpr std::string_view EXPECTED_STATUS               = "101";
    static constexpr std::string_view EXPECTED_CONNECTION           = "Upgrade";
    static constexpr std::string_view EXPECTED_UPGRADE              = "websocket";
    static constexpr std::string_view EXPECTED_SEC_WEBSOCKET_KEY    = "AyuTxzyBTJJdViDskomT0Q==";
};

HWTEST_F(HttpDecoderTest, TestRequestDecode, testing::ext::TestSize.Level0)
{
    HttpRequest parsed;
    auto succeeded = HttpRequest::Decode(requestHeaders, parsed);

    ASSERT_TRUE(succeeded);
    ASSERT_EQ(parsed.version, EXPECTED_VERSION);
    ASSERT_EQ(parsed.connection, EXPECTED_CONNECTION);
    ASSERT_EQ(parsed.upgrade, EXPECTED_UPGRADE);
    ASSERT_EQ(parsed.secWebSocketKey, EXPECTED_SEC_WEBSOCKET_KEY);
}

HWTEST_F(HttpDecoderTest, TestAbnormalRequestDecode, testing::ext::TestSize.Level0)
{
    HttpRequest parsed;
    ASSERT_FALSE(HttpRequest::Decode(errRequestHeaders, parsed));
    ASSERT_TRUE(HttpRequest::Decode(noInfoRequestHeaders, parsed));

    ASSERT_EQ(parsed.version, "");
    ASSERT_EQ(parsed.connection, "");
    ASSERT_EQ(parsed.upgrade, "");
    ASSERT_EQ(parsed.secWebSocketKey, "");
}

HWTEST_F(HttpDecoderTest, TestResponseDecode, testing::ext::TestSize.Level0)
{
    HttpResponse parsed;
    auto succeeded = HttpResponse::Decode(responseHeaders, parsed);

    ASSERT_TRUE(succeeded);
    ASSERT_EQ(parsed.version, EXPECTED_VERSION);
    ASSERT_EQ(parsed.status, EXPECTED_STATUS);
    ASSERT_EQ(parsed.connection, EXPECTED_CONNECTION);
    ASSERT_EQ(parsed.upgrade, EXPECTED_UPGRADE);
    ASSERT_EQ(parsed.secWebSocketAccept, EXPECTED_SEC_WEBSOCKET_KEY);
}

HWTEST_F(HttpDecoderTest, TestAbnormalResponseDecode, testing::ext::TestSize.Level0)
{
    HttpResponse parsed;
    ASSERT_FALSE(HttpResponse::Decode(errResponseHeaders, parsed));
    ASSERT_TRUE(HttpResponse::Decode(noInfoResponseHeaders, parsed));

    ASSERT_EQ(parsed.version, "");
    ASSERT_EQ(parsed.status, "");
    ASSERT_EQ(parsed.connection, "");
    ASSERT_EQ(parsed.upgrade, "");
    ASSERT_EQ(parsed.secWebSocketAccept, "");
}

}  // namespace panda::test
