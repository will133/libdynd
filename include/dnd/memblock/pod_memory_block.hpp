//
// Copyright (C) 2011-12, Dynamic NDArray Developers
// BSD 2-Clause License, see LICENSE.txt
//

#ifndef _DND__POD_MEMORY_BLOCK_HPP_
#define _DND__POD_MEMORY_BLOCK_HPP_

#include <dnd/memblock/memory_block.hpp>

namespace dnd {

/**
 * Creates a memory block which can be used to allocate POD output memory
 * for blockref dtypes.
 *
 * The initial capacity can be set if a good estimate is known.
 */
memory_block_ptr make_pod_memory_block(intptr_t initial_capacity_bytes = 2048);

/**
 * Creates a memory block which can be used to allocate POD output memory
 * for blockref dtypes. This version of the the function includes a list of
 * other memory_blocks that this memory_block should hold a reference to,
 * intended for use when the memory allocated in this block is itself another
 * blockref dtype pointing into another memory block.
 *
 * The initial capacity can be set if a good estimate is known.
 */
memory_block_ptr make_pod_memory_block(memory_block_ptr *blockrefs_begin, memory_block_ptr *blockrefs_end, intptr_t initial_capacity_bytes = 2048);

/**
 * Creates a memory block which can be used to allocate POD output memory
 * for blockref dtypes. This version of the the function includes a list of
 * other memory_blocks that this memory_block should hold a reference to,
 * intended for use when the memory allocated in this block is itself another
 * blockref dtype pointing into another memory block.
 *
 * The initial capacity can be set if a good estimate is known.
 */
memory_block_ptr make_pod_memory_block(memory_block_data **blockrefs_begin, memory_block_data **blockrefs_end, intptr_t initial_capacity_bytes = 2048);

} // namespace dnd

#endif // _DND__POD_MEMORY_BLOCK_HPP_