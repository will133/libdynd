//
// Copyright (C) 2011-14 Irwin Zaid, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include "inc_gtest.hpp"

#include <dynd/func/functor_arrfunc.hpp>
#include <dynd/types/struct_type.hpp>

using namespace std;
using namespace dynd;

struct func_aux_buffer : aux_buffer {
    int val;

    static void unpack(func_aux_buffer &dst, const nd::array &src) {
        dst.val = src.p("val").as<int>();
    }
};

struct func_thread_aux_buffer : thread_aux_buffer {
    int val;

    func_thread_aux_buffer() : val(9) {
    }

    func_thread_aux_buffer(func_aux_buffer *aux) : val(aux->val + 3) {
    }
};

int ret_func_with_aux(int src, func_aux_buffer *aux) {
    return src + aux->val;
}

void ref_func_with_aux(int &dst, int src, func_aux_buffer *aux) {
    dst = src + aux->val;
}

/*
TODO: Need thread_aux to reenable.

void func_with_thread_aux(int &dst, int src, func_thread_aux_buffer *thread_aux) {
    dst = src + thread_aux->val;
}

void func_with_aux_and_thread_aux(int &dst, int src, func_aux_buffer *aux, func_thread_aux_buffer *thread_aux) {
    dst = src + aux->val + thread_aux->val;
}
*/

TEST(Buffer, Aux) {
    nd::array aux = nd::empty(ndt::make_struct(ndt::make_type<int>(), "val"));
    aux.p("val").vals() = 7;

    nd::arrfunc af = nd::make_functor_arrfunc(ret_func_with_aux);
    EXPECT_EQ(12, af(5, aux, true).as<int>());

    af = nd::make_functor_arrfunc(ref_func_with_aux);
    EXPECT_EQ(12, af(5, aux, true).as<int>());

/*
TODO: Need thread_aux to reenable.

    af = nd::make_functor_arrfunc(func_with_thread_aux);
    EXPECT_EQ(14, af(5).as<int>());

    af = nd::make_functor_arrfunc(func_with_aux_and_thread_aux);
    EXPECT_EQ(22, af(5, &aux).as<int>());
*/
}