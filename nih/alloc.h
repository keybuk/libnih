/* libnih
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
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

/**
 * All functions in libnih use these functions to allocate and destroy
 * objects in memory, they wrap the usual malloc(), realloc() and free()
 * calls to provide a multi-reference hierarchial allocator.
 *
 * When you allocate an object using nih_alloc() or nih_new(), you pass
 * an optional parent.  This may be any other object allocated with these
 * functions.  A reference from the parent to the newly allocated object
 * is created.
 *
 * You may add additional references to the object using nih_ref(), again
 * passing any other object allocated with these functions are the parent.
 *
 * Thus any object may have zero or more parents.
 *
 * When an object is freed, the references to its children are discarded
 * and if it held the last reference to one of those children, the child
 * is freed as well.
 *
 * This takes away a lof the effort of object management; allocating the
 * members of a structure with the structure as a parent means that if you
 * free the structure, all of its members will be freed as well.
 *
 * You may still need to do additional clean-up, for example closing file
 * descriptors or other non-allocated resources.  You can set a destructor
 * function for the object with nih_alloc_set_destructor(), this is called
 * during the free process.
 *
 * To remove a reference to a child, potentially freeing it but without
 * freeing the parent object, use nih_unref().
 *
 * To free a top-level object, use nih_free().  nih_free() always frees the
 * object, even if it has parent references which it will discard.  This
 * obviously does not clean up any pointers in the parent object which
 * point at the freed child.
 *
 * In many situations, you will allocate an object using nih_alloc() with
 * no parent and pass that to functions which may take a reference to it.
 * When finished, you need to discard the object safely; if no references
 * were taken, it should be freed - otherwise it's safe to leave.  Use
 * nih_discard() instead of nih_free() to do this.
 **/

#include <nih/macros.h>


/**
 * NihDestructor:
 * @ptr: pointer to be destroyed.
 *
 * A destructor is a function that can be associated with an allocated
 * object in memory and is called before the object is freed.  The pointer
 * @ptr passed to the destructor is that of the object being freed.
 *
 * A typical use of a destructor would be to close a file descriptor held
 * by an object.
 *
 * When the destructor is called, the parent references to the object will
 * have already been discarded but all children references will be intact
 * and none of the children will have been freed.  There is no need to use
 * a destructor to unreference or free children, that is automatic.
 *
 * Returns: value returned by nih_free() or nih_discard() if used directly
 * on the object.
 **/
typedef int (*NihDestructor) (void *ptr);


/**
 * nih_new:
 * @parent: parent object for new object,
 * @type: type of data to store.
 *
 * Allocates an object in memory large enough to store a @type object
 * and returns a pointer to it.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned object.  When all parents
 * of the returned object are freed, the returned object will also be
 * freed.
 *
 * If you have clean-up that you would like to run, you can assign a
 * destructor using the nih_alloc_set_destructor() function.
 *
 * Returns: newly allocated object or NULL if insufficient memory.
 **/
#define nih_new(parent, type) (type *)nih_alloc(parent, sizeof (type))

/**
 * nih_alloc_set_destructor:
 * @ptr: pointer to object,
 * @destructor: destructor function to set.
 *
 * Sets the destructor of the allocated object @ptr to @destructor, which
 * may be NULL to unset an existing destructor.  This is a macro that casts
 * @destructor to the NihDestructor type, since almost all destructors
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
#define nih_alloc_set_destructor(ptr, destructor)	\
	nih_alloc_real_set_destructor (ptr, (NihDestructor)destructor)


NIH_BEGIN_EXTERN

void * nih_alloc                     (const void *parent, size_t size)
	__attribute__ ((warn_unused_result, malloc));

void * nih_realloc                   (void *ptr, const void *parent,
				      size_t size)
	__attribute__ ((warn_unused_result, malloc));

int    nih_free                      (void *ptr);
int    nih_discard                   (void *ptr);

void   nih_alloc_real_set_destructor (void *ptr, NihDestructor destructor);

void   nih_ref                       (void *ptr, const void *parent);
void   nih_unref                     (void *ptr, const void *parent);

int    nih_alloc_has_ref             (void *ptr, const void *parent);

size_t nih_alloc_size                (void *ptr);

NIH_END_EXTERN

#endif /* NIH_ALLOC_H */
