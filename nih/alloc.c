/* libnih
 *
 * alloc.c - multi-reference hierarchial allocator
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2009 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <malloc.h>
#include <stdlib.h>

#include <nih/macros.h>
#include <nih/logging.h>
#include <nih/list.h>

#include "alloc.h"


/**
 * NihAllocCtx:
 * @parents: parents of this context,
 * @children: children of this context,
 * @destructor: function to be called when freed,
 * @size: allocation size.
 *
 * This structure is placed before all allocations in memory and is used
 * to build up an n-ary tree of them.  Allocations may have multiple
 * parent references and multiple children.  Allocations are automatically
 * freed if the last parent reference is freed.  When an allocation is
 * freed, all children are unreferenced and any destructors called.
 *
 * Members of @parents and @children are both NihAllocRef objects.
 **/
typedef struct nih_alloc_ctx {
	NihList       parents;
	NihList       children;
	NihDestructor destructor;
	size_t        size;
} NihAllocCtx;

/**
 * NihAllocRef:
 * @children_entry: list head in parent's children list,
 * @parents_entry: list head in child's parents list,
 * @parent: pointer to parent context,
 * @child: pointer to child context.
 *
 * This structure is shared by both @parent and @child denoting a reference
 * between the two of them.  It is placed in @parent's children list through
 * @children_entry and @child's parents list through @parents_entry.
 **/
typedef struct nih_alloc_ref {
	NihList      children_entry;
	NihList      parents_entry;
	NihAllocCtx *parent;
	NihAllocCtx *child;
} NihAllocRef;


/**
 * NIH_ALLOC_SIZE:
 *
 * Expands to the size of the NihAllocCtx structure plus whetever padding
 * is needed to ensure the following pointer is generically aligned.
 **/
#define NIH_ALLOC_SIZE (NIH_ALIGN_SIZE * (((sizeof (NihAllocCtx) - 1)	\
					   / NIH_ALIGN_SIZE) + 1))

/**
 * NIH_ALLOC_CTX:
 * @ptr: pointer to block of memory.
 *
 * Obtain the location of the NihAllocCtx structure given a pointer to the
 * block of memory beyond it.
 *
 * Returns: pointer to NihAllocCtx structure or NULL if @ptr is NULL.
 **/
#define NIH_ALLOC_CTX(ptr) (ptr ? (void *)(ptr) - NIH_ALLOC_SIZE : NULL)

/**
 * NIH_ALLOC_PTR:
 * @ctx: pointer to NihAllocCtx structure.
 *
 * Obtain the location of the block of memory given a pointer to the
 * NihAllocCtx structure in front of it.
 *
 * Returns: pointer to block of memory.
 **/
#define NIH_ALLOC_PTR(ctx) ((void *)(ctx) + NIH_ALLOC_SIZE)

/**
 * NIH_ALLOC_FINALISED:
 *
 * Flag placed in the destructor field of a context to indicate the
 * destructor has been called and the object is pending being freed.
 **/
#define NIH_ALLOC_FINALISED ((void *)-1)


/* Prototypes for static functions */
static inline int          nih_alloc_context_free   (NihAllocCtx *ctx);

static inline NihAllocRef *nih_alloc_ref_new        (NihAllocCtx *parent,
						     NihAllocCtx *child)
	__attribute__ ((malloc));
static inline void         nih_alloc_ref_free       (NihAllocRef *ref);
static inline NihAllocRef *nih_alloc_ref_lookup     (NihAllocCtx *parent,
						     NihAllocCtx *child);


/* Point to the functions we actually call for allocation. */
void *(*__nih_malloc)  (size_t size)            = malloc;
void *(*__nih_realloc) (void *ptr, size_t size) = realloc;
void  (*__nih_free)    (void *ptr)              = free;


/**
 * nih_alloc:
 * @parent: parent object for new object,
 * @size: size of requested object.
 *
 * Allocates an object in memory of at least @size bytes and returns a
 * pointer to it.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned object otherwise the special
 * NULL parent will be used instead.  When all parents of the returned
 * object are freed, the returned object will also be freed.
 *
 * If you have clean-up that you would like to run, you can assign a
 * destructor using the nih_alloc_set_destructor() function.
 *
 * Returns: newly allocated object or NULL if insufficient memory.
 **/
