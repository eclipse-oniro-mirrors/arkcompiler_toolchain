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

#include "debugger/debug_info_cache.h"

#include <array>
#include <memory>
#include <optional>
#include <utility>

#include "gtest/gtest.h"

#include "assembly-emitter.h"
#include "assembly-parser.h"
#include "include/runtime.h"
#include "include/runtime_options.h"
#include "include/thread_scopes.h"
#include "runtime/include/coretypes/tagged_value.h"

#include "test_frame.h"

// NOLINTBEGIN

namespace ark::tooling::inspector::test {

static constexpr const char *g_source = R"(
    .record Test {}

    .function i32 Test.foo(u64 a0, u64 a1) {
        mov v0, v1         # line 2, offset 0, 1
        mov v100, v101     # line 3, offset 2, 3, 4
        movi v0, 4         # line 4, offset 5, 6
        ldai 222           # line 5, offset 7, 8, 9
        return             # line 6, offset 10
    }
)";

static constexpr const char *g_second_source = R"(
    .record Test2 {}

    .function i32 Test2.second() {
        movi v0, 7         # line 7, offset 0
        return             # line 8, offset 1
    }
)";

static constexpr const char *g_sameFirstSource = R"(
    .record SameFirst {}

    .function i32 SameFirst.foo() {
        movi v0, 20        # line 20, offset 0
        return             # line 21, offset 1
    }
)";

static constexpr const char *g_sameSecondSource = R"(
    .record SameSecond {}

    .function i32 SameSecond.second() {
        movi v0, 30        # line 30, offset 0
        return             # line 31, offset 1
    }
)";

static constexpr size_t PANDA_FILES_COUNT = 4U;
static constexpr uint32_t SAME_FIRST_START_LINE = 20U;
static constexpr uint32_t SAME_SECOND_START_LINE = 30U;

class DebugInfoCacheTest : public testing::Test {
protected:
    static std::optional<pandasm::Program> ParseProgram(const char *source, const char *sourceFileName)
    {
        pandasm::Parser parser;
        auto result = parser.Parse(source, sourceFileName);
        if (!result.HasValue()) {
            return std::nullopt;
        }
        return std::move(result.Value());
    }

    static std::unique_ptr<const panda_file::File> EmitPandaFile(pandasm::Program &program, const char *asmFileName)
    {
        if (!pandasm::AsmEmitter::Emit(asmFileName, program)) {
            return nullptr;
        }
        return panda_file::OpenPandaFile(asmFileName);
    }

    static void SetSourceCodeAndLineNumbers(pandasm::Program &program, const char *sourceCode, uint32_t startLine)
    {
        for (auto &[name, function] : program.functionStaticTable) {
            (void)name;
            function.sourceCode = sourceCode;
            for (size_t i = 0; i < function.ins.size(); ++i) {
                function.ins[i].insDebug.SetLineNumber(startLine + i);
            }
        }
    }

    static void LinkPandaFiles(std::array<std::unique_ptr<const panda_file::File>, PANDA_FILES_COUNT> &pandaFiles)
    {
        RuntimeOptions options;
        options.SetShouldInitializeIntrinsics(false);
        options.SetShouldLoadBootPandaFiles(false);
        Runtime::Create(options);

        thread_ = ManagedThread::GetCurrent();
        ScopedManagedCodeThread scopedThread(thread_);
        ClassLinker *classLinker = Runtime::GetCurrent()->GetClassLinker();
        classLinker->AddPandaFile(std::move(pandaFiles[0]));

        PandaString descriptorHolder;
        const auto *descriptor = ClassHelper::GetDescriptor(utf::CStringAsMutf8("Test"), &descriptorHolder);
        auto *extension = classLinker->GetExtension(panda_file::SourceLang::PANDA_ASSEMBLY);
        Class *klass = extension->GetClass(descriptor, true, extension->GetBootContext());
        ASSERT_NE(klass, nullptr);

        auto methods = klass->GetMethods();
        ASSERT_EQ(methods.size(), 1U);
        methodFoo = &methods[0];

        for (size_t i = 1; i < pandaFiles.size(); ++i) {
            classLinker->AddPandaFile(std::move(pandaFiles[i]));
        }
    }

