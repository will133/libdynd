//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cmath>

#include "inc_gtest.hpp"

#include <dynd/types/fixed_string_type.hpp>
#include <dynd/types/date_type.hpp>
#include <dynd/func/arrfunc.hpp>
#include <dynd/func/apply.hpp>
#include <dynd/kernels/assignment_kernels.hpp>
#include <dynd/kernels/expr_kernel_generator.hpp>
#include <dynd/func/elwise.hpp>
#include <dynd/func/take.hpp>
#include <dynd/func/call_callable.hpp>
#include <dynd/array.hpp>

using namespace std;
using namespace dynd;

TEST(ArrFunc, Assignment)
{
  // Create an arrfunc for converting string to int
  nd::arrfunc af = make_arrfunc_from_assignment(
      ndt::make_type<int>(), ndt::make_fixed_string(16), assign_error_default);
  // Validate that its types, etc are set right
  ASSERT_EQ(1, af.get_type()->get_narg());
  ASSERT_EQ(ndt::make_type<int>(), af.get_type()->get_return_type());
  ASSERT_EQ(ndt::make_fixed_string(16), af.get_type()->get_pos_type(0));

  const char *src_arrmeta[1] = {NULL};

  // Instantiate a single ckernel
  ckernel_builder<kernel_request_host> ckb;
  af.get()->instantiate(
      af.get(), af.get_type(), NULL, &ckb, 0, af.get_type()->get_return_type(), NULL,
      af.get_type()->get_npos(), af.get_type()->get_pos_types_raw(), src_arrmeta, kernel_request_single,
      &eval::default_eval_context, nd::array(), std::map<nd::string, ndt::type>());
  int int_out = 0;
  char str_in[16] = "3251";
  const char *str_in_ptr = str_in;
  expr_single_t usngo = ckb.get()->get_function<expr_single_t>();
  usngo(reinterpret_cast<char *>(&int_out), const_cast<char **>(&str_in_ptr),
        ckb.get());
  EXPECT_EQ(3251, int_out);

  // Instantiate a strided ckernel
  ckb.reset();
  af.get()->instantiate(
      af.get(), af.get_type(), NULL, &ckb, 0, af.get_type()->get_return_type(), NULL,
      af.get_type()->get_npos(), af.get_type()->get_pos_types_raw(), src_arrmeta, kernel_request_strided,
      &eval::default_eval_context, nd::array(), std::map<nd::string, ndt::type>());
  int ints_out[3] = {0, 0, 0};
  char strs_in[3][16] = {"123", "4567", "891029"};
  const char *strs_in_ptr = strs_in[0];
  expr_strided_t ustro = ckb.get()->get_function<expr_strided_t>();
  intptr_t strs_in_stride = sizeof(strs_in[0]);
  ustro(reinterpret_cast<char *>(&ints_out), sizeof(int),
        const_cast<char **>(&strs_in_ptr), &strs_in_stride, 3, ckb.get());
  EXPECT_EQ(123, ints_out[0]);
  EXPECT_EQ(4567, ints_out[1]);
  EXPECT_EQ(891029, ints_out[2]);
}

static double func(int x, double y) { return 2.0 * x + y; }

TEST(ArrFunc, Construction)
{
  nd::arrfunc af0 = nd::functional::apply(&func);
  EXPECT_EQ(4.5, af0(1, 2.5).as<double>());

  nd::arrfunc af1 = nd::functional::apply(&func, "y");
  EXPECT_EQ(4.5, af1(1, kwds("y", 2.5)).as<double>());

  nd::arrfunc af2 = nd::functional::apply([](int x, int y) { return x - y; });
  EXPECT_EQ(-4, af2(3, 7).as<int>());

  nd::arrfunc af3 =
      nd::functional::apply([](int x, int y) { return x - y; }, "y");
  EXPECT_EQ(-4, af3(3, kwds("y", 7)).as<int>());
}

