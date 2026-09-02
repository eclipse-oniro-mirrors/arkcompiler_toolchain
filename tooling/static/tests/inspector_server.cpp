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

#include "inspector_server.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <deque>
#include <filesystem>
#include <optional>
#include <set>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "assembly-emitter.h"
#include "assembly-parser.h"
#include "debugger/debug_info_cache.h"
#include "include/runtime.h"
#include "plugins/ets/runtime/ani/ani.h"
#include "plugins/ets/runtime/ets_coroutine.h"
#include "plugins/ets/runtime/ets_vm.h"
#include "runtime/tooling/tools.h"
#include "runtime/execution/job.h"
#include "types/location.h"
#include "libarkbase/utils/json_builder.h"

#include "connection/server.h"
#include "connection/server_endpoint_base.h"
#include "libarkbase/os/mutex.h"

#include "common.h"
#include "inspector.h"
#include "json_object_matcher.h"

// NOLINTBEGIN

using namespace std::placeholders;

namespace ark::ets::interop::js {
// arkinspector_tests intentionally cover the pure Static ArkTS path. A full hybrid run links the interop
// implementation from ets_runtime; here the stub only reports that no dynamic frames are present.
bool ForEachFrameInUnionStack(const std::function<void(const void *, bool)> &)
{
    return false;
}

bool UnionStackIsEmpty(bool *isEmpty)
{
    if (isEmpty != nullptr) {
        *isEmpty = true;
    }
    return true;
}

const void *GetEcmaVM()
{
    return nullptr;
}
}  // namespace ark::ets::interop::js

namespace ark::tooling::inspector::test {

class TestServer : public Server {
public:
    void OnValidate([[maybe_unused]] std::function<void()> &&handler) override {};
    void OnOpen([[maybe_unused]] std::function<void()> &&handler) override {};
    void OnFail([[maybe_unused]] std::function<void()> &&handler) override {};

    void Call(const std::string &session, const char *method_call,
              std::function<void(JsonObjectBuilder &)> &&parameters) override
    {
        std::string tmp(method_call);
        CallMock(session, tmp, std::move(parameters));
    }

    MOCK_METHOD(void, CallMock,
                (const std::string &session, const std::string &method_call,
                 std::function<void(JsonObjectBuilder &)> &&parameters));

    MOCK_METHOD(void, OnCallMock, (const std::string &method_call, Handler &&handler));

    bool RunOne()
    {
        return true;
    };

    bool ParseMessage(const std::string &msg) override
    {
        return true;
    }

private:
    void OnCallImpl(const char *method_call, Handler &&handler) override
    {
        std::string tmp(method_call);
        OnCallMock(tmp, std::move(handler));
    }
};

static PtThread g_mthread = PtThread(PtThread::NONE);
static const std::string g_sessionId;
static const std::string g_sourceFile = "source";
static bool g_handlerCalled = false;
static constexpr int EXPECTED_RESUMED_EVENT_COUNT = 2;
static constexpr int EXPECTED_PAUSE_EVENT_COUNT = 2;
static constexpr int EXPECTED_ASYNC_SCRIPT_PARSED_COUNT = 2;
static constexpr uint32_t ASYNC_CALL_STACK_DEPTH = 8U;

using SourceFileComparator = std::function<bool(std::string_view, std::string_view)>;
using ComparatorRef = const SourceFileComparator &;
using StringPtr = const std::string *;
using BreakpointResult = std::optional<BreakpointId>;

enum class SetBreakpointResult { FOUND, NOT_FOUND };

struct SetBreakpointHandler {
    explicit SetBreakpointHandler(SetBreakpointResult result) : result_(result) {}

    BreakpointResult operator()(PtThread, ComparatorRef, int32_t line, SourceFileSet &sources, StringPtr) const
    {
        sources.insert({"source", {}});
        return result_ == SetBreakpointResult::FOUND ? BreakpointResult(line) : BreakpointResult();
    }

private:
    SetBreakpointResult result_;
};

static SetBreakpointHandler handlerForSetBreak(SetBreakpointResult::FOUND);
static SetBreakpointHandler handlerForSetBreakEmpty(SetBreakpointResult::NOT_FOUND);

std::set<int32_t> GetLinesTrue(std::string_view source, std::string_view scriptIdentity, int32_t startLine,
                               int32_t endLine, bool restrictToFunction);
std::set<int32_t> GetLinesFalse(std::string_view source, std::string_view scriptIdentity, int32_t startLine,
                                int32_t endLine, bool restrictToFunction);

#if defined(PANDA_TARGET_LINUX)

static std::unique_ptr<AsyncStackSnapshotView> g_realAsyncStackSnapshot;
static Server::Handler g_realPauseHandler;
static Inspector *g_realInspector {nullptr};
static os::memory::Mutex g_realRegisteredThreadsMutex;
static std::set<PtThread> g_realRegisteredThreads GUARDED_BY(g_realRegisteredThreadsMutex);
static constexpr const char *asyncE2EAbcPath =
    "gen/arkcompiler/toolchain/tooling/static/tests/async_stack_promise_test_abc/AsyncStackPromiseTest.abc";
static constexpr const char *asyncE2EStdlibPath = "gen/arkcompiler/runtime_core/static_core/plugins/ets/etsstdlib.abc";

#endif

class ServerTest : public testing::Test {
public:
    void SetUp() override
    {
        RuntimeOptions options;
        options.SetShouldInitializeIntrinsics(false);
        options.SetShouldLoadBootPandaFiles(false);
        Runtime::Create(options);
        g_mthread = PtThread(ManagedThread::GetCurrent());
        g_handlerCalled = false;
    }
    void TearDown() override
    {
        Runtime::Destroy();
    }
    TestServer server;
    InspectorServer inspectorServer {server};
};

TEST_F(ServerTest, CallDebuggerResumed)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);
    EXPECT_CALL(server, CallMock(g_sessionId, "Debugger.resumed", testing::_)).Times(1);

    inspectorServer.CallDebuggerResumed(g_mthread);
}

TEST_F(ServerTest, CallDebuggerScriptParsed)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    size_t scriptId = 4;
    EXPECT_CALL(server, CallMock(g_sessionId, "Debugger.scriptParsed", testing::_))
        .WillOnce([&](testing::Unused, testing::Unused, auto s) {
            ASSERT_THAT(ToObject(std::move(s)),
                        JsonProperties(JsonProperty<JsonObject::NumT> {"executionContextId", 0},
                                       JsonProperty<JsonObject::StringT> {"scriptId", std::to_string(scriptId)},
                                       JsonProperty<JsonObject::NumT> {"startLine", 0},
                                       JsonProperty<JsonObject::NumT> {"startColumn", 0},
                                       JsonProperty<JsonObject::NumT> {"endLine", std::numeric_limits<int>::max()},
                                       JsonProperty<JsonObject::NumT> {"endColumn", std::numeric_limits<int>::max()},
                                       JsonProperty<JsonObject::StringT> {"hash", ""},
                                       JsonProperty<JsonObject::StringT> {"url", g_sourceFile.c_str()}));
        });
    inspectorServer.CallDebuggerScriptParsed(ScriptId(scriptId), g_sourceFile);
}

using ResultHolder = std::optional<JsonObject>;

static void GetResult(Expected<std::unique_ptr<JsonSerializable>, JRPCError> &&returned, ResultHolder &result)
{
    ASSERT_TRUE(returned.HasValue());
    if (returned.Value() != nullptr) {
        JsonObjectBuilder builder;
        returned.Value()->Serialize(builder);
        result.emplace(std::move(builder).Build());
    } else {
        result.emplace("{}");
    }
}

TEST_F(ServerTest, DebuggerEnable)
{
    TestServer server1;
    EXPECT_CALL(server1, OnCallMock("Target.attachToTarget", testing::_)).WillOnce([&](testing::Unused, auto handler) {
        JsonObjectBuilder params;
        params.AddProperty("targetId", g_sessionId);
        auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
        ASSERT_FALSE(res.HasValue());
    });
    EXPECT_CALL(server1, OnCallMock("Debugger.enable", testing::_)).WillOnce([&](testing::Unused, auto handler) {
        JsonObject empty;
        auto res = handler(g_sessionId, empty);
        ResultHolder result;
        GetResult(std::move(res), result);
        std::vector<testing::Matcher<JsonObject::JsonObjPointer>> protocols;
        ASSERT_THAT(*result,
                    JsonProperties(JsonProperty<JsonObject::NumT> {"debuggerId", 0},
                                   JsonProperty<JsonObject::ArrayT> {"protocols", JsonElementsAreArray(protocols)}));
    });
    InspectorServer inspectorServer1(server1);
    inspectorServer1.OnCallDebuggerEnable([] {});
}

TEST_F(ServerTest, OnCallAttachToTarget)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Target.attachToTarget", testing::_)).WillOnce([&](testing::Unused, auto handler) {
        JsonObjectBuilder params;
        params.AddProperty("targetId", g_sessionId);
        auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
        ResultHolder result;
        GetResult(std::move(res), result);
        ASSERT_TRUE(result);
        ASSERT_THAT(*result, JsonProperties(JsonProperty<JsonObject::StringT> {"sessionId", g_sessionId}));
    });
    inspectorServer.OnCallTargetAttachToTarget();
}

static auto g_simpleHandler = []([[maybe_unused]] auto unused, auto handler) {
    JsonObject empty;
    auto res = handler(g_sessionId, empty);
    ResultHolder result;
    GetResult(std::move(res), result);
    ASSERT_THAT(*result, JsonProperties());
    ASSERT_TRUE(g_handlerCalled);
};

TEST_F(ServerTest, OnCallDebuggerPause)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.pause", testing::_)).WillOnce(g_simpleHandler);
    inspectorServer.OnCallDebuggerPause([](PtThread thread) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallDebuggerRemoveBreakpoint)
{
    size_t breakId = 14;

    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.removeBreakpoint", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObject empty;
            auto res = handler(g_sessionId, empty);
            ASSERT_FALSE(res.HasValue());
            ASSERT_FALSE(g_handlerCalled);
        });

    auto breaks = [breakId](PtThread thread, BreakpointId bid) {
        ASSERT_EQ(bid, BreakpointId(breakId));
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        g_handlerCalled = true;
    };
    inspectorServer.OnCallDebuggerRemoveBreakpoint(std::move(breaks));

    EXPECT_CALL(server, OnCallMock("Debugger.removeBreakpoint", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObjectBuilder params;
            params.AddProperty("breakpointId", std::to_string(breakId));
            auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
            ResultHolder result;
            GetResult(std::move(res), result);
            ASSERT_THAT(*result, JsonProperties());
            ASSERT_TRUE(g_handlerCalled);
        });

    inspectorServer.OnCallDebuggerRemoveBreakpoint(std::move(breaks));
}

