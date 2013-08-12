/*
 * Copyright © 2010 Intel Corporation
 * Copyright © 2010 Francisco Jerez <currojerez@riseup.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#ifndef _GLVND_LIST_H_
#define _GLVND_LIST_H_

#include <stddef.h> /* offsetof() */

/**
 * @file Classic doubly-link circular list implementation.
 * For real usage examples of the linked list, see the file test/list.c
 *
 * Example:
 * We need to keep a list of struct foo in the parent struct bar, i.e. what
 * we want is something like this.
 *
 *     struct bar {
 *          ...
 *          struct foo *list_of_foos; -----> struct foo {}, struct foo {}, struct foo{}
 *          ...
 *     }
 *
 * We need one list head in bar and a list element in all list_of_foos (both are of
 * data type 'struct glvnd_list').
 *
 *     struct bar {
 *          ...
 *          struct glvnd_list list_of_foos;
 *          ...
 *     }
 *
 *     struct foo {
 *          ...
 *          struct glvnd_list entry;
 *          ...
 *     }
 *
 * Now we initialize the list head:
 *
 *     struct bar bar;
 *     ...
 *     glvnd_list_init(&bar.list_of_foos);
 *
 * Then we create the first element and add it to this list:
 *
 *     struct foo *foo = malloc(...);
 *     ....
 *     glvnd_list_add(&foo->entry, &bar.list_of_foos);
 *
 * Repeat the above for each element you want to add to the list. Deleting
 * works with the element itself.
 *      glvnd_list_del(&foo->entry);
 *      free(foo);
 *
 * Note: calling glvnd_list_del(&bar.list_of_foos) will set bar.list_of_foos to an empty
 * list again.
 *
 * Looping through the list requires a 'struct foo' as iterator and the
 * name of the field the subnodes use.
 *
 * struct foo *iterator;
 * glvnd_list_for_each_entry(iterator, &bar.list_of_foos, entry) {
 *      if (iterator->something == ...)
 *             ...
 * }
 *
 * Note: You must not call glvnd_list_del() on the iterator if you continue the
 * loop. You need to run the safe for-each loop instead:
 *
 * struct foo *iterator, *next;
 * glvnd_list_for_each_entry_safe(iterator, next, &bar.list_of_foos, entry) {
 *      if (...)
 *              glvnd_list_del(&iterator->entry);
 * }
 *
 */

/**
 * The linkage struct for list nodes. This struct must be part of your
 * to-be-linked struct. struct glvnd_list is required for both the head of the
 * list and for each list node.
 *
 * Position and name of the struct glvnd_list field is irrelevant.
 * There are no requirements that elements of a list are of the same type.
 * There are no requirements for a list head, any struct glvnd_list can be a list
 * head.
 */
struct glvnd_list {
    struct glvnd_list *next, *prev;
};

/**
 * Initialize the list as an empty list.
 *
 * Example:
 * glvnd_list_init(&bar->list_of_foos);
 *
 * @param The list to initialized.
 */
static inline void
glvnd_list_init(struct glvnd_list *list)
{
    list->next = list->prev = list;
}

static inline void
__glvnd_list_add(struct glvnd_list *entry,
                struct glvnd_list *prev, struct glvnd_list *next)
{
    next->prev = entry;
    entry->next = next;
    entry->prev = prev;
    prev->next = entry;
}

/**
 * Insert a new element after the given list head. The new element does not
 * need to be initialised as empty list.
 * The list changes from:
 *      head → some element → ...
 * to
 *      head → new element → older element → ...
 *
 * Example:
 * struct foo *newfoo = malloc(...);
 * glvnd_list_add(&newfoo->entry, &bar->list_of_foos);
 *
 * @param entry The new element to prepend to the list.
 * @param head The existing list.
 */
static inline void
glvnd_list_add(struct glvnd_list *entry, struct glvnd_list *head)
{
    __glvnd_list_add(entry, head, head->next);
}

/**
 * Append a new element to the end of the list given with this list head.
 *
 * The list changes from:
 *      head → some element → ... → lastelement
 * to
 *      head → some element → ... → lastelement → new element
 *
 * Example:
 * struct foo *newfoo = malloc(...);
 * glvnd_list_append(&newfoo->entry, &bar->list_of_foos);
 *
 * @param entry The new element to prepend to the list.
 * @param head The existing list.
 */
static inline void
glvnd_list_append(struct glvnd_list *entry, struct glvnd_list *head)
{
    __glvnd_list_add(entry, head->prev, head);
}

static inline void
__glvnd_list_del(struct glvnd_list *prev, struct glvnd_list *next)
{
    next->prev = prev;
    prev->next = next;
}

