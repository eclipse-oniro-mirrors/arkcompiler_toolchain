/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "debugger/conditional_breakpoint.h"

#include <cstring>
#include <limits>
#include <string>

#include "gtest/gtest.h"

#include "evaluation/evaluation_engine.h"
#include "include/runtime.h"
#include "include/runtime_options.h"
#include "include/typed_value.h"
#include "runtime/include/class_linker.h"
#include "runtime/include/coretypes/array.h"
#include "runtime/include/coretypes/string.h"
#include "runtime/include/coretypes/tagged_value.h"
#include "runtime/include/object_header.h"
#include "runtime/include/panda_vm.h"
#include "tooling/debugger.h"

// NOLINTBEGIN

namespace ark::tooling::inspector::test {

class MockEvaluationEngine final : public EvaluationEngine {
public:
    Expected<std::tuple<TypedValue, ObjectHeader *, std::string>, Error> EvaluateExpression(
        [[maybe_unused]] uint32_t frameNumber, [[maybe_unused]] const ExpressionWrapper &bytecode,
        [[maybe_unused]] Method **method) override
    {
        return std::make_tuple(value_, static_cast<ObjectHeader *>(nullptr), std::string());
    }

    void SetValue(TypedValue value)
    {
        value_ = value;
    }

private:
    TypedValue value_ = TypedValue::Invalid();
};

class ConditionalBreakpointTest : public testing::Test {
protected:
    static void SetUpTestSuite()
    {
        RuntimeOptions options;
        options.SetShouldInitializeIntrinsics(false);
        options.SetShouldLoadBootPandaFiles(false);
        Runtime::Create(options);
        thread_ = ark::MTManagedThread::GetCurrent();
        thread_->ManagedCodeBegin();

        LanguageContext ctx = Runtime::GetCurrent()->GetLanguageContext(panda_file::SourceLang::PANDA_ASSEMBLY);
        auto *extension = Runtime::GetCurrent()->GetClassLinker()->GetExtension(ctx);
        objectClass_ = extension->GetClassRoot(ClassRoot::OBJECT);
        ASSERT_NE(objectClass_, nullptr);
        arrayClass_ = extension->GetClass(utf::CStringAsMutf8("[I"));
        ASSERT_NE(arrayClass_, nullptr);
    }

    static void TearDownTestSuite()
    {
        thread_->ManagedCodeEnd();
        Runtime::Destroy();
    }

    static coretypes::String *CreateString(const char *data)
    {
        LanguageContext ctx = Runtime::GetCurrent()->GetLanguageContext(panda_file::SourceLang::PANDA_ASSEMBLY);
        return coretypes::String::CreateFromUtf8(utf::CStringAsMutf8(data), std::strlen(data), ctx,
                                                 Runtime::GetCurrent()->GetPandaVM());
    }

    static constexpr float F32_NAN = std::numeric_limits<float>::quiet_NaN();
    static constexpr double F64_NAN = std::numeric_limits<double>::quiet_NaN();
    static constexpr float F32_INF = std::numeric_limits<float>::infinity();
    static constexpr double F64_INF = std::numeric_limits<double>::infinity();

    // Numeric sample values: any non-boolean condition result never stops
    static constexpr int32_t INT_ZERO = 0;
    static constexpr int32_t INT_ONE = 1;
    static constexpr int32_t INT_NEGATIVE_ONE = -1;
    static constexpr int32_t INT_VALUE = 42;
    static constexpr int32_t INT_NEGATIVE_VALUE = -42;
    static constexpr float F32_ZERO = 0.0F;
    static constexpr float F32_NEGATIVE_ZERO = -0.0F;
    static constexpr float F32_VALUE = 1.5F;
    static constexpr float F32_NEGATIVE_VALUE = -1.5F;
    static constexpr double F64_ZERO = 0.0;
    static constexpr double F64_NEGATIVE_ZERO = -0.0;
    static constexpr double F64_VALUE = 1.5;
    static constexpr double F64_NEGATIVE_VALUE = -1.5;