void *
nih_alloc (const void *parent,
	   size_t      size)
{
	NihAllocCtx *ctx;

	ctx = __nih_malloc (NIH_ALLOC_SIZE + size);
	if (! ctx)
		return NULL;

	nih_list_init (&ctx->parents);
	nih_list_init (&ctx->children);

	ctx->destructor = NULL;
	ctx->size = size;

	nih_alloc_ref_new (NIH_ALLOC_CTX (parent), ctx);

	return NIH_ALLOC_PTR (ctx);
}


/**
 * nih_realloc:
 * @ptr: object to reallocate,
 * @parent: parent object of new object,
 * @size: size of new object.
 *
 * Adjusts the size of the object @ptr to be at least @size bytes, which
 * may be larger or smaller than the existing object, and returns the
 * new pointer.
 *
 * If @ptr is NULL, this simply calls nih_alloc() and passes both @parent
 * and @size to it, returning the returned object.
 *
 * If @ptr is not NULL, @parent is ignored; though it is usual to pass a
 * parent of @ptr for style reasons.
 *
 * Returns: reallocated object or NULL if insufficient memory.
 **/
void *
nih_realloc (void *      ptr,
	     const void *parent,
	     size_t      size)
{
	NihAllocCtx *ctx;
	NihList *    first_parent = NULL;
	NihList *    first_child = NULL;

	if (! ptr)
		return nih_alloc (parent, size);

	ctx = NIH_ALLOC_CTX (ptr);
	nih_assert (ctx->destructor != NIH_ALLOC_FINALISED);

	/* This is somewhat more difficult than alloc or free because we
	 * have two lists of pointers to worry about.  Fortunately the
	 * properties of NihList help us a lot here.
	 *
	 * The problem is that references between us and our parents,
	 * and references between us and our children, all contain list
	 * pointers that are potentially invalid once relloc has been
	 * called.
	 *
	 * We could strip it all down before calling realloc then rebuild
	 * it afterwards, but that's expensive and could be error-prone in
	 * the case where the allocator fails.
	 *
	 * The solution is to rely on a property of nih_list_add().  The
	 * entry passed (to be added) is cut out of its containing list
	 * without dereferencing the return pointers, this means we can
	 * cut the bad pointers out simply by calling nih_list_add()
	 * to put the new entry back in the same position.
	 *
	 * Of course, this only works in the non-empty list case as trying
	 * to cut an entry out of an empty list would dereference those
	 * invalid pointers.  Happily all we need to do for the empty
	 * list case is call nih_list_init() again.
	 *
	 * So we just remember the first parent and first child reference,
	 * or NULL if the list is empty.
	 */

	if (! NIH_LIST_EMPTY (&ctx->parents))
		first_parent = ctx->parents.next;
	if (! NIH_LIST_EMPTY (&ctx->children))
		first_child = ctx->children.next;

	/* Now do the actual realloc(), if this fails then we can just
	 * return NULL since we've not actually changed anything.
	 */
	ctx = __nih_realloc (ctx, NIH_ALLOC_SIZE + size);
	if (! ctx)
		return NULL;

	ctx->size = size;

	/* Now update our parents and children lists, or reinitialise,
	 * as noted above this ensures that all the pointers are correct
	 */
	if (first_parent) {
		nih_list_add_after (first_parent, &ctx->parents);
	} else {
		nih_list_init (&ctx->parents);
	}

	if (first_child) {
		nih_list_add_after (first_child, &ctx->children);
	} else {
		nih_list_init (&ctx->children);
	}

	/* We still have to fix up the parent and child pointers, but
	 * that's easy.
	 */
	NIH_LIST_FOREACH (&ctx->parents, iter) {
		NihAllocRef *ref = NIH_LIST_ITER (iter, NihAllocRef,
						  parents_entry);

		ref->child = ctx;
	}

	NIH_LIST_FOREACH (&ctx->children, iter) {
		NihAllocRef *ref = NIH_LIST_ITER (iter, NihAllocRef,
						  children_entry);

		ref->parent = ctx;
	}

	return NIH_ALLOC_PTR (ctx);
}


/**
 * nih_free:
 * @ptr: object to free.
 *
 * Returns the object @ptr to the allocator so the memory consumed may be
 * re-used by something else.
 *
 * All parent references are discarded and the destructor for @ptr is called.
 * Then all children are recursively unreferenced.  Those that have no
 * remaining parent references will also have their destructors called and
 * their children unreferenced, etc.  Once all destructors have been called,
 * the objects themselves are freed.
 *
 * If you call nih_free() on an object with parent references, you should
 * make sure that any pointers to the object are reset.
 *
 * If you are unsure whether or not there are references you should call
 * nih_discard() which will discard the special NULL reference only if it
 * exists, only freeing the object if no other references remain.
 *
 * Otherwise to remove a particular parent reference you should call
 * nih_unref().
 *
 * Returns: return value from @ptr's destructor, or 0.
 **/
