/* libnih
 *
 * alloc.c - hierarchial allocator
 *
 * Copyright © 2006 Scott James Remnant <scott@netsplit.com>.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <stdlib.h>

#include <nih/macros.h>
#include <nih/logging.h>
#include <nih/list.h>

#include "alloc.h"


/**
 * NihAllocCtx:
 * @entry: list header,
 * @size: size of requested allocation,
 * @parent: parent context, when freed we will be,
 * @children: child blocks that will be freed when we are,
 * @allocator: function to call to return memory,
 * @destructor: function to be called when freed.
 *
 * This structure is placed before all allocations in memory and is used
 * to build up a tree of them.  When an allocation is freed, all children
 * are also freed and any destructors are called.
 **/
typedef struct nih_alloc_ctx {
	NihList               entry;
	size_t                size;

	struct nih_alloc_ctx *parent;
	NihList               children;

	NihAllocator          allocator;
	NihDestructor         destructor;
} NihAllocCtx;

/**
 * NIH_ALLOC_CTX:
 * @ptr: pointer to block of memory.
 *
 * Obtain the location of the NihAllocCtx structure given a pointer to the
 * block of memory beyond it.
 *
 * Returns: pointer to NihAllocCtx structure.
 **/
#define NIH_ALLOC_CTX(ptr) ((NihAllocCtx *)(ptr) - 1)

/**
 * NIH_ALLOC_PTR:
 * @ctx: pointer to NihAllocCtx structure.
 *
 * Obtain the location of the block of memory given a pointer to the
 * NihAllocCtx structure in front of it.
 *
 * Returns: pointer to block of memory.
 **/
#define NIH_ALLOC_PTR(ctx) ((void *)((NihAllocCtx *)(ctx) + 1))


/**
 * allocator:
 *
 * Function used to allocate and free memory for the majority of blocks.
 **/
static NihAllocator allocator = NULL;


/**
 * nih_alloc_init:
 *
 * Initialise the default allocator.
 **/
static inline void
nih_alloc_init (void)
{
	if (! allocator)
		nih_alloc_set_allocator (realloc);
}

/**
 * nih_alloc_set_allocator:
 * @new_allocator: new default allocator function.
 *
 * Sets the function that will be used to allocate memory for all further
 * blocks requested and return it to the system.  The behaviour of the
 * function should be the same of that as the standard realloc() function.
 *
 * This function should generally only be used in the initialisation
 * portion of your program, and should not be used to switch allocators
 * temporarily.  Use nih_alloc_using() to allocate a block with an
 * alternate allocator.
 **/
void
nih_alloc_set_allocator (NihAllocator new_allocator)
{
	nih_assert (new_allocator != NULL);

	allocator = new_allocator;
}


/**
 * nih_alloc_using:
 * @allocator: allocator to use for this block,
 * @parent: parent block of allocation,
 * @size: size of requested block.
 *
 * Allocates a block of memory of at least @size bytes with the @allocator
 * function and returns a pointer to it.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: requested memory block or NULL if allocation fails.
 **/
void *
nih_alloc_using (NihAllocator  allocator,
		 const void   *parent,
		 size_t        size)
{
	NihAllocCtx *ctx;

	nih_assert (allocator != NULL);

	ctx = allocator (NULL, sizeof (NihAllocCtx) + size);
	if (! ctx)
		return NULL;

	ctx->size = size;

	nih_list_init (&ctx->entry);
	nih_list_init (&ctx->children);

	ctx->allocator = allocator;
	ctx->destructor = NULL;

	if (parent) {
		ctx->parent = NIH_ALLOC_CTX (parent);

		nih_list_add (&ctx->parent->children, &ctx->entry);
	} else {
		ctx->parent = NULL;
	}

	return NIH_ALLOC_PTR (ctx);
}

/**
 * nih_alloc:
 * @parent: parent block of allocation,
 * @size: size of requested block.
 *
 * Allocates a block of memory of at least @size bytes and returns
 * a pointer to it.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: requested memory block or NULL if allocation fails.
 **/
void *
nih_alloc (const void *parent,
	   size_t      size)
{
	nih_alloc_init ();

	return nih_alloc_using (allocator, parent, size);
}

/**
 * nih_realloc:
 * @ptr: block to reallocate or NULL,
 * @parent: parent block of allocation,
 * @size: size of new block.
 *
 * Adjusts the size of the block of memory allocated for @ptr to be at
 * least @size bytes and returns the new pointer.  If @ptr is NULL then
 * this is equivalent to nih_alloc().
 *
 * If @parent is not NULL, it must be the same object as the parent to
 * @ptr, unless that is also NULL in which case it should be a pointer to
 * another block which will be used as the parent for the newly allocated
 * block.  When @parent is freed, the returned block will be freed too.
 * If you have clean-up that would need to be run, you can assign a
 * destructor function using the nih_alloc_set_destructor() function.
 *
 * Returns: reallocated block or NULL if reallocation fails.
 **/
