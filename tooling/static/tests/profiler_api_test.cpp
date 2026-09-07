/**
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "inspector.h"

#include <map>
#include <memory>
#include <string>

#include "gtest/gtest.h"

#include "connection/server.h"
#include "include/runtime.h"
#include "include/runtime_options.h"
#include "libarkbase/utils/json_builder.h"
#include "libarkbase/utils/json_parser.h"
#include "tooling/inspector/debugger_arkapi.h"

namespace ark::tooling::inspector::test {

// Captures handlers registered by the Inspector for the profiler methods so the tests can invoke
// them exactly like the CDP message routing does.
class HandlerCapturingServer final : public Server {
public:
    void OnValidate([[maybe_unused]] std::function<void()> &&handler) override {};
    void OnOpen([[maybe_unused]] std::function<void()> &&handler) override {};
    void OnFail([[maybe_unused]] std::function<void()> &&handler) override {};

    void Call([[maybe_unused]] const std::string &session, [[maybe_unused]] const char *method,
              [[maybe_unused]] std::function<void(JsonObjectBuilder &)> &&params) override {};

    bool ParseMessage([[maybe_unused]] const std::string &msg) override
    {
        return true;
    }

    std::map<std::string, Server::Handler> handlers_;

private:
    void OnCallImpl(const char *method, Handler &&handler) override
    {
        handlers_[std::string(method)] = std::move(handler);
    }
};

class ProfilerApiTest : public testing::Test {
protected:
    void SetUp() override
    {
        RuntimeOptions options;
        options.SetShouldInitializeIntrinsics(false);
        options.SetShouldLoadBootPandaFiles(false);
        Runtime::Create(options);
        // The debug session is created lazily and asserts debug mode, enable it first.
        Runtime::GetCurrent()->SetDebugMode(true);
        debugSession_ = Runtime::GetCurrent()->StartDebugSession();
        // The Inspector constructor registers all method handlers on the server, including
        // Profiler.start / Profiler.stop.
        inspector_ = std::make_unique<Inspector>(server_, debugSession_->GetDebugger());
    }

    void TearDown() override
    {
        inspector_.reset();
        debugSession_.reset();
        // Close any session left over by a failed assertion through the public stop API, so the
        // next case starts from a clean global state.
        ArkDebugNativeAPI::StopProfilingSession();
        ArkDebugNativeAPI::ResetProfileInfoBuffer();
        Runtime::Destroy();
    }

    // Invokes a captured CDP handler the same way the server event loop would and returns the
    // error message of the response ("" when the response is successful).
    std::string Invoke(const std::string &method)
    {
        auto it = server_.handlers_.find(method);
        if (it == server_.handlers_.end()) {
            ADD_FAILURE() << "handler not registered: " << method;
            return "handler not registered";
        }
        JsonObject emptyParams;
        auto response = it->second("", emptyParams);
        if (response.HasValue()) {
            return "";
        }
        JsonObjectBuilder builder;
        response.Error().Serialize(builder);
        auto message = JsonObject(std::move(builder).Build()).GetValue<JsonObject::StringT>("message");
        return message != nullptr ? *message : "";
    }

    HandlerCapturingServer server_;
    Runtime::DebugSessionHandle debugSession_;
    std::unique_ptr<Inspector> inspector_;
};

TEST_F(ProfilerApiTest, StopWithoutSessionReturnsInactive)
{
    auto message = Invoke("Profiler.stop");
    EXPECT_NE(message.find("profiler inactive"), std::string::npos);
}

TEST_F(ProfilerApiTest, StartTwiceReturnsAlreadyRunning)
{
    ASSERT_TRUE(Invoke("Profiler.start").empty());

    auto second = Invoke("Profiler.start");
    EXPECT_NE(second.find("already running"), std::string::npos);
}

TEST_F(ProfilerApiTest, StartThenStopCompletesRoundtrip)
{
    ASSERT_TRUE(Invoke("Profiler.start").empty());

    auto message = Invoke("Profiler.stop");
    // No managed code ran, so aggregation is empty: the stop chain must still complete past the
    // session guards (inactive / failed to start) and terminate at the empty-profile stage.
    EXPECT_NE(message.find("info is empty"), std::string::npos);
}

}  // namespace ark::tooling::inspector::test