int
nih_free (void *ptr)
{
	NihAllocCtx *ctx;

	nih_assert (ptr != NULL);

	ctx = NIH_ALLOC_CTX (ptr);
	nih_assert (ctx->destructor != NIH_ALLOC_FINALISED);

	/* Cast off our parents first, without recursing.  This ensures
	 * we always have zero references before we call the destructor,
	 * and has the somewhat neat property of breaking any reference
	 * loops.
	 */
	NIH_LIST_FOREACH_SAFE (&ctx->parents, iter) {
		NihAllocRef *ref = NIH_LIST_ITER (iter, NihAllocRef,
						  parents_entry);

		nih_alloc_ref_free (ref);
	}

	return nih_alloc_context_free (ctx);
}

/**
 * nih_discard:
 * @ptr: object to discard.
 *
 * Discards the special NULL parent reference from @ptr if present; if
 * no other references have been taken @ptr will be freed and the value
 * from the destructor returned otherwise this function takes no
 * further action.
 *
 * You would use nih_discard() when you allocated @ptr without any parent
 * but have passed it to functions that may have taken a reference to it
 * in the meantime.  Compare with nih_free() which acts even if there are
 * parent references, and nih_unref() which only removes a single parent
 * reference that is known to exist.
 *
 * Returns: return value from @ptr's destructor, or 0.
 **/
int
nih_discard (void *ptr)
{
	NihAllocCtx *ctx;
	NihAllocRef *ref;

	nih_assert (ptr != NULL);

	ctx = NIH_ALLOC_CTX (ptr);
	nih_assert (ctx->destructor != NIH_ALLOC_FINALISED);

	ref = nih_alloc_ref_lookup (NULL, ctx);
	if (! ref)
		return 0;

	nih_alloc_ref_free (ref);

	if (NIH_LIST_EMPTY (&ctx->parents))
		return nih_alloc_context_free (ctx);

	return 0;
}

/**
 * _nih_discard_local:
 * @ptr: address of local object to be discarded.
 *
 * This function should never be called directly, it is used as part of the
 * implementation of nih_local and simply calls nih_discard() with the
 * local variable itself if non-NULL.
 **/
void
_nih_discard_local (void *ptraddr)
{
	/* Can't just take void ** as a parameter, since that will upset
	 * gcc typechecking, and we want to be able to be used on any
	 * pointer type.
	 */
	void **ptr = (void **)ptraddr;

	if (*ptr)
		nih_discard (*ptr);
}


/**
 * nih_alloc_context_free:
 * @ctx: context to free.
 *
 * This is the internal function called by nih_free(), nih_discard() and
 * nih_unref() to actually free an allocated context and its attached
 * objects.
 *
 * All parent references must have been discarded prior to calling this
 * function.
 *
 * The destructor for @ctx is called, and then all children are recursively
 * unreferenced.  Those that have no remaining parent references will also
 * have their destructors called and their children unreferenced, etc.
 * Once all destructors have been called, the objects themselves are freed.
 *
 * Returns: return value from @ptr's destructor, or 0.
 **/