TEST(ArrFunc, CallOperator)
{
  nd::arrfunc af = nd::functional::apply(&func);
  // Calling with positional arguments
  EXPECT_EQ(4.5, af(1, 2.5).as<double>());
  EXPECT_EQ(7.5, af(2, 3.5).as<double>());
  // Wrong number of positional argumetns
  EXPECT_THROW(af(2), invalid_argument);
  EXPECT_THROW(af(2, 3.5, 7), invalid_argument);
  // Extra keyword argument
  EXPECT_THROW(af(2, 3.5, kwds("x", 10)), invalid_argument);

  af = nd::functional::apply(&func, "x");
  // Calling with positional and keyword arguments
  EXPECT_EQ(4.5, af(1, kwds("x", 2.5)).as<double>());
  EXPECT_EQ(7.5, af(2, kwds("x", 3.5)).as<double>());
  // Wrong number of positional/keyword arguments
  EXPECT_THROW(af(2), invalid_argument);
  EXPECT_THROW(af(2, 3.5), invalid_argument);
  EXPECT_THROW(af(2, 3.5, 7), invalid_argument);
  // Extra/wrong keyword argument
  EXPECT_THROW(af(2, kwds("y", 3.5)), invalid_argument);
  EXPECT_THROW(af(2, kwds("x", 10, "y", 20)), invalid_argument);
  EXPECT_THROW(af(2, 3.5, kwds("x", 10, "y", 20)), invalid_argument);

  af = nd::functional::apply([]() { return 10; });
  // Calling with no arguments
  EXPECT_EQ(10, af().as<int>());
  // Calling with empty keyword arguments
  EXPECT_EQ(10, af(kwds()).as<int>());
  // Wrong number of positional/keyword arguments
  EXPECT_THROW(af(2), invalid_argument);
  EXPECT_THROW(af(kwds("y", 3.5)), invalid_argument);
}

TEST(Arrfunc, DynamicCall)
{
  nd::arrfunc af;

  nd::array values[3] = {7, 2.5, 5};
  const char *names[3] = {"x", "y", "z"};

  af = nd::functional::apply([](int x, double y, int z) { return 2 * x - y + 3 * z; });
  EXPECT_EQ(26.5, af(3, values).as<double>());

  af = nd::functional::apply([](int x, double y, int z) { return 2 * x - y + 3 * z; }, "z");
  EXPECT_EQ(26.5, af(2, values, kwds(1, names + 2, values + 2)).as<double>());

  af = nd::functional::apply([](int x, double y, int z) { return 2 * x - y + 3 * z; }, "y", "z");
  EXPECT_EQ(26.5, af(1, values, kwds(2, names + 1, values + 1)).as<double>());

  af = nd::functional::apply([](int x, double y, int z) { return 2 * x - y + 3 * z; }, "x", "y", "z");
  EXPECT_EQ(26.5, af(kwds(3, names, values)).as<double>());
}

TEST(Arrfunc, DecomposedDynamicCall)
{
  nd::arrfunc af;

  nd::array values[3] = {7, 2.5, 5};
  ndt::type types[3] = {values[0].get_type(), values[1].get_type(),
                        values[2].get_type()};
  const char *const arrmetas[3] = {values[0].get_arrmeta(),
                                   values[1].get_arrmeta(),
                                   values[2].get_arrmeta()};
  char *const datas[3] = {values[0].get_ndo()->m_data_pointer,
                          values[1].get_ndo()->m_data_pointer,
                          values[2].get_ndo()->m_data_pointer};
  const char *names[3] = {"x", "y", "z"};

  af = nd::functional::apply(
      [](int x, double y, int z) { return 2 * x - y + 3 * z; });
  EXPECT_EQ(26.5, af(3, types, arrmetas, datas).as<double>());

  af = nd::functional::apply(
      [](int x, double y, int z) { return 2 * x - y + 3 * z; }, "z");
  EXPECT_EQ(26.5, af(2, types, arrmetas, datas, kwds(1, names + 2, values + 2))
                      .as<double>());

  af = nd::functional::apply(
      [](int x, double y, int z) { return 2 * x - y + 3 * z; }, "y", "z");
  EXPECT_EQ(26.5, af(1, types, arrmetas, datas, kwds(2, names + 1, values + 1))
                      .as<double>());

  af = nd::functional::apply(
      [](int x, double y, int z) { return 2 * x - y + 3 * z; }, "x", "y", "z");
  EXPECT_EQ(26.5, af(kwds(3, names, values)).as<double>());
}

TEST(ArrFunc, KeywordParsing)
{
  nd::arrfunc af0 =
      nd::functional::apply([](int x, int y) { return x + y; }, "y");
  EXPECT_EQ(5, af0(1, kwds("y", 4)).as<int>());
  EXPECT_THROW(af0(1, kwds("z", 4)).as<int>(), std::invalid_argument);
  EXPECT_THROW(af0(1, kwds("Y", 4)).as<int>(), std::invalid_argument);
  EXPECT_THROW(af0(1, kwds("y", 2.5)).as<int>(), std::invalid_argument);
  EXPECT_THROW(af0(1, kwds("y", 4, "y", 2.5)).as<int>(), std::invalid_argument);
}

