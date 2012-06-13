//
// Copyright (C) 2011-12, Dynamic NDArray Developers
// BSD 2-Clause License, see LICENSE.txt
//

#ifndef _DND__ALIGNMENT_KERNELS_HPP_
#define _DND__ALIGNMENT_KERNELS_HPP_

#include <dnd/kernels/kernel_instance.hpp>
#include <stdint.h>

namespace dnd {

/**
 * Gets a kernel which does an unaligned raw byte-copy of the specified
 * element size.
 *
 * If a stride is unknown or non-fixed, pass INTPTR_MAX for that stride.
 */
void get_unaligned_copy_kernel(intptr_t element_size,
                intptr_t dst_fixedstride, intptr_t src_fixedstride,
                kernel_instance<unary_operation_t>& out_kernel);

} // namespace dnd

#endif // _DND__ALIGNMENT_KERNELS_HPP_