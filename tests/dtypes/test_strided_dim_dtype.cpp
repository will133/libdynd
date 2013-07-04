
// Copyright (C) 2011-13 Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <iostream>
#include <sstream>
#include <stdexcept>
#include "inc_gtest.hpp"

#include <dynd/dtype_assign.hpp>
#include <dynd/dtypes/tuple_dtype.hpp>
#include <dynd/dtypes/strided_dim_dtype.hpp>
#include <dynd/dtypes/convert_dtype.hpp>
#include <dynd/exceptions.hpp>
#include <dynd/array.hpp>
#include <dynd/kernels/assignment_kernels.hpp>

using namespace std;
using namespace dynd;

TEST(StridedArrayDType, Basic) {
    dtype d = make_strided_dim_dtype(make_dtype<int32_t>());

    EXPECT_EQ(make_dtype<int32_t>(), d.p("element_dtype").as<dtype>());
}

TEST(StridedArrayDType, ReplaceScalarTypes) {
    dtype dafloat, dadouble, daint32;
    dafloat = make_strided_dim_dtype(make_dtype<float>());
    dadouble = make_strided_dim_dtype(make_dtype<double>());

    EXPECT_EQ(make_strided_dim_dtype(make_convert_dtype<float, double>()),
            dadouble.with_replaced_scalar_types(make_dtype<float>()));

    // Two dimensional array
    dafloat = make_strided_dim_dtype(dafloat);
    dadouble = make_strided_dim_dtype(dadouble);

    EXPECT_EQ(make_strided_dim_dtype(make_strided_dim_dtype(make_convert_dtype<double, float>())),
            dafloat.with_replaced_scalar_types(make_dtype<double>()));
}

TEST(StridedArrayDType, DTypeAt) {
    dtype dfloat = make_dtype<float>();
    dtype darr1 = make_strided_dim_dtype(dfloat);
    dtype darr2 = make_strided_dim_dtype(darr1);
    dtype dtest;

    // indexing into a dtype with a slice produces another
    // strided array, so the dtype is unchanged.
    EXPECT_EQ(darr1, darr1.at(1 <= irange() < 3));
    EXPECT_EQ(darr2, darr2.at(1 <= irange() < 3));
    EXPECT_EQ(darr2, darr2.at(1 <= irange() < 3, irange() < 2));

    // Even if it's just one element, a slice still produces another array
    EXPECT_EQ(darr1, darr1.at(1 <= irange() <= 1));
    EXPECT_EQ(darr2, darr2.at(1 <= irange() <= 1));
    EXPECT_EQ(darr2, darr2.at(1 <= irange() <= 1, 2 <= irange() <= 2));

    // Indexing with an integer collapses a dimension
    EXPECT_EQ(dfloat, darr1.at(1));
    EXPECT_EQ(darr1, darr2.at(1));
    EXPECT_EQ(darr1, darr2.at(1 <= irange() <= 1, 1));
    EXPECT_EQ(dfloat, darr2.at(2, 1));

    // Should get an exception with too many indices
    EXPECT_THROW(dfloat.at(1), too_many_indices);
    EXPECT_THROW(darr1.at(1, 2), too_many_indices);
    EXPECT_THROW(darr2.at(1, 2, 3), too_many_indices);
}

TEST(StridedArrayDType, IsExpression) {
    dtype dfloat = make_dtype<float>();
    dtype darr1 = make_strided_dim_dtype(dfloat);
    dtype darr2 = make_strided_dim_dtype(darr1);

    EXPECT_FALSE(darr1.is_expression());
    EXPECT_FALSE(darr2.is_expression());

    dfloat = make_convert_dtype(make_dtype<double>(), dfloat);
    darr1 = make_strided_dim_dtype(dfloat);
    darr2 = make_strided_dim_dtype(darr1);

    EXPECT_TRUE(darr1.is_expression());
    EXPECT_TRUE(darr2.is_expression());
}

TEST(StridedArrayDType, AssignKernel) {
    nd::array a, b;
    assignment_kernel k;
    int vals_int[] = {3,5,7};

    // Assignment scalar -> strided array
    a = vals_int;
    b = 9.0;
    EXPECT_EQ(strided_dim_type_id, a.get_dtype().get_type_id());
    make_assignment_kernel(&k, 0, a.get_dtype(), a.get_ndo_meta(),
                    b.get_dtype(), b.get_ndo_meta(),
                    kernel_request_single, assign_error_default, &eval::default_eval_context);
    k(a.get_readwrite_originptr(), b.get_readonly_originptr());
    EXPECT_EQ(9, a.at(0).as<int>());
    EXPECT_EQ(9, a.at(1).as<int>());
    EXPECT_EQ(9, a.at(2).as<int>());
    k.reset();

    // Assignment strided array -> strided array
    a = nd::make_strided_array(3, make_dtype<float>());
    a.vals() = 0;
    b = vals_int;
    EXPECT_EQ(strided_dim_type_id, a.get_dtype().get_type_id());
    EXPECT_EQ(strided_dim_type_id, b.get_dtype().get_type_id());
    make_assignment_kernel(&k, 0, a.get_dtype(), a.get_ndo_meta(),
                    b.get_dtype(), b.get_ndo_meta(),
                    kernel_request_single, assign_error_default, &eval::default_eval_context);
    k(a.get_readwrite_originptr(), b.get_readonly_originptr());
    EXPECT_EQ(3, a.at(0).as<int>());
    EXPECT_EQ(5, a.at(1).as<int>());
    EXPECT_EQ(7, a.at(2).as<int>());
    k.reset();

    // Assignment strided array -> scalar
    a = 9.0;
    b = vals_int;
    EXPECT_EQ(strided_dim_type_id, b.get_dtype().get_type_id());
    EXPECT_THROW(make_assignment_kernel(&k, 0, a.get_dtype(), a.get_ndo_meta(),
                    b.get_dtype(), b.get_ndo_meta(),
                    kernel_request_single, assign_error_default, &eval::default_eval_context),
                broadcast_error);
}