TEST_F(ServerTest, OnCallDebuggerRestartFrame)
{
    size_t fid = 5;

    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.restartFrame", testing::_)).WillOnce([&](testing::Unused, auto handler) {
        std::vector<testing::Matcher<JsonObject::JsonObjPointer>> callFrames;
        JsonObjectBuilder params;
        auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
        ASSERT_FALSE(res.HasValue());
        ASSERT_FALSE(g_handlerCalled);
    });

    inspectorServer.OnCallDebuggerRestartFrame([&](auto, auto) { g_handlerCalled = true; });

    EXPECT_CALL(server, OnCallMock("Debugger.restartFrame", testing::_)).WillOnce([&](testing::Unused, auto handler) {
        std::vector<testing::Matcher<JsonObject::JsonObjPointer>> callFrames;
        JsonObjectBuilder params;
        params.AddProperty("callFrameId", std::to_string(fid));
        auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
        ResultHolder result;
        GetResult(std::move(res), result);
        ASSERT_THAT(*result,
                    JsonProperties(JsonProperty<JsonObject::ArrayT> {"callFrames", JsonElementsAreArray(callFrames)}));
        ASSERT_TRUE(g_handlerCalled);
    });

    inspectorServer.OnCallDebuggerRestartFrame([&](PtThread thread, FrameId id) {
        ASSERT_EQ(id, FrameId(fid));
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallDebuggerResume)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.resume", testing::_)).WillOnce(g_simpleHandler);
    inspectorServer.OnCallDebuggerResume([](PtThread thread) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        g_handlerCalled = true;
    });
}

static JsonObject CreatePossibleBreakpointsRequest(ScriptId startScriptId, int32_t start, ScriptId endScriptId,
                                                   int32_t end, bool restrictToFunction)
{
    JsonObjectBuilder params;
    params.AddProperty("start", Location(startScriptId, start));
    params.AddProperty("end", Location(endScriptId, end));
    params.AddProperty("restrictToFunction", restrictToFunction);
    return JsonObject(std::move(params).Build());
}

static auto g_getPossibleBreakpointsHandler = [](ScriptId scriptId, int32_t start, int32_t end, bool restrictToFunction,
                                                 testing::Unused, auto handler) {
    auto res =
        handler(g_sessionId, CreatePossibleBreakpointsRequest(scriptId, start, scriptId, end, restrictToFunction));
    ResultHolder result;
    GetResult(std::move(res), result);
    std::vector<testing::Matcher<JsonObject::JsonObjPointer>> locations;
    for (auto i = start; i < end; i++) {
        locations.push_back(
            testing::Pointee(JsonProperties(JsonProperty<JsonObject::StringT> {"scriptId", std::to_string(scriptId)},
                                            JsonProperty<JsonObject::NumT> {"lineNumber", i})));
    }
    ASSERT_THAT(*result,
                JsonProperties(JsonProperty<JsonObject::ArrayT> {"locations", JsonElementsAreArray(locations)}));
};

static void DefaultFrameEnumerator(const InspectorServer::FrameInfoHandler &handler)
{
    std::optional<RemoteObject> objThis;
    auto scope_chain = std::vector {Scope(Scope::Type::LOCAL, RemoteObject::Number(72))};
    handler(FrameId(0), std::to_string(0), g_sourceFile, {}, 0, scope_chain, objThis, true);
}

class InMemoryCdpServer final : public ServerEndpointBase {
public:
    bool ParseMessage(const std::string &message) override
    {
        HandleMessage(message);
        return true;
    }

    uint64_t NextRequestId()
    {
        return nextRequestId_++;
    }

    void Clear()
    {
        os::memory::LockHolder lock(messagesMutex_);
        messages_.clear();
    }

    std::optional<std::string> WaitForResponse(uint64_t requestId)
    {
        return WaitForMessage([requestId](const JsonObject &message) {
            auto id = message.GetValue<JsonObject::NumT>("id");
            return id != nullptr && static_cast<uint64_t>(*id) == requestId;
        });
    }

    std::optional<std::string> WaitForEvent(std::string_view methodName)
    {
        return WaitForMessage([methodName](const JsonObject &message) {
            auto method = message.GetValue<JsonObject::StringT>("method");
            return method != nullptr && *method == methodName;
        });
    }

    Server::Handler GetPauseHandler() const
    {
        return pauseHandler_;
    }

private:
    class EmptyResponse final : public JsonSerializable {
    public:
        EmptyResponse() = default;

        DEFAULT_COPY_SEMANTIC(EmptyResponse);
        DEFAULT_MOVE_SEMANTIC(EmptyResponse);

        ~EmptyResponse() override = default;

        void Serialize([[maybe_unused]] JsonObjectBuilder &builder) const override {}
    };

    std::optional<std::string> WaitForMessage(const std::function<bool(const JsonObject &)> &predicate)
    {
        constexpr auto timeout = std::chrono::milliseconds(5000);
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        os::memory::LockHolder lock(messagesMutex_);
        while (std::chrono::steady_clock::now() < deadline) {
            if (auto message = FindMessage(predicate)) {
                return message;
            }

            auto remaining =
                std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
            if (remaining.count() <= 0) {
                return std::nullopt;
            }
            (void)messageCondition_.TimedWait(&messagesMutex_, static_cast<uint64_t>(remaining.count()));
        }
        return FindMessage(predicate);
    }

    void OnCallImpl(const char *method, Handler &&handler) override
    {
        if (std::string_view(method) == "Debugger.pause") {
            pauseHandler_ = handler;
        }

        EndpointBase::OnCall(method, [this, handler = std::move(handler)](auto &sessionId, auto id, auto &params) {
            if (!id) {
                LOG(INFO, DEBUGGER) << "Invalid request: request has no \"id\"";
                return;
            }

            auto optResult = handler(sessionId, params);
            if (optResult) {
                if (*optResult) {
                    JsonSerializable &result = **optResult;
                    Reply(sessionId, *id, result);
                } else {
                    Reply(sessionId, *id, EmptyResponse());
                }
            } else {
                ReplyError(sessionId, *id, std::move(optResult.Error()));
            }
        });
    }

    std::optional<std::string> FindMessage(const std::function<bool(const JsonObject &)> &predicate)
        REQUIRES(messagesMutex_)
    {
        for (auto it = messages_.begin(); it != messages_.end(); ++it) {
            JsonObject message(*it);
            if (message.IsValid() && predicate(message)) {
                auto result = *it;
                messages_.erase(it);
                return result;
            }
        }
        return std::nullopt;
    }

    void SendMessage(const std::string &message) override
    {
        os::memory::LockHolder lock(messagesMutex_);
        messages_.push_back(message);
        messageCondition_.SignalAll();
    }

    uint64_t nextRequestId_ = 1U;
    Server::Handler pauseHandler_;
    os::memory::Mutex messagesMutex_;
    os::memory::ConditionVariable messageCondition_ GUARDED_BY(messagesMutex_);
    std::deque<std::string> messages_ GUARDED_BY(messagesMutex_);
};

#if defined(PANDA_TARGET_LINUX)

ani_int CaptureRealAsyncStack([[maybe_unused]] ani_env *env, ani_int value)
{
    g_realAsyncStackSnapshot.reset();

    auto handle = ark::Job::CloneCurrentAsyncDebuggerStack();
    if (handle != nullptr) {
        g_realAsyncStackSnapshot = handle->CreateSnapshotView();
    }

    auto continuationThread = PtThread(ManagedThread::GetCurrent());
    if (g_realInspector != nullptr && continuationThread != g_mthread) {
        // The test creates Inspector after the VM worker pool, so it cannot replay ThreadStart events.
        // Register the continuation thread explicitly to test a pause emitted on that thread.
        bool shouldRegisterThread = false;
        {
            os::memory::LockHolder lock(g_realRegisteredThreadsMutex);
            shouldRegisterThread = g_realRegisteredThreads.insert(continuationThread).second;
        }
        if (shouldRegisterThread) {
            g_realInspector->ThreadStart(continuationThread);
        }
    }

    if (g_realPauseHandler != nullptr) {
        (void)g_realPauseHandler(g_sessionId, JsonObject {});
    }

    return value;
}

class InspectorAsyncStackE2ETest : public testing::Test {
protected:
    static constexpr int32_t PROMISE_OBSERVE_LINE = 22;              // zero-based source line
    static constexpr int32_t AWAIT_CALL_SITE_LINE = 33;              // zero-based source line
    static constexpr int32_t PROMISE_CATCH_OBSERVE_LINE = 56;        // zero-based source line
    static constexpr int32_t PROMISE_FINALLY_OBSERVE_LINE = 63;      // zero-based source line
    static constexpr int32_t CDP_PROMISE_OBSERVE_LINE = 21;          // zero-based CDP line number
    static constexpr int32_t CDP_AWAIT_OBSERVE_LINE = 32;            // zero-based CDP line number
    static constexpr int32_t CDP_PROMISE_CATCH_OBSERVE_LINE = 55;    // zero-based CDP line number
    static constexpr int32_t CDP_PROMISE_FINALLY_OBSERVE_LINE = 62;  // zero-based CDP line number

    void SetUp() override
    {
        const auto buildRoot = GetBuildRoot();
        stdlibPath_ = (buildRoot / asyncE2EStdlibPath).string();
        abcPath_ = (buildRoot / asyncE2EAbcPath).string();

        std::string bootFiles = "--ext:boot-panda-files=" + stdlibPath_ + ":" + abcPath_;
        std::string debuggerAttachAllowedOption = "--ext:debugger-attach-allowed=true";
        std::string compilerEnableJitOption = "--ext:compiler-enable-jit=false";
        std::array<ani_option, 3U> optionValues = {
            ani_option {bootFiles.c_str(), nullptr},
            ani_option {debuggerAttachAllowedOption.c_str(), nullptr},
            ani_option {compilerEnableJitOption.c_str(), nullptr},
        };
        ani_options options = {optionValues.size(), optionValues.data()};
        ASSERT_EQ(ANI_CreateVM(&options, ANI_VERSION_1, &vm_), ANI_OK);
        ASSERT_EQ(vm_->GetEnv(ANI_VERSION_1, &env_), ANI_OK);

        auto *coroutine = ark::ets::EtsCoroutine::GetCurrent();
        ASSERT_NE(coroutine, nullptr);
        pandaVm_ = coroutine->GetPandaVM();
        ASSERT_NE(pandaVm_, nullptr);
        constexpr uint32_t defaultMaxAsyncDepth = 8U;
        pandaVm_->SetAsyncDebuggerMaxAsyncDepth(defaultMaxAsyncDepth);
        g_mthread = PtThread(ManagedThread::GetCurrent());
        g_realAsyncStackSnapshot.reset();
        {
            os::memory::LockHolder lock(g_realRegisteredThreadsMutex);
            g_realRegisteredThreads.clear();
        }

        BindNativeFunctions();
        AddModuleDebugInfo();
    }