static inline int
nih_alloc_context_free (NihAllocCtx *ctx)
{
	int ret = 0;

	nih_assert (ctx != NULL);
	nih_assert (ctx->destructor != NIH_ALLOC_FINALISED);
	nih_assert (NIH_LIST_EMPTY (&ctx->parents));

	/* We have no parents, call our destructor before doing anything
	 * to our children.  Save the return value, since this is what
	 * we return.
	 */
	if (ctx->destructor)
		ret = ctx->destructor (NIH_ALLOC_PTR (ctx));
	ctx->destructor = NIH_ALLOC_FINALISED;

	/* Recursively finalise all of our children. */
	NIH_LIST_FOREACH_SAFE (&ctx->children, iter) {
		NihAllocRef *ref = NIH_LIST_ITER (iter, NihAllocRef,
						  children_entry);

		/* Disassociate the child from its parent.
		 * If that was not the last parent, the child should not
		 * be freed, so destroy the rest of the reference and move
		 * on.
		 */
		nih_list_destroy (&ref->parents_entry);
		if (! NIH_LIST_EMPTY (&ref->child->parents)) {
			nih_list_destroy (&ref->children_entry);
			free (ref);
			continue;
		}

		/* Child is to be destroyed and has no links back to its
		 * parents.  We call the destructor now.
		 */
		if (ref->child->destructor)
			ref->child->destructor (NIH_ALLOC_PTR (ref->child));
		ref->child->destructor = NIH_ALLOC_FINALISED;

		/* Reparent all of its own children to us so that they too
		 * will be finalised if the last reference is removed.
		 *
		 * In order to do this depth-first while preserving order,
		 * we insert the items before our cursor; and then put the
		 * cursor back at the head of them.
		 */
		NIH_LIST_FOREACH_SAFE (&ref->child->children, citer) {
			NihAllocRef *cref = NIH_LIST_ITER (citer, NihAllocRef,
							   children_entry);

			nih_list_add (&_iter, &cref->children_entry);
		}

		nih_list_add_after (iter, &_iter);
	}

	/* We now have a single list of children all of which have no
	 * references back to us as their parent, and all of had their
	 * destructors called.
	 *
	 * Now we free them.
	 */
	NIH_LIST_FOREACH_SAFE (&ctx->children, iter) {
		NihAllocRef *ref = NIH_LIST_ITER (iter, NihAllocRef,
						  children_entry);

		__nih_free (ref->child);

		nih_list_destroy (&ref->children_entry);
		free (ref);
	}

	/* And now we can free ourselves. */
	__nih_free (ctx);

	return ret;
}


/**
 * nih_alloc_real_set_destructor:
 * @ptr: pointer to object,
 * @destructor: destructor function to set.
 *
 * Sets the destructor of the allocated object @ptr to @destructor, which
 * may be NULL to unset an existing destructor.  Normally you would use
 * the nih_alloc_set_destructor() macro which expands to this function
 * but casts @destructor to the correct type, since almost all destructors
 * will be defined with their argument to be the type of the object
 * rather than void *.
 *
 * The destructor will be called before the object is freed, either
 * explicitly by nih_free() or nih_discard(), or because the last parent
 * has unreferenced the object.
 *
 * When the destructor is called, the parent references to the object will
 * have already been discarded but all children references will be intact
 * and none of the children will have been freed.  There is no need to use
 * a destructor to unreference or free children, that is automatic.
 *
 * The pointer @ptr passed to the destructor is that of the object being
 * freed, and the destructor may return a value which will be the return
 * value of nih_free() or nih_discard() if used directly on the object.
 *
 * Since objects may also be freed by unreferencing, and the value is not
 * returned in this case, it should only be used for informational or
 * debugging purposes.
 **/
void
nih_alloc_real_set_destructor (const void *  ptr,
			       NihDestructor destructor)
{
	NihAllocCtx *ctx;

	nih_assert (ptr != NULL);

	ctx = NIH_ALLOC_CTX (ptr);
	nih_assert (ctx->destructor != NIH_ALLOC_FINALISED);

	ctx->destructor = destructor;
}


/**
 * nih_ref:
 * @ptr: object to reference,
 * @parent: new parent object.
 *
 * Adds a reference to the object @ptr from @parent, adding to any other
 * objects referencing @ptr.  @parent may be the special NULL parent.
 *
 * The reference can be broken using nih_unref().
 *
 * @ptr will only be automatically freed when the last parent unreferences
 * it.  It may still be manually freed with nih_free(), though this doesn't
 * sort out any pointers.
 *
 * This function is generally used when accepting an object that you wish
 * to hold a reference to, which is cheaper than making a copy.  The caller
 * must be careful to only use nih_discard() or nih_unref() to drop its own
 * reference.
 **/
void
nih_ref (const void *ptr,
	 const void *parent)
{
	nih_assert (ptr != NULL);

	nih_alloc_ref_new (NIH_ALLOC_CTX (parent), NIH_ALLOC_CTX (ptr));
}

/**
 * nih_alloc_ref_new:
 * @parent: parent context,
 * @child: child context.
 *
 * This is the internal function used by nih_ref() and nih_alloc() to
 * create a new reference between the @parent and @child contexts.
 *
 * Returns: new reference, already linked to both objects.
 **/
