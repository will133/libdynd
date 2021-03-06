//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cmath>

#include "inc_gtest.hpp"
#include "dynd_assertions.hpp"

#include <dynd/array.hpp>
#include <dynd/func/apply.hpp>
#include <dynd/func/elwise.hpp>
#include <dynd/types/fixed_dim_type.hpp>

using namespace std;
using namespace dynd;

template <typename T>
class FunctorArrfunc_FuncRetRes : public ::testing::Test {
};
template <typename T>
class FunctorArrfunc_CallRetRes : public ::testing::Test {
};
template <typename T>
class FunctorArrfunc_MethRetRes : public ::testing::Test {
};

TYPED_TEST_CASE_P(FunctorArrfunc_FuncRetRes);
TYPED_TEST_CASE_P(FunctorArrfunc_CallRetRes);
TYPED_TEST_CASE_P(FunctorArrfunc_MethRetRes);

template <typename T>
int func0(T x, const T &y) {
    return static_cast<int>(2 * (x - y));
}
template <typename T>
T func1(const T (&x)[3]) {
    return x[0] + x[1] + x[2];
}
template <typename T>
T func2(const T (&x)[3], const T (&y)[3]) {
    return static_cast<T>(x[0] * y[0] + x[1] * y[1] + x[2] * y[2]);
}
template <typename T>
T func3(const T (&x)[2][3]) {
    return x[0][0] + x[0][1] + x[1][2];
}
template <typename T>
void func0(int &res, T x, const T &y) {
    res = func0(x, y);
}
template <typename T>
void func1(T &res, const T (&x)[3]) {
    res = func1(x);
}
template <typename T>
void func2(T &res, const T (&x)[3], const T (&y)[3]) {
    res = func2(x, y);
}
template <typename T>
void func3(T &res, const T (&x)[2][3]) {
    res = func3(x);
}
template <typename T>
void func4(T (&res)[2], T x, T y) {
    res[0] = y;
    res[1] = x;
}
template <typename T>
void func5(T (&res)[3], const T(&x)[3][3], const T(&y)[3]) {
    res[0] = x[0][0] * y[0] + x[0][1] * y[1] + x[0][2] * y[2];
    res[1] = x[1][0] * y[0] + x[1][1] * y[1] + x[1][2] * y[2];
    res[2] = x[2][0] * y[0] + x[2][1] * y[1] + x[2][2] * y[2];
}
template <typename T>
void func6(double (&res)[2][2], T x) {
    res[0][0] = cos((double) x);
    res[0][1] = -sin((double) x);
    res[1][0] = sin((double) x);
    res[1][1] = cos((double) x);
}
template <typename T>
void func7(T (&res)[3][3], const T(&x)[3][3], const T(&y)[3][3]) {
    res[0][0] = x[0][0] * y[0][0] + x[0][1] * y[1][0] + x[0][2] * y[2][0];
    res[0][1] = x[0][0] * y[0][1] + x[0][1] * y[1][1] + x[0][2] * y[2][1];
    res[0][2] = x[0][0] * y[0][2] + x[0][1] * y[1][2] + x[0][2] * y[2][2];
    res[1][0] = x[1][0] * y[0][0] + x[1][1] * y[1][0] + x[1][2] * y[2][0];
    res[1][1] = x[1][0] * y[0][1] + x[1][1] * y[1][1] + x[1][2] * y[2][1];
    res[1][2] = x[1][0] * y[0][2] + x[1][1] * y[1][2] + x[1][2] * y[2][2];
    res[2][0] = x[2][0] * y[0][0] + x[2][1] * y[1][0] + x[2][2] * y[2][0];
    res[2][1] = x[2][0] * y[0][1] + x[2][1] * y[1][1] + x[2][2] * y[2][1];
    res[2][2] = x[2][0] * y[0][2] + x[2][1] * y[1][2] + x[2][2] * y[2][2];
}

template <typename T>
class Callable;