    static ark::MTManagedThread *thread_;
    static Class *objectClass_;
    static Class *arrayClass_;
};

ark::MTManagedThread *ConditionalBreakpointTest::thread_ = nullptr;
Class *ConditionalBreakpointTest::objectClass_ = nullptr;
Class *ConditionalBreakpointTest::arrayClass_ = nullptr;

TEST_F(ConditionalBreakpointTest, IsConditionTrueInvalidAndVoid)
{
    bool invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::Invalid(), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::Void(), invalid));
    ASSERT_TRUE(invalid);
}

TEST_F(ConditionalBreakpointTest, IsConditionTrueBoolean)
{
    bool invalid = false;
    ASSERT_TRUE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U1(true), invalid));
    ASSERT_FALSE(invalid);
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U1(false), invalid));
    ASSERT_FALSE(invalid);
}

TEST_F(ConditionalBreakpointTest, IsConditionTrueIntegers)
{
    // Any integer never stops, even non-zero ones, and marks the condition invalid
    bool invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::I8(INT_ZERO), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::I8(INT_NEGATIVE_ONE), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U8(INT_ZERO), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U8(INT_ONE), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::I16(INT_ZERO), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::I16(INT_NEGATIVE_ONE), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U16(INT_ZERO), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U16(INT_ONE), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::I32(INT_ZERO), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::I32(INT_NEGATIVE_VALUE), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U32(INT_ZERO), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U32(INT_VALUE), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::I64(INT_ZERO), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::I64(INT_NEGATIVE_VALUE), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U64(INT_ZERO), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U64(INT_VALUE), invalid));
    ASSERT_TRUE(invalid);
}

TEST_F(ConditionalBreakpointTest, IsConditionTrueFloatingPoint)
{
    // Any floating-point number never stops, and marks the condition invalid
    bool invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F32(F32_ZERO), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F32(F32_NEGATIVE_ZERO), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F32(F32_NAN), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F32(F32_VALUE), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F32(F32_NEGATIVE_VALUE), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F32(F32_INF), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F64(F64_ZERO), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F64(F64_NEGATIVE_ZERO), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F64(F64_NAN), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F64(F64_VALUE), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F64(F64_NEGATIVE_VALUE), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F64(F64_INF), invalid));
    ASSERT_TRUE(invalid);
}

TEST_F(ConditionalBreakpointTest, IsConditionTrueTaggedSpecialValues)
{
    using TaggedValue = coretypes::TaggedValue;

    // Non-boolean, non-heap-object tagged values are invalid conditions
    bool invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(
        TypedValue::Tagged(TaggedValue(TaggedValue::VALUE_UNDEFINED)), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(
        TypedValue::Tagged(TaggedValue(TaggedValue::VALUE_NULL)), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(
        TypedValue::Tagged(TaggedValue(TaggedValue::VALUE_HOLE)), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(
        TypedValue::Tagged(TaggedValue(TaggedValue::VALUE_EXCEPTION)), invalid));
    ASSERT_TRUE(invalid);

    // TaggedValue overload: same semantics
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(
        TaggedValue(TaggedValue::VALUE_UNDEFINED), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(
        TaggedValue(TaggedValue::VALUE_NULL), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(
        TaggedValue(TaggedValue::VALUE_HOLE), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(
        TaggedValue(TaggedValue::VALUE_EXCEPTION), invalid));
    ASSERT_TRUE(invalid);
}

TEST_F(ConditionalBreakpointTest, IsConditionTrueTaggedPrimitives)
{
    using TaggedValue = coretypes::TaggedValue;

    bool invalid = false;
    ASSERT_TRUE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(true), invalid));
    ASSERT_FALSE(invalid);
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(false), invalid));
    ASSERT_FALSE(invalid);

    // Numbers are invalid conditions
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(INT_ZERO), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(INT_ONE), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(INT_NEGATIVE_ONE), invalid));
    ASSERT_TRUE(invalid);

    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(F64_ZERO), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(F64_NEGATIVE_ZERO), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(TaggedValue::VALUE_NAN), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(F64_VALUE), invalid));
    ASSERT_TRUE(invalid);
    invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(F64_NEGATIVE_VALUE), invalid));
    ASSERT_TRUE(invalid);
}

