/* libnih
 *
 * alloc.c - hierarchial allocator
 *
 * Copyright © 2005 Scott James Remnant <scott@netsplit.com>.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <stdlib.h>

#include <nih/macros.h>
#include <nih/list.h>

#include "alloc.h"


/**
 * NihAllocCtx:
 * @entry: list header,
 * @parent: parent context, when freed we will be,
 * @children: child blocks that will be freed when we are,
 * @name: arbitrary string name for the context,
 * @destructor: function to be called when freed,
 * @size: size of requested allocation.
 *
 * This structure is placed before all allocations in memory and is used
 * to build up a tree of them.  When an allocation is freed, all children
 * are also freed and any destructors are called.
 */
typedef struct nih_alloc_ctx {
	NihList               entry;

	struct nih_alloc_ctx *parent;
	NihList               children;

	const char           *name;
	NihAllocDestructor    destructor;
	size_t                size;
} NihAllocCtx;

/**
 * NIH_ALLOC_CTX:
 * @ptr: pointer to block of memory.
 *
 * Obtain the location of the #NihAllocCtx structure given a pointer to the
 * block of memory beyond it.
 *
 * Returns: pointer to #NihAllocCtx structure.
 */
#define NIH_ALLOC_CTX(ptr) ((NihAllocCtx *)(ptr) - 1)

/**
 * NIH_ALLOC_PTR:
 * @ctx: pointer to #NihAllocCtx structure.
 *
 * Obtain the location of the block of memory given a pointer to the
 * #NihAllocCtx structure in front of it.
 *
 * Returns: pointer to block of memory.
 */
#define NIH_ALLOC_PTR(ctx) ((void *)((NihAllocCtx *)(ctx) + 1))


/**
 * used_pool:
 *
 * Pool of #NihAllocCtx structures that are currently in use and don't have
 * any @parent set; those that do are held in their parents children list.
 */
static NihList *used_pool;

/**
 * unused_pool:
 *
 * Pools of #NihAllocCtx structures that are not currently in use.
 * The first list are for blocks of %NIH_ALLOC_SMALLEST in size (used for
 * anything smaller), the second is for blocks that are larger than this.
 */
static NihList *unused_pool[2];


/**
 * NIH_ALLOC_SMALLEST:
 *
 * The size of the smallest block we'll actually allocate, anything
 * smaller than this will just use this block size.  We then just first-fit
 * return anything from the first @unused_pool if available.
 *
 * Anything larger is allocated as the desired size, or best-fit from the
 * second @unused_pool.
 */
#define NIH_ALLOC_SMALLEST 64 /* FIXME find a real value */


/**
 * nih_alloc_init:
 *
 * Initialise the global lists.
 */
static void
nih_alloc_init (void)
{
	used_pool = nih_list_new ();

	unused_pool[0] = nih_list_new ();
	unused_pool[1] = nih_list_new ();
}

/**
 * nih_alloc_set:
 * @ctx: context to set data for,
 * @parent: parent for the context,
 * @name: name for the content.
 *
 * Completes the allocation and fills in the members according to the
 * data given.
 *
 * Returns: pointer to the block beyond it.
 */
static void *
nih_alloc_set (NihAllocCtx * ctx,
	       void *      parent,
	       const char *name)
{
	if (parent) {
		ctx->parent = NIH_ALLOC_CTX (parent);

		nih_list_add (&ctx->parent->children, &ctx->entry);
	} else {
		ctx->parent = NULL;

		nih_list_add (used_pool, &ctx->entry);
	}

	ctx->name = name;
	ctx->destructor = NULL;

	return NIH_ALLOC_PTR (ctx);
}

/**
 * nih_alloc_new:
 * @parent: parent block of allocation,
 * @size: size of requested block,
 * @name: name to assign to block.
 *
 * Creates a new #NihAllocCtx structure and fills it with the details given.
 * This should be used when you already know you need a new structure, or
 * just want one for some reason.
 *
 * Returns: newly allocated block.
 */
void *
nih_alloc_new (void *      parent,
	       size_t      size,
	       const char *name)
{
	NihAllocCtx *ctx;

	if (! used_pool)
		nih_alloc_init ();

	size = MAX (size, NIH_ALLOC_SMALLEST);
	ctx = malloc (sizeof (NihAllocCtx) + size);
	/* FIXME malloc might break */

	ctx->size = size;

	nih_list_init (&ctx->entry);
	nih_list_init (&ctx->children);

	return nih_alloc_set (ctx, parent, name);
}