template <typename R, typename A0>
class Callable<R (*)(A0)> {
private:
    R (*m_func)(A0);
public:
    Callable(R (*func)(A0)) : m_func(func) {}
    R operator ()(A0 a0) const {
        return (*m_func)(a0);
    }
};
template <typename R, typename A0, typename A1>
class Callable<R (*)(A0, A1)> {
private:
    R (*m_func)(A0, A1);
public:
    Callable(R (*func)(A0, A1)) : m_func(func) {}
    R operator ()(A0 a0, A1 a1) const {
        return (*m_func)(a0, a1);
    }
};
template <typename R, typename A0>
class Callable<void (*)(R &, A0)> {
private:
    void (*m_func)(R &, A0);
public:
    Callable(void (*func)(R &, A0)) : m_func(func) {}
    void operator ()(R &res, A0 a0) const {
        (*m_func)(res, a0);
    }
};
template <typename R, typename A0, typename A1>
class Callable<void (*)(R &, A0, A1)> {
private:
    void (*m_func)(R &, A0, A1);
public:
    Callable(void (*func)(R &, A0, A1)) : m_func(func) {}
    void operator ()(R &res, A0 a0, A1 a1) const {
        (*m_func)(res, a0, a1);
    }
};

TYPED_TEST_P(FunctorArrfunc_FuncRetRes, FuncRetRes) {
  nd::array res, a, b;
  nd::arrfunc af;

  a = static_cast<TypeParam>(10);
  b = static_cast<TypeParam>(20);

  af = nd::functional::apply(
      static_cast<int (*)(TypeParam, const TypeParam &)>(&func0));
  res = af(a, b);
  EXPECT_EQ(-20, res.as<int>());

  TypeParam vals1[2][3] = {{0, 1, 2}, {3, 4, 5}};

  a = nd::empty(ndt::make_fixed_dim(3, ndt::make_type<TypeParam>()));

  a.vals() = vals1[0];
  af = nd::functional::apply(
      static_cast<TypeParam (*)(const TypeParam(&)[3])>(&func1));
  res = af(a);
  EXPECT_EQ(ndt::make_type<TypeParam>(), res.get_type());
  EXPECT_EQ(3, res.as<TypeParam>());

  a.vals() = vals1[1];
  af = nd::functional::apply(
      static_cast<TypeParam (*)(const TypeParam(&)[3])>(&func1));
  res = af(a);
  EXPECT_EQ(ndt::make_type<TypeParam>(), res.get_type());
  EXPECT_EQ(12, res.as<TypeParam>());

  b = nd::empty(ndt::make_fixed_dim(3, ndt::make_type<TypeParam>()));

  a.vals() = vals1[0];
  b.vals() = vals1[1];
  af = nd::functional::apply(
      static_cast<TypeParam (*)(const TypeParam(&)[3], const TypeParam(&)[3])>(
          &func2));
  res = af(a, b);
  EXPECT_EQ(ndt::make_type<TypeParam>(), res.get_type());
  EXPECT_EQ(14, res.as<TypeParam>());

  a = nd::empty(ndt::fixed_dim_from_array<TypeParam[2][3]>::make());

  a.vals() = vals1;
  af = nd::functional::apply(
      static_cast<TypeParam (*)(const TypeParam(&)[2][3])>(&func3));
  res = af(a);
  EXPECT_EQ(ndt::make_type<TypeParam>(), res.get_type());
  EXPECT_EQ(6, res.as<TypeParam>());
}

