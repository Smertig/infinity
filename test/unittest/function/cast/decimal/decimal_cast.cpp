//
// Created by jinhai on 22-12-22.
//

#include <gtest/gtest.h>
#include "base_test.h"
#include "common/column_vector/column_vector.h"
#include "common/types/value.h"
#include "main/logger.h"
#include "main/stats/global_resource_usage.h"
#include "main/infinity.h"
#include "function/cast/decimal_cast.h"


class DecimalCastTest : public BaseTest {
    void
    SetUp() override {
        infinity::GlobalResourceUsage::Init();
        infinity::Infinity::instance().Init();
    }

    void
    TearDown() override {
        infinity::Infinity::instance().UnInit();
        EXPECT_EQ(infinity::GlobalResourceUsage::GetObjectCount(), 0);
        EXPECT_EQ(infinity::GlobalResourceUsage::GetRawMemoryCount(), 0);
        infinity::GlobalResourceUsage::UnInit();
    }
};

TEST_F(DecimalCastTest, decimal_cast0) {
    using namespace infinity;

    // DecimalT to TinyIntT, throw exception
    {
        DecimalT source;
        TinyIntT target;
        EXPECT_THROW(DecimalTryCastToFixlen::Run(source, target), NotImplementException);
    }

    // DecimalT to SmallIntT, throw exception
    {
        DecimalT source;
        SmallIntT target;
        EXPECT_THROW(DecimalTryCastToFixlen::Run(source, target), NotImplementException);
    }

    // DecimalT to IntegerT, throw exception
    {
        DecimalT source;
        IntegerT target;
        EXPECT_THROW(DecimalTryCastToFixlen::Run(source, target), NotImplementException);
    }

    // DecimalT to BigIntT, throw exception
    {
        DecimalT source;
        BigIntT target;
        EXPECT_THROW(DecimalTryCastToFixlen::Run(source, target), NotImplementException);
    }

    // DecimalT to HugeIntT, throw exception
    {
        DecimalT source;
        HugeIntT target;
        EXPECT_THROW(DecimalTryCastToFixlen::Run(source, target), NotImplementException);
    }

    // DecimalT to FloatT, throw exception
    {
        DecimalT source;
        FloatT target;
        EXPECT_THROW(DecimalTryCastToFixlen::Run(source, target), NotImplementException);
    }

    // DecimalT to DoubleT, throw exception
    {
        DecimalT source;
        DoubleT target;
        EXPECT_THROW(DecimalTryCastToFixlen::Run(source, target), NotImplementException);
    }

    // DecimalT to DecimalT, throw exception
    {
        DecimalT source;
        DecimalT target;
        EXPECT_THROW(DecimalTryCastToFixlen::Run(source, target), FunctionException);
    }

    // DecimalT to VarcharT, throw exception
    {
        DecimalT source;
        VarcharT target;
        EXPECT_THROW(DecimalTryCastToVarlen::Run(source, target, nullptr), NotImplementException);
    }
}

TEST_F(DecimalCastTest, decimal_cast1) {
    using namespace infinity;

    // cast decimal column vector to tiny int column vector
    {
        DataType source_type(LogicalType::kDecimal);
        DataType target_type(LogicalType::kTinyInt);
        EXPECT_THROW(BindDecimalCast<DecimalT>(source_type, target_type), NotImplementException);
    }
    // cast decimal column vector to small int column vector
    {
        DataType source_type(LogicalType::kDecimal);
        DataType target_type(LogicalType::kSmallInt);
        EXPECT_THROW(BindDecimalCast<DecimalT>(source_type, target_type), NotImplementException);
    }
    // cast decimal column vector to integer column vector
    {
        DataType source_type(LogicalType::kDecimal);
        DataType target_type(LogicalType::kInteger);
        EXPECT_THROW(BindDecimalCast<DecimalT>(source_type, target_type), NotImplementException);
    }
    // cast decimal column vector to bigint column vector
    {
        DataType source_type(LogicalType::kDecimal);
        DataType target_type(LogicalType::kBigInt);
        EXPECT_THROW(BindDecimalCast<DecimalT>(source_type, target_type), NotImplementException);
    }
    // cast decimal column vector to hugeint column vector
    {
        DataType source_type(LogicalType::kDecimal);
        DataType target_type(LogicalType::kHugeInt);
        EXPECT_THROW(BindDecimalCast<DecimalT>(source_type, target_type), NotImplementException);
    }
    // cast decimal column vector to float column vector
    {
        DataType source_type(LogicalType::kDecimal);
        DataType target_type(LogicalType::kFloat);
        EXPECT_THROW(BindDecimalCast<DecimalT>(source_type, target_type), NotImplementException);
    }
    // cast decimal column vector to double column vector
    {
        DataType source_type(LogicalType::kDecimal);
        DataType target_type(LogicalType::kDouble);
        EXPECT_THROW(BindDecimalCast<DecimalT>(source_type, target_type), NotImplementException);
    }
    // cast decimal column vector to decimal column vector
    {
        DataType source_type(LogicalType::kDecimal);
        DataType target_type(LogicalType::kDecimal);
        EXPECT_THROW(BindDecimalCast<DecimalT>(source_type, target_type), TypeException);
    }
    // cast decimal column vector to varchar column vector
    {
        DataType source_type(LogicalType::kDecimal);
        DataType target_type(LogicalType::kVarchar);
        EXPECT_THROW(BindDecimalCast<DecimalT>(source_type, target_type), NotImplementException);
    }

    // cast decimal column vector to other type column vector
    {
        DataType source_type(LogicalType::kDecimal);
        DataType target_type(LogicalType::kTimestamp);
        EXPECT_THROW(BindDecimalCast<DecimalT>(source_type, target_type), TypeException);
    }
}