/*
TEST(ArrFunc, Option)
{
  struct callable {
    int operator()(int x, int y) { return x + y; }

    static void
    resolve_option_vals(const arrfunc_type_data *DYND_UNUSED(self),
                        const arrfunc_type *DYND_UNUSED(self_tp),
                        intptr_t DYND_UNUSED(nsrc),
                        const ndt::type *DYND_UNUSED(src_tp), nd::array &kwds,
                        const std::map<nd::string, ndt::type> &DYND_UNUSED(tp_vars))
    {
      nd::array x = kwds.p("x");
      if (x.is_missing()) {
        x.vals() = 4;
      }
    }
  };

  nd::arrfunc af = nd::functional::apply(callable(), "x");
  EXPECT_EQ(5, af(1, kwds("x", 4)).as<int>());

  af.set_as_option(&callable::resolve_option_vals, "x");
  EXPECT_EQ(6, af(1, kwds("x", 5)).as<int>());
  EXPECT_EQ(5, af(1).as<int>());
}
*/

TEST(ArrFunc, Assignment_CallInterface)
{
  // Test with the unary operation prototype
  nd::arrfunc af = make_arrfunc_from_assignment(
      ndt::make_type<int>(), ndt::make_string(), assign_error_default);

  // Call it through the call() interface
  nd::array b = af("12345678");
  EXPECT_EQ(ndt::make_type<int>(), b.get_type());
  EXPECT_EQ(12345678, b.as<int>());

  // Call it with some incompatible arguments
  EXPECT_THROW(af(12345), invalid_argument);
  EXPECT_THROW(af(false), invalid_argument);

  // Test with the expr operation prototype
  af = make_arrfunc_from_assignment(ndt::make_type<int>(), ndt::make_string(),
                                    assign_error_default);

  // Call it through the call() interface
  b = af("12345678");
  EXPECT_EQ(ndt::make_type<int>(), b.get_type());
  EXPECT_EQ(12345678, b.as<int>());

  // Call it with some incompatible arguments
  EXPECT_THROW(af(12345), invalid_argument);
  EXPECT_THROW(af(false), invalid_argument);
}

TEST(ArrFunc, Property)
{
  // Create an arrfunc for getting the year from a date
  nd::arrfunc af = make_arrfunc_from_property(ndt::make_date(), "year");
  // Validate that its types, etc are set right
  ASSERT_EQ(1, af.get_type()->get_narg());
  ASSERT_EQ(ndt::make_type<int>(), af.get_type()->get_return_type());
  ASSERT_EQ(ndt::make_date(), af.get_type()->get_pos_type(0));

  const char *src_arrmeta[1] = {NULL};

  // Instantiate a single ckernel
  ckernel_builder<kernel_request_host> ckb;
  af.get()->instantiate(
      af.get(), af.get_type(), NULL, &ckb, 0, af.get_type()->get_return_type(), NULL,
      af.get_type()->get_npos(), af.get_type()->get_pos_types_raw(), src_arrmeta, kernel_request_single,
      &eval::default_eval_context, nd::array(), std::map<nd::string, ndt::type>());
  int int_out = 0;
  int date_in = date_ymd::to_days(2013, 12, 30);
  const char *date_in_ptr = reinterpret_cast<const char *>(&date_in);
  expr_single_t usngo = ckb.get()->get_function<expr_single_t>();
  usngo(reinterpret_cast<char *>(&int_out), const_cast<char **>(&date_in_ptr),
        ckb.get());
  EXPECT_EQ(2013, int_out);
}

