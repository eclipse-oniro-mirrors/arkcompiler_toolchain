/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "tooling/base/pt_events.h"

namespace panda::ecmascript::tooling {
std::unique_ptr<PtJson> BreakpointResolved::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();
    result->Add("breakpointId", breakpointId_.c_str());
    ASSERT(location_ != nullptr);
    result->Add("location", location_->ToJson());

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> Paused::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    size_t len = callFrames_.size();
    for (size_t i = 0; i < len; i++) {
        ASSERT(callFrames_[i] != nullptr);
        array->Push(callFrames_[i]->ToJson());
    }
    result->Add("callFrames", array);
    result->Add("reason", reason_.c_str());
    if (data_) {
        ASSERT(data_.value() != nullptr);
        result->Add("data", data_.value()->ToJson());
    }
    if (hitBreakpoints_) {
        std::unique_ptr<PtJson> breakpoints = PtJson::CreateArray();
        len = hitBreakpoints_->size();
        for (size_t i = 0; i < len; i++) {
            breakpoints->Push(hitBreakpoints_.value()[i].c_str());
        }
        result->Add("hitBreakpoints", breakpoints);
    }

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> Resumed::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> NativeCalling::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("nativeAddress", reinterpret_cast<int64_t>(GetNativeAddress()));
    result->Add("isStepInto", GetIntoStatus());

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> MixedStack::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    std::unique_ptr<PtJson> nativePointerArray = PtJson::CreateArray();
    size_t nativePointerLength = nativePointer_.size();
    for (size_t i = 0; i < nativePointerLength; ++i) {
        nativePointerArray->Push(reinterpret_cast<int64_t>(nativePointer_[i]));
    }
    result->Add("nativePointer", nativePointerArray);