TYPED_TEST_P(FunctorArrfunc_CallRetRes, CallRetRes) {
  typedef Callable<int (*)(TypeParam, const TypeParam &)> Callable0;
  typedef Callable<TypeParam (*)(const TypeParam(&)[3])> Callable1;
  typedef Callable<TypeParam (*)(const TypeParam(&)[3], const TypeParam(&)[3])>
    Callable2;
  typedef Callable<TypeParam (*)(const TypeParam(&)[2][3])> Callable3;

  nd::array res, a, b;
  nd::arrfunc af;

  a = static_cast<TypeParam>(10);
  b = static_cast<TypeParam>(20);

  af = nd::functional::apply(Callable0(&func0));
  res = af(a, b);
  EXPECT_EQ(-20, res.as<int>());

  TypeParam avals0[2][3] = {{0, 1, 2}, {5, 6, 7}};
  TypeParam bvals0[3] = {5, 2, 4};

  a = avals0;
  b = bvals0;
  af = nd::functional::apply(Callable0(&func0));
  af = nd::functional::elwise(af);
  res = af(a, b);
  EXPECT_EQ(ndt::type("2 * 3 * int"), res.get_type());
  EXPECT_JSON_EQ_ARR("[[-10,-2,-4], [0,8,6]]", res);

  TypeParam vals1[2][3] = {{0, 1, 2}, {3, 4, 5}};

  a = nd::empty(ndt::make_fixed_dim(3, ndt::make_type<TypeParam>()));

  a.vals() = vals1[0];
  af = nd::functional::apply(Callable1(&func1));
  res = af(a);
  EXPECT_EQ(ndt::make_type<TypeParam>(), res.get_type());
  EXPECT_EQ(3, res.as<TypeParam>());

  a.vals() = vals1[1];
  af = nd::functional::apply(Callable1(&func1));
  res = af(a);
  EXPECT_EQ(ndt::make_type<TypeParam>(), res.get_type());
  EXPECT_EQ(12, res.as<TypeParam>());

  b = nd::empty(ndt::make_fixed_dim(3, ndt::make_type<TypeParam>()));

  a.vals() = vals1[0];
  b.vals() = vals1[1];
  af = nd::functional::apply(Callable2(&func2));
  res = af(a, b);
  EXPECT_EQ(ndt::make_type<TypeParam>(), res.get_type());
  EXPECT_EQ(14, res.as<TypeParam>());

  a = nd::empty(ndt::fixed_dim_from_array<TypeParam[2][3]>::make());

  a.vals() = vals1;
  af = nd::functional::apply(Callable3(&func3));
  res = af(a);
  EXPECT_EQ(ndt::make_type<TypeParam>(), res.get_type());
  EXPECT_EQ(6, res.as<TypeParam>());
}

// TODO: Figure out what's up with clang and reenable it. It appears
//       that clang creates lambda functions that are not copy-constructable.
#if defined(DYND_CXX_LAMBDAS) && !defined(__clang__)
TEST(FunctorArrfunc, LambdaFunc) {
  nd::array a, b, res;
  nd::arrfunc af;

  a = 100;
  b = 1.5;
  af = nd::functional::apply([](int x, double y) { return (float)(x + y); });
  res = af(a, b);
  EXPECT_EQ(ndt::make_type<float>(), res.get_type());
  EXPECT_EQ(101.5f, res.as<float>());

  double a_val[3] = {1.5, 2.0, 3.125};
  a = a_val;
  b = 3.25;
  af = nd::functional::apply([](double x, double y,
                                   int z) { return x * z + y; });
  af = nd::functional::elwise(af);
  res = af(a, b, 10);
  EXPECT_EQ(ndt::type("3 * float64"), res.get_type());
  EXPECT_JSON_EQ_ARR("[18.25,23.25,34.5]", res);
}
#endif

template <typename T>
class FuncWrapper;

template <typename R, typename A0>
class FuncWrapper<R (*)(A0)> {
private:
  R (*m_func)(A0);

public:
  FuncWrapper(R (*func)(A0)) : m_func(func) {}
  R meth(A0 a0) const { return (*m_func)(a0); }
  R operator()(A0 a0) const { return (*m_func)(a0); }
};
template <typename R, typename A0, typename A1>
class FuncWrapper<R (*)(A0, A1)> {
private:
  R (*m_func)(A0, A1);

public:
  FuncWrapper(R (*func)(A0, A1)) : m_func(func) {}
  R meth(A0 a0, A1 a1) const { return (*m_func)(a0, a1); }
  R operator()(A0 a0, A1 a1) const { return (*m_func)(a0, a1); }
};
template <typename R, typename A0>
class FuncWrapper<void (*)(R &, A0)> {
private:
  void (*m_func)(R &, A0);

public:
  FuncWrapper(void (*func)(R &, A0)) : m_func(func) {}
  void meth(R &res, A0 a0) const { (*m_func)(res, a0); }
  void operator()(R &res, A0 a0) const { (*m_func)(res, a0); }
};
template <typename R, typename A0, typename A1>
class FuncWrapper<void (*)(R &, A0, A1)> {
private:
  void (*m_func)(R &, A0, A1);

public:
  FuncWrapper(void (*func)(R &, A0, A1)) : m_func(func) {}
  void meth(R &res, A0 a0, A1 a1) const { (*m_func)(res, a0, a1); }
  void operator()(R &res, A0 a0, A1 a1) const { (*m_func)(res, a0, a1); }
};

