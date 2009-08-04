/* libnih
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
 * is created.  If no parent is passed the object is referenced from the
 * special NULL parent.
 *
 * You may add additional references to the object using nih_ref(), again
 * passing any other object allocated with these functions as the parent
 * or the special NULL parent.
 *
 * Thus any object may have one or more parents.  Indeed, an object
 * (including the NULL parent) may hold multiple references to another
 * object.
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
 *
 * Such constructs are often better handled using nih_local variables.
 *
 *
 * = Common patterns =
 *
 * At first, it seems like there's a bewildering array of different ways
 * you can shoot yourself in the foot with this library, however if you
 * stick to the usual patterns it's a very safe and versatile library.
 *
 * == Structures ==
 *
 * When allocating structures, you tend to write your function so that
 * the prospective parent is passed in as the first argument and always
 * allocate with that.
 *
 *   obj = nih_new (parent, Object);
 *
 * That way, the caller decides how they want your object linked to
 * other things.  If there's an error while populating the structure,
 * the standard style is just to call nih_free() rather than
 * unreferencing
 *
 *   error:
 *     nih_free (obj);
 *     return NULL;
 *
 * Since you're putting it together, this use of nih_free() is perfectly
 * acceptable.
 *
 * == Structure members ==
 *
 * Structure members are just about always allocated with the structure
 * as their parent context.
 *
 *   obj->child = nih_new (obj, Child);
 *
 * This pretty much saves you from ever worrying about them, as they
 * will be automatically freed whenever you free the parent object;
 * including error handling cases.
 *
 * Should you ever replace the child, you shouldn't call nih_free() but
 * nih_unref(), to be safe against other code having taken a reference.
 *
 *   nih_unref (obj->child, obj);
 *   obj->child = nih_new (obj, Child);
 *
 * This will clean up the child, unless someone else is using it.
 *
 * == Floating objects ==
 *
 * Often in a function you'll want to allocate an object but won't yet
 * have anything to attach it to.  This also often applies to global
 * variables as well.
 *
 * You simply pass NULL in as the parent; the returned object has only
 * this special reference.
 *
 *   obj = nih_new (NULL, Object);
 *
 * To discard the floating object you should use nih_discard() instead
 * of nih_free(), which will not free the object if another function
 * you've called in the meantime took a reference to it.
 *
 * Better yet, use nih_local to have the object automatically discarded
 * when it goes out of scope:
 *
 *   {
 *     nih_local Object *obj = NULL;
 *     obj = nih_new (NULL, Object);
 *
 *     // work with obj, including passing it to functions that may
 *     // reference it
 *   }
 *
 * == Taking a reference ==
 *
 * Provided the above patterns are followed, taking a reference to an
 * object you are passed is perfectly safe.  Simply call nih_ref(),
 * for example to store it in your own structure:
 *
 *   adopt->obj = obj;
 *   nih_ref (adopt->obj, adopt);
 *
 * When you want to drop your reference, you should only ever use
 * nih_unref().
 *
 *   nih_unref (adopt->obj, adopt);
 *   adopt->obj = NULL;
 *
 * == Returning a member ==
 *
 * This is a relatively rare case, but examples exist.
 *
 * Sometimes you want to provide a function that returns one of your
 * structure members, disowning it in the process.  Your function will
 * most likely take a parent object to which you want to reparent the
 * member.
 *
 * This is as easy as referencing the new parent and dropping your own
 * reference.
 *
 *   nih_ref (child, parent);
 *
 *   child = obj->child;
 *   obj->child = NULL;
 *
 *   nih_unref (obj->child, obj);
 *
 *   // child may now be returned
 *
 * == Worker objects ==
 *
 * Finally another pattern exits in the nih_alloc() world that doesn't
 * quite obey the above rules, and instead takes advantage of the design
 * to provide something that wouldn't be possible otherwise.
 *
 * A worker is an object that performs some function out-of-band on
 * behalf of another object.  They may be stored elsewhere, such as a
 * linked list, and will be set up such that if they are freed, the
 * work they are doing is cancelled.
 *
 * A good example would be a timer object; you'd have a list of timers
 * which you iterate in the main loop.  The destructor of the timer
 * object removes it from this list.  When the timer expires, some
 * work occurs, and the timer object frees itself.
 *
 * Thus to attach a timer to our object, all we need do is create the
 * timer with our object as a parent.  There is absolutely no need
 * to store the timer as a structure member, unless we would need to
 * cancel it for other reasons.
 *
 * If our object is freed, the timer is freed to so there's no danger
 * of the callback firing and acting on a freed object.  If the timer
 * fires, the callback can do its work, and the timer will be freed
 * afterwards.
 *
 * Much of the main loop related objects in libnih behave in this way.
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
 * will be used as a parent for the returned object otherwise the special
 * NULL parent will be used instead.  When all parents of the returned
 * object are freed, the returned object will also be freed.
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


/**
 * nih_local:
 *
 * This macro may be used in a variable definition when the variable
 * should be automatically cleaned up when it goes out of scope.  You
 * should ensure that the pointer is either allocated with nih_alloc()
 * or set to NULL;
 *
 *   {
 *     nih_local char *foo = NULL;
 *
 *     foo = nih_strdup (NULL, "some data");
 *   } // foo is automatically discarded
 *
 * It is permissible to take references to foo within its scope, or by
 * functions called, in which case it will not be freed.  Also it is
 * generally nonsensical to allocate with a parent, since this too will
 * prevent it from beign freed.
 **/
#define nih_local __attribute__ ((cleanup(_nih_discard_local)))


NIH_BEGIN_EXTERN

void * nih_alloc                     (const void *parent, size_t size)
	__attribute__ ((warn_unused_result, malloc));

void * nih_realloc                   (void *ptr, const void *parent,
				      size_t size)
	__attribute__ ((warn_unused_result, malloc));

int    nih_free                      (void *ptr);
int    nih_discard                   (void *ptr);
void   _nih_discard_local            (void *ptraddr);

void   nih_alloc_real_set_destructor (const void *ptr,
				      NihDestructor destructor);

void   nih_ref                       (const void *ptr, const void *parent);
void   nih_unref                     (void *ptr, const void *parent);

int    nih_alloc_parent              (const void *ptr, const void *parent);

size_t nih_alloc_size                (const void *ptr);

NIH_END_EXTERN

#endif /* NIH_ALLOC_H */