/**
 * Remove the element from the list it is in. Using this function will reset
 * the pointers to/from this element so it is removed from the list. It does
 * NOT free the element itself or manipulate it otherwise.
 *
 * Using glvnd_list_del on a pure list head (like in the example at the top of
 * this file) will NOT remove the first element from
 * the list but rather reset the list as empty list.
 *
 * Example:
 * glvnd_list_del(&foo->entry);
 *
 * @param entry The element to remove.
 */
static inline void
glvnd_list_del(struct glvnd_list *entry)
{
    __glvnd_list_del(entry->prev, entry->next);
    glvnd_list_init(entry);
}

/**
 * Check if the list is empty.
 *
 * Example:
 * glvnd_list_is_empty(&bar->list_of_foos);
 *
 * @return True if the list contains one or more elements or False otherwise.
 */
static inline int
glvnd_list_is_empty(struct glvnd_list *head)
{
    return head->next == head;
}

/**
 * Returns a pointer to the container of this list element.
 *
 * Example:
 * struct foo* f;
 * f = glvnd_container_of(&foo->entry, struct foo, entry);
 * assert(f == foo);
 *
 * @param ptr Pointer to the struct glvnd_list.
 * @param type Data type of the list element.
 * @param member Member name of the struct glvnd_list field in the list element.
 * @return A pointer to the data struct containing the list head.
 */
#ifndef glvnd_container_of
#define glvnd_container_of(ptr, type, member) \
    (type *)((char *)(ptr) - offsetof(type, member))
#endif

/**
 * Alias of glvnd_container_of
 */
#define glvnd_list_entry(ptr, type, member) \
    glvnd_container_of(ptr, type, member)

/**
 * Retrieve the first list entry for the given list pointer.
 *
 * Example:
 * struct foo *first;
 * first = glvnd_list_first_entry(&bar->list_of_foos, struct foo, list_of_foos);
 *
 * @param ptr The list head
 * @param type Data type of the list element to retrieve
 * @param member Member name of the struct glvnd_list field in the list element.
 * @return A pointer to the first list element.
 */
#define glvnd_list_first_entry(ptr, type, member) \
    glvnd_list_entry((ptr)->next, type, member)

/**
 * Retrieve the last list entry for the given listpointer.
 *
 * Example:
 * struct foo *first;
 * first = glvnd_list_last_entry(&bar->list_of_foos, struct foo, list_of_foos);
 *
 * @param ptr The list head
 * @param type Data type of the list element to retrieve
 * @param member Member name of the struct glvnd_list field in the list element.
 * @return A pointer to the last list element.
 */
#define glvnd_list_last_entry(ptr, type, member) \
    glvnd_list_entry((ptr)->prev, type, member)

#ifdef HAVE_TYPEOF
#define __glvnd_container_of(ptr, sample, member)			\
    glvnd_container_of(ptr, typeof(*sample), member)
#else
/* This implementation of __glvnd_container_of has undefined behavior according
 * to the C standard, but it works in many cases.  If your compiler doesn't
 * support typeof() and fails with this implementation, please try a newer
 * compiler.
 */
#define __glvnd_container_of(ptr, sample, member)                            \
    (void *)((char *)(ptr)                                             \
            - ((char *)&(sample)->member - (char *)(sample)))
#endif

/**
 * Loop through the list given by head and set pos to struct in the list.
 *
 * Example:
 * struct foo *iterator;
 * glvnd_list_for_each_entry(iterator, &bar->list_of_foos, entry) {
 *      [modify iterator]
 * }
 *
 * This macro is not safe for node deletion. Use glvnd_list_for_each_entry_safe
 * instead.
 *
 * @param pos Iterator variable of the type of the list elements.
 * @param head List head
 * @param member Member name of the struct glvnd_list in the list elements.
 *
 */
#define glvnd_list_for_each_entry(pos, head, member)				\
    for (pos = __glvnd_container_of((head)->next, pos, member);		\
	 &pos->member != (head);					\
	 pos = __glvnd_container_of(pos->member.next, pos, member))

/**
 * Loop through the list, keeping a backup pointer to the element. This
 * macro allows for the deletion of a list element while looping through the
 * list.
 *
 * See glvnd_list_for_each_entry for more details.
 */
#define glvnd_list_for_each_entry_safe(pos, tmp, head, member)		\
    for (pos = __glvnd_container_of((head)->next, pos, member),		\
	 tmp = __glvnd_container_of(pos->member.next, pos, member);		\
	 &pos->member != (head);					\
	 pos = tmp, tmp = __glvnd_container_of(pos->member.next, tmp, member))

/* NULL-Terminated List Interface
 *
 * The interface below does _not_ use the struct glvnd_list as described above.
 * It is mainly for legacy structures that cannot easily be switched to
 * struct glvnd_list.
 *
 * This interface is for structs like
 *      struct foo {
 *          [...]
 *          struct foo *next;
 *           [...]
 *      };
 *
 * The position and field name of "next" are arbitrary.
 */