/**
 * nih_alloc_named:
 * @parent: parent block of allocation,
 * @size: size of requested block,
 * @name: name to assign to block.
 *
 * Allocates a block of memory of at least @size bytes and returns
 * a pointer to it.  The block will be assigned the descriptive name
 * of @name.
 *
 * If @parent is not %NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor function.
 *
 * Returns: requested memory block.
 */
void *
nih_alloc_named (void *      parent,
		 size_t      size,
		 const char *name)
{
	if (! used_pool)
		nih_alloc_init ();

	if (size <= NIH_ALLOC_SMALLEST) {
		if (unused_pool[0]->next != unused_pool[0]) {
			NihAllocCtx *ctx = (NihAllocCtx *)unused_pool[0]->next;

			return nih_alloc_set (ctx, parent, name);
		}
	} else {
		NihList     *iter;
		NihAllocCtx *best = NULL;
		int          best_diff = 0;

		for (iter = unused_pool[1]->next; iter != unused_pool[1];
		     iter = iter->next) {
			NihAllocCtx *ctx = (NihAllocCtx *)iter;
			int          diff;

			diff = ctx->size - size;
			if ((diff >= 0) && ((diff < best_diff) || !best))
				best = ctx;
		}

		if (best)
			return nih_alloc_set (best, parent, name);
	}

	return nih_alloc_new (parent, size, name);
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
 */
int
nih_free (void *ptr)
{
	NihAllocCtx *ctx;
	NihList     *iter;
	int          ret = 0;

	/* FIXME check that ptr is not NULL */
	ctx = NIH_ALLOC_CTX (ptr);

	iter = ctx->children.next;
	while (iter != &ctx->children) {
		void *ptr;

		ptr = NIH_ALLOC_PTR (iter);
		iter = iter->next;

		ret = nih_free (ptr);
	}

	if (ctx->destructor)
		ret = ctx->destructor (ptr);

	if (ctx->size == NIH_ALLOC_SMALLEST) {
		nih_list_add (unused_pool[0], &ctx->entry);
	} else {
		nih_list_add (unused_pool[1], &ctx->entry);
	}

	return ret;
}


/**
 * nih_alloc_set_name:
 * @ptr: pointer to block,
 * @name: name to set.
 *
 * Sets the name of the block to @name, which should be a string that will
 * last at least as long as the block as it is not copied.
 */
void
nih_alloc_set_name (void       *ptr,
		    const char *name)
{
	NihAllocCtx *ctx;

	/* FIXME check ptr and name are not NULL */
	ctx = NIH_ALLOC_CTX (ptr);
	ctx->name = name;
}

/**
 * nih_alloc_set_destructor:
 * @ptr: pointer to block,
 * @destructor: destructor function to set.
 *
 * Sets the destructor function of the block to @destructor, which may be
 * %NULL.
 *
 * The destructor function will be called when the block is freed, either
 * directly or as a result as a parent being freed.  The block will be
 * passed as a pointer to the destructor, and the destructor may return
 * a value which will be the return value of the nih_free() function.
 */
void
nih_alloc_set_destructor (void               *ptr,
			  NihAllocDestructor  destructor)
{
	NihAllocCtx *ctx;

	/* FIXME check ptr is not NULL */
	ctx = NIH_ALLOC_CTX (ptr);
	ctx->destructor = destructor;
}

/**
 * nih_alloc_name:
 * @ptr: pointer to block.
 *
 * Returns: the name associated with the block.
 */
const char *
nih_alloc_name (void *ptr)
{
	NihAllocCtx *ctx;

	/* FIXME check ptr is not NULL */
	ctx = NIH_ALLOC_CTX (ptr);
	return ctx->name;
}


/**
 * nih_alloc_return_unused:
 * @large: 1 to return large blocks, 0 to return small.
 *
 * Return unused blocks to the operating system (or, at least, the libc).
 */
void
nih_alloc_return_unused (int large)
{
	NihList *iter;

	iter = unused_pool[large]->next;
	while (iter != unused_pool[large]) {
		void *ptr;

		ptr = NIH_ALLOC_PTR (iter);
		iter = iter->next;

		nih_free (ptr);
	}
}
