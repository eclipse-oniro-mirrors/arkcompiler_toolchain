/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "tooling/hybrid_step/frame_info_extractor.h"
#include "tooling/hybrid_step/frame_info.h"

namespace panda::test {

class MockFrameInfoProvider : public panda::tooling::hybrid_step::IFrameInfoProvider {
public:
    MockFrameInfoProvider() = default;
    ~MockFrameInfoProvider() override = default;

    bool ExtractFrameInfo(const void *framePtr, panda::tooling::hybrid_step::UnifiedFrameInfo &info) override
    {
        if (framePtr == nullptr) {
            return false;
        }
        info.hasInfo = true;
        info.methodName = "testMethod";
        info.sourceFile = "test.ts";
        info.lineNumber = TEST_LINE_NUMBER;
        info.columnNumber = TEST_COL_NUMBER;
        return true;
    }

    static constexpr int32_t TEST_LINE_NUMBER = 10;
    static constexpr int32_t TEST_COL_NUMBER = 5;
};

class FrameInfoExtractorTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownTestCase";
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

HWTEST_F_L0(FrameInfoExtractorTest, GetInstanceTest)
{
    panda::tooling::hybrid_step::FrameInfoExtractor &ext1 = panda::tooling::hybrid_step::FrameInfoExtractor::Get();
    panda::tooling::hybrid_step::FrameInfoExtractor &ext2 = panda::tooling::hybrid_step::FrameInfoExtractor::Get();
    EXPECT_TRUE(&ext1 == &ext2);
}

HWTEST_F_L0(FrameInfoExtractorTest, RegisterStaticProviderTest)
{
    panda::tooling::hybrid_step::FrameInfoExtractor &ext = panda::tooling::hybrid_step::FrameInfoExtractor::Get();
    auto provider = std::make_unique<MockFrameInfoProvider>();
    ext.RegisterProvider(true, nullptr, std::move(provider));

    panda::tooling::hybrid_step::UnifiedFrameInfo info;
    void *framePtr = reinterpret_cast<void *>(0x1234);
    bool result = ext.ExtractStaticFrameInfo(framePtr, info);
    EXPECT_TRUE(result);
    EXPECT_TRUE(info.hasInfo);
    EXPECT_EQ(info.methodName, "testMethod");
    EXPECT_EQ(info.sourceFile, "test.ts");
    EXPECT_EQ(info.lineNumber, 10);
    EXPECT_EQ(info.columnNumber, 5);
    EXPECT_TRUE(info.isStaticFrame);
}

HWTEST_F_L0(FrameInfoExtractorTest, RegisterDynamicProviderTest)
{
    panda::tooling::hybrid_step::FrameInfoExtractor &ext = panda::tooling::hybrid_step::FrameInfoExtractor::Get();
    auto provider = std::make_unique<MockFrameInfoProvider>();
    const void *vm = reinterpret_cast<const void *>(0x5678);
    ext.RegisterProvider(false, vm, std::move(provider));

    panda::tooling::hybrid_step::UnifiedFrameInfo info;
    void *framePtr = reinterpret_cast<void *>(0x1234);
    bool result = ext.ExtractDynamicFrameInfo(vm, framePtr, info);
    EXPECT_TRUE(result);
    EXPECT_TRUE(info.hasInfo);
    EXPECT_EQ(info.methodName, "testMethod");
    EXPECT_FALSE(info.isStaticFrame);
}

HWTEST_F_L0(FrameInfoExtractorTest, UnregisterProviderTest)
{
    panda::tooling::hybrid_step::FrameInfoExtractor &ext = panda::tooling::hybrid_step::FrameInfoExtractor::Get();
    auto provider = std::make_unique<MockFrameInfoProvider>();
    const void *vm = reinterpret_cast<const void *>(0x5678);
    ext.RegisterProvider(false, vm, std::move(provider));

    ext.UnregisterProvider(vm);

    panda::tooling::hybrid_step::UnifiedFrameInfo info;
    void *framePtr = reinterpret_cast<void *>(0x1234);
    bool result = ext.ExtractDynamicFrameInfo(vm, framePtr, info);
    EXPECT_FALSE(result);
    EXPECT_FALSE(info.hasInfo);
}

HWTEST_F_L0(FrameInfoExtractorTest, ExtractFrameInfoWithNullptrProviderTest)
{
    panda::tooling::hybrid_step::FrameInfoExtractor &ext = panda::tooling::hybrid_step::FrameInfoExtractor::Get();
    const void *uniqueVm = reinterpret_cast<const void *>(0xDEADBEEF);

    panda::tooling::hybrid_step::UnifiedFrameInfo info;
    void *framePtr = reinterpret_cast<void *>(0x1234);
    bool result = ext.ExtractDynamicFrameInfo(uniqueVm, framePtr, info);
    EXPECT_FALSE(result);
    EXPECT_FALSE(info.hasInfo);
}

HWTEST_F_L0(FrameInfoExtractorTest, ExtractDynamicFrameInfoWithUnregisteredVmTest)
{
    panda::tooling::hybrid_step::FrameInfoExtractor &ext = panda::tooling::hybrid_step::FrameInfoExtractor::Get();

    panda::tooling::hybrid_step::UnifiedFrameInfo info;
    void *framePtr = reinterpret_cast<void *>(0x1234);
    const void *unregisteredVm = reinterpret_cast<const void *>(0x9999);
    bool result = ext.ExtractDynamicFrameInfo(unregisteredVm, framePtr, info);
    EXPECT_FALSE(result);
    EXPECT_FALSE(info.hasInfo);
}

HWTEST_F_L0(FrameInfoExtractorTest, ExtractFrameInfoWithNullFramePtrTest)
{
    panda::tooling::hybrid_step::FrameInfoExtractor &ext = panda::tooling::hybrid_step::FrameInfoExtractor::Get();
    auto provider = std::make_unique<MockFrameInfoProvider>();
    ext.RegisterProvider(true, nullptr, std::move(provider));

    panda::tooling::hybrid_step::UnifiedFrameInfo info;
    bool result = ext.ExtractStaticFrameInfo(nullptr, info);
    EXPECT_FALSE(result);
}
} // namespace panda::test