    void TearDown() override
    {
        g_realAsyncStackSnapshot.reset();
        {
            os::memory::LockHolder lock(g_realRegisteredThreadsMutex);
            g_realRegisteredThreads.clear();
        }
        g_realPauseHandler = nullptr;
        g_realInspector = nullptr;
        if (vm_ != nullptr) {
            ASSERT_EQ(vm_->DestroyVM(), ANI_OK);
        }
    }

    void AssertSingleSegment(std::string_view description) const
    {
        AssertSegments(description, 1U);
    }

    void AssertSegments(std::string_view description, size_t expectedSegmentCount) const
    {
        ASSERT_NE(g_realAsyncStackSnapshot, nullptr);
        ASSERT_EQ(g_realAsyncStackSnapshot->segments.size(), expectedSegmentCount);
        ASSERT_EQ(g_realAsyncStackSnapshot->segments.front().description, description);
        ASSERT_TRUE(std::any_of(g_realAsyncStackSnapshot->segments.begin(), g_realAsyncStackSnapshot->segments.end(),
                                [](const AsyncStackSegmentView &segment) { return !segment.frames.empty(); }));
    }

    void AssertSegmentFrameFunctionNamesContain(size_t segmentIndex, std::string_view functionName) const
    {
        ASSERT_NE(g_realAsyncStackSnapshot, nullptr);
        if (segmentIndex >= g_realAsyncStackSnapshot->segments.size()) {
            ADD_FAILURE() << "Invalid async stack segment index: " << segmentIndex;
            return;
        }
        const auto &frames = g_realAsyncStackSnapshot->segments[segmentIndex].frames;
        ASSERT_FALSE(frames.empty());
        ASSERT_TRUE(std::any_of(frames.begin(), frames.end(), [functionName](const AsyncStackFrameView &frame) {
            return frame.functionName.find(functionName) != std::string::npos;
        }));
    }

    std::vector<int32_t> GetSegmentFrameLineNumbers(size_t segmentIndex) const
    {
        if (segmentIndex >= g_realAsyncStackSnapshot->segments.size()) {
            ADD_FAILURE() << "Invalid async stack segment index: " << segmentIndex;
            return {};
        }
        std::vector<int32_t> lineNumbers;
        for (const auto &frame : g_realAsyncStackSnapshot->segments[segmentIndex].frames) {
            auto location =
                debugInfoCache_.GetAsyncFrameSourceLocation(frame.pandaFile, frame.methodId, frame.bytecodeOffset);
            if (!location.has_value()) {
                continue;
            }
            EXPECT_EQ(location->sourceFile, "AsyncStackPromiseTest.ets");
            lineNumbers.push_back(location->lineNumber);
        }
        return lineNumbers;
    }

    std::vector<int32_t> GetFrontFrameLineNumbers() const
    {
        return GetSegmentFrameLineNumbers(0U);
    }

    std::unique_ptr<AsyncStackTrace> CreateAsyncStackTrace(InspectorServer &inspectorServer)
    {
        return inspectorServer.CreateAsyncStackTrace(*g_realAsyncStackSnapshot, [this](
                                                                                    const AsyncStackFrameView &frame) {
            return debugInfoCache_.GetAsyncFrameSourceLocation(frame.pandaFile, frame.methodId, frame.bytecodeOffset);
        });
    }

    void ExpectScriptParsed(std::vector<std::string> &scriptIds)
    {
        EXPECT_CALL(server_, CallMock(g_sessionId, "Debugger.scriptParsed", testing::_))
            .WillOnce([&scriptIds](testing::Unused, testing::Unused, auto params) {
                auto scriptParsed = ToObject(std::move(params));
                scriptIds.push_back(*scriptParsed.template GetValue<JsonObject::StringT>("scriptId"));
            });
    }

    void ExpectAsyncStackTrace(std::string_view description)
    {
        EXPECT_CALL(server_, CallMock(g_sessionId, "Debugger.paused", testing::_))
            .WillOnce([description](testing::Unused, testing::Unused, auto params) {
                auto paused = ToObject(std::move(params));
                auto asyncStackTrace = paused.template GetValue<JsonObject::JsonObjPointer>("asyncStackTrace");
                ASSERT_NE(asyncStackTrace, nullptr);

                auto actualDescription = (*asyncStackTrace)->template GetValue<JsonObject::StringT>("description");
                ASSERT_NE(actualDescription, nullptr);
                ASSERT_EQ(*actualDescription, description);

                auto callFrames = (*asyncStackTrace)->template GetValue<JsonObject::ArrayT>("callFrames");
                ASSERT_NE(callFrames, nullptr);
                ASSERT_FALSE(callFrames->empty());

                auto frameObject = callFrames->front().template Get<JsonObject::JsonObjPointer>();
                ASSERT_NE(frameObject, nullptr);
                auto url = (*frameObject)->template GetValue<JsonObject::StringT>("url");
                auto lineNumber = (*frameObject)->template GetValue<JsonObject::NumT>("lineNumber");
                ASSERT_NE(url, nullptr);
                ASSERT_NE(lineNumber, nullptr);
                ASSERT_EQ(*url, "AsyncStackPromiseTest.ets");
                ASSERT_GE(*lineNumber, 0);
            });
    }

    static void QueueResume(Server::Handler &resumeHandler, std::vector<std::thread> &resumeThreads)
    {
        resumeThreads.emplace_back([&resumeHandler]() { (void)resumeHandler(g_sessionId, JsonObject {}); });
    }

    void ExpectCommonDebuggerEvents(std::vector<std::thread> &resumeThreads)
    {
        EXPECT_CALL(server_, CallMock(testing::_, "Target.attachedToTarget", testing::_))
            .Times(testing::AnyNumber())
            .WillRepeatedly([](testing::Unused, testing::Unused, auto) {});
        EXPECT_CALL(server_, CallMock(testing::_, "Target.detachedFromTarget", testing::_))
            .Times(testing::AnyNumber())
            .WillRepeatedly([](testing::Unused, testing::Unused, auto) {});
        EXPECT_CALL(server_, CallMock(g_sessionId, "Debugger.scriptParsed", testing::_))
            .Times(1)
            .WillRepeatedly([](testing::Unused, testing::Unused, auto) {});
        EXPECT_CALL(server_, CallMock(testing::_, "Debugger.resumed", testing::_))
            .Times(EXPECTED_RESUMED_EVENT_COUNT)
            .WillRepeatedly([](testing::Unused, testing::Unused, auto) {});
    }

    void ExpectPauseEvents(Server::Handler &resumeHandler, std::vector<std::thread> &resumeThreads)
    {
        EXPECT_CALL(server_, CallMock(testing::_, "Debugger.paused", testing::_))
            .Times(EXPECTED_PAUSE_EVENT_COUNT)
            .WillOnce([&resumeHandler, &resumeThreads](testing::Unused, testing::Unused, auto params) {
                auto paused = ToObject(std::move(params));
                ASSERT_EQ(paused.template GetValue<JsonObject::JsonObjPointer>("asyncStackTrace"), nullptr);
                QueueResume(resumeHandler, resumeThreads);
            })
            .WillOnce([&resumeHandler, &resumeThreads](testing::Unused, testing::Unused, auto params) {
                auto paused = ToObject(std::move(params));
                auto asyncStackTrace = paused.template GetValue<JsonObject::JsonObjPointer>("asyncStackTrace");
                ASSERT_NE(asyncStackTrace, nullptr);
                auto description = (*asyncStackTrace)->template GetValue<JsonObject::StringT>("description");
                ASSERT_NE(description, nullptr);
                ASSERT_EQ(*description, "promise.then");
                QueueResume(resumeHandler, resumeThreads);
            });
    }

    static void JoinResumeThreads(std::vector<std::thread> &resumeThreads)
    {
        for (auto &resumeThread : resumeThreads) {
            ASSERT_TRUE(resumeThread.joinable());
            resumeThread.join();
        }
    }

    static Runtime::DebugSessionHandle StartCdpDebugSession()
    {
        if (Runtime::GetCurrent()->GetTools().AttachDebugSession() != DebugSessionAttachErrorCode::OK) {
            return {};
        }

        return Runtime::GetCurrent()->StartDebugSession();
    }

    std::optional<std::string> PrepareCdpInspectorWithoutIde(InMemoryCdpServer &server, Inspector &inspector)
    {
        g_realInspector = &inspector;
        inspector.LoadModule(abcPath_);
        inspector.ThreadStart(g_mthread);
        if (!SendCdpCommand(inspector, server, g_sessionId, "Debugger.enable", "{}").has_value()) {
            return std::nullopt;
        }

        std::optional<std::string> initialPauseEvent;
        std::thread initialResponder([&inspector, &server, &initialPauseEvent]() {
            initialPauseEvent = server.WaitForEvent("Debugger.paused");
            if (initialPauseEvent.has_value()) {
                (void)SendCdpCommand(inspector, server, g_sessionId, "Debugger.resume", "{}");
            }
        });

        std::optional<std::string> attachedEvent;
        std::thread attachedTargetWaiter(
            [&server, &attachedEvent]() { attachedEvent = server.WaitForEvent("Target.attachedToTarget"); });
        if (CallEtsFunction("alreadySettled") != 0) {
            initialResponder.join();
            attachedTargetWaiter.join();
            return std::nullopt;
        }

        initialResponder.join();
        attachedTargetWaiter.join();
        if (!initialPauseEvent.has_value() || !attachedEvent.has_value()) {
            return std::nullopt;
        }

        return ParseTargetId(*attachedEvent);
    }

    static std::string BuildCdpRequest(uint64_t requestId, const std::string &sessionId, const std::string &method,
                                       const std::string &params)
    {
        std::string request = "{\"id\":" + std::to_string(requestId);
        if (!sessionId.empty()) {
            request += ",\"sessionId\":\"" + sessionId + "\"";
        }
        request += ",\"method\":\"" + method + "\",\"params\":" + params + "}";
        return request;
    }

    static std::optional<std::string> SendCdpCommand(Inspector &inspector, InMemoryCdpServer &server,
                                                     const std::string &sessionId, const std::string &method,
                                                     const std::string &params)
    {
        const auto requestId = server.NextRequestId();
        inspector.Run(BuildCdpRequest(requestId, sessionId, method, params));
        auto response = server.WaitForResponse(requestId);
        if (response.has_value()) {
            JsonObject responseObject(*response);
            EXPECT_TRUE(responseObject.IsValid());
            EXPECT_EQ(responseObject.GetValue<JsonObject::JsonObjPointer>("error"), nullptr) << *response;
        }
        return response;
    }

