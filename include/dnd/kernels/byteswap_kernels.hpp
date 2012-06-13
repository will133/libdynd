//
// Copyright (C) 2012 Continuum Analytics
// All rights reserved.
//
#ifndef _DND__BYTESWAP_KERNELS_HPP_
#define _DND__BYTESWAP_KERNELS_HPP_

#include <dnd/kernels/kernel_instance.hpp>
#include <stdint.h>

namespace dnd {

/**
 * Function for byteswapping a single value.
 */
inline uint16_t byteswap_value(uint16_t value) {
    return ((value&0xffu) << 8) | (value >> 8);
}

/**
 * Function for byteswapping a single value.
 */
inline uint32_t byteswap_value(uint32_t value) {
    return ((value&0xffu) << 24) |
            ((value&0xff00u) << 8) |
            ((value&0xff0000u) >> 8) |
            (value >> 24);
}

/**
 * Function for byteswapping a single value.
 */
inline uint64_t byteswap_value(uint64_t value) {
    return ((value&0xffULL) << 56) |
            ((value&0xff00ULL) << 40) |
            ((value&0xff0000ULL) << 24) |
            ((value&0xff000000ULL) << 8) |
            ((value&0xff00000000ULL) >> 8) |
            ((value&0xff0000000000ULL) >> 24) |
            ((value&0xff000000000000ULL) >> 40) |
            (value >> 56);
}

/**
 * Gets a kernel which swaps the byte-order of each element.
 * Requires that alignment == element_size for primitive-sized
 * types including 2, 4, and 8 bytes.
 *
 * If a stride is unknown or non-fixed, pass INTPTR_MAX for that stride.
 */
void get_byteswap_kernel(intptr_t element_size,
                intptr_t dst_fixedstride, intptr_t src_fixedstride,
                kernel_instance<unary_operation_t>& out_kernel);

/**
 * Gets a kernel which swaps the byte-order of two values within each element.
 * Requires that alignment == element_size/2 for primitive-sized
 * types including 4, 8, and 16 bytes. This is intended for use with
 * types with two primitives such as complex numbers.
 *
 * If a stride is unknown or non-fixed, pass INTPTR_MAX for that stride.
 */
void get_pairwise_byteswap_kernel(intptr_t element_size,
                intptr_t dst_fixedstride, intptr_t src_fixedstride,
                kernel_instance<unary_operation_t>& out_kernel);

} // namespace dnd

#endif // _DND__BYTESWAP_KERNELS_HPP_