TYPED_TEST_P(FunctorArrfunc_MethRetRes, MethRetRes) {
    typedef FuncWrapper<int (*)(TypeParam, const TypeParam &)> FuncWrapper0;
    typedef FuncWrapper<TypeParam (*)(const TypeParam (&)[3])> FuncWrapper1;
    typedef FuncWrapper<TypeParam (*)(const TypeParam (&)[3], const TypeParam (&)[3])> FuncWrapper2;
    typedef FuncWrapper<TypeParam (*)(const TypeParam (&)[2][3])> FuncWrapper3;

    nd::array res, a, b;
    nd::arrfunc af;

    a = static_cast<TypeParam>(10);
    b = static_cast<TypeParam>(20);

    af = nd::functional::apply(FuncWrapper0(&func0), &FuncWrapper0::meth);
    res = af(a, b);
    EXPECT_EQ(-20, res.as<int>());

    TypeParam avals0[2][3] = {{0, 1, 2}, {5, 6, 7}};
    TypeParam bvals0[3] = {5, 2, 4};

    a = avals0;
    b = bvals0;
    af = nd::functional::apply(FuncWrapper0(&func0), &FuncWrapper0::meth);
    af = nd::functional::elwise(af);
    res = af(a, b);
    EXPECT_EQ(ndt::type("2 * 3 * int"), res.get_type());
    EXPECT_JSON_EQ_ARR("[[-10,-2,-4], [0,8,6]]", res);

    TypeParam vals1[2][3] = {{0, 1, 2}, {3, 4, 5}};

    a = nd::empty(ndt::make_fixed_dim(3, ndt::make_type<TypeParam>()));

    a.vals() = vals1[0];
    af = nd::functional::apply(FuncWrapper1(&func1), &FuncWrapper1::meth);
    res = af(a);
    EXPECT_EQ(ndt::make_type<TypeParam>(), res.get_type());
    EXPECT_EQ(3, res.as<TypeParam>());

    a.vals() = vals1[1];
    af = nd::functional::apply(FuncWrapper1(&func1), &FuncWrapper1::meth);
    res = af(a);
    EXPECT_EQ(ndt::make_type<TypeParam>(), res.get_type());
    EXPECT_EQ(12, res.as<TypeParam>());

    b = nd::empty(ndt::make_fixed_dim(3, ndt::make_type<TypeParam>()));

    a.vals() = vals1[0];
    b.vals() = vals1[1];
    af = nd::functional::apply(FuncWrapper2(&func2), &FuncWrapper2::meth);
    res = af(a, b);
    EXPECT_EQ(ndt::make_type<TypeParam>(), res.get_type());
    EXPECT_EQ(14, res.as<TypeParam>());

    a = nd::empty(ndt::fixed_dim_from_array<TypeParam[2][3]>::make());

    a.vals() = vals1;
    af = nd::functional::apply(FuncWrapper3(&func3), &FuncWrapper3::meth);
    res = af(a);
    EXPECT_EQ(ndt::make_type<TypeParam>(), res.get_type());
    EXPECT_EQ(6, res.as<TypeParam>());
}

typedef ::testing::Types<int, float, long, double> test_types;

REGISTER_TYPED_TEST_CASE_P(FunctorArrfunc_FuncRetRes, FuncRetRes);
INSTANTIATE_TYPED_TEST_CASE_P(Builtin, FunctorArrfunc_FuncRetRes, test_types);

REGISTER_TYPED_TEST_CASE_P(FunctorArrfunc_CallRetRes, CallRetRes);
INSTANTIATE_TYPED_TEST_CASE_P(Builtin, FunctorArrfunc_CallRetRes, test_types);

/*
REGISTER_TYPED_TEST_CASE_P(FunctorArrfunc_MethRetRes, MethRetRes);
INSTANTIATE_TYPED_TEST_CASE_P(Builtin, FunctorArrfunc_MethRetRes, test_types);
*/