    static void SetUpTestSuite()
    {
        auto program = ParseProgram(g_source, SOURCE_FILE_NAME);
        ASSERT_TRUE(program.has_value());
        auto pandaFile = EmitPandaFile(*program, ASM_FILE_NAME);
        ASSERT_NE(pandaFile, nullptr);

        auto secondProgram = ParseProgram(g_second_source, SOURCE_FILE_NAME);
        ASSERT_TRUE(secondProgram.has_value());
        auto secondPandaFile = EmitPandaFile(*secondProgram, SECOND_ASM_FILE_NAME);
        ASSERT_NE(secondPandaFile, nullptr);

        auto sameFirstProgram = ParseProgram(g_sameFirstSource, SAME_SOURCE_FILE_NAME);
        ASSERT_TRUE(sameFirstProgram.has_value());
        SetSourceCodeAndLineNumbers(*sameFirstProgram, g_sameFirstSource, SAME_FIRST_START_LINE);
        auto sameFirstPandaFile = EmitPandaFile(*sameFirstProgram, SAME_FIRST_ASM_FILE_NAME);
        ASSERT_NE(sameFirstPandaFile, nullptr);

        auto sameSecondProgram = ParseProgram(g_sameSecondSource, SAME_SOURCE_FILE_NAME);
        ASSERT_TRUE(sameSecondProgram.has_value());
        SetSourceCodeAndLineNumbers(*sameSecondProgram, g_sameSecondSource, SAME_SECOND_START_LINE);
        auto sameSecondPandaFile = EmitPandaFile(*sameSecondProgram, SAME_SECOND_ASM_FILE_NAME);
        ASSERT_NE(sameSecondPandaFile, nullptr);

        std::array<std::unique_ptr<const panda_file::File>, PANDA_FILES_COUNT> pandaFiles = {
            std::move(pandaFile), std::move(secondPandaFile), std::move(sameFirstPandaFile),
            std::move(sameSecondPandaFile)};
        for (auto &file : pandaFiles) {
            ASSERT_NE(file, nullptr);
            cache.AddPandaFile(*file, true);
        }
        LinkPandaFiles(pandaFiles);
    }

    static void TearDownTestSuite()
    {
        Runtime::Destroy();
    }

    static constexpr const char *ASM_FILE_NAME = "source.abc";
    static constexpr const char *SECOND_ASM_FILE_NAME = "second-source.abc";
    static constexpr const char *SAME_SOURCE_FILE_NAME = "same-source.ets";
    static constexpr const char *SAME_FIRST_ASM_FILE_NAME = "same-source-first.abc";
    static constexpr const char *SAME_SECOND_ASM_FILE_NAME = "same-source-second.abc";
    // This test intentionally sets empty source file name to ensure that disassembly is used for debug info
    static constexpr const char *SOURCE_FILE_NAME = "";
    static DebugInfoCache cache;
    static ManagedThread *thread_;
    static Method *methodFoo;
};

DebugInfoCache DebugInfoCacheTest::cache {};
ManagedThread *DebugInfoCacheTest::thread_ = nullptr;
Method *DebugInfoCacheTest::methodFoo = nullptr;

TEST_F(DebugInfoCacheTest, GetCurrentLineLocations)
{
    auto fr0 = TestFrame(methodFoo, 2U);   // offset 2, line 3 of function
    auto fr1 = TestFrame(methodFoo, 6U);   // offset 6, line 4 of function
    auto fr2 = TestFrame(methodFoo, 10U);  // offset 10, line 6 of function

    auto curr = cache.GetCurrentLineLocations(fr0);
    ASSERT_EQ(curr.size(), 3U);
    ASSERT_NE(curr.find(PtLocation(ASM_FILE_NAME, methodFoo->GetFileId(), 2U)), curr.end());
    ASSERT_NE(curr.find(PtLocation(ASM_FILE_NAME, methodFoo->GetFileId(), 3U)), curr.end());
    ASSERT_NE(curr.find(PtLocation(ASM_FILE_NAME, methodFoo->GetFileId(), 4U)), curr.end());

    curr = cache.GetCurrentLineLocations(fr1);
    ASSERT_EQ(curr.size(), 2U);
    ASSERT_NE(curr.find(PtLocation(ASM_FILE_NAME, methodFoo->GetFileId(), 5U)), curr.end());
    ASSERT_NE(curr.find(PtLocation(ASM_FILE_NAME, methodFoo->GetFileId(), 6U)), curr.end());

    curr = cache.GetCurrentLineLocations(fr2);
    ASSERT_EQ(curr.size(), 1);
    ASSERT_NE(curr.find(PtLocation(ASM_FILE_NAME, methodFoo->GetFileId(), 10U)), curr.end());
}

