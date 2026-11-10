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

#include "thread_state.h"

#include <memory>
#include <optional>
#include <sstream>

#include "include/tooling/pt_location.h"
#include "libarkbase/macros.h"
#include "breakpoint.h"
#include "error.h"
#include "types/numeric_id.h"
#include "tooling/hybrid_step/hybrid_single_stepper.h"

namespace ark::tooling::inspector {

void ThreadState::Reset()
{
    stepKind_ = StepKind::BREAK_ON_START;
    stepLocations_.clear();
    methodEntered_ = false;
    skipAllPauses_ = false;
    pauseOnExceptionsState_ = PauseOnExceptionsState::NONE;
    lastLineNumber_ = 0;
}

void ThreadState::BreakOnStart()
{
    if (!paused_) {
        stepKind_ = StepKind::BREAK_ON_START;
    }
    breakOnStart_ = true;
}

void ThreadState::Continue()
{
    stepKind_ = StepKind::NONE;
    paused_ = false;
    pauseReason_ = PauseReason::OTHER;
    // Reset STATIC_TO_DYNAMIC flag to be false when resuming
    HybridSingleStepper::GetInstance().SetHybridSingleStepFlag(HybridStepDirection::STATIC_TO_DYNAMIC, false);
}

void ThreadState::ContinueTo(std::unordered_set<PtLocation, HashLocation> locations)
{
    stepKind_ = StepKind::CONTINUE_TO;
    stepLocations_ = std::move(locations);
    paused_ = false;
    pauseReason_ = PauseReason::OTHER;
    // Reset STATIC_TO_DYNAMIC flag to be false when resuming
    HybridSingleStepper::GetInstance().SetHybridSingleStepFlag(HybridStepDirection::STATIC_TO_DYNAMIC, false);
}

void ThreadState::StepInto(std::unordered_set<PtLocation, HashLocation> locations)
{
    stepKind_ = StepKind::STEP_INTO;
    methodEntered_ = false;
    stepLocations_ = std::move(locations);
    paused_ = false;
    pauseReason_ = PauseReason::STEP;
    // Set STATIC_TO_DYNAMIC flag to be true in order for
    // static side to pause when stepping from static to dynamic
    HybridSingleStepper::GetInstance().SetHybridSingleStepFlag(HybridStepDirection::STATIC_TO_DYNAMIC, true);
}

void ThreadState::StepOver(std::unordered_set<PtLocation, HashLocation> locations)
{
    stepKind_ = StepKind::STEP_OVER;
    methodEntered_ = false;
    stepLocations_ = std::move(locations);
    paused_ = false;
    pauseReason_ = PauseReason::STEP;
}

void ThreadState::StepOut()
{
    stepKind_ = StepKind::STEP_OUT;
    methodEntered_ = true;
    paused_ = false;
    pauseReason_ = PauseReason::STEP;
    // Set STATIC_TO_DYNAMIC flag to be true in order for
    // static side to pause when stepping from static to dynamic
    HybridSingleStepper::GetInstance().SetHybridSingleStepFlag(HybridStepDirection::STATIC_TO_DYNAMIC, true);
}

void ThreadState::Pause()
{
    if (!paused_ && !skipAllPauses_) {
        stepKind_ = StepKind::PAUSE;
    }
}

void ThreadState::SetSkipAllPauses(bool skip)
{
    skipAllPauses_ = skip;
}

void ThreadState::SetPauseOnExceptions(PauseOnExceptionsState state)
{
    pauseOnExceptionsState_ = state;
}

void ThreadState::OnException(bool uncaught)
{
    ASSERT(!paused_);
    if (skipAllPauses_) {
        return;
    }
    switch (pauseOnExceptionsState_) {
        case PauseOnExceptionsState::NONE:
            break;
        case PauseOnExceptionsState::CAUGHT:
            paused_ = !uncaught;
            break;
        case PauseOnExceptionsState::UNCAUGHT:
            paused_ = uncaught;
            break;
        case PauseOnExceptionsState::ALL:
            paused_ = true;
            break;
        default:
            UNREACHABLE();
    }
    if (paused_) {
        pauseReason_ = PauseReason::EXCEPTION;
    }
}

void ThreadState::OnFramePop()
{
    ASSERT(!paused_);
    switch (stepKind_) {
        case StepKind::NONE:
        case StepKind::BREAK_ON_START:
        case StepKind::CONTINUE_TO:
        case StepKind::PAUSE:
        case StepKind::STEP_INTO: {
            break;
        }

        case StepKind::STEP_OUT:
        case StepKind::STEP_OVER: {
            methodEntered_ = false;
            break;
        }

        default: {
            UNREACHABLE();
        }
    }
}

bool ThreadState::OnMethodEntry()
{
    ASSERT(!paused_);
    switch (stepKind_) {
        case StepKind::NONE:
        case StepKind::BREAK_ON_START:
        case StepKind::CONTINUE_TO:
        case StepKind::PAUSE:
        case StepKind::STEP_INTO: {
            return false;
        }

        case StepKind::STEP_OUT:
        case StepKind::STEP_OVER: {
            return !std::exchange(methodEntered_, true);
        }

        default: {
            UNREACHABLE();
        }
    }
}

void ThreadState::UpdateLineNumber(int32_t lineNumber)
{
    // SKIP_LINE_NUMBER (cctor) nerver update
    // dynamic->static->dynamic. After passing through the static cctor function, it returns to the same line.
    if (lineNumber != SKIP_LINE_NUMBER) {
        LOG(INFO, DEBUGGER) << "ThreadState::OnSingleStep lineNumber update from " << lastLineNumber_ << " to "
                            << lineNumber;
        lastLineNumber_ = lineNumber;
    }
}

bool ThreadState::HandleBreakOnStart(const PtLocation &location)
{
    if (ShouldStopAtBreakpoint(location)) {
        paused_ = true;
        UpdateLineNumber(location.GetLineNumber());
        pauseReason_ = PauseReason::OTHER;
        return true;
    }
    if (stepKind_ != StepKind::STEP_INTO) {
        pauseReason_ = PauseReason::BREAK_ON_START;
    }
    stepKind_ = StepKind::BREAK_ON_START;
    paused_ = true;
    UpdateLineNumber(location.GetLineNumber());
    breakOnStart_ = false;
    return true;
}

bool ThreadState::HandleDynamicToStatic(int32_t lineNumber)
{
    if (!HybridSingleStepper::GetInstance().GetHybridSingleStepFlag(HybridStepDirection::DYNAMIC_TO_STATIC)) {
        return false;
    }
    LOG(DEBUG, DEBUGGER) << "ThreadState::OnSingleStep paused by single step from dynamic side";
    // reset DYNAMIC_TO_STATIC to false to avoid redundant pauses
    HybridSingleStepper::GetInstance().SetHybridSingleStepFlag(HybridStepDirection::DYNAMIC_TO_STATIC, false);
    paused_ = true;
    UpdateLineNumber(lineNumber);
    pauseReason_ = PauseReason::OTHER;
    return true;
}

bool ThreadState::HandleBreakpointHit(const PtLocation &location)
{
    if (!ShouldStopAtBreakpoint(location)) {
        return false;
    }
    paused_ = true;
    UpdateLineNumber(location.GetLineNumber());
    return true;
}

void ThreadState::HandleStep(const PtLocation &location)
{
    int32_t lineNumber = location.GetLineNumber();
    switch (stepKind_) {
        case StepKind::NONE:
            paused_ = false;
            break;
        case StepKind::BREAK_ON_START:
            break;
        case StepKind::CONTINUE_TO:
            paused_ = stepLocations_.find(location) != stepLocations_.end();
            break;
        case StepKind::PAUSE:
            paused_ = true;
            UpdateLineNumber(lineNumber);
            break;
        case StepKind::STEP_INTO:
            // Same source line as the previous step: keep stepping without pausing.
            if (lineNumber == lastLineNumber_) {
                paused_ = false;
                break;
            }
            paused_ = stepLocations_.find(location) == stepLocations_.end();
            UpdateLineNumber(lineNumber);
            break;
        case StepKind::STEP_OUT:
            paused_ = !methodEntered_;
            break;
        case StepKind::STEP_OVER:
            paused_ = !methodEntered_ && stepLocations_.find(location) == stepLocations_.end();
            break;
        default:
            UNREACHABLE();
            break;
    }
}

void ThreadState::OnSingleStep(const PtLocation &location)
{
    ASSERT(!paused_);
    if (breakOnStart_ && HandleBreakOnStart(location)) {
        return;
    }
    if (HandleDynamicToStatic(location.GetLineNumber())) {
        return;
    }
    if (HandleBreakpointHit(location)) {
        return;
    }
    HandleStep(location);
}

bool ThreadState::ShouldStopAtBreakpoint(const PtLocation &location)
{
    if (skipAllPauses_) {
        return false;
    }
    return bpStorage_.ShouldStopAtBreakpoint(location, engine_);
}

std::vector<BreakpointId> ThreadState::GetBreakpointsByLocation(const PtLocation &location) const
{
    return bpStorage_.GetBreakpointsByLocation(location);
}
}  // namespace ark::tooling::inspector
