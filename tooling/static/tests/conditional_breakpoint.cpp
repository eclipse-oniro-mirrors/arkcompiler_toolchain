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

    static ark::MTManagedThread *thread_;
    static Class *objectClass_;
    static Class *arrayClass_;
};

ark::MTManagedThread *ConditionalBreakpointTest::thread_ = nullptr;
Class *ConditionalBreakpointTest::objectClass_ = nullptr;
Class *ConditionalBreakpointTest::arrayClass_ = nullptr;

TEST_F(ConditionalBreakpointTest, IsConditionTrueInvalidAndVoid)
{
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::Invalid()));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::Void()));
}

TEST_F(ConditionalBreakpointTest, IsConditionTrueBoolean)
{
    ASSERT_TRUE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U1(true)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U1(false)));
}

TEST_F(ConditionalBreakpointTest, IsConditionTrueIntegers)
{
    // Any integer never stops, even non-zero ones
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::I8(0)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::I8(-1)));

    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U8(0)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U8(1)));

    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::I16(0)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::I16(-1)));

    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U16(0)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U16(1)));

    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::I32(0)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::I32(-42)));

    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U32(0)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U32(42)));

    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::I64(0)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::I64(-42)));

    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U64(0)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::U64(42)));
}

TEST_F(ConditionalBreakpointTest, IsConditionTrueFloatingPoint)
{
    // Any floating-point number never stops
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F32(0.F)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F32(-0.F)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F32(F32_NAN)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F32(1.5F)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F32(-1.5F)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F32(F32_INF)));

    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F64(0.0)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F64(-0.0)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F64(F64_NAN)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F64(1.5)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F64(-1.5)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::F64(F64_INF)));
}

TEST_F(ConditionalBreakpointTest, IsConditionTrueTaggedSpecialValues)
{
    using TaggedValue = coretypes::TaggedValue;

    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::Tagged(TaggedValue(TaggedValue::VALUE_UNDEFINED))));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::Tagged(TaggedValue(TaggedValue::VALUE_NULL))));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::Tagged(TaggedValue(TaggedValue::VALUE_HOLE))));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::Tagged(TaggedValue(TaggedValue::VALUE_EXCEPTION))));

    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(TaggedValue::VALUE_UNDEFINED)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(TaggedValue::VALUE_NULL)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(TaggedValue::VALUE_HOLE)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(TaggedValue::VALUE_EXCEPTION)));
}

TEST_F(ConditionalBreakpointTest, IsConditionTrueTaggedPrimitives)
{
    using TaggedValue = coretypes::TaggedValue;

    ASSERT_TRUE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(true)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(false)));

    // Numbers never stop
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(0)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(1)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(-1)));

    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(0.0)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(-0.0)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(TaggedValue::VALUE_NAN)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(1.5)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TaggedValue(-1.5)));
}

TEST_F(ConditionalBreakpointTest, IsConditionTrueTaggedHeapObjects)
{
    auto *object = ObjectHeader::Create(objectClass_);
    ASSERT_NE(object, nullptr);

    // Any heap object never stops, unless it unwraps to a boxed boolean
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(coretypes::TaggedValue(object)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::Tagged(coretypes::TaggedValue(object))));
}

TEST_F(ConditionalBreakpointTest, IsConditionTrueReferences)
{
    // Any reference never stops
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(static_cast<ObjectHeader *>(nullptr)));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::Reference(nullptr)));

    auto *object = ObjectHeader::Create(objectClass_);
    ASSERT_NE(object, nullptr);
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(object));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::Reference(object)));

    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(CreateString("")));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(CreateString("true")));
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(TypedValue::Reference(CreateString("true"))));

    auto *emptyArray = coretypes::Array::Create(arrayClass_, 0);
    ASSERT_NE(emptyArray, nullptr);
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(emptyArray));

    auto *array = coretypes::Array::Create(arrayClass_, 5U);
    ASSERT_NE(array, nullptr);
    ASSERT_FALSE(ConditionalBreakpoint::IsConditionTrue(array));
}

TEST_F(ConditionalBreakpointTest, ShouldStopAtUnresolvedBreakpoint)
{
    const std::string bytecode;
    ConditionalBreakpoint breakpoint(1, [](std::string_view, std::string_view) { return true; }, 1, &bytecode);

    MockEvaluationEngine engine;
    engine.SetValue(TypedValue::U1(true));

    // Unresolved breakpoint never stops, no matter what the condition evaluates to
    ASSERT_FALSE(breakpoint.ShouldStopAt(PtLocation("test.abc", panda_file::File::EntityId(1), 0), engine));
    ASSERT_FALSE(breakpoint.ShouldStopAt(PtLocation("test.abc", panda_file::File::EntityId(1), 1), engine));
}

}  // namespace ark::tooling::inspector::test

// NOLINTEND
