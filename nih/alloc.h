/* libnih
 *
 * Copyright © 2007 Scott James Remnant <scott@netsplit.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef NIH_ALLOC_H
#define NIH_ALLOC_H

#include <nih/macros.h>


/**
 * NihAllocator:
 * @ptr: pointer to be reallocated,
 * @size: size of allocation.
 *
 * An allocator is a function that can be used to both allocate memory
 * and return it to the system (or cache it, etc.)  The behaviour should
 * be the same of that as the standard realloc() function.
 **/
typedef void *(*NihAllocator) (void *ptr, size_t size);

/**
 * NihDestructor:
 * @ptr: pointer to be destroyed.
 *
 * A destructor is a function that can be associated with an allocated
 * block of memory and is called when the block is freed; the pointer
 * given is that of the block being freed.
 *
 * This can be used, for example, to close a file descriptor when the
 * structure for it is being closed.
 **/
typedef int (*NihDestructor) (void *ptr);


/**
 * nih_new:
 * @parent: parent block for new allocation,
 * @type: type of data to store.
 *
 * Allocates a block of memory large enough to store a @type object and
 * returns a pointer to it.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: requested memory block.
 **/
#define nih_new(parent, type) nih_alloc(parent, sizeof (type))


NIH_BEGIN_EXTERN

void   nih_alloc_set_allocator  (NihAllocator new_allocator);

void * nih_alloc_using          (NihAllocator allocator, const void *parent,
				 size_t size)
	__attribute__ ((warn_unused_result, malloc));

void * nih_alloc                (const void *parent, size_t size)
	__attribute__ ((warn_unused_result, malloc));

void * nih_realloc              (void *ptr, const void *parent, size_t size)
	__attribute__ ((warn_unused_result, malloc));

int    nih_free                 (void *ptr);

void   nih_alloc_set_destructor (void *ptr, NihDestructor destructor);

void   nih_alloc_reparent       (void *ptr, const void *parent);

size_t nih_alloc_size           (const void *ptr);
void * nih_alloc_parent         (const void *ptr);

NIH_END_EXTERN

#endif /* NIH_ALLOC_H */
