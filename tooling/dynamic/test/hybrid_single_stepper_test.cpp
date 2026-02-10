/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "ecmascript/tests/test_helper.h"
#include "tooling/hybrid_step/hybrid_single_stepper.h"

namespace panda::test {
class HybridSingleStepperTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(ecmaVm, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm, scope);
    }

protected:
    EcmaVM *ecmaVm {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(HybridSingleStepperTest, GetInstanceTest)
{
    HybridSingleStepper &stepper1 = HybridSingleStepper::GetInstance();
    HybridSingleStepper &stepper2 = HybridSingleStepper::GetInstance();
    EXPECT_TRUE(&stepper1 == &stepper2);
}

HWTEST_F_L0(HybridSingleStepperTest, GetHybridSingleStepFlagTest)
{
    HybridSingleStepper &stepper = HybridSingleStepper::GetInstance();
    bool result = stepper.GetHybridSingleStepFlag(HybridStepDirection::DYNAMIC_TO_STATIC);
    EXPECT_FALSE(result);
    result = stepper.GetHybridSingleStepFlag(HybridStepDirection::STATIC_TO_DYNAMIC);
    EXPECT_FALSE(result);
}

HWTEST_F_L0(HybridSingleStepperTest, SetHybridSingleStepFlagTest)
{
    HybridSingleStepper &stepper = HybridSingleStepper::GetInstance();
    bool result = stepper.GetHybridSingleStepFlag(HybridStepDirection::DYNAMIC_TO_STATIC);
    EXPECT_FALSE(result);
    stepper.SetHybridSingleStepFlag(HybridStepDirection::DYNAMIC_TO_STATIC, true);
    result = stepper.GetHybridSingleStepFlag(HybridStepDirection::DYNAMIC_TO_STATIC);
    EXPECT_TRUE(result);
    stepper.SetHybridSingleStepFlag(HybridStepDirection::STATIC_TO_DYNAMIC, true);
    result = stepper.GetHybridSingleStepFlag(HybridStepDirection::STATIC_TO_DYNAMIC);
    EXPECT_TRUE(result);
}

HWTEST_F_L0(HybridSingleStepperTest, ResetHybridSingleStepFlagTest)
{
    HybridSingleStepper &stepper = HybridSingleStepper::GetInstance();
    stepper.SetHybridSingleStepFlag(HybridStepDirection::DYNAMIC_TO_STATIC, true);
    stepper.SetHybridSingleStepFlag(HybridStepDirection::STATIC_TO_DYNAMIC, true);
    bool result = stepper.GetHybridSingleStepFlag(HybridStepDirection::DYNAMIC_TO_STATIC);
    EXPECT_TRUE(result);
    result = stepper.GetHybridSingleStepFlag(HybridStepDirection::STATIC_TO_DYNAMIC);
    EXPECT_TRUE(result);
    stepper.SetHybridSingleStepFlag(HybridStepDirection::DYNAMIC_TO_STATIC, false);
    result = stepper.GetHybridSingleStepFlag(HybridStepDirection::DYNAMIC_TO_STATIC);
    EXPECT_FALSE(result);
    stepper.SetHybridSingleStepFlag(HybridStepDirection::STATIC_TO_DYNAMIC, false);
    result = stepper.GetHybridSingleStepFlag(HybridStepDirection::STATIC_TO_DYNAMIC);
    EXPECT_FALSE(result);
}
} // namespace panda::test