    std::unique_ptr<PtJson> callFrameArray = PtJson::CreateArray();
    size_t callFrameLength = callFrames_.size();
    for (size_t i = 0; i < callFrameLength; ++i) {
        ASSERT(callFrames_[i] != nullptr);
        callFrameArray->Push(callFrames_[i]->ToJson());
    }
    result->Add("callFrames", callFrameArray);

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> ScriptFailedToParse::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("scriptId", std::to_string(scriptId_).c_str());
    result->Add("url", url_.c_str());
    result->Add("startLine", startLine_);
    result->Add("startColumn", startColumn_);
    result->Add("endLine", endLine_);
    result->Add("endColumn", endColumn_);
    result->Add("executionContextId", executionContextId_);
    result->Add("hash", hash_.c_str());
    if (sourceMapUrl_) {
        result->Add("sourceMapURL", sourceMapUrl_->c_str());
    }
    if (hasSourceUrl_) {
        result->Add("hasSourceURL", hasSourceUrl_.value());
    }
    if (isModule_) {
        result->Add("isModule", isModule_.value());
    }
    if (length_) {
        result->Add("length", length_.value());
    }
    if (codeOffset_) {
        result->Add("codeOffset", codeOffset_.value());
    }
    if (scriptLanguage_) {
        result->Add("scriptLanguage", scriptLanguage_->c_str());
    }
    if (embedderName_) {
        result->Add("embedderName", embedderName_->c_str());
    }

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> ScriptParsed::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("scriptId", std::to_string(scriptId_).c_str());
    result->Add("url", url_.c_str());
    result->Add("startLine", startLine_);
    result->Add("startColumn", startColumn_);
    result->Add("endLine", endLine_);
    result->Add("endColumn", endColumn_);
    result->Add("executionContextId", executionContextId_);
    result->Add("hash", hash_.c_str());
    if (isLiveEdit_) {
        result->Add("isLiveEdit", isLiveEdit_.value());
    }
    if (sourceMapUrl_) {
        result->Add("sourceMapURL", sourceMapUrl_->c_str());
    }
    if (hasSourceUrl_) {
        result->Add("hasSourceURL", hasSourceUrl_.value());
    }
    if (isModule_) {
        result->Add("isModule", isModule_.value());
    }
    if (length_) {
        result->Add("length", length_.value());
    }
    if (codeOffset_) {
        result->Add("codeOffset", codeOffset_.value());
    }
    if (scriptLanguage_) {
        result->Add("scriptLanguage", scriptLanguage_->c_str());
    }
    if (embedderName_) {
        result->Add("embedderName", embedderName_->c_str());
    }

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> AddHeapSnapshotChunk::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("chunk", chunk_.c_str());

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> ConsoleProfileFinished::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("id", id_.c_str());
    ASSERT(location_ != nullptr);
    result->Add("location", location_->ToJson());
    ASSERT(profile_ != nullptr);
    result->Add("profile", profile_->ToJson());
    if (title_) {
        result->Add("title", title_->c_str());
    }

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> ConsoleProfileStarted::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("id", id_.c_str());
    ASSERT(location_ != nullptr);
    result->Add("location", location_->ToJson());
    if (title_) {
        result->Add("title", title_->c_str());
    }

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> PreciseCoverageDeltaUpdate::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("timestamp", timestamp_);
    result->Add("occasion", occasion_.c_str());
    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    size_t len = result_.size();
    for (size_t i = 0; i < len; i++) {
        ASSERT(result_[i] != nullptr);
        std::unique_ptr<PtJson> res = result_[i]->ToJson();
        array->Push(res);
    }
    result->Add("result", array);

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> HeapStatsUpdate::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    size_t len = statsUpdate_.size();
    for (size_t i = 0; i < len; i++) {
        array->Push(statsUpdate_[i]);
    }
    result->Add("statsUpdate", array);

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> LastSeenObjectId::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("lastSeenObjectId", lastSeenObjectId_);
    result->Add("timestamp", timestamp_);

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> ReportHeapSnapshotProgress::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("done", done_);
    result->Add("total", total_);
    if (finished_) {
        result->Add("finished", finished_.value());
    }

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> BufferUsage::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    if (percentFull_) {
        result->Add("percentFull", percentFull_.value());
    }
    if (eventCount_) {
        result->Add("eventCount", eventCount_.value());
    }
    if (value_) {
        result->Add("value", value_.value());
    }

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> DataCollected::ToJson() const
{
    std::unique_ptr<PtJson> traceEvents = PtJson::CreateArray();
    std::unique_ptr<PtJson> metadata = PtJson::CreateObject();

    CpuProfileToJson(traceEvents.get(), metadata.get());

    std::unique_ptr<PtJson> value = PtJson::CreateObject();
    value->Add("value", traceEvents);

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", value);

    return object;
}

std::unique_ptr<PtJson> TraceEvent::ToJson() const
{
    std::unique_ptr<PtJson> event = PtJson::CreateObject();
    event->Add("cat", cat_.c_str());
    event->Add("name", name_.c_str());
    event->Add("ph", ph_.c_str());
    event->Add("pid", pid_);
    event->Add("tid", tid_);
    event->Add("ts", ts_);

    if (dur_.has_value()) {
        event->Add("dur", dur_.value());
    }

    if (args_ != nullptr) {
        event->Add("args", args_);
    } else {
        event->Add("args", std::move(PtJson::CreateObject()));
    }

    return event;
}

void DataCollected::CpuProfileToJson(PtJson *traceEvents, [[maybe_unused]] PtJson *metadata) const
{
    // timeline : TracingStartedInPage
    std::unique_ptr<PtJson> pageArgsData = PtJson::CreateObject();
    pageArgsData->Add("sessionId", "1");
    std::unique_ptr<PtJson> pageArgs = PtJson::CreateObject();
    pageArgs->Add("data", pageArgsData);
    TraceEvent page("disabled-by-default-devtools.timeline", "TracingStartedInPage", "M", 1, cpuProfile_->GetTid());
    page.SetArgs(std::move(pageArgs));
    traceEvents->Push(page.ToJson());

    // __metadata : thread_name
    std::unique_ptr<PtJson> threadNameArgs = PtJson::CreateObject();
    threadNameArgs->Add("name", ("Thread " + std::to_string(cpuProfile_->GetTid())).c_str());
    TraceEvent threadName("__metadata", "thread_name", "M", 1, cpuProfile_->GetTid());
    threadName.SetArgs(std::move(threadNameArgs));
    traceEvents->Push(threadName.ToJson());

    // toplevel : JSRoot
    TraceEvent toplevel("toplevel", "JSRoot", "X", 1, cpuProfile_->GetTid());
    toplevel.SetTs(cpuProfile_->GetStartTime());
    toplevel.SetDur(cpuProfile_->GetEndTime() - cpuProfile_->GetStartTime());
    traceEvents->Push(toplevel.ToJson());

    // timeline : CpuProfile
    std::unique_ptr<PtJson> cpuProfile = PtJson::CreateObject();
    std::unique_ptr<PtJson> nodes = PtJson::CreateArray();
    auto node = cpuProfile_->GetNodes();
    for (size_t i = 0; i < node->size(); i++) {
        nodes->Push((*node)[i]->ToJson());
    }
    cpuProfile->Add("nodes", nodes);

    cpuProfile->Add("startTime", cpuProfile_->GetStartTime());
    cpuProfile->Add("endTime", cpuProfile_->GetEndTime());

    auto samples = cpuProfile_->GetSamples();
    if (samples) {
        std::unique_ptr<PtJson> samplesData = PtJson::CreateArray();
        for (size_t i = 0; i < samples->size(); i++) {
            samplesData->Push((*samples)[i]);
        }
        cpuProfile->Add("samples", samplesData);
    }

    auto timeDeltas = cpuProfile_->GetTimeDeltas();
    if (timeDeltas) {
        std::unique_ptr<PtJson> timeDeltasData = PtJson::CreateArray();
        for (size_t i = 0; i < timeDeltas->size(); i++) {
            timeDeltasData->Push((*timeDeltas)[i]);
        }
        cpuProfile->Add("timeDeltas", timeDeltasData);
    }

    std::unique_ptr<PtJson> cpuProfileArgsData = PtJson::CreateObject();
    cpuProfileArgsData->Add("cpuProfile", cpuProfile);
    std::unique_ptr<PtJson> cpuProfileArgs = PtJson::CreateObject();
    cpuProfileArgs->Add("data", cpuProfileArgsData);
    TraceEvent cpuProfileEvent("disabled-by-default-devtools.timeline", "CpuProfile", "I", 1, cpuProfile_->GetTid());
    cpuProfileEvent.SetTs(cpuProfile_->GetEndTime());
    cpuProfileEvent.SetArgs(std::move(cpuProfileArgs));
    traceEvents->Push(cpuProfileEvent.ToJson());
}

std::unique_ptr<PtJson> TracingComplete::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("dataLossOccurred", dataLossOccurred_);
    if (traceFormat_) {
        result->Add("traceFormat", traceFormat_.value()->c_str());
    }
    if (streamCompression_) {
        result->Add("streamCompression", streamCompression_.value()->c_str());
    }

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}
}  // namespace panda::ecmascript::tooling