    static bool ValidatePausedEvent(const std::string &message, const std::string &expectedDescription,
                                    int32_t expectedLineNumber, std::string &sessionId)
    {
        JsonObject paused(message);
        if (!paused.IsValid()) {
            return false;
        }

        auto params = paused.GetValue<JsonObject::JsonObjPointer>("params");
        if (params == nullptr) {
            return false;
        }

        auto pausedSessionId = (*params)->GetValue<JsonObject::StringT>("sessionId");
        if (pausedSessionId != nullptr) {
            sessionId = *pausedSessionId;
        }

        auto asyncStackTrace = (*params)->GetValue<JsonObject::JsonObjPointer>("asyncStackTrace");
        if (expectedDescription.empty()) {
            return asyncStackTrace == nullptr;
        }
        if (asyncStackTrace == nullptr) {
            return false;
        }

        auto description = (*asyncStackTrace)->GetValue<JsonObject::StringT>("description");
        if (description == nullptr || *description != expectedDescription) {
            return false;
        }

        auto callFrames = (*asyncStackTrace)->GetValue<JsonObject::ArrayT>("callFrames");
        if (callFrames == nullptr || callFrames->empty()) {
            return false;
        }

        for (const auto &frameValue : *callFrames) {
            auto frame = frameValue.Get<JsonObject::JsonObjPointer>();
            if (frame == nullptr) {
                continue;
            }
            auto url = (*frame)->GetValue<JsonObject::StringT>("url");
            auto lineNumber = (*frame)->GetValue<JsonObject::NumT>("lineNumber");
            if (url != nullptr && *url == "AsyncStackPromiseTest.ets" && lineNumber != nullptr &&
                *lineNumber == expectedLineNumber) {
                return true;
            }
        }
        return false;
    }

    static std::optional<std::string> ParseTargetId(const std::string &message)
    {
        JsonObject attached(message);
        if (!attached.IsValid()) {
            return std::nullopt;
        }

        auto params = attached.GetValue<JsonObject::JsonObjPointer>("params");
        if (params == nullptr) {
            return std::nullopt;
        }

        auto targetInfo = (*params)->GetValue<JsonObject::JsonObjPointer>("targetInfo");
        if (targetInfo == nullptr) {
            return std::nullopt;
        }

        auto targetId = (*targetInfo)->GetValue<JsonObject::StringT>("targetId");
        if (targetId == nullptr || targetId->empty()) {
            return std::nullopt;
        }
        return *targetId;
    }

    void RunPausedScenario(Inspector &inspector, InMemoryCdpServer &server, const std::string &sessionId,
                           const std::string &functionName, const std::string &expectedDescription,
                           int32_t expectedLineNumber, size_t expectedSegmentCount = 1U)
    {
        auto pauseResponse = SendCdpCommand(inspector, server, sessionId, "Debugger.pause", "{}");
        ASSERT_TRUE(pauseResponse.has_value());

        std::thread responder([&inspector, &server, &expectedDescription, expectedLineNumber]() {
            auto entryPauseEvent = server.WaitForEvent("Debugger.paused");
            if (!entryPauseEvent.has_value()) {
                ADD_FAILURE() << "Timed out waiting for the entry Debugger.paused event";
                return;
            }

            std::string entrySessionId;
            if (!ValidatePausedEvent(*entryPauseEvent, "", -1, entrySessionId)) {
                ADD_FAILURE() << "Invalid entry Debugger.paused event: " << *entryPauseEvent;
                return;
            }
            (void)SendCdpCommand(inspector, server, entrySessionId, "Debugger.resume", "{}");

            auto pausedEvent = server.WaitForEvent("Debugger.paused");
            if (!pausedEvent.has_value()) {
                ADD_FAILURE() << "Timed out waiting for Debugger.paused";
                return;
            }

            std::string pausedSessionId;
            if (!ValidatePausedEvent(*pausedEvent, expectedDescription, expectedLineNumber, pausedSessionId)) {
                ADD_FAILURE() << "Invalid Debugger.paused event: " << *pausedEvent;
                return;
            }

            auto resumeResponse = SendCdpCommand(inspector, server, pausedSessionId, "Debugger.resume", "{}");
            if (!resumeResponse.has_value()) {
                ADD_FAILURE() << "Timed out waiting for Debugger.resume response";
            }
        });

        ASSERT_EQ(CallEtsFunction(functionName), 0);
        responder.join();
        server.Clear();

        if (!expectedDescription.empty()) {
            AssertSegments(expectedDescription, expectedSegmentCount);
        }
    }

    ani_int CallEtsFunction(const std::string &functionName)
    {
        ani_module module {};
        ani_function function {};
        ani_int result = 0;
        EXPECT_EQ(env_->FindModule(MODULE_NAME, &module), ANI_OK);
        EXPECT_EQ(env_->Module_FindFunction(module, functionName.c_str(), nullptr, &function), ANI_OK);
        EXPECT_EQ(env_->Function_Call_Int(function, &result), ANI_OK);
        return result;
    }

    static std::filesystem::path GetBuildRoot()
    {
        std::error_code error;
        auto executable = std::filesystem::read_symlink("/proc/self/exe", error);
        if (error) {
            return {};
        }

        return executable.parent_path().parent_path().parent_path().parent_path().parent_path();
    }

    TestServer server_;
    DebugInfoCache debugInfoCache_;
    ani_env *env_ {nullptr};
    ani_vm *vm_ {nullptr};
    ark::ets::PandaEtsVM *pandaVm_ {nullptr};
    std::string stdlibPath_;
    std::string abcPath_;

private:
    static constexpr const char *MODULE_NAME = "AsyncStackPromiseTest";

    void BindNativeFunctions()
    {
        ani_module module {};
        ASSERT_EQ(env_->FindModule(MODULE_NAME, &module), ANI_OK);

        ani_native_function function = {"observeAsyncStack", "i:i", reinterpret_cast<void *>(CaptureRealAsyncStack)};
        ASSERT_EQ(env_->Module_BindNativeFunctions(module, &function, 1U), ANI_OK);
    }

    void AddModuleDebugInfo()
    {
        Runtime::GetCurrent()->GetClassLinker()->EnumeratePandaFiles([this](auto &file) {
            constexpr std::string_view MODULE_FILE_NAME = "AsyncStackPromiseTest.abc";
            const auto isModuleFile = file.GetFilename().find(MODULE_FILE_NAME) != std::string_view::npos;
            debugInfoCache_.AddPandaFile(file, isModuleFile);
            return true;
        });
    }
};

TEST_F(InspectorAsyncStackE2ETest, RealPromiseThenProducesAsyncStackTrace)
{
    InspectorServer inspectorServer(server_);
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    ASSERT_EQ(CallEtsFunction("alreadySettled"), 0);
    AssertSingleSegment("promise.then");

    const auto lineNumbers = GetFrontFrameLineNumbers();
    ASSERT_NE(std::find(lineNumbers.begin(), lineNumbers.end(), PROMISE_OBSERVE_LINE), lineNumbers.end());

    auto [defaultScriptId, isNewDefaultSource] = inspectorServer.GetSourceManager().GetScriptId(g_sourceFile);
    ASSERT_TRUE(isNewDefaultSource);

    std::vector<std::string> scriptIds;
    ExpectScriptParsed(scriptIds);
    ExpectAsyncStackTrace("promise.then");

    auto asyncStackTrace = CreateAsyncStackTrace(inspectorServer);
    ASSERT_NE(asyncStackTrace, nullptr);
    inspectorServer.CallDebuggerPaused(
        {g_mthread, {}, {}, PauseReason::OTHER, DefaultFrameEnumerator, asyncStackTrace.get()});
    ASSERT_THAT(scriptIds, testing::ElementsAre(std::to_string(defaultScriptId + 1U)));
}

TEST_F(InspectorAsyncStackE2ETest, RealPromiseCatchProducesAsyncStackTrace)
{
    InspectorServer inspectorServer(server_);
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    ASSERT_EQ(CallEtsFunction("alreadyRejectedCatch"), 0);
    ASSERT_NE(g_realAsyncStackSnapshot, nullptr);
    ASSERT_EQ(g_realAsyncStackSnapshot->segments.size(), 1U);
    ASSERT_EQ(g_realAsyncStackSnapshot->segments.front().description, "promise.catch");
    ASSERT_FALSE(g_realAsyncStackSnapshot->segments.front().frames.empty());

    auto [defaultScriptId, isNewDefaultSource] = inspectorServer.GetSourceManager().GetScriptId(g_sourceFile);
    (void)defaultScriptId;
    ASSERT_TRUE(isNewDefaultSource);

    EXPECT_CALL(server_, CallMock(g_sessionId, "Debugger.scriptParsed", testing::_))
        .WillOnce([](testing::Unused, testing::Unused, auto) {});
    EXPECT_CALL(server_, CallMock(g_sessionId, "Debugger.paused", testing::_))
        .WillOnce([](testing::Unused, testing::Unused, auto params) {
            auto paused = ToObject(std::move(params));
            auto asyncStackTrace = paused.template GetValue<JsonObject::JsonObjPointer>("asyncStackTrace");
            ASSERT_NE(asyncStackTrace, nullptr);

            auto description = (*asyncStackTrace)->template GetValue<JsonObject::StringT>("description");
            ASSERT_NE(description, nullptr);
            ASSERT_EQ(*description, "promise.catch");

            auto callFrames = (*asyncStackTrace)->template GetValue<JsonObject::ArrayT>("callFrames");
            ASSERT_NE(callFrames, nullptr);
            ASSERT_FALSE(callFrames->empty());
        });

    auto asyncStackTrace =
        inspectorServer.CreateAsyncStackTrace(*g_realAsyncStackSnapshot, [this](const AsyncStackFrameView &frame) {
            return debugInfoCache_.GetAsyncFrameSourceLocation(frame.pandaFile, frame.methodId, frame.bytecodeOffset);
        });
    ASSERT_NE(asyncStackTrace, nullptr);
    inspectorServer.CallDebuggerPaused(
        {g_mthread, {}, {}, PauseReason::OTHER, DefaultFrameEnumerator, asyncStackTrace.get()});
}

