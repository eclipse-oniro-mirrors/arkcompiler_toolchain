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

#include "debugger/thread_state.h"

#include "gtest/gtest.h"

#include "evaluation/evaluation_engine.h"
#include "libarkfile/file.h"
#include "include/object_header.h"
#include "include/runtime_options.h"
#include "include/runtime.h"
#include "include/typed_value.h"

// NOLINTBEGIN

namespace ark::tooling::inspector::test {
class MockEvaluationEngine final : public EvaluationEngine {
public:
    Expected<std::tuple<TypedValue, ObjectHeader *, std::string>, Error> EvaluateExpression(
        [[maybe_unused]] uint32_t frameNumber, [[maybe_unused]] const ExpressionWrapper &bytecode,
        [[maybe_unused]] Method **method) override
    {
        return std::make_tuple(TypedValue::Invalid(), static_cast<ObjectHeader *>(nullptr), std::string());
    }
};

class ThreadStateTest : public testing::Test {
    void SetUp()
    {
        state = std::make_unique<ThreadState>(mockEvaluationEngine_, bpStorage_);
    }

protected:
    MockEvaluationEngine mockEvaluationEngine_;
    BreakpointStorage bpStorage_;
    std::unique_ptr<ThreadState> state;
    PtLocation fake = PtLocation("", {}, 0, 1);
    PtLocation location0 = PtLocation("test.abc", panda_file::File::EntityId(1), 0, 1);
    PtLocation location1 = PtLocation("test.abc", panda_file::File::EntityId(1), 1, 2);
    PtLocation location2 = PtLocation("test.abc", panda_file::File::EntityId(1), 2, 3);
    PtLocation location3 = PtLocation("test.abc", panda_file::File::EntityId(1), 3, 4);
    const char *source = "/test.ets";
};

TEST_F(ThreadStateTest, BreakOnStart)
{
    ASSERT_FALSE(state->IsPaused());
    state->OnSingleStep(fake);
    ASSERT_FALSE(state->IsPaused());

    state->BreakOnStart();
    state->OnSingleStep(fake);
    ASSERT_TRUE(state->IsPaused());
}

TEST_F(ThreadStateTest, PauseAndContinue)
{
    state->Pause();
    state->OnSingleStep(fake);
    ASSERT_TRUE(state->IsPaused());
    state->Continue();
    state->OnSingleStep(fake);
    ASSERT_FALSE(state->IsPaused());

    state->Pause();
    ASSERT_FALSE(state->IsPaused());
    state->Continue();
    state->OnSingleStep(fake);
    ASSERT_FALSE(state->IsPaused());
}

TEST_F(ThreadStateTest, StepInto)
{
    state->Pause();
    state->OnSingleStep(fake);

    std::unordered_set<PtLocation, HashLocation> locs;
    locs.insert(location1);
    locs.insert(location2);

    ASSERT_TRUE(state->IsPaused());
    state->StepInto(locs);
    state->OnSingleStep(location2);
    ASSERT_FALSE(state->IsPaused());
    state->OnSingleStep(location3);
    ASSERT_TRUE(state->IsPaused());
}

TEST_F(ThreadStateTest, ContinueTo)
{
    state->Pause();
    state->OnSingleStep(fake);

    std::unordered_set<PtLocation, HashLocation> locs;
    locs.insert(location1);
    locs.insert(location2);

    ASSERT_TRUE(state->IsPaused());
    state->ContinueTo(locs);
    state->OnSingleStep(location0);
    ASSERT_FALSE(state->IsPaused());
    state->OnSingleStep(location1);
    ASSERT_TRUE(state->IsPaused());
}

TEST_F(ThreadStateTest, StepOut)
{
    state->Pause();
    state->OnSingleStep(fake);

    ASSERT_TRUE(state->IsPaused());
    state->StepOut();
    state->OnSingleStep(location0);
    ASSERT_FALSE(state->IsPaused());
    state->OnSingleStep(location1);
    ASSERT_FALSE(state->IsPaused());
    state->OnSingleStep(location2);
    ASSERT_FALSE(state->IsPaused());
    state->OnFramePop();
    state->OnSingleStep(fake);
    ASSERT_TRUE(state->IsPaused());
}

TEST_F(ThreadStateTest, StepOver)
{
    state->Pause();
    state->OnSingleStep(fake);

    std::unordered_set<PtLocation, HashLocation> locs;
    locs.insert(location1);
    locs.insert(location2);

    ASSERT_TRUE(state->IsPaused());
    state->StepOver(locs);
    state->OnSingleStep(location1);
    ASSERT_FALSE(state->IsPaused());
    state->OnMethodEntry();
    state->OnSingleStep(fake);
    ASSERT_FALSE(state->IsPaused());
    state->OnFramePop();
    state->OnSingleStep(location2);
    ASSERT_FALSE(state->IsPaused());
    state->OnFramePop();
    state->OnSingleStep(fake);
    ASSERT_TRUE(state->IsPaused());
}

TEST_F(ThreadStateTest, OnException)
{
    ASSERT_FALSE(state->IsPaused());

    state->SetPauseOnExceptions(PauseOnExceptionsState::NONE);
    state->OnException(false);
    ASSERT_FALSE(state->IsPaused());
    state->OnException(true);
    ASSERT_FALSE(state->IsPaused());

    state->SetPauseOnExceptions(PauseOnExceptionsState::CAUGHT);
    state->OnException(true);
    ASSERT_FALSE(state->IsPaused());
    state->OnException(false);
    ASSERT_TRUE(state->IsPaused());
    state->Continue();
    ASSERT_FALSE(state->IsPaused());

    state->SetPauseOnExceptions(PauseOnExceptionsState::UNCAUGHT);
    state->OnException(false);
    ASSERT_FALSE(state->IsPaused());
    state->OnException(true);
    ASSERT_TRUE(state->IsPaused());
    state->Continue();
    ASSERT_FALSE(state->IsPaused());

    state->SetPauseOnExceptions(PauseOnExceptionsState::ALL);
    state->OnException(false);
    ASSERT_TRUE(state->IsPaused());
    state->Continue();
    ASSERT_FALSE(state->IsPaused());
    state->OnException(true);
    ASSERT_TRUE(state->IsPaused());
}

TEST_F(ThreadStateTest, OnSingleStepPausesOnNonPositiveLineNumber)
{
    PtLocation skipLocation = PtLocation("test.abc", panda_file::File::EntityId(1), 0, SKIP_LINE_NUMBER);
    PtLocation unknownLocation = PtLocation("test.abc", panda_file::File::EntityId(1), 0, UNKNOWN_LINE_NUMBER);

    state->Pause();
    state->OnSingleStep(skipLocation);
    ASSERT_TRUE(state->IsPaused());

    state->Continue();
    state->Pause();
    state->OnSingleStep(unknownLocation);
    ASSERT_TRUE(state->IsPaused());

    state->Continue();
    state->BreakOnStart();
    state->OnSingleStep(unknownLocation);
    ASSERT_TRUE(state->IsPaused());
}

TEST_F(ThreadStateTest, UpdateLineNumberIgnoresSkipLineNumber)
{
    state->Pause();
    state->OnSingleStep(fake);
    ASSERT_TRUE(state->IsPaused());

    std::unordered_set<PtLocation, HashLocation> locs;
    locs.insert(location1);
    state->StepInto(locs);

    constexpr int32_t SOURCE_LINE = 5;
    PtLocation location1Line5 = PtLocation("test.abc", panda_file::File::EntityId(1), 1, SOURCE_LINE);
    PtLocation cctorSkipLine = PtLocation("test.abc", panda_file::File::EntityId(1), 0, SKIP_LINE_NUMBER);
    PtLocation location2Line5 = PtLocation("test.abc", panda_file::File::EntityId(1), 2, SOURCE_LINE);

    // Step onto line 5 (in stepLocations_): keeps stepping and records lastLineNumber_ = 5.
    state->OnSingleStep(location1Line5);
    ASSERT_FALSE(state->IsPaused());

    // The static cctor step carries SKIP_LINE_NUMBER and stays in stepLocations_, so it keeps
    // stepping; the guard keeps lastLineNumber_ = 5 instead of overwriting it with SKIP_LINE_NUMBER.
    locs.insert(cctorSkipLine);
    state->StepInto(locs);
    state->OnSingleStep(cctorSkipLine);
    ASSERT_FALSE(state->IsPaused());

    // Returning to the same source line (dynamic->static->dynamic round trip) is still
    // suppressed by the same-line guard, even outside stepLocations_.
    state->OnSingleStep(location2Line5);
    ASSERT_FALSE(state->IsPaused());
}

TEST_F(ThreadStateTest, StepIntoDoesNotPauseOnSameLineNumber)
{
    state->Pause();
    state->OnSingleStep(fake);

    std::unordered_set<PtLocation, HashLocation> locs;
    locs.insert(location1);

    ASSERT_TRUE(state->IsPaused());
    state->StepInto(locs);

    PtLocation location1Line5 = PtLocation("test.abc", panda_file::File::EntityId(1), 1, 5);
    PtLocation location2Line5 = PtLocation("test.abc", panda_file::File::EntityId(1), 2, 5);
    PtLocation location2Line6 = PtLocation("test.abc", panda_file::File::EntityId(1), 2, 6);

    // Stepping to location1 (in stepLocations_) on line 5 does not pause, but records
    // lastLineNumber_ = 5.
    state->OnSingleStep(location1Line5);
    ASSERT_FALSE(state->IsPaused());

    // A subsequent step to another location still on line 5 is suppressed by the same-line guard.
    state->OnSingleStep(location2Line5);
    ASSERT_FALSE(state->IsPaused());

    // A step onto a different line (6) re-enables pausing (location2 not in stepLocations_).
    state->OnSingleStep(location2Line6);
    ASSERT_TRUE(state->IsPaused());
}

}  // namespace ark::tooling::inspector::test

// NOLINTEND