TEST_F(DebugInfoCacheTest, GetLocals)
{
    static constexpr size_t ARGUMENTS_COUNT = 2U;
    static constexpr size_t LOCALS_COUNT = 103U;

    auto fr0 = TestFrame(methodFoo, 2U);  // offset 2, line 3 of function

    for (size_t i = 0; i < ARGUMENTS_COUNT; i++) {
        fr0.SetArgument(i, i + 1);
        fr0.SetArgumentKind(i, PtFrame::RegisterKind::PRIMITIVE);
    }
    for (size_t i = 0; i < LOCALS_COUNT; i++) {
        fr0.SetVReg(i, i);
        fr0.SetVRegKind(i, PtFrame::RegisterKind::PRIMITIVE);
    }
    auto mapLocals = cache.GetLocals(fr0);
    ASSERT_EQ(ARGUMENTS_COUNT + LOCALS_COUNT, mapLocals.size());

    EXPECT_TRUE(mapLocals.find("a0") != mapLocals.end());
    EXPECT_TRUE(mapLocals.find("a1") != mapLocals.end());
    EXPECT_TRUE(mapLocals.find("v101") != mapLocals.end());

    ASSERT_EQ(mapLocals.at("a0").GetAsU64(), 1U);
    ASSERT_EQ(mapLocals.at("a1").GetAsU64(), 2U);
    ASSERT_EQ(mapLocals.at("v101").GetAsU64(), 101U);
}

TEST_F(DebugInfoCacheTest, GetLocalsWithTaggedSpecialValues)
{
    static constexpr size_t ARGUMENTS_COUNT = 2U;
    static constexpr size_t LOCALS_COUNT = 103U;

    auto fr0 = TestFrame(methodFoo, 2U);  // offset 2, line 3 of function

    // Arguments use signature-based type ('U' -> U32), RegisterKind is ignored.
    // Set them as PRIMITIVE to match the existing debug info signature.
    for (size_t i = 0; i < ARGUMENTS_COUNT; i++) {
        fr0.SetArgument(i, i + 1);
        fr0.SetArgumentKind(i, PtFrame::RegisterKind::PRIMITIVE);
    }

    // For vregs, if the debug info has no typeSignature, RegisterKind is used.
    // Set v0 = VALUE_HOLE (0x00) with REFERENCE kind to test REFERENCE -> TAGGED conversion
    fr0.SetVReg(0, coretypes::TaggedValue::VALUE_HOLE);
    fr0.SetVRegKind(0, PtFrame::RegisterKind::REFERENCE);
    // Set v1 = VALUE_NULL (0x02) with REFERENCE kind to test REFERENCE -> TAGGED conversion
    fr0.SetVReg(1, coretypes::TaggedValue::VALUE_NULL);
    fr0.SetVRegKind(1, PtFrame::RegisterKind::REFERENCE);
    for (size_t i = 2; i < LOCALS_COUNT; i++) {
        fr0.SetVReg(i, i);
        fr0.SetVRegKind(i, PtFrame::RegisterKind::PRIMITIVE);
    }

    auto mapLocals = cache.GetLocals(fr0);

    // v0 (VALUE_HOLE via REFERENCE register) should be converted to TAGGED type
    ASSERT_TRUE(mapLocals.find("v0") != mapLocals.end());
    EXPECT_TRUE(mapLocals.at("v0").IsTagged());
    EXPECT_TRUE(mapLocals.at("v0").GetAsTagged().IsHole());

    // v1 (VALUE_NULL via REFERENCE register) should be converted to TAGGED type
    ASSERT_TRUE(mapLocals.find("v1") != mapLocals.end());
    EXPECT_TRUE(mapLocals.at("v1").IsTagged());
    EXPECT_TRUE(mapLocals.at("v1").GetAsTagged().IsNull());
}