TEST(ArrFunc, AssignmentAsExpr)
{
  // Create an arrfunc for converting string to int
  nd::arrfunc af = make_arrfunc_from_assignment(
      ndt::make_type<int>(), ndt::make_fixed_string(16), assign_error_default);
  // Validate that its types, etc are set right
  ASSERT_EQ(1, af.get_type()->get_narg());
  ASSERT_EQ(ndt::make_type<int>(), af.get_type()->get_return_type());
  ASSERT_EQ(ndt::make_fixed_string(16), af.get_type()->get_pos_type(0));

  const char *src_arrmeta[1] = {NULL};

  // Instantiate a single ckernel
  ckernel_builder<kernel_request_host> ckb;
  af.get()->instantiate(
      af.get(), af.get_type(), NULL, &ckb, 0, af.get_type()->get_return_type(), NULL,
      af.get_type()->get_npos(), af.get_type()->get_pos_types_raw(), src_arrmeta, kernel_request_single,
      &eval::default_eval_context, nd::array(), std::map<nd::string, ndt::type>());
  int int_out = 0;
  char str_in[16] = "3251";
  char *str_in_ptr = str_in;
  expr_single_t usngo = ckb.get()->get_function<expr_single_t>();
  usngo(reinterpret_cast<char *>(&int_out), &str_in_ptr, ckb.get());
  EXPECT_EQ(3251, int_out);

  // Instantiate a strided ckernel
  ckb.reset();
  af.get()->instantiate(
      af.get(), af.get_type(), NULL, &ckb, 0, af.get_type()->get_return_type(), NULL,
      af.get_type()->get_npos(), af.get_type()->get_pos_types_raw(), src_arrmeta, kernel_request_strided,
      &eval::default_eval_context, nd::array(), std::map<nd::string, ndt::type>());
  int ints_out[3] = {0, 0, 0};
  char strs_in[3][16] = {"123", "4567", "891029"};
  char *strs_in_ptr = strs_in[0];
  intptr_t strs_in_stride = 16;
  expr_strided_t ustro = ckb.get()->get_function<expr_strided_t>();
  ustro(reinterpret_cast<char *>(&ints_out), sizeof(int), &strs_in_ptr,
        &strs_in_stride, 3, ckb.get());
  EXPECT_EQ(123, ints_out[0]);
  EXPECT_EQ(4567, ints_out[1]);
  EXPECT_EQ(891029, ints_out[2]);
}

/*
// TODO Reenable once there's a convenient way to make the binary arrfunc
TEST(ArrFunc, Expr) {
    arrfunc_type_data af;
    // Create an arrfunc for adding two ints
    ndt::type add_ints_type = (nd::array((int)0) +
nd::array((int)0)).get_type();
    make_arrfunc_from_assignment(
                    ndt::make_type<int>(), add_ints_type,
                    expr_operation_funcproto, assign_error_default, af);
    // Validate that its types, etc are set right
    ASSERT_EQ(expr_operation_funcproto, (arrfunc_proto_t)af.ckernel_funcproto);
    ASSERT_EQ(2, af.get_narg());
    ASSERT_EQ(ndt::make_type<int>(), af.get_return_type());
    ASSERT_EQ(ndt::make_type<int>(), af.get_arg_type(0));
    ASSERT_EQ(ndt::make_type<int>(), af.get_arg_type(1));

    const char *src_arrmeta[2] = {NULL, NULL};

    // Instantiate a single ckernel
    ckernel_builder ckb;
    af.instantiate(&af, &ckb, 0, af.get_return_type(), NULL,
                        af.get_arg_types(), src_arrmeta,
                        kernel_request_single, &eval::default_eval_context);
    int int_out = 0;
    int int_in1 = 1, int_in2 = 3;
    char *int_in_ptr[2] = {reinterpret_cast<char *>(&int_in1),
                        reinterpret_cast<char *>(&int_in2)};
    expr_single_t usngo = ckb.get()->get_function<expr_single_t>();
    usngo(reinterpret_cast<char *>(&int_out), int_in_ptr, ckb.get());
    EXPECT_EQ(4, int_out);

    // Instantiate a strided ckernel
    ckb.reset();
    af.instantiate(&af, &ckb, 0, af.get_return_type(), NULL,
                        af.get_arg_types(), src_arrmeta,
                        kernel_request_strided, &eval::default_eval_context);
    int ints_out[3] = {0, 0, 0};
    int ints_in1[3] = {1,2,3}, ints_in2[3] = {5,-210,1234};
    char *ints_in_ptr[2] = {reinterpret_cast<char *>(&ints_in1),
                        reinterpret_cast<char *>(&ints_in2)};
    intptr_t ints_in_strides[2] = {sizeof(int), sizeof(int)};
    expr_strided_t ustro = ckb.get()->get_function<expr_strided_t>();
    ustro(reinterpret_cast<char *>(ints_out), sizeof(int),
                    ints_in_ptr, ints_in_strides, 3, ckb.get());
    EXPECT_EQ(6, ints_out[0]);
    EXPECT_EQ(-208, ints_out[1]);
    EXPECT_EQ(1237, ints_out[2]);
}
*/