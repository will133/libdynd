//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <dynd/kernels/assignment_kernels.hpp>
#include <dynd/types/date_type.hpp>

namespace dynd {

/**
 * Makes a kernel which converts strings to datetimes.
 */
size_t make_string_to_datetime_assignment_kernel(
    void *ckb, intptr_t ckb_offset, const ndt::type &dst_datetime_dt,
    const char *dst_arrmeta, const ndt::type &src_string_dt,
    const char *src_arrmeta, kernel_request_t kernreq,
    const eval::eval_context *ectx);

/**
 * Makes a kernel which converts datetimes to strings.
 */
size_t make_datetime_to_string_assignment_kernel(
    void *ckb, intptr_t ckb_offset, const ndt::type &dst_string_dt,
    const char *dst_arrmeta, const ndt::type &src_datetime_dt,
    const char *src_arrmeta, kernel_request_t kernreq,
    const eval::eval_context *ectx);

} // namespace dynd