void *
nih_realloc (void       *ptr,
	     const void *parent,
	     size_t      size)
{
	NihAllocCtx *ctx;
	NihList     *first_child = NULL;

	if (! ptr)
		return nih_alloc (parent, size);

	ctx = NIH_ALLOC_CTX (ptr);

	if (parent)
		nih_assert (ctx->parent == NIH_ALLOC_CTX (parent));

	/* This is somewhat more difficult than alloc or free because we
	 * have a tree of pointers to worry about.  Fortunately the
	 * properties of the nih_list_* functions we use help us a lot here.
	 *
	 * The problem is that references in the parent to this ptr's list
	 * entry or references in children to this ptr's children list
	 * entry are invalid once the allocator has been called.
	 *
	 * We could strip it all down before the allocator, then rebuild it
	 * afterwards, but that's expensive and could be error-prone in the
	 * case where the allocator fails.
	 *
	 * The solution is to rely on a property of nih_list_add(); the
	 * entry passed (to be added) is cut out of its containing list
	 * without deferencing the return pointers, this means we can cut
	 * the bad pointers out simply by calling nih_list_add() to put the
	 * new entry back in the same position.
	 *
	 * Of course, this only works in the non-empty list case as trying
	 * to cut an entry out of an empty list would deference those
	 * invalid pointers.  Happily all we need to do for the non-empty
	 * list case is call nih_list_init() again.
	 *
	 * For the primary list entry this is if we don't have a parent.
	 * For the children list entry, this is if we don't have children,
	 * which is difficult to detect after the allocator has been
	 * called.  The easiest thing to do is stash a pointer to the first
	 * child, if non-NULL then we use it for nih_list_add(), if NULL
	 * then we call nih_list_init().
	 */

	if (! NIH_LIST_EMPTY (&ctx->children))
		first_child = ctx->children.next;

	/* Now we do the actual realloc(), if this fails then the original
	 * structure is still intact in memory so we can just return NULL
	 */
	ctx = ctx->allocator (ctx, sizeof (NihAllocCtx) + size);
	if (! ctx)
		return NULL;

	ctx->size = size;

	/* Either update our entry in our parent's list of children,
	 * or reinitialise the list entry so it doesn't point to nowhere.
	 */
	if (ctx->parent) {
		nih_list_add (&ctx->parent->children, &ctx->entry);
	} else {
		nih_list_init (&ctx->entry);
	}

	/* Likewise update the head entry in our own list of children,
	 * or reinitialise it so it doesn't point to nowhere.
	 */
	if (first_child) {
		nih_list_add (first_child, &ctx->children);
	} else {
		nih_list_init (&ctx->children);
	}

	/* Finally fix up the parent pointer in all of our children so they
	 * point to our new location
	 */
	NIH_LIST_FOREACH (&ctx->children, iter) {
		NihAllocCtx *child_ctx = (NihAllocCtx *)iter;

		child_ctx->parent = ctx;
	}

	return NIH_ALLOC_PTR (ctx);
}


/**
 * nih_free:
 * @ptr: pointer to block to free.
 *
 * Return the block of memory at @ptr to the allocator so it may be
 * re-used by something else.  Any children of the block are also freed,
 * and any destructors called.
 *
 * Returns: return value from destructor, or 0.
 **/
int
nih_free (void *ptr)
{
	NihAllocCtx *ctx;
	int          ret = 0;

	nih_assert (ptr != NULL);

	ctx = NIH_ALLOC_CTX (ptr);

	NIH_LIST_FOREACH_SAFE (&ctx->children, iter) {
		void *ptr;

		ptr = NIH_ALLOC_PTR (iter);
		ret = nih_free (ptr);
	}

	if (ctx->destructor)
		ret = ctx->destructor (ptr);

	nih_list_remove (&ctx->entry);

	ctx->allocator (ctx, 0);

	return ret;
}

/**
 * nih_alloc_set_destructor:
 * @ptr: pointer to block,
 * @destructor: destructor function to set.
 *
 * Sets the destructor function of the block to @destructor, which may be
 * NULL.
 *
 * The destructor function will be called when the block is freed, either
 * directly or as a result as a parent being freed.  The block will be
 * passed as a pointer to the destructor, and the destructor may return
 * a value which will be the return value of the nih_free() function.
 **/
void
nih_alloc_set_destructor (void          *ptr,
			  NihDestructor  destructor)
{
	NihAllocCtx *ctx;

	nih_assert (ptr != NULL);

	ctx = NIH_ALLOC_CTX (ptr);
	ctx->destructor = destructor;
}


/**
 * nih_alloc_reparent:
 * @ptr: pointer to block to reparent,
 * @parent: new parent block.
 *
 * Disassociate the block of memory at @ptr from its current parent, if
 * any, and optionally assign a new parent.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.
 **/
void
nih_alloc_reparent (void       *ptr,
		    const void *parent)
{
	NihAllocCtx *ctx;

	nih_assert (ptr != NULL);

	ctx = NIH_ALLOC_CTX (ptr);

	if (parent) {
		ctx->parent = NIH_ALLOC_CTX (parent);

		nih_list_add (&ctx->parent->children, &ctx->entry);
	} else {
		ctx->parent = NULL;

		nih_list_remove (&ctx->entry);
	}
}


/**
 * nih_alloc_size:
 * @ptr: pointer to block.
 *
 * Returns: the size of the allocated block, excluding the context.
 **/
size_t
nih_alloc_size (const void *ptr)
{
	NihAllocCtx *ctx;

	nih_assert (ptr != NULL);

	ctx = NIH_ALLOC_CTX (ptr);
	return ctx->size;
}

/**
 * nih_alloc_parent:
 * @ptr: pointer to block.
 *
 * Returns: the parent block or NULL if none.
 **/
void *
nih_alloc_parent (const void *ptr)
{
	NihAllocCtx *ctx;

	nih_assert (ptr != NULL);

	ctx = NIH_ALLOC_CTX (ptr);
	if (ctx->parent) {
		return NIH_ALLOC_PTR (ctx->parent);
	} else {
		return NULL;
	}
}