TEST_F(InspectorAsyncStackE2ETest, RealPromiseFinallyProducesAsyncStackTrace)
{
    InspectorServer inspectorServer(server_);
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    ASSERT_EQ(CallEtsFunction("alreadySettledFinally"), 0);
    ASSERT_NE(g_realAsyncStackSnapshot, nullptr);
    ASSERT_EQ(g_realAsyncStackSnapshot->segments.size(), 1U);
    ASSERT_EQ(g_realAsyncStackSnapshot->segments.front().description, "promise.finally");
    ASSERT_FALSE(g_realAsyncStackSnapshot->segments.front().frames.empty());

    auto [defaultScriptId, isNewDefaultSource] = inspectorServer.GetSourceManager().GetScriptId(g_sourceFile);
    (void)defaultScriptId;
    ASSERT_TRUE(isNewDefaultSource);

    EXPECT_CALL(server_, CallMock(g_sessionId, "Debugger.scriptParsed", testing::_))
        .WillOnce([](testing::Unused, testing::Unused, auto) {});
    EXPECT_CALL(server_, CallMock(g_sessionId, "Debugger.paused", testing::_))
        .WillOnce([](testing::Unused, testing::Unused, auto params) {
            auto paused = ToObject(std::move(params));
            auto asyncStackTrace = paused.template GetValue<JsonObject::JsonObjPointer>("asyncStackTrace");
            ASSERT_NE(asyncStackTrace, nullptr);

            auto description = (*asyncStackTrace)->template GetValue<JsonObject::StringT>("description");
            ASSERT_NE(description, nullptr);
            ASSERT_EQ(*description, "promise.finally");

            auto callFrames = (*asyncStackTrace)->template GetValue<JsonObject::ArrayT>("callFrames");
            ASSERT_NE(callFrames, nullptr);
            ASSERT_FALSE(callFrames->empty());
        });

    auto asyncStackTrace =
        inspectorServer.CreateAsyncStackTrace(*g_realAsyncStackSnapshot, [this](const AsyncStackFrameView &frame) {
            return debugInfoCache_.GetAsyncFrameSourceLocation(frame.pandaFile, frame.methodId, frame.bytecodeOffset);
        });
    ASSERT_NE(asyncStackTrace, nullptr);
    inspectorServer.CallDebuggerPaused(
        {g_mthread, {}, {}, PauseReason::OTHER, DefaultFrameEnumerator, asyncStackTrace.get()});
}

TEST_F(InspectorAsyncStackE2ETest, RealAwaitSurvivesDisableAndReenable)
{
    InspectorServer inspectorServer(server_);
    inspectorServer.CallTargetAttachedToTarget(g_mthread);
    auto [defaultScriptId, isNewDefaultSource] = inspectorServer.GetSourceManager().GetScriptId(g_sourceFile);
    (void)defaultScriptId;
    ASSERT_TRUE(isNewDefaultSource);

    pandaVm_->SetAsyncDebuggerMaxAsyncDepth(0U);
    ASSERT_EQ(CallEtsFunction("alreadySettled"), 0);
    ASSERT_EQ(g_realAsyncStackSnapshot, nullptr);

    EXPECT_CALL(server_, CallMock(g_sessionId, "Debugger.paused", testing::_))
        .WillOnce([](testing::Unused, testing::Unused, auto params) {
            auto paused = ToObject(std::move(params));
            ASSERT_EQ(paused.template GetValue<JsonObject::JsonObjPointer>("asyncStackTrace"), nullptr);
        });
    inspectorServer.CallDebuggerPaused({g_mthread, {}, {}, PauseReason::OTHER, DefaultFrameEnumerator});

    constexpr uint32_t maxAsyncDepth = 8U;
    pandaVm_->SetAsyncDebuggerMaxAsyncDepth(maxAsyncDepth);
    ASSERT_EQ(CallEtsFunction("submitAwaitFirst"), 0);
    ASSERT_EQ(CallEtsFunction("resolveAwaitFirst"), 0);
    AssertSegments("await", 2U);
    AssertSegmentFrameFunctionNamesContain(1U, "submitAwaitFirst");
    ASSERT_GT(g_realAsyncStackSnapshot->generation, 0U);

    const auto lineNumbers = GetSegmentFrameLineNumbers(1U);
    ASSERT_NE(std::find(lineNumbers.begin(), lineNumbers.end(), AWAIT_CALL_SITE_LINE), lineNumbers.end());

    EXPECT_CALL(server_, CallMock(g_sessionId, "Debugger.scriptParsed", testing::_))
        .WillOnce([](testing::Unused, testing::Unused, auto) {});
    ExpectAsyncStackTrace("await");

    auto asyncStackTrace = CreateAsyncStackTrace(inspectorServer);
    ASSERT_NE(asyncStackTrace, nullptr);
    inspectorServer.CallDebuggerPaused(
        {g_mthread, {}, {}, PauseReason::OTHER, DefaultFrameEnumerator, asyncStackTrace.get()});
}

TEST_F(InspectorAsyncStackE2ETest, RealPromisePauseEmitsAndResumesAsyncStackTrace)
{
    Server::Handler pauseHandler;
    Server::Handler resumeHandler;
    EXPECT_CALL(server_, OnCallMock(testing::_, testing::_)).Times(testing::AnyNumber());
    EXPECT_CALL(server_, OnCallMock("Debugger.pause", testing::_))
        .WillOnce([&pauseHandler](testing::Unused, auto handler) { pauseHandler = std::move(handler); });
    EXPECT_CALL(server_, OnCallMock("Debugger.resume", testing::_))
        .WillOnce([&resumeHandler](testing::Unused, auto handler) { resumeHandler = std::move(handler); });

    ASSERT_EQ(Runtime::GetCurrent()->GetTools().AttachDebugSession(), DebugSessionAttachErrorCode::OK);
    auto debugSession = Runtime::GetCurrent()->StartDebugSession();
    ASSERT_NE(debugSession, nullptr);
    Inspector inspector(server_, debugSession->GetDebugger());
    g_realInspector = &inspector;
    inspector.LoadModule(abcPath_);
    inspector.ThreadStart(g_mthread);
    ASSERT_TRUE(pauseHandler != nullptr);
    ASSERT_TRUE(resumeHandler != nullptr);

    std::vector<std::thread> resumeThreads;
    ExpectCommonDebuggerEvents(resumeThreads);
    ExpectPauseEvents(resumeHandler, resumeThreads);

    g_realPauseHandler = pauseHandler;
    ASSERT_EQ(CallEtsFunction("alreadySettled"), 0);
    ASSERT_EQ(resumeThreads.size(), 2U);
    JoinResumeThreads(resumeThreads);
    g_realInspector = nullptr;
    g_realPauseHandler = nullptr;

    AssertSingleSegment("promise.then");
}

TEST_F(InspectorAsyncStackE2ETest, CdpJsonRpcWithoutIdeCoversAsyncStackScenarios)
{
    InMemoryCdpServer server;
    auto debugSession = StartCdpDebugSession();
    ASSERT_NE(debugSession, nullptr);
    Inspector inspector(server, debugSession->GetDebugger());
    auto targetId = PrepareCdpInspectorWithoutIde(server, inspector);
    ASSERT_TRUE(targetId.has_value());
    auto sessionId = g_sessionId;
    server.Clear();

    ASSERT_TRUE(SendCdpCommand(inspector, server, sessionId, "Debugger.enable", "{}").has_value());
    ASSERT_TRUE(SendCdpCommand(inspector, server, sessionId, "Debugger.setAsyncCallStackDepth", "{\"maxDepth\":8}")
                    .has_value());

    g_realPauseHandler = server.GetPauseHandler();
    ASSERT_TRUE(g_realPauseHandler != nullptr);
    RunPausedScenario(inspector, server, sessionId, "alreadySettled", "promise.then", CDP_PROMISE_OBSERVE_LINE);
    RunPausedScenario(inspector, server, sessionId, "alreadyRejectedCatch", "promise.catch",
                      CDP_PROMISE_CATCH_OBSERVE_LINE);
    RunPausedScenario(inspector, server, sessionId, "alreadySettledFinally", "promise.finally",
                      CDP_PROMISE_FINALLY_OBSERVE_LINE);

    ASSERT_EQ(CallEtsFunction("submitAwaitFirst"), 0);
    RunPausedScenario(inspector, server, sessionId, "resolveAwaitFirst", "await", CDP_AWAIT_OBSERVE_LINE, 2U);
    AssertSegmentFrameFunctionNamesContain(1U, "submitAwaitFirst");

    ASSERT_TRUE(SendCdpCommand(inspector, server, sessionId, "Debugger.setAsyncCallStackDepth", "{\"maxDepth\":0}")
                    .has_value());
    RunPausedScenario(inspector, server, sessionId, "alreadySettled", "", -1);
    ASSERT_TRUE(SendCdpCommand(inspector, server, sessionId, "Debugger.setAsyncCallStackDepth", "{\"maxDepth\":8}")
                    .has_value());
    RunPausedScenario(inspector, server, sessionId, "alreadySettled", "promise.then", CDP_PROMISE_OBSERVE_LINE);

    g_realInspector = nullptr;
    g_realPauseHandler = nullptr;
}

#endif  // PANDA_TARGET_LINUX

testing::Matcher<JsonObject::JsonObjPointer> CreateFrameMatcher(std::string functionName, std::string scriptId)
{
    return testing::Pointee(JsonProperties(
        JsonProperty<JsonObject::StringT> {"functionName", functionName},
        JsonProperty<JsonObject::StringT> {"scriptId", scriptId}, JsonProperty<JsonObject::StringT> {"url", "main.ets"},
        JsonProperty<JsonObject::NumT> {"lineNumber", 23}, JsonProperty<JsonObject::NumT> {"columnNumber", 0}));
}

void ExpectSerializedAsyncStackTrace(TestServer &server)
{
    EXPECT_CALL(server, CallMock(g_sessionId, "Debugger.paused", testing::_))
        .WillOnce([](testing::Unused, testing::Unused, auto params) {
            auto paused = ToObject(std::move(params));
            auto asyncStackTrace = paused.template GetValue<JsonObject::JsonObjPointer>("asyncStackTrace");
            ASSERT_NE(asyncStackTrace, nullptr);

            std::vector<testing::Matcher<JsonObject::JsonObjPointer>> parentFrames;
            parentFrames.push_back(CreateFrameMatcher("runAwait", "0"));
            auto parent = testing::Pointee(
                JsonProperties(JsonProperty<JsonObject::StringT> {"description", "await"},
                               JsonProperty<JsonObject::ArrayT> {"callFrames", JsonElementsAreArray(parentFrames)}));

            std::vector<testing::Matcher<JsonObject::JsonObjPointer>> rootFrames;
            rootFrames.push_back(CreateFrameMatcher("runThen", "1"));
            ASSERT_THAT(
                **asyncStackTrace,
                JsonProperties(JsonProperty<JsonObject::StringT> {"description", "promise.then"},
                               JsonProperty<JsonObject::ArrayT> {"callFrames", JsonElementsAreArray(rootFrames)},
                               JsonProperty<JsonObject::JsonObjPointer> {"parent", parent}));
        });
}

