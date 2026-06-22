/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <common.h>
#include <runtime/tooling/bigint_helper.h>

namespace arkinspector {
namespace test {

class BigIntDecimalConversionTest : public testing::Test {
};

TEST_F(BigIntDecimalConversionTest, ZeroValue)
{
    std::vector<uint32_t> bytes = {};
    EXPECT_EQ(ark::tooling::BigIntBytesToDecimalString(bytes, 0), "0n");
}

TEST_F(BigIntDecimalConversionTest, SimpleValue)
{
    std::vector<uint32_t> bytes = {1};
    EXPECT_EQ(ark::tooling::BigIntBytesToDecimalString(bytes, 1), "1n");
}

TEST_F(BigIntDecimalConversionTest, SmallValue)
{
    std::vector<uint32_t> bytes = {42};
    EXPECT_EQ(ark::tooling::BigIntBytesToDecimalString(bytes, 1), "42");
}

TEST_F(BigIntDecimalConversionTest, LargeValue)
{
    std::vector<uint32_t> bytes = {4294967295};
    EXPECT_EQ(ark::tooling::BigIntBytesToDecimalString(bytes, 1), "4294967295n");
}

TEST_F(BigIntDecimalConversionTest, MultiWordValue)
{
    std::vector<uint32_t> bytes = {0, 1};
    EXPECT_EQ(ark::tooling::BigIntBytesToDecimalString(bytes, 1), "4294967296n");
}

TEST_F(BigIntDecimalConversionTest, TwoWordsValue)
{
    std::vector<uint32_t> bytes = {0, 2};
    EXPECT_EQ(ark::tooling::BigIntBytesToDecimalString(bytes, 1), "8589934592n");
}

TEST_F(BigIntDecimalConversionTest, NegativeValue)
{
    std::vector<uint32_t> bytes = {42};
    EXPECT_EQ(ark::tooling::BigIntBytesToDecimalString(bytes, -1), "-42n");
}

TEST_F(BigIntDecimalConversionTest, AddDecimalArrays)
{
    std::vector<int> lhs = {1, 2, 3};
    std::vector<int> rhs = {1, 2, 3};
    auto result = ark::tooling::AddDecimalArrays(lhs, rhs);
    EXPECT_EQ(ark::tooling::DecimalArrayToString(result), "246");
}

TEST_F(BigIntDecimalConversionTest, AddDecimalArraysWithCarry)
{
    std::vector<int> lhs = {1, 9, 9};
    std::vector<int> rhs = {1};
    auto result = ark::tooling::AddDecimalArrays(lhs, rhs);
    EXPECT_EQ(ark::tooling::DecimalArrayToString(result), "200");
}

TEST_F(BigIntDecimalConversionTest, MultiplyDecimalArrays)
{
    std::vector<int> lhs = {1, 2};
    std::vector<int> rhs = {3, 4};
    auto result = ark::tooling::MultiplyDecimalArrayByBigInt(lhs, rhs);
    EXPECT_EQ(ark::tooling::DecimalArrayToString(result), "408");
}

TEST_F(BigIntDecimalConversionTest, MultiplyByPowerOfTwo)
{
    const int mask[] = {4, 2, 9, 4, 9, 6, 7, 2, 9, 6};
    std::vector<int> maskVec(std::begin(mask), std::end(mask));
    std::vector<int> digits = {1};
    auto result = ark::tooling::MultiplyDecimalArrayByBigInt(digits, maskVec);
    EXPECT_EQ(ark::tooling::DecimalArrayToString(result), "4294967296");
}

} // namespace arkinspector::test
} // namespace arkinspector