TEST_F(ConditionalBreakpointTest, IsConditionTrueTaggedHeapObjects)
{
    auto *object = ObjectHeader::Create(objectClass_);
    ASSERT_NE(object, nullptr);

    // Any heap object never stops, unless it unwraps to a boxed boolean.
    // Heap objects are valid references, so conditionInvalid is NOT set.
    bool invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(coretypes::TaggedValue(object), invalid));
    ASSERT_FALSE(invalid);
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(
        TypedValue::Tagged(coretypes::TaggedValue(object)), invalid));
    ASSERT_FALSE(invalid);
}

TEST_F(ConditionalBreakpointTest, IsConditionTrueReferences)
{
    // Any reference never stops. References are valid values, so conditionInvalid is NOT set.
    bool invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(static_cast<ObjectHeader *>(nullptr), invalid));
    ASSERT_FALSE(invalid);
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::Reference(nullptr), invalid));
    ASSERT_FALSE(invalid);

    auto *object = ObjectHeader::Create(objectClass_);
    ASSERT_NE(object, nullptr);
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(object, invalid));
    ASSERT_FALSE(invalid);
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::Reference(object), invalid));
    ASSERT_FALSE(invalid);

    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(CreateString(""), invalid));
    ASSERT_FALSE(invalid);
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(CreateString("true"), invalid));
    ASSERT_FALSE(invalid);
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::Reference(CreateString("true")), invalid));
    ASSERT_FALSE(invalid);

    constexpr uint32_t EMPTY_ARRAY_LENGTH = 0U;
    constexpr uint32_t ARRAY_LENGTH = 5U;

    auto *emptyArray = coretypes::Array::Create(arrayClass_, EMPTY_ARRAY_LENGTH);
    ASSERT_NE(emptyArray, nullptr);
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(emptyArray, invalid));
    ASSERT_FALSE(invalid);

    auto *array = coretypes::Array::Create(arrayClass_, ARRAY_LENGTH);
    ASSERT_NE(array, nullptr);
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(array, invalid));
    ASSERT_FALSE(invalid);
}

TEST_F(ConditionalBreakpointTest, IsConditionTrueAssignmentExpression)
{
    // User mistakenly writes `i = 5` (assignment) instead of `i === 5` (comparison).
    // The assignment expression evaluates to the assigned value, which is Int 5,
    // not a boolean. This should mark the condition as invalid.
    constexpr int32_t ASSIGNED_INT_VALUE = 5;
    bool invalid = false;
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::I32(ASSIGNED_INT_VALUE), invalid));
    ASSERT_TRUE(invalid);
}

TEST_F(ConditionalBreakpointTest, ShouldStopAtUnresolvedBreakpoint)
{
    constexpr BreakpointId BREAKPOINT_ID = 1U;
    constexpr int32_t BREAKPOINT_LINE = 1;
    constexpr uint32_t PANDA_FILE_ID = 1U;
    constexpr uint32_t BYTECODE_OFFSET_FIRST = 0U;
    constexpr uint32_t BYTECODE_OFFSET_SECOND = 1U;

    const std::string bytecode;
    ConditionalBreakpoint breakpoint(
        BREAKPOINT_ID,
        []([[maybe_unused]] std::string_view file, [[maybe_unused]] std::string_view pattern) { return true; },
        BREAKPOINT_LINE, &bytecode);

    MockEvaluationEngine engine;
    engine.SetValue(TypedValue::U1(true));

    // Unresolved breakpoint never stops, no matter what the condition evaluates to
    ASSERT_FALSE(breakpoint.ShouldStopAt(
        PtLocation("test.abc", panda_file::File::EntityId(PANDA_FILE_ID), BYTECODE_OFFSET_FIRST), engine));
    ASSERT_FALSE(breakpoint.ShouldStopAt(
        PtLocation("test.abc", panda_file::File::EntityId(PANDA_FILE_ID), BYTECODE_OFFSET_SECOND), engine));
}

}  // namespace ark::tooling::inspector::test

// NOLINTEND