TEST_F(ServerTest, CallDebuggerPausedSerializesAsyncStackTrace)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    AsyncStackSnapshotView snapshotView;
    snapshotView.generation = 7U;
    snapshotView.segments = {
        AsyncStackSegmentView {
            "promise.then", 7U, {AsyncStackFrameView {"runThen", "modules/latest.abc", 101U, 12U, 34U}}},
        AsyncStackSegmentView {"await", 7U, {AsyncStackFrameView {"runAwait", "modules/older.abc", 202U, 20U, 56U}}},
    };

    auto resolveFrame = [](const AsyncStackFrameView &frame) {
        return std::optional<AsyncFrameSourceLocation> {AsyncFrameSourceLocation {"main.ets", 24U}};
    };
    std::vector<std::string> scriptIds;
    EXPECT_CALL(server, CallMock(g_sessionId, "Debugger.scriptParsed", testing::_))
        .Times(EXPECTED_ASYNC_SCRIPT_PARSED_COUNT)
        .WillRepeatedly([&scriptIds](testing::Unused, testing::Unused, auto params) {
            auto scriptParsed = ToObject(std::move(params));
            scriptIds.push_back(*scriptParsed.template GetValue<JsonObject::StringT>("scriptId"));
        });
    ExpectSerializedAsyncStackTrace(server);

    auto asyncStackTrace = inspectorServer.CreateAsyncStackTrace(snapshotView, resolveFrame);
    ASSERT_NE(asyncStackTrace, nullptr);
    auto [currentScriptId, isNew] = inspectorServer.GetSourceManager().GetScriptId(g_sourceFile);
    ASSERT_TRUE(isNew);
    ASSERT_EQ(currentScriptId, 2U);

    inspectorServer.CallDebuggerPaused(
        {g_mthread, {}, {}, PauseReason::OTHER, DefaultFrameEnumerator, asyncStackTrace.get()});
    ASSERT_THAT(scriptIds, testing::ElementsAre("0", "1"));
}

TEST_F(ServerTest, CallDebuggerPausedOmitsAsyncStackTraceWhenDisabled)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, CallMock(g_sessionId, "Debugger.scriptParsed", testing::_))
        .WillOnce([](testing::Unused, testing::Unused, auto) {});
    EXPECT_CALL(server, CallMock(g_sessionId, "Debugger.paused", testing::_))
        .WillOnce([](testing::Unused, testing::Unused, auto params) {
            auto paused = ToObject(std::move(params));
            ASSERT_EQ(paused.template GetValue<JsonObject::JsonObjPointer>("asyncStackTrace"), nullptr);
        });

    inspectorServer.CallDebuggerPaused({g_mthread, {}, {}, PauseReason::OTHER, DefaultFrameEnumerator});
}

TEST_F(ServerTest, CreateAsyncStackTraceSkipsSegmentsWithoutResolvableFrames)
{
    AsyncStackSnapshotView snapshotView;
    snapshotView.segments = {
        AsyncStackSegmentView {"await", 1U, {}},
        AsyncStackSegmentView {"await", 1U, {AsyncStackFrameView {"lambda", "main.abc", 1U, 2U, 3U}}},
    };

    auto resolveFrame = [](const AsyncStackFrameView &) {
        return std::optional<AsyncFrameSourceLocation> {AsyncFrameSourceLocation {"main.ets", -1}};
    };

    ASSERT_EQ(inspectorServer.CreateAsyncStackTrace(snapshotView, resolveFrame), nullptr);
}

TEST_F(ServerTest, OnCallDebuggerGetPossibleBreakpoints)
{
    auto scriptId = 0;
    int32_t start = 5;
    int32_t end = 5;

    inspectorServer.CallTargetAttachedToTarget(g_mthread);
    inspectorServer.CallDebuggerPaused({g_mthread, {}, {}, PauseReason::OTHER, DefaultFrameEnumerator});

    EXPECT_CALL(server, OnCallMock("Debugger.getPossibleBreakpoints", testing::_))
        .WillOnce(std::bind(g_getPossibleBreakpointsHandler, scriptId, start, end,  // NOLINT(modernize-avoid-bind)
                            true, _1, _2));
    inspectorServer.OnCallDebuggerGetPossibleBreakpoints(GetLinesTrue);

    EXPECT_CALL(server, OnCallMock("Debugger.getPossibleBreakpoints", testing::_))
        .WillOnce(std::bind(g_getPossibleBreakpointsHandler, scriptId, start, end,  // NOLINT(modernize-avoid-bind)
                            false, _1, _2));
    auto getLinesFalse = [](std::string_view source, [[maybe_unused]] std::string_view scriptIdentity,
                            int32_t startLine, int32_t endLine, bool restrictToFunction) {
        std::set<int32_t> result;
        if ((source == g_sourceFile) && !restrictToFunction) {
            for (auto i = startLine; i < endLine; i++) {
                result.insert(i);
            }
        }
        return result;
    };
    inspectorServer.OnCallDebuggerGetPossibleBreakpoints(getLinesFalse);

    EXPECT_CALL(server, OnCallMock("Debugger.getPossibleBreakpoints", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            auto res =
                handler(g_sessionId, CreatePossibleBreakpointsRequest(scriptId, start, scriptId + 1, end, false));
            ASSERT_FALSE(res.HasValue());
        });
    inspectorServer.OnCallDebuggerGetPossibleBreakpoints(getLinesFalse);
}

TEST_F(ServerTest, OnCallDebuggerGetPossibleBreakpointsInVaildNumber)
{
    auto scriptId = 0;
    int32_t start = -1;
    int32_t end = -1;

    inspectorServer.CallTargetAttachedToTarget(g_mthread);
    inspectorServer.CallDebuggerPaused({g_mthread, {}, {}, PauseReason::OTHER, DefaultFrameEnumerator});

    EXPECT_CALL(server, OnCallMock("Debugger.getPossibleBreakpoints", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            auto res = handler(g_sessionId, CreatePossibleBreakpointsRequest(scriptId, start, scriptId, end, false));
            ASSERT_FALSE(res.HasValue());
        });
    inspectorServer.OnCallDebuggerGetPossibleBreakpoints(GetLinesFalse);
}

TEST_F(ServerTest, OnCallDebuggerGetScriptSource)
{
    auto scriptId = 0;

    EXPECT_CALL(server, CallMock(g_sessionId, "Debugger.scriptParsed", testing::_))
        .WillOnce([&](testing::Unused, testing::Unused, auto s) {});

    EXPECT_CALL(server, CallMock(g_sessionId, "Debugger.paused", testing::_))
        .WillOnce([&](testing::Unused, testing::Unused, auto s) { ToObject(std::move(s)); });

    inspectorServer.CallTargetAttachedToTarget(g_mthread);
    inspectorServer.CallDebuggerPaused({g_mthread, {}, {}, PauseReason::OTHER, DefaultFrameEnumerator});

    EXPECT_CALL(server, OnCallMock("Debugger.getScriptSource", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObjectBuilder params;
            params.AddProperty("scriptId", std::to_string(scriptId));
            auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
            ResultHolder result;
            GetResult(std::move(res), result);
            ASSERT_THAT(*result, JsonProperties(JsonProperty<JsonObject::StringT> {"scriptSource", g_sourceFile}));
        });
    inspectorServer.OnCallDebuggerGetScriptSource([](auto source, [[maybe_unused]] auto scriptIdentity) {
        std::string s(source);
        return s;
    });

    EXPECT_CALL(server, OnCallMock("Debugger.getScriptSource", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObject empty;
            auto res = handler(g_sessionId, empty);
            ASSERT_FALSE(res.HasValue());
        });
    inspectorServer.OnCallDebuggerGetScriptSource([](auto, auto) { return "a"; });
}

TEST_F(ServerTest, OnCallDebuggerGetScriptSourceUsesPandaFileIdentity)
{
    constexpr std::string_view SOURCE_FILE = "main.ets";
    constexpr std::string_view FIRST_IDENTITY = "modules/first.abc";
    constexpr std::string_view SECOND_IDENTITY = "modules/second.abc";

    auto first = inspectorServer.GetSourceManager().GetScriptId(SOURCE_FILE, FIRST_IDENTITY);
    auto second = inspectorServer.GetSourceManager().GetScriptId(SOURCE_FILE, SECOND_IDENTITY);
    ASSERT_NE(first.first, second.first);

    EXPECT_CALL(server, OnCallMock("Debugger.getScriptSource", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObjectBuilder params;
            params.AddProperty("scriptId", std::to_string(second.first));
            auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
            ResultHolder result;
            GetResult(std::move(res), result);
            auto expected =
                JsonProperties(JsonProperty<JsonObject::StringT> {"scriptSource", std::string(SECOND_IDENTITY)});
            ASSERT_THAT(*result, expected);
        });

    inspectorServer.OnCallDebuggerGetScriptSource([SOURCE_FILE](auto sourceFile, auto scriptIdentity) {
        EXPECT_EQ(sourceFile, SOURCE_FILE);
        return std::string(scriptIdentity);
    });
}

TEST_F(ServerTest, OnCallDebuggerStepOut)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.stepOut", testing::_)).WillOnce(g_simpleHandler);

    inspectorServer.OnCallDebuggerStepOut([](PtThread thread) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallDebuggerStepInto)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.stepInto", testing::_)).WillOnce(g_simpleHandler);

    inspectorServer.OnCallDebuggerStepInto([](PtThread thread) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallDebuggerStepOver)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.stepOver", testing::_)).WillOnce(g_simpleHandler);

    inspectorServer.OnCallDebuggerStepOver([](PtThread thread) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        g_handlerCalled = true;
    });
}

void handlerForRemoveBreakpointsByUrl([[maybe_unused]] PtThread thread, [[maybe_unused]] const char *url,
                                      [[maybe_unused]] SourceFileFilter sourceFileFilter)
{
    return;
}

class RequestLocation : public JsonSerializable {
public:
    explicit RequestLocation(std::string url, int32_t lineNumber) : url_(url), lineNumber_(lineNumber) {}

    void Serialize(JsonObjectBuilder &builder) const override
    {
        builder.AddProperty("url", url_);
        builder.AddProperty("lineNumber", lineNumber_);
    }

private:
    std::string url_;
    int32_t lineNumber_;
};