static inline NihAllocRef *
nih_alloc_ref_new (NihAllocCtx *parent,
		   NihAllocCtx *child)
{
	NihAllocRef *ref;

	nih_assert ((parent == NULL)
		    || (parent->destructor != NIH_ALLOC_FINALISED));
	nih_assert (child != NULL);
	nih_assert (child->destructor != NIH_ALLOC_FINALISED);

	ref = NIH_MUST (malloc (sizeof (NihAllocRef)));

	nih_list_init (&ref->children_entry);
	nih_list_init (&ref->parents_entry);

	ref->parent = parent;
	ref->child = child;

	if (parent)
		nih_list_add_after (&parent->children, &ref->children_entry);
	nih_list_add_after (&child->parents, &ref->parents_entry);

	return ref;
}


/**
 * nih_unref:
 * @ptr: object to unreference,
 * @parent: parent object to remove.
 *
 * Removes the reference to the object @ptr from @parent, if this is the
 * last reference to @ptr then @ptr will be automatically freed.  @parent
 * may be the special NULL parent.
 *
 * You never need to call this in your own destructors since children
 * are unreferenced automatically, however this function is useful if you
 * only hold a reference to an object for a short period and wish to
 * discard it.
 **/
void
nih_unref (void *      ptr,
	   const void *parent)
{
	NihAllocCtx *ctx;
	NihAllocRef *ref;

	nih_assert (ptr != NULL);

	ctx = NIH_ALLOC_CTX (ptr);
	nih_assert (ctx->destructor != NIH_ALLOC_FINALISED);

	ref = nih_alloc_ref_lookup (NIH_ALLOC_CTX (parent), ctx);

	nih_assert (ref != NULL);
	nih_alloc_ref_free (ref);

	if (NIH_LIST_EMPTY (&ctx->parents))
		nih_alloc_context_free (ctx);
}

/**
 * nih_alloc_ref_free:
 * @ref: reference to free.
 *
 * This is the internal function used by nih_free() and nih_unref() to
 * remove the reference @ref from its parent and child contexts.  It does
 * not free the child context, even if this is the last reference.
 *
 * This function is notably not called by nih_alloc_context_unref() since
 * that manipulates the references to perform finalisation.
 **/
static inline void
nih_alloc_ref_free (NihAllocRef *ref)
{
	nih_assert (ref != NULL);

	nih_list_destroy (&ref->children_entry);
	nih_list_destroy (&ref->parents_entry);

	free (ref);
}


/**
 * nih_alloc_parent:
 * @ptr: object to query,
 * @parent: parent object to look for.
 *
 * @parent may be the special NULL parent.
 *
 * Returns: TRUE if @parent has a reference to @ptr, FALSE otherwise.
 **/
int
nih_alloc_parent (const void *ptr,
		  const void *parent)
{
	NihAllocCtx *ctx;
	NihAllocRef *ref;

	nih_assert (ptr != NULL);

	ctx = NIH_ALLOC_CTX (ptr);
	nih_assert (ctx->destructor != NIH_ALLOC_FINALISED);

	ref = nih_alloc_ref_lookup (NIH_ALLOC_CTX (parent), ctx);

	return ref ? TRUE : FALSE;
}

/**
 * nih_alloc_ref_lookup:
 * @parent: parent context,
 * @child: child context.
 *
 * This is the internal function used by nih_unref() and nih_alloc_parent()
 * to lookup a reference between the @parent and @child contexts.  @parent
 * may be the special NULL parent.
 *
 * Returns: NihAllocRef structure or NULL if no reference exists.
 **/
static inline NihAllocRef *
nih_alloc_ref_lookup (NihAllocCtx *parent,
		      NihAllocCtx *child)
{
	nih_assert ((parent == NULL)
		    || (parent->destructor != NIH_ALLOC_FINALISED));
	nih_assert (child != NULL);
	nih_assert (child->destructor != NIH_ALLOC_FINALISED);

	NIH_LIST_FOREACH (&child->parents, iter) {
		NihAllocRef *ref = NIH_LIST_ITER (iter, NihAllocRef,
						  parents_entry);

		if (ref->parent == parent)
			return ref;
	}

	return NULL;
}


/**
 * nih_alloc_size:
 * @ptr: pointer to object.
 *
 * Returns: the size of the allocated object, which may be larger than
 * originally requested.
 **/
size_t
nih_alloc_size (const void *ptr)
{
	NihAllocCtx *ctx;

	nih_assert (ptr != NULL);

	ctx = NIH_ALLOC_CTX (ptr);
	nih_assert (ctx->destructor != NIH_ALLOC_FINALISED);

	return ctx->size;
}
