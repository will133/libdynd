//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/type.hpp>
#include <dynd/kernels/comparison_kernels.hpp>
#include "single_comparer_builtin.hpp"

using namespace std;
using namespace dynd;

size_t dynd::make_comparison_kernel(void *ckb, intptr_t ckb_offset,
                                    const ndt::type &src0_dt,
                                    const char *src0_arrmeta,
                                    const ndt::type &src1_dt,
                                    const char *src1_arrmeta,
                                    comparison_type_t comptype,
                                    const eval::eval_context *ectx)
{
    if (src0_dt.is_builtin()) {
        if (src1_dt.is_builtin()) {
            return make_builtin_type_comparison_kernel(ckb, ckb_offset,
                            src0_dt.get_type_id(), src1_dt.get_type_id(),
                            comptype);
        } else {
            return src1_dt.extended()->make_comparison_kernel(ckb, ckb_offset,
                            src0_dt, src0_arrmeta,
                            src1_dt, src1_arrmeta,
                            comptype, ectx);
        }
    } else {
        return src0_dt.extended()->make_comparison_kernel(ckb, ckb_offset,
                        src0_dt, src0_arrmeta,
                        src1_dt, src1_arrmeta,
                        comptype, ectx);
    }
}

static expr_predicate_t compare_kernel_table[builtin_type_id_count-2][builtin_type_id_count-2][7] =
{
#define INNER_LEVEL(src0_type, src1_type) { \
                &single_comparison_builtin<src0_type, src1_type>::sorting_less, \
                &single_comparison_builtin<src0_type, src1_type>::less, \
                &single_comparison_builtin<src0_type, src1_type>::less_equal, \
                &single_comparison_builtin<src0_type, src1_type>::equal, \
                &single_comparison_builtin<src0_type, src1_type>::not_equal, \
                &single_comparison_builtin<src0_type, src1_type>::greater_equal, \
                &single_comparison_builtin<src0_type, src1_type>::greater }
        
#define SRC0_TYPE_LEVEL(src0_type) { \
        INNER_LEVEL(src0_type, dynd_bool), \
        INNER_LEVEL(src0_type, int8_t), \
        INNER_LEVEL(src0_type, int16_t), \
        INNER_LEVEL(src0_type, int32_t), \
        INNER_LEVEL(src0_type, int64_t), \
        INNER_LEVEL(src0_type, dynd_int128), \
        INNER_LEVEL(src0_type, uint8_t), \
        INNER_LEVEL(src0_type, uint16_t), \
        INNER_LEVEL(src0_type, uint32_t), \
        INNER_LEVEL(src0_type, uint64_t), \
        INNER_LEVEL(src0_type, dynd_uint128), \
        INNER_LEVEL(src0_type, dynd_float16), \
        INNER_LEVEL(src0_type, float), \
        INNER_LEVEL(src0_type, double), \
        INNER_LEVEL(src0_type, dynd_float128), \
        INNER_LEVEL(src0_type, dynd::complex<float>), \
        INNER_LEVEL(src0_type, dynd::complex<double>) \
    }

    SRC0_TYPE_LEVEL(dynd_bool),
    SRC0_TYPE_LEVEL(int8_t),
    SRC0_TYPE_LEVEL(int16_t),
    SRC0_TYPE_LEVEL(int32_t),
    SRC0_TYPE_LEVEL(int64_t),
    SRC0_TYPE_LEVEL(dynd_int128),
    SRC0_TYPE_LEVEL(uint8_t),
    SRC0_TYPE_LEVEL(uint16_t),
    SRC0_TYPE_LEVEL(uint32_t),
    SRC0_TYPE_LEVEL(uint64_t),
    SRC0_TYPE_LEVEL(dynd_uint128),
    SRC0_TYPE_LEVEL(dynd_float16),
    SRC0_TYPE_LEVEL(float),
    SRC0_TYPE_LEVEL(double),
    SRC0_TYPE_LEVEL(dynd_float128),
    SRC0_TYPE_LEVEL(dynd::complex<float>),
    SRC0_TYPE_LEVEL(dynd::complex<double>)
#undef SRC0_TYPE_LEVEL
#undef INNER_LEVEL
};

size_t dynd::make_builtin_type_comparison_kernel(
                void *ckb, intptr_t ckb_offset,
                type_id_t src0_type_id, type_id_t src1_type_id,
                comparison_type_t comptype)
{
  // Do a table lookup for the built-in range of dynd types
  if (src0_type_id >= bool_type_id && src0_type_id <= complex_float64_type_id &&
      src1_type_id >= bool_type_id && src1_type_id <= complex_float64_type_id &&
      comptype >= 0 && comptype <= comparison_type_greater) {
    // No need to reserve more space, the space for a leaf is already there
    ckernel_prefix *result =
        reinterpret_cast<ckernel_builder<kernel_request_host> *>(ckb)
            ->alloc_ck<ckernel_prefix>(ckb_offset);
    result->set_function<expr_predicate_t>(
        compare_kernel_table[src0_type_id - bool_type_id]
                            [src1_type_id - bool_type_id][comptype]);
    return ckb_offset;
  } else {
    throw not_comparable_error(ndt::type(src0_type_id), ndt::type(src1_type_id),
                               comptype);
  }
}