std::set<int32_t> GetLinesTrue(std::string_view source, [[maybe_unused]] std::string_view scriptIdentity,
                               int32_t startLine, int32_t endLine, bool restrictToFunction)
{
    std::set<int32_t> result;
    if ((source == g_sourceFile) && restrictToFunction) {
        for (auto line = startLine; line < endLine; line++) {
            result.insert(line);
        }
    }
    return result;
}

std::set<int32_t> GetLinesFalse(std::string_view source, [[maybe_unused]] std::string_view scriptIdentity,
                                int32_t startLine, int32_t endLine, bool restrictToFunction)
{
    std::set<int32_t> result;
    if ((source == g_sourceFile) && !restrictToFunction) {
        for (auto line = startLine; line <= endLine; line++) {
            result.insert(line);
        }
    }
    return result;
}

TEST_F(ServerTest, OnCallDebuggerRemoveBreakpointsByUrl)
{
    auto scriptId = 0;
    int32_t start = 5;
    int32_t end = 5;
    int32_t start1 = 5;
    int32_t start2 = 6;
    inspectorServer.CallTargetAttachedToTarget(g_mthread);
    EXPECT_CALL(server, OnCallMock("Debugger.getPossibleAndSetBreakpointByUrl", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            std::vector<RequestLocation> requestLocations = {RequestLocation("file://source1", start1),
                                                             RequestLocation("file://source2", start2)};
            JsonObjectBuilder builder;
            builder.AddProperty("locations", [&](JsonArrayBuilder &locations) {
                for (const auto &loc : requestLocations) {
                    locations.Add(loc);
                }
            });
            handler(g_sessionId, JsonObject(std::move(builder).Build()));
        });
    inspectorServer.OnCallDebuggerGetPossibleAndSetBreakpointByUrl(handlerForSetBreak);
    EXPECT_CALL(server, OnCallMock("Debugger.removeBreakpointsByUrl", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObjectBuilder params;
            params.AddProperty("url", "file://source2");
            handler(g_sessionId, JsonObject(std::move(params).Build()));
        });
    inspectorServer.OnCallDebuggerRemoveBreakpointsByUrl(handlerForRemoveBreakpointsByUrl);
    EXPECT_CALL(server, OnCallMock("Debugger.getPossibleBreakpoints", testing::_))
        .WillOnce(std::bind(g_getPossibleBreakpointsHandler, scriptId, start, end, true, _1, _2));
    inspectorServer.OnCallDebuggerGetPossibleBreakpoints(GetLinesTrue);
}

TEST_F(ServerTest, OnCallDebuggerSetBreakpoint)
{
    auto scriptId = 0;
    int32_t start = 5;

    inspectorServer.CallTargetAttachedToTarget(g_mthread);
    inspectorServer.CallDebuggerPaused({g_mthread, {}, {}, PauseReason::OTHER, DefaultFrameEnumerator});

    EXPECT_CALL(server, OnCallMock("Debugger.setBreakpoint", testing::_)).WillOnce([&](testing::Unused, auto handler) {
        JsonObjectBuilder params;
        params.AddProperty("location", Location(scriptId, start));
        auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
        ResultHolder result;
        GetResult(std::move(res), result);
        ASSERT_THAT(*result,
                    JsonProperties(JsonProperty<JsonObject::StringT> {"breakpointId", std::to_string(start)},
                                   JsonProperty<JsonObject::JsonObjPointer> {
                                       "actualLocation",
                                       testing::Pointee(JsonProperties(
                                           JsonProperty<JsonObject::StringT> {"scriptId", std::to_string(scriptId)},
                                           JsonProperty<JsonObject::NumT> {"lineNumber", start - 1}))}));
    });

    inspectorServer.OnCallDebuggerSetBreakpoint(handlerForSetBreak);

    EXPECT_CALL(server, OnCallMock("Debugger.setBreakpoint", testing::_)).WillOnce([&](testing::Unused, auto handler) {
        JsonObject empty;
        auto res = handler(g_sessionId, empty);
        ASSERT_FALSE(res.HasValue());
    });

    inspectorServer.OnCallDebuggerSetBreakpoint(handlerForSetBreak);

    EXPECT_CALL(server, OnCallMock("Debugger.setBreakpoint", testing::_)).WillOnce([&](testing::Unused, auto handler) {
        JsonObjectBuilder params;
        params.AddProperty("location", Location(scriptId, start));
        auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
        ASSERT_FALSE(res.HasValue());
    });

    inspectorServer.OnCallDebuggerSetBreakpoint(handlerForSetBreakEmpty);
}

TEST_F(ServerTest, OnCallDebuggerSetBreakpointByUrl)
{
    auto scriptId = 0;
    int32_t start = 5;

    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.setBreakpointByUrl", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObjectBuilder params;
            params.AddProperty("lineNumber", start);
            auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
            ASSERT_FALSE(res.HasValue());
        });

    inspectorServer.OnCallDebuggerSetBreakpointByUrl(handlerForSetBreak);

    EXPECT_CALL(server, OnCallMock("Debugger.setBreakpointByUrl", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObjectBuilder params;
            params.AddProperty("lineNumber", start);
            params.AddProperty("url", "file://source");
            auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
            ResultHolder result;
            GetResult(std::move(res), result);

            std::vector<testing::Matcher<JsonObject::JsonObjPointer>> locations;
            locations.push_back(testing::Pointee(
                JsonProperties(JsonProperty<JsonObject::StringT> {"scriptId", std::to_string(scriptId)},
                               JsonProperty<JsonObject::NumT> {"lineNumber", start})));
            auto expected =
                JsonProperties(JsonProperty<JsonObject::StringT> {"breakpointId", std::to_string(start + 1)},
                               JsonProperty<JsonObject::ArrayT> {"locations", JsonElementsAreArray(locations)});

            ASSERT_THAT(*result, expected);
        });

    inspectorServer.OnCallDebuggerSetBreakpointByUrl(handlerForSetBreak);
}

TEST_F(ServerTest, OnCallDebuggerSetBreakpointsActive)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.setBreakpointsActive", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObject empty;
            auto res = handler(g_sessionId, empty);
            ASSERT_FALSE(res.HasValue());
            ASSERT_FALSE(g_handlerCalled);
        });

    inspectorServer.OnCallDebuggerSetBreakpointsActive([](auto thread, auto value) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        ASSERT_FALSE(value);
        g_handlerCalled = true;
    });

    EXPECT_CALL(server, OnCallMock("Debugger.setBreakpointsActive", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObjectBuilder params;
            params.AddProperty("active", true);
            auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
            ResultHolder result;
            GetResult(std::move(res), result);
            ASSERT_THAT(*result, JsonProperties());
            ASSERT_TRUE(g_handlerCalled);
            g_handlerCalled = false;
        });

    inspectorServer.OnCallDebuggerSetBreakpointsActive([](auto thread, auto value) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        ASSERT_TRUE(value);
        g_handlerCalled = true;
    });

    EXPECT_CALL(server, OnCallMock("Debugger.setBreakpointsActive", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObjectBuilder params;
            params.AddProperty("active", false);
            auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
            ResultHolder result;
            GetResult(std::move(res), result);
            ASSERT_THAT(*result, JsonProperties());
            ASSERT_TRUE(g_handlerCalled);
        });

    inspectorServer.OnCallDebuggerSetBreakpointsActive([](auto thread, auto value) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        ASSERT_FALSE(value);
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallDebuggerSetPauseOnExceptions)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.setPauseOnExceptions", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObjectBuilder params;
            params.AddProperty("state", "none");
            auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
            ResultHolder result;
            GetResult(std::move(res), result);
            ASSERT_THAT(*result, JsonProperties());
            ASSERT_TRUE(g_handlerCalled);
        });

    inspectorServer.OnCallDebuggerSetPauseOnExceptions([](PtThread thread, PauseOnExceptionsState state) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        ASSERT_EQ(PauseOnExceptionsState::NONE, state);
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallDebuggerDisable)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.disable", testing::_)).WillOnce(g_simpleHandler);

    inspectorServer.OnCallDebuggerDisable([](PtThread thread) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallDebuggerClientDisconnect)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.clientDisconnect", testing::_)).WillOnce(g_simpleHandler);

    inspectorServer.OnCallDebuggerClientDisconnect([](PtThread thread) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallDebuggerSetAsyncCallStackDepth)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.setAsyncCallStackDepth", testing::_))
        .WillOnce([](testing::Unused, auto handler) {
            JsonObjectBuilder params;
            params.AddProperty("maxDepth", ASYNC_CALL_STACK_DEPTH);
            auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
            ResultHolder result;
            GetResult(std::move(res), result);
            ASSERT_THAT(*result, JsonProperties());
        });

    inspectorServer.OnCallDebuggerSetAsyncCallStackDepth([](PtThread thread, uint32_t maxDepth) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        ASSERT_EQ(maxDepth, ASYNC_CALL_STACK_DEPTH);
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallDebuggerSetAsyncCallStackDepthAcceptsZero)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.setAsyncCallStackDepth", testing::_))
        .WillOnce([](testing::Unused, auto handler) {
            JsonObjectBuilder params;
            params.AddProperty("maxDepth", 0);
            auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
            ResultHolder result;
            GetResult(std::move(res), result);
            ASSERT_THAT(*result, JsonProperties());
        });

    inspectorServer.OnCallDebuggerSetAsyncCallStackDepth([](PtThread thread, uint32_t maxDepth) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        ASSERT_EQ(maxDepth, 0U);
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallDebuggerSetAsyncCallStackDepthRejectsInvalidParams)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.setAsyncCallStackDepth", testing::_))
        .WillOnce([](testing::Unused, auto handler) {
            JsonObject empty;
            auto res = handler(g_sessionId, empty);
            ASSERT_FALSE(res.HasValue());
            ASSERT_FALSE(g_handlerCalled);

            std::vector<JsonObject::NumT> invalidDepths = {-1, 1.5, 4294967296.0};
            for (auto maxDepth : invalidDepths) {
                JsonObjectBuilder params;
                params.AddProperty("maxDepth", maxDepth);
                res = handler(g_sessionId, JsonObject(std::move(params).Build()));
                ASSERT_FALSE(res.HasValue());
                ASSERT_FALSE(g_handlerCalled);
            }
        });

    inspectorServer.OnCallDebuggerSetAsyncCallStackDepth(
        []([[maybe_unused]] PtThread thread, [[maybe_unused]] uint32_t maxDepth) { g_handlerCalled = true; });
}

