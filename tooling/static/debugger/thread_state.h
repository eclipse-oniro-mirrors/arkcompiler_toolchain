/**
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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

#ifndef PANDA_TOOLING_INSPECTOR_THREAD_STATE_H
#define PANDA_TOOLING_INSPECTOR_THREAD_STATE_H

#include <cstdint>
#include <memory>
#include <unordered_set>
#include <vector>

#include "breakpoint.h"
#include "breakpoint_storage.h"
#include "common.h"
#include "debug_info_cache.h"
#include "runtime/tooling/debugger.h"

#include "include/tooling/debug_interface.h"
#include "types/numeric_id.h"
#include "types/pause_on_exceptions_state.h"

namespace ark::tooling::inspector {

// Sentinel line number values used by OnSingleStep.
// OnSingleStep never pauses on a location whose line number is not positive;
// both sentinels below are covered by that early return.
// SKIP_LINE_NUMBER is equivalent to INVALID_LINE_NO in ets_frontend/ets2panda/compiler/core/emitter.cpp.
// UNKNOWN_LINE_NUMBER is equivalent to a LINE_NUMBER that has not been computed.
constexpr int32_t SKIP_LINE_NUMBER = -1;
constexpr int32_t UNKNOWN_LINE_NUMBER = 0;

class DebuggableThread;

class ThreadState final {
public:
    explicit ThreadState(EvaluationEngine &engine, BreakpointStorage &bpStorage)
        : engine_(engine), bpStorage_(bpStorage)
    {
    }
    ~ThreadState() = default;

    NO_COPY_SEMANTIC(ThreadState);
    NO_MOVE_SEMANTIC(ThreadState);

    bool IsPaused() const
    {
        return paused_;
    }

    std::vector<BreakpointId> GetBreakpointsByLocation(const PtLocation &location) const;

    void Reset();
    void BreakOnStart();
    void Continue();
    void ContinueTo(std::unordered_set<PtLocation, HashLocation> locations);
    void StepInto(std::unordered_set<PtLocation, HashLocation> locations);
    void StepOver(std::unordered_set<PtLocation, HashLocation> locations);
    void StepOut();
    void Pause();
    void SetSkipAllPauses(bool skip);
    void SetPauseOnExceptions(PauseOnExceptionsState state);

    void OnException(bool uncaught);
    void OnFramePop();
    bool OnMethodEntry();
    void OnSingleStep(const PtLocation &location);

    PauseReason GetPauseReason() const
    {
        return pauseReason_;
    }

    bool IsStepInto() const
    {
        return stepKind_ == StepKind::STEP_INTO;
    }

private:
    enum class StepKind {
        // Just continue execution
        NONE,

        // Stop on the next step event. It is not reset to NONE during resetting the state on a new connection
        BREAK_ON_START,

        // Continue execution until one of the specified locations is reached
        CONTINUE_TO,

        // Stop on the next step event
        PAUSE,

        // Continue execution until one of the locations, other than the current line, is reached
        STEP_INTO,

        // Continue execution until the current frame is popped
        STEP_OUT,

        // Continue execution until both:
        // - The call frame is not a child to the starting position (e.g. we have not entered some method);
        // - One of the locations, other than the current line, is reached.
        STEP_OVER
    };

private:
    bool ShouldStopAtBreakpoint(const PtLocation &location);

    // Helpers for OnSingleStep. Each returns true when it has fully decided the pause
    // state for this step and the caller should return immediately.
    bool HandleBreakOnStart(const PtLocation &location);
    bool HandleDynamicToStatic(int32_t lineNumber);
    bool HandleBreakpointHit(const PtLocation &location);
    void HandleStep(const PtLocation &location);
    void UpdateLineNumber(int32_t lineNumber);

private:
    StepKind stepKind_ {StepKind::NONE};

    // The set of locations has different semantics for various kinds of stepping:
    // - for CONTINUE_TO it contains the set of locations we should reach to end the step;
    // - for STEP_INTO and STEP_OVER it contains the set of locations corresponding to the current line
    //   (then, the locations we should leave to end the step).
    std::unordered_set<PtLocation, HashLocation> stepLocations_;

    bool methodEntered_ {false};

    EvaluationEngine &engine_;
    BreakpointStorage &bpStorage_;

    PauseOnExceptionsState pauseOnExceptionsState_ {PauseOnExceptionsState::NONE};

    bool paused_ {false};
    bool skipAllPauses_ {false};
    bool breakOnStart_ {false};
    PauseReason pauseReason_ {PauseReason::OTHER};

    // Source line number seen on the most recent single step. Used by STEP_INTO to
    // avoid pausing again when the new location is still on the same source line.
    int32_t lastLineNumber_ {0};
};
}  // namespace ark::tooling::inspector

#endif  // PANDA_TOOLING_INSPECTOR_THREAD_STATE_H