/**
 * Init the element as null-terminated list.
 *
 * Example:
 * struct foo *list = malloc();
 * glvnd_nt_list_init(list, next);
 *
 * @param list The list element that will be the start of the list
 * @param member Member name of the field pointing to next struct
 */
#define glvnd_nt_list_init(_list, _member) \
	(_list)->_member = NULL

/**
 * Returns the next element in the list or NULL on termination.
 *
 * Example:
 * struct foo *element = list;
 * while ((element = glvnd_nt_list_next(element, next)) { }
 *
 * This macro is not safe for node deletion. Use glvnd_nt_list_for_each_entry_safe
 * instead.
 *
 * @param list The list or current element.
 * @param member Member name of the field pointing to next struct.
 */
#define glvnd_nt_list_next(_list, _member) \
	(_list)->_member

/**
 * Iterate through each element in the list.
 *
 * Example:
 * struct foo *iterator;
 * glvnd_nt_list_for_each_entry(iterator, list, next) {
 *      [modify iterator]
 * }
 *
 * @param entry Assigned to the current list element
 * @param list The list to iterate through.
 * @param member Member name of the field pointing to next struct.
 */
#define glvnd_nt_list_for_each_entry(_entry, _list, _member)			\
	for (_entry = _list; _entry; _entry = (_entry)->_member)

/**
 * Iterate through each element in the list, keeping a backup pointer to the
 * element. This macro allows for the deletion of a list element while
 * looping through the list.
 *
 * See glvnd_nt_list_for_each_entry for more details.
 *
 * @param entry Assigned to the current list element
 * @param tmp The pointer to the next element
 * @param list The list to iterate through.
 * @param member Member name of the field pointing to next struct.
 */
#define glvnd_nt_list_for_each_entry_safe(_entry, _tmp, _list, _member)	\
	for (_entry = _list, _tmp = (_entry) ? (_entry)->_member : NULL;\
		_entry;							\
		_entry = _tmp, _tmp = (_tmp) ? (_tmp)->_member: NULL)

/**
 * Append the element to the end of the list. This macro may be used to
 * merge two lists.
 *
 * Example:
 * struct foo *elem = malloc(...);
 * glvnd_nt_list_init(elem, next)
 * glvnd_nt_list_append(elem, list, struct foo, next);
 *
 * Resulting list order:
 * list_item_0 -> list_item_1 -> ... -> elem_item_0 -> elem_item_1 ...
 *
 * @param entry An entry (or list) to append to the list
 * @param list The list to append to. This list must be a valid list, not
 * NULL.
 * @param type The list type
 * @param member Member name of the field pointing to next struct
 */
#define glvnd_nt_list_append(_entry, _list, _type, _member)		        \
    do {								\
	_type *__iterator = _list;					\
	while (__iterator->_member) { __iterator = __iterator->_member;}\
	__iterator->_member = _entry;					\
    } while (0)

/**
 * Insert the element at the next position in the list. This macro may be
 * used to insert a list into a list.
 *
 * struct foo *elem = malloc(...);
 * glvnd_nt_list_init(elem, next)
 * glvnd_nt_list_insert(elem, list, struct foo, next);
 *
 * Resulting list order:
 * list_item_0 -> elem_item_0 -> elem_item_1 ... -> list_item_1 -> ...
 *
 * @param entry An entry (or list) to append to the list
 * @param list The list to insert to. This list must be a valid list, not
 * NULL.
 * @param type The list type
 * @param member Member name of the field pointing to next struct
 */
#define glvnd_nt_list_insert(_entry, _list, _type, _member)			\
    do {								\
	glvnd_nt_list_append((_list)->_member, _entry, _type, _member);	\
	(_list)->_member = _entry;					\
    } while (0)

/**
 * Delete the entry from the list by iterating through the list and
 * removing any reference from the list to the entry.
 *
 * Example:
 * struct foo *elem = <assign to right element>
 * glvnd_nt_list_del(elem, list, struct foo, next);
 *
 * @param entry The entry to delete from the list. entry is always
 * re-initialized as a null-terminated list.
 * @param list The list containing the entry, set to the new list without
 * the removed entry.
 * @param type The list type
 * @param member Member name of the field pointing to the next entry
 */
#define glvnd_nt_list_del(_entry, _list, _type, _member)		\
	do {							\
		_type *__e = _entry;				\
		if (__e == NULL || _list == NULL) break;        \
		if ((_list) == __e) {				\
		    _list = __e->_member;			\
		} else {					\
		    _type *__prev = _list;			\
		    while (__prev->_member && __prev->_member != __e)	\
			__prev = glvnd_nt_list_next(__prev, _member);	\
		    if (__prev->_member)			\
			__prev->_member = __e->_member;		\
		}						\
		glvnd_nt_list_init(__e, _member);			\
	} while(0)

#endif