TEST_F(ServerTest, OnCallDebuggerSetBlackboxPatterns)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.setBlackboxPatterns", testing::_)).WillOnce(g_simpleHandler);

    inspectorServer.OnCallDebuggerSetBlackboxPatterns([](PtThread thread) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallDebuggerSmartStepInto)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.smartStepInto", testing::_)).WillOnce(g_simpleHandler);

    inspectorServer.OnCallDebuggerSmartStepInto([](PtThread thread) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallDebuggerDropFrame)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.dropFrame", testing::_)).WillOnce(g_simpleHandler);

    inspectorServer.OnCallDebuggerDropFrame([](PtThread thread) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallDebuggerSetNativeRange)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.setNativeRange", testing::_)).WillOnce(g_simpleHandler);

    inspectorServer.OnCallDebuggerSetNativeRange([](PtThread thread) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallDebuggerReplyNativeMethodCall)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.replyNativeCalling", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObjectBuilder params;
            params.AddProperty("userCode", false);
            auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
            ResultHolder result;
            GetResult(std::move(res), result);
            ASSERT_THAT(*result, JsonProperties());
            ASSERT_TRUE(g_handlerCalled);
        });

    inspectorServer.OnCallDebuggerReplyNativeMethodCall([](PtThread thread, bool userCode) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        ASSERT_FALSE(userCode);
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallDebuggerContinueToLocation)
{
    auto scriptId = 0;
    int32_t start = 5;

    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.continueToLocation", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObjectBuilder params;
            params.AddProperty("location", Location(scriptId, start));
            handler(g_sessionId, JsonObject(std::move(params).Build()));
        });

    inspectorServer.OnCallDebuggerContinueToLocation([](PtThread thread, std::string_view, std::string_view, int32_t) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallDebuggerSetSkipAllPauses)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.setSkipAllPauses", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObjectBuilder params;
            params.AddProperty("skip", true);
            handler(g_sessionId, JsonObject(std::move(params).Build()));
        });

    inspectorServer.OnCallDebuggerSetSkipAllPauses([](PtThread thread, bool) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallDebuggerSetMixedDebugEnabled)
{
    EXPECT_CALL(server, OnCallMock("Debugger.setMixedDebugEnabled", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObjectBuilder params;
            params.AddProperty("enabled", true);
            handler(g_sessionId, JsonObject(std::move(params).Build()));
        });

    inspectorServer.OnCallDebuggerSetMixedDebugEnabled([](bool mixedDebugEnabled) {
        ASSERT_TRUE(mixedDebugEnabled);
        g_handlerCalled = true;
    });
}

Expected<EvaluationResult, std::string> handlerForEvaluateFailed([[maybe_unused]] PtThread thread,
                                                                 [[maybe_unused]] const std::string &bytecodeBase64,
                                                                 [[maybe_unused]] size_t frameNumber)
{
    return Unexpected(std::string("evaluate failed"));
}

Expected<EvaluationResult, std::string> handlerForEvaluate([[maybe_unused]] PtThread thread,
                                                           [[maybe_unused]] const std::string &bytecodeBase64,
                                                           [[maybe_unused]] size_t frameNumber)
{
    return Unexpected(std::string("evaluation failed"));
}

TEST_F(ServerTest, OnCallDebuggerEvaluateOnCallFrame)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.evaluateOnCallFrame", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObjectBuilder params;
            params.AddProperty("callFrameId", -1);
            params.AddProperty("expression", "any expression");

            auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
            ASSERT_FALSE(res.HasValue());
        });

    inspectorServer.OnCallDebuggerEvaluateOnCallFrame(handlerForEvaluateFailed);
}

TEST_F(ServerTest, OnCallDebuggerCallFunctionOn)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.callFunctionOn", testing::_)).WillOnce([&](testing::Unused, auto handler) {
        JsonObjectBuilder params;
        params.AddProperty("callFrameId", -1);
        params.AddProperty("functionDeclaration", "any functionDeclaration");

        auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
        ASSERT_FALSE(res.HasValue());
    });

    inspectorServer.OnCallDebuggerCallFunctionOn(handlerForEvaluateFailed);
}

TEST_F(ServerTest, OnCallDebuggerGetPossibleAndSetBreakpointByUrl)
{
    auto scriptId = 0;
    int32_t start1 = 5;
    int32_t start2 = 6;

    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Debugger.getPossibleAndSetBreakpointByUrl", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            class RequestLocation : public JsonSerializable {
            public:
                explicit RequestLocation(std::string url, int32_t lineNumber) : url_(url), lineNumber_(lineNumber) {}

                void Serialize(JsonObjectBuilder &builder) const override
                {
                    builder.AddProperty("url", url_);
                    builder.AddProperty("lineNumber", lineNumber_);
                }

            private:
                std::string url_;
                int32_t lineNumber_;
            };
            std::vector<RequestLocation> requestLocations = {RequestLocation("file://source", start1),
                                                             RequestLocation("file://source", start2)};
            JsonObjectBuilder builder;
            builder.AddProperty("locations", [&](JsonArrayBuilder &locations) {
                for (const auto &loc : requestLocations) {
                    locations.Add(loc);
                }
            });

            auto res = handler(g_sessionId, JsonObject(std::move(builder).Build()));
            ResultHolder result;
            GetResult(std::move(res), result);

            std::vector<testing::Matcher<JsonObject::JsonObjPointer>> locations;
            locations.push_back(testing::Pointee(JsonProperties(JsonProperty<JsonObject::NumT> {"scriptId", scriptId},
                                                                JsonProperty<JsonObject::NumT> {"lineNumber", start1},
                                                                JsonProperty<JsonObject::NumT> {"columnNumber", 0},
                                                                JsonProperty<JsonObject::StringT> {"id", "6"})));
            locations.push_back(testing::Pointee(JsonProperties(JsonProperty<JsonObject::NumT> {"scriptId", scriptId},
                                                                JsonProperty<JsonObject::NumT> {"lineNumber", start2},
                                                                JsonProperty<JsonObject::NumT> {"columnNumber", 0},
                                                                JsonProperty<JsonObject::StringT> {"id", "7"})));
            auto expected =
                JsonProperties(JsonProperty<JsonObject::ArrayT> {"locations", JsonElementsAreArray(locations)});

            ASSERT_THAT(*result, expected);
        });

    inspectorServer.OnCallDebuggerGetPossibleAndSetBreakpointByUrl(handlerForSetBreak);
}

TEST_F(ServerTest, OnCallRuntimeEnable)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Runtime.enable", testing::_)).WillOnce([&](testing::Unused, auto handler) {
        JsonObject empty;
        auto res = handler(g_sessionId, empty);
        ResultHolder result;
        GetResult(std::move(res), result);
        ASSERT_THAT(*result, JsonProperties());
        ASSERT_TRUE(g_handlerCalled);
    });
    inspectorServer.OnCallRuntimeEnable([](PtThread thread) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        g_handlerCalled = true;
    });
}

TEST_F(ServerTest, OnCallRuntimeGetProperties)
{
    auto object_id = 6;
    auto preview = true;

    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Runtime.getProperties", testing::_)).WillOnce([&](testing::Unused, auto handler) {
        JsonObjectBuilder params;
        params.AddProperty("objectId", std::to_string(object_id));
        params.AddProperty("generatePreview", preview);
        auto res = handler(g_sessionId, JsonObject(std::move(params).Build()));
        ResultHolder result;
        GetResult(std::move(res), result);

        std::vector<testing::Matcher<JsonObject::JsonObjPointer>> expected;
        expected.push_back(testing::Pointee(JsonProperties(
            JsonProperty<JsonObject::StringT> {"name", "object"},
            JsonProperty<JsonObject::JsonObjPointer> {
                "value", testing::Pointee(JsonProperties(JsonProperty<JsonObject::NumT> {"value", object_id},
                                                         JsonProperty<JsonObject::StringT> {"type", "number"}))},
            JsonProperty<JsonObject::BoolT> {"writable", testing::_},
            JsonProperty<JsonObject::BoolT> {"configurable", testing::_},
            JsonProperty<JsonObject::BoolT> {"enumerable", testing::_})));
        expected.push_back(testing::Pointee(JsonProperties(
            JsonProperty<JsonObject::StringT> {"name", "preview"},
            JsonProperty<JsonObject::JsonObjPointer> {
                "value", testing::Pointee(JsonProperties(JsonProperty<JsonObject::BoolT> {"value", preview},
                                                         JsonProperty<JsonObject::StringT> {"type", "boolean"}))},
            JsonProperty<JsonObject::BoolT> {"writable", testing::_},
            JsonProperty<JsonObject::BoolT> {"configurable", testing::_},
            JsonProperty<JsonObject::BoolT> {"enumerable", testing::_})));
        expected.push_back(testing::Pointee(JsonProperties(
            JsonProperty<JsonObject::StringT> {"name", "threadId"},
            JsonProperty<JsonObject::JsonObjPointer> {
                "value", testing::Pointee(JsonProperties(JsonProperty<JsonObject::NumT> {"value", g_mthread.GetId()},
                                                         JsonProperty<JsonObject::StringT> {"type", "number"}))},
            JsonProperty<JsonObject::BoolT> {"writable", testing::_},
            JsonProperty<JsonObject::BoolT> {"configurable", testing::_},
            JsonProperty<JsonObject::BoolT> {"enumerable", testing::_})));

        ASSERT_THAT(*result,
                    JsonProperties(JsonProperty<JsonObject::ArrayT> {"result", JsonElementsAreArray(expected)}));
    });

    auto getProperties = [](PtThread thread, RemoteObjectId id, bool need_preview) {
        std::vector<PropertyDescriptor> res;
        res.push_back(PropertyDescriptor("object", RemoteObject::Number(id)));
        res.push_back(PropertyDescriptor("preview", RemoteObject::Boolean(need_preview)));
        res.push_back(PropertyDescriptor("threadId", RemoteObject::Number(thread.GetId())));
        return res;
    };

    inspectorServer.OnCallRuntimeGetProperties(getProperties);
}

TEST_F(ServerTest, OnCallRuntimeRunIfWaitingForDebugger)
{
    inspectorServer.CallTargetAttachedToTarget(g_mthread);

    EXPECT_CALL(server, OnCallMock("Runtime.runIfWaitingForDebugger", testing::_))
        .WillOnce([&](testing::Unused, auto handler) {
            JsonObject empty;
            auto res = handler(g_sessionId, empty);
            ResultHolder result;
            GetResult(std::move(res), result);
            ASSERT_THAT(*result, JsonProperties());
            ASSERT_TRUE(g_handlerCalled);
        });

    inspectorServer.OnCallRuntimeRunIfWaitingForDebugger([](PtThread thread) {
        ASSERT_EQ(thread.GetId(), g_mthread.GetId());
        g_handlerCalled = true;
    });
}

}  // namespace ark::tooling::inspector::test

// NOLINTEND