TEST_F(DebugInfoCacheTest, GetSourceLocation)
{
    auto fr0 = TestFrame(methodFoo, 2U);  // offset 2, line 3 of function
    auto fr1 = TestFrame(methodFoo, 6U);  // offset 6, line 4 of function

    std::string_view disasm_file;
    std::string_view method_name;
    int32_t line_number = 0;

    cache.GetSourceLocation(fr0, disasm_file, method_name, line_number);
    ASSERT_NE(disasm_file.find(ASM_FILE_NAME), std::string::npos);
    ASSERT_EQ(method_name, "foo");
    ASSERT_EQ(line_number, 3U);

    cache.GetSourceLocation(fr1, disasm_file, method_name, line_number);
    ASSERT_NE(disasm_file.find(ASM_FILE_NAME), std::string::npos);
    ASSERT_EQ(method_name, "foo");
    ASSERT_EQ(line_number, 4U);

    auto set_locs = cache.GetContinueToLocations(disasm_file, {}, 4U);
    ASSERT_EQ(set_locs.size(), 2U);
    ASSERT_NE(set_locs.find(PtLocation(ASM_FILE_NAME, methodFoo->GetFileId(), 6U)), set_locs.end());
    ASSERT_NE(set_locs.find(PtLocation(ASM_FILE_NAME, methodFoo->GetFileId(), 5U)), set_locs.end());

    set_locs = cache.GetContinueToLocations(disasm_file, {}, 6U);
    ASSERT_EQ(set_locs.size(), 1);
    ASSERT_NE(set_locs.find(PtLocation(ASM_FILE_NAME, methodFoo->GetFileId(), 10U)), set_locs.end());

    set_locs = cache.GetContinueToLocations(disasm_file, {}, 1);
    ASSERT_EQ(set_locs.size(), 0);

    auto valid_locs = cache.GetValidLineNumbers(disasm_file, {}, 0U, 100U, false);
    ASSERT_EQ(valid_locs.size(), 5U);

    ASSERT_NE(valid_locs.find(2U), valid_locs.end());
    ASSERT_NE(valid_locs.find(3U), valid_locs.end());
    ASSERT_NE(valid_locs.find(4U), valid_locs.end());
    ASSERT_NE(valid_locs.find(5U), valid_locs.end());
    ASSERT_NE(valid_locs.find(6U), valid_locs.end());

    auto s = cache.GetSourceCode(disasm_file);
    ASSERT_NE(s.find(".function i32 Test.foo(u64 a0, u64 a1)"), std::string::npos);

    s = cache.GetSourceCode("source.pa", ASM_FILE_NAME);
    ASSERT_TRUE(s.empty());

    SourceFileSet sets;
    auto breaks = cache.GetBreakpointLocations([](auto, [[maybe_unused]] auto) { return true; }, 4U, sets);
    ASSERT_EQ(breaks.size(), 1);
    ASSERT_EQ(sets.size(), 1);
    ASSERT_EQ(sets.begin()->first, disasm_file);

    ASSERT_NE(std::find(breaks.begin(), breaks.end(), PtLocation(ASM_FILE_NAME, methodFoo->GetFileId(), 5U)),
              breaks.end());
}

TEST_F(DebugInfoCacheTest, GetAsyncFrameSourceLocation)
{
    auto location = cache.GetAsyncFrameSourceLocation(ASM_FILE_NAME, methodFoo->GetFileId().GetOffset(), 2U);
    ASSERT_TRUE(location.has_value());
    ASSERT_NE(location->sourceFile.find(ASM_FILE_NAME), std::string::npos);
    ASSERT_EQ(location->lineNumber, 3U);

    ASSERT_FALSE(cache.GetAsyncFrameSourceLocation("missing.abc", methodFoo->GetFileId().GetOffset(), 2U).has_value());
    ASSERT_FALSE(cache.GetAsyncFrameSourceLocation(ASM_FILE_NAME, 0xDEADBEEFU, 2U).has_value());
}

TEST_F(DebugInfoCacheTest, SameSourceNameUsesPandaFileIdentity)
{
    auto firstSource = cache.GetSourceCode(SAME_SOURCE_FILE_NAME, SAME_FIRST_ASM_FILE_NAME);
    ASSERT_NE(firstSource.find("SameFirst.foo"), std::string::npos);

    auto secondSource = cache.GetSourceCode(SAME_SOURCE_FILE_NAME, SAME_SECOND_ASM_FILE_NAME);
    ASSERT_NE(secondSource.find("SameSecond.second"), std::string::npos);
    ASSERT_EQ(secondSource.find("SameFirst.foo"), std::string::npos);

    auto firstLines = cache.GetValidLineNumbers(SAME_SOURCE_FILE_NAME, SAME_FIRST_ASM_FILE_NAME, 0, 100, false);
    ASSERT_EQ(firstLines, (std::set<int32_t> {20, 21}));

    auto secondLines = cache.GetValidLineNumbers(SAME_SOURCE_FILE_NAME, SAME_SECOND_ASM_FILE_NAME, 0, 100, false);
    ASSERT_EQ(secondLines, (std::set<int32_t> {30, 31}));
}

}  // namespace ark::tooling::inspector::test

// NOLINTEND
