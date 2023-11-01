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

#ifndef ECMASCRIPT_TOOLING_AGENT_TRACING_IMPL_H
#define ECMASCRIPT_TOOLING_AGENT_TRACING_IMPL_H

#include <uv.h>
#ifdef ERROR
#undef ERROR
#endif
#ifdef CONST
#undef CONST
#endif
#ifdef VOID
#undef VOID
#endif
#ifdef GetObject
#undef GetObject
#endif

#include "tooling/base/pt_params.h"
#include "tooling/base/pt_returns.h"
#include "dispatcher.h"

#include "ecmascript/dfx/cpu_profiler/samples_record.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::tooling {
class TracingImpl final {
public:
    explicit TracingImpl(const EcmaVM *vm, ProtocolChannel *channel) : vm_(vm), frontend_(channel) {}
    ~TracingImpl() = default;

    std::unique_ptr<ProfileInfo> End();
    DispatchResponse GetCategories(std::vector<std::string> categories);
    DispatchResponse RecordClockSyncMarker(std::string syncId);
    DispatchResponse RequestMemoryDump(std::unique_ptr<RequestMemoryDumpParams> params,
                                       std::string dumpGuid,  bool success);
    DispatchResponse Start(std::unique_ptr<StartParams> params);
    static void TracingBufferUsageReport(uv_timer_t* handle);

    class DispatcherImpl final : public DispatcherBase {
    public:
        DispatcherImpl(ProtocolChannel *channel, std::unique_ptr<TracingImpl> tracing)
            : DispatcherBase(channel), tracing_(std::move(tracing)) {}
        ~DispatcherImpl() override = default;
        void Dispatch(const DispatchRequest &request) override;
        void End(const DispatchRequest &request);
        void GetCategories(const DispatchRequest &request);
        void RecordClockSyncMarker(const DispatchRequest &request);
        void RequestMemoryDump(const DispatchRequest &request);
        void Start(const DispatchRequest &request);

    private:
        NO_COPY_SEMANTIC(DispatcherImpl);
        NO_MOVE_SEMANTIC(DispatcherImpl);

        using AgentHandler = void (TracingImpl::DispatcherImpl::*)(const DispatchRequest &request);
        std::unique_ptr<TracingImpl> tracing_ {};
    };

    class Frontend {
    public:
        explicit Frontend(ProtocolChannel *channel) : channel_(channel) {}
        ~Frontend() = default;

        void BufferUsage(double percentFull, int32_t eventCount, double value);
        void DataCollected(std::unique_ptr<ProfileInfo> cpuProfileInfo);
        void TracingComplete();

    private:
        bool AllowNotify() const;
        ProtocolChannel *channel_ {nullptr};
    };

private:
    NO_COPY_SEMANTIC(TracingImpl);
    NO_MOVE_SEMANTIC(TracingImpl);

    static constexpr uint64_t MAX_BUFFER_SIZE_DEFAULT = 200 * 1024 * 1024;
    const EcmaVM *vm_ {nullptr};
    Frontend frontend_;
    uv_timer_t handle_ {};
    uint64_t maxBufferSize_ {MAX_BUFFER_SIZE_DEFAULT};
};
}  // namespace panda::ecmascript::tooling
#endif