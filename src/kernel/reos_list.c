#include <assert.h>
#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include "reos_list.h"
#include "reos_stdlib.h"

/**
 * \file
 *
 * Several specialized stack and queue implementations are used in ReOS:
 * ReOS_SimpleList, ReOS_CompoundList, and IndexList. Despite their names, the first two are not
 * intended for random access and only achieve O(n) time if used in such a way.
 * This is because they are all types of linked lists; hence the "List" part of
 * their names. However, appending, prepending and removing from either end are
 * all constant-time operations.
 *
 * Most of the time, ReOS_CompoundList is appropriate, since it is a malloc()-optimized
 * version of ReOS_SimpleList. Using the latter is only advantageous when space usage
 * must be kept absolutely minimal. However, both are quite small when empty.
 *
 * We can assume that if you're reading this, you're probably the type of person
 * who knows how a linked list works, so we can stick to the important stuff.
 * List functions operate on the \c head or the \c tail end of the list.
 * Directions can be freely mixed on a single list; for example, adding elements
 * with reos_simplelist_push_head() and removing them with reos_simplelist_pop_head()
 * emulates a stack, while adding with reos_simplelist_push_head() and removing with
 * reos_simplelist_pop_tail() emulates a queue. Performance is identical in both
 * directions.
 *
 * \section ReOS_SimpleList
 * ReOS_SimpleList is an ordinary doubly-linked list that keeps pointers to its
 * \c head and \c tail. It also keeps a \c destructor that is called on every element
 * in the list when free_reos_simplelist() is called. The \c destructor is not called
 * when elements are removed with reos_simplelist_pop_head() or reos_simplelist_pop_tail().
 *
 * To save calls to malloc(), instead of free()ing a ReOS_SimpleListNode when its element
 * is removed, the node is appended to \c free_head. The next simplelist_push_*()
 * call then pops a free ReOS_SimpleListNode off this internal list, trading a little
 * extra space for a big gain in execution performance.
 *
 * \section ReOS_CompoundList
 * ReOS_CompoundList, as its name implies, is a linked list of arrays that attempts
 * to minimize allocations and deallocations. When the data structure runs out
 * of space, it pushes a new ReOS_CompoundListNode onto its head or tail that
 * contains an array twice the length of the last one allocated. It uses
 * internal indexes to emulate a normal linked list. Like ReOS_SimpleList, it keeps
 * an internal list of freed nodes at \c free_head to minimize malloc() and
 * free() calls.
 *
 * \sa compoundlist_example.c in the Examples section.
 */

/**
 * \example doc/compoundlist_example.c
 *
 * This example demonstrates the usage and internal structure of a ReOS_CompoundList.
 * It's really not all that complicated. Although I originally created this code
 * for debugging, it still makes for a pretty nice show. Immediately below is
 * the code's output; below that is the code itself.
 *
 * \verbinclude doc/compoundlist_example_output
 */

#define list_free_head(node_type, impl) \
	{ \
		node_type *next = impl->head->next; \
		impl->head->next = impl->free_head; \
		impl->free_head = impl->head; \
		impl->head = next; \
		impl->head->prev = 0; \
	}

#define list_free_tail(node_type, impl) \
	{ \
		node_type *prev = impl->tail->prev; \
		impl->tail->next = impl->free_head; \
		impl->free_head = impl->tail; \
		impl->tail = prev; \
		impl->tail->next = 0; \
	}

#ifdef OPTIMIZE_FOR_SIZE
#define CLIST_MAX_LEN(node) (node)->max_len
#else
#define CLIST_MAX_LEN(node) (CACHE_LINE_SIZE / sizeof(void *))
#endif

static ReOS_SimpleListNode *new_reos_simplelistnode();
static void free_reos_simplelistnode(ReOS_SimpleListNode *, VoidPtrFunc);

static ReOS_CompoundListImpl *new_reos_compoundlistimpl(int, VoidPtrFunc, CloneFunc);

#ifdef OPTIMIZE_FOR_SIZE
static ReOS_CompoundListNode *new_reos_compoundlistnode(int);
#else
static ReOS_CompoundListNode *new_reos_compoundlistnode();
#endif

static void free_reos_compoundlistnode(ReOS_CompoundListNode *, VoidPtrFunc);
static ReOS_CompoundListNode *reos_compoundlistnode_clone(ReOS_CompoundListNode *, CloneFunc);
static void compoundlist_push_head_node(ReOS_CompoundList *);
static void compoundlist_push_tail_node(ReOS_CompoundList *);

static ReOS_JudyListImpl *new_reos_judylistimpl(VoidPtrFunc, VoidPtrFunc, CloneFunc,
									  VoidPtrFunc);

/**
 * Allocates a new ReOS_SimpleList object. The \c head and \c tail are not allocated,
 * keeping space to a minimum.
 *
 * \param destructor The destructor that will be called on every element
 * when freeing the list
 * \return A new ReOS_SimpleList object
 */
ReOS_SimpleList *new_reos_simplelist(VoidPtrFunc destructor)
{
	ReOS_SimpleList *l = malloc(sizeof(ReOS_SimpleList));
	l->head = 0;
	l->tail = 0;
	l->free_head = 0;
	l->destructor = destructor;
	return l;
}

/**
 * Frees all nodes, including those in \c free_head, and calls \c destructor on
 * the elements of non-free nodes.
 *
 * \param l The list to free
 */
void free_reos_simplelist(ReOS_SimpleList *l)
{
	if (l) {
		ReOS_SimpleListNode *n = l->head;
		while (n) {
			ReOS_SimpleListNode *prev = n;
			n = n->next;
			free_reos_simplelistnode(prev, l->destructor);
		}

		n = l->free_head;
		while (n) {
			ReOS_SimpleListNode *prev = n;
			n = n->next;
			free_reos_simplelistnode(prev, 0);
		}

		free(l);
	}
}

static ReOS_SimpleListNode *new_reos_simplelistnode()
{
	ReOS_SimpleListNode *n = malloc(sizeof(ReOS_SimpleListNode));
	n->next = 0;
	n->prev = 0;
	return n;
}

static void free_reos_simplelistnode(ReOS_SimpleListNode *n, VoidPtrFunc destructor)
{
	if (n) {
		if (destructor)
			destructor(n->data);
		free(n);
	}
}

/**
 * Allocates and returns a copy of a list. If \a func is not \c NULL, \c func
 * will be called on every element in the list and their return values added to
 * the clone. Otherwise, the element pointers will simply be copied over.
 *
 * \param l The list to copy
 * \param func The function to copy list elements, or \c NULL
 */
ReOS_SimpleList *reos_simplelist_clone(ReOS_SimpleList *l, CloneFunc func)
{
	ReOS_SimpleList *clone = new_reos_simplelist(l->destructor);

	foreach_simple(void, d, l) {
		if (func)
			reos_simplelist_push_tail(clone, func(d));
		else
			reos_simplelist_push_tail(clone, d);
	}

	return clone;
}

/**
 * Prettyprints a list to standard output.
 *
 * \param l The list to print
 */
void reos_simplelist_dump(ReOS_SimpleList *l)
{
	printf("simplelist %p: head %p... tail %p, free_head %p\n", l, l->head,
		   l->tail, l->free_head);

	ReOS_SimpleListNode *node = l->head;
	while (node) {
		printf("prev %p <- this %p, value %p -> next %p\n", node->prev, node, node->data, node->next);
		node = node->next;
	}

	if (l->free_head) {
		printf("\nfree_head: %p\n", l->free_head);
		node = l->free_head;
		while (node) {
			printf("this %p -> next %p\n", node, node->next);
			node = node->next;
		}
	}
	printf("\n");
}

/**
 * Prepends \a element to the list \c head.
 *
 * \param l The list to prepend \a element to
 * \param element Any pointer
 *
 * \sa reos_simplelist_pop_head()
 */
void reos_simplelist_push_head(ReOS_SimpleList *l, void *element)
{
	ReOS_SimpleListNode *node;

	if (l->free_head) {
		node = l->free_head;
		l->free_head = l->free_head->next;
		node->prev = 0;
	}
	else
		node = new_reos_simplelistnode();

	node->data = element;
	node->next = l->head;
	if (l->head) {
		l->head->prev = node;
		l->head = l->head->prev;
	}
	else
		l->head = l->tail = node;
}

/**
 * Appends \a element to the list \c tail.
 *
 * \param l The list to append \a element to
 * \param element Any pointer
 *
 * \sa reos_simplelist_pop_tail()
 */
void reos_simplelist_push_tail(ReOS_SimpleList *l, void *element)
{
	ReOS_SimpleListNode *node;

	if (l->free_head) {
		node = l->free_head;
		l->free_head = l->free_head->next;
		node->next = 0;
	}
	else
		node = new_reos_simplelistnode();

	node->data = element;
	node->prev = l->tail;
	if (l->tail) {
		l->tail->next = node;
		l->tail = l->tail->next;
	}
	else
		l->head = l->tail = node;
}

/**
 * Removes and returns an element from the list \c head. This function does not modify the
 * element, and the list's \c destructor is not called on it.
 *
 * \param l The list to remove an element from
 * \return The element at \c head
 *
 * \sa reos_simplelist_push_head()
 */
void *reos_simplelist_pop_head(ReOS_SimpleList *l)
{
	void *d = l->head->data;
	list_free_head(ReOS_SimpleListNode, l);
	return d;
}

/**
 * Removes and returns an element from the list \c tail. This function does not modify the
 * element, and the list's \c destructor is not called on it.
 *
 * \param l The list to remove an element from
 * \return The element at \c tail
 *
 * \sa reos_simplelist_push_tail()
 */
void *reos_simplelist_pop_tail(ReOS_SimpleList *l)
{
	void *d = l->tail->data;
	list_free_tail(ReOS_SimpleListNode, l);
	return d;
}

/**
 * Returns the element at the list \c head without removing it from the list.
 *
 * \param l The list to peek
 * \return The element at \c head
 */
void *reos_simplelist_peek_head(ReOS_SimpleList *l)
{
	return l->head->data;
}

/**
 * Returns the element at the list \c tail without removing it from the list.
 *
 * \param l The list to peek
 * \return The element at \c tail
 */
void *reos_simplelist_peek_tail(ReOS_SimpleList *l)
{
	return l->tail->data;
}

/**
 * Initializes a pre-allocated iterator. It is recommended that \a iter be
 * allocated on the stack for speed. The results of modifying a list while
 * iterating over it are undefined. ReOS_SimpleListIter advances in
 * <tt>head</tt>-to-<tt>tail</tt> order.
 *
 * \param iter A pre-allocated iterator
 * \param l The list to iterate over
 */
void reos_simplelistiter_init(ReOS_SimpleListIter *iter, ReOS_SimpleList *l)
{
	iter->current = l->head;
}

/**
 * Tests whether there is another element after the current iterator position.
 *
 * \param iter The iterator to test
 */
int reos_simplelistiter_has_next_func(ReOS_SimpleListIter *iter)
{
	return reos_simplelistiter_has_next_body(iter);
}

/**
 * Returns the current element under the iterator and advances the iterator
 * position.
 *
 * \param iter The iterator to advance
 */
void *reos_simplelistiter_get_next(ReOS_SimpleListIter *iter)
{
	void *d = iter->current->data;
	iter->current = iter->current->next;
	return d;
}

/**
 * Returns the current element under the iterator without advancing the iterator
 * position.
 *
 * \param iter The iterator to peek
 */
void *reos_simplelistiter_peek_next(ReOS_SimpleListIter *iter)
{
	return iter->current->data;
}

/**
 * Allocates a new ReOS_CompoundList object. The \c head and \c tail are not
 * allocated, keeping space to a minimum. The object is initialized with a
 * reference count of 1.
 *
 * \param len The array length that will be allocated for the first node,
 * doubling for every node appended
 * \param destructor A function that will be called on every element
 * when freeing the list, or NULL
 * \param clone_element A function that clones an element of the list, or NULL
 * \return A new ReOS_SimpleList object
 *
 * \memberof ReOS_CompoundList
 */
ReOS_CompoundList *new_reos_compoundlist(int len, VoidPtrFunc destructor,
							CloneFunc clone_element)
{
	ReOS_CompoundList *l = malloc(sizeof(ReOS_CompoundList));
	l->impl = new_reos_compoundlistimpl(len, destructor, clone_element);
	return l;
}

static ReOS_CompoundListImpl *new_reos_compoundlistimpl_no_head(int len,
								VoidPtrFunc destructor,
								 CloneFunc clone_element)
{
	ReOS_CompoundListImpl *impl = malloc(sizeof(ReOS_CompoundListImpl));
	impl->refs = 1;
	impl->head = 0;
	impl->tail = 0;
	impl->free_head = 0;
	impl->destructor = destructor;
	impl->clone_element = clone_element;
	return impl;
}

static ReOS_CompoundListImpl *new_reos_compoundlistimpl(int len,
							VoidPtrFunc destructor,
							CloneFunc clone_element)
{
	ReOS_CompoundListImpl *impl = new_reos_compoundlistimpl_no_head(len,
									destructor,
									clone_element);

#ifdef OPTIMIZE_FOR_SIZE
	impl->head = new_reos_compoundlistnode(len);
#else
	impl->head = new_reos_compoundlistnode();
#endif

	impl->tail = impl->head;
	return impl;
}

#ifdef OPTIMIZE_FOR_SIZE
static ReOS_CompoundListNode *new_reos_compoundlistnode(int max_len)
#else
static ReOS_CompoundListNode *new_reos_compoundlistnode()
#endif
{
	ReOS_CompoundListNode *node = malloc(sizeof(ReOS_CompoundListNode));
	node->pos = 0;
	node->len = 0;
#ifdef OPTIMIZE_FOR_SIZE
	node->max_len = max_len;
#endif

	long requested_size;
#if defined(OPTIMIZE_FOR_SIZE) && !defined(CONST_CLIST_SIZE)
	do {
		requested_size = sizeof(void *)*max_len;
#else
		requested_size = CACHE_LINE_SIZE;
#endif
		node->array = malloc(requested_size);

#ifdef OPTIMIZE_FOR_SIZE
	} while (!node->array && (max_len/=2) > 0);
#endif

	if (!node->array) {
#ifdef OPTIMIZE_FOR_SIZE
		fprintf(stderr, "malloc failure: could not allocate compoundlistnode of size %d taking %ld bytes\n",
				max_len, requested_size);
#else
		fprintf(stderr, "malloc failure: could not allocate compoundlistnode of size %d taking %ld bytes\n",
				CACHE_LINE_SIZE, requested_size);
#endif

		abort();
	}

#if defined(HAS_MALLOC_USABLE_SIZE) && defined(OPTIMIZE_FOR_SIZE)
	long available_size = malloc_usable_size(node->array);
	node->max_len = available_size / sizeof(void *);
#ifndef NDEBUG
	// technically unnecessary, but shuts valgrind up
	if (available_size > requested_size)
		node->array = realloc(node->array, available_size);
#endif
#endif

	node->next = 0;
	node->prev = 0;
	return node;
}

/**
 * Decrements a list's reference count. This function should be called every
 * time a pointer to \c l is destroyed. If the list's reference count is 0, this
 * function frees all nodes, including those in \c free_head, and calls \c
 * destructor on the elements of non-free nodes.
 *
 * \param l The list to dereference
 * \return The new reference count
 *
 * \memberof ReOS_CompoundList
 */
int free_reos_compoundlist(ReOS_CompoundList *l)
{
	if (l) {
		ReOS_CompoundListImpl *impl = l->impl;

		impl->refs--;
		if (impl->refs > 0) {
			free(l);
			return impl->refs;
		}

		ReOS_CompoundListNode *prev;
		ReOS_CompoundListNode *node = impl->head;
		while (node)
		{
			prev = node;
			node = node->next;
			free_reos_compoundlistnode(prev, impl->destructor);
		}

		node = impl->free_head;
		while (node)
		{
			prev = node;
			node = node->next;
			free_reos_compoundlistnode(prev, 0);
		}

		free(impl);
		free(l);
	}

	return 0;
}

static void free_reos_compoundlistnode(ReOS_CompoundListNode *node, VoidPtrFunc destructor)
{
	if (node) {
		if (destructor) {
			int i;
			for (i = node->pos; i < node->len; i++)
				destructor(node->array[i]);
		}

		free(node->array);
		free(node);
	}
}

/**
 * Allocates and returns a copy of a list. If \a func is not \c NULL, \c
 * clone_func will be called on every element in the list and their return
 * values added to the clone. Otherwise, the element pointers will simply be
 * copied over.
 *
 * \param l The list to copy
 * \param clone_func The function to copy list elements, or \c NULL
 *
 * \memberof ReOS_CompoundList
 */
ReOS_CompoundList *reos_compoundlist_clone(ReOS_CompoundList *l)
{
	ReOS_CompoundList *clone = malloc(sizeof(ReOS_CompoundList));
	if (l->impl->refs != -1) {
		clone->impl = l->impl;
		clone->impl->refs++;
	}
	else {
		clone->impl = reos_compoundlistimpl_clone(l->impl);
		clone->impl->refs = -1;
	}

	return clone;
}

ReOS_CompoundList *reos_compoundlist_clone_with_func(ReOS_CompoundList *l, CloneFunc func)
{
	ReOS_CompoundList *clone = malloc(sizeof(ReOS_CompoundList));
	if (l->impl->refs != -1) {
		clone->impl = l->impl;
		clone->impl->refs++;
	}
	else {
		CloneFunc old_func = l->impl->clone_element;
		l->impl->clone_element = func;
		clone->impl = reos_compoundlistimpl_clone(l->impl);
		l->impl->clone_element = old_func;
		clone->impl->refs = -1;
	}

	return clone;
}

ReOS_CompoundListImpl *reos_compoundlistimpl_clone(ReOS_CompoundListImpl *impl)
{
	ReOS_CompoundListImpl *clone_impl = new_reos_compoundlistimpl_no_head(CLIST_MAX_LEN(impl->head),
									      impl->destructor,
									      impl->clone_element);

	ReOS_CompoundListNode *clone_prev;
	ReOS_CompoundListNode *clone_node = 0;
	ReOS_CompoundListNode *node = impl->head;

	clone_impl->head = reos_compoundlistnode_clone(impl->head, impl->clone_element);
	clone_prev = clone_impl->head;
	node = impl->head->next;

	while (node) {
		clone_node = reos_compoundlistnode_clone(node, impl->clone_element);
		clone_node->prev = clone_prev;
		clone_prev->next = clone_node;
		clone_prev = clone_node;
		node = node->next;
	}

	if (clone_node)
		clone_impl->tail = clone_node;
	else
		clone_impl->tail = clone_impl->head;

	return clone_impl;
}

static ReOS_CompoundListNode *reos_compoundlistnode_clone(ReOS_CompoundListNode *node,
												CloneFunc clone_element)
{
#ifdef OPTIMIZE_FOR_SIZE
	ReOS_CompoundListNode *clone_node = new_reos_compoundlistnode(node->max_len);
#else
	ReOS_CompoundListNode *clone_node = new_reos_compoundlistnode();
#endif

	clone_node->pos = node->pos;
	clone_node->len = node->len;

	if (clone_element) {
		int i;
		for (i = node->pos; i < node->len; i++)
			clone_node->array[i] = clone_element(node->array[i]);
	}
	else
		memcpy(clone_node->array + node->pos, node->array + node->pos,
			   sizeof(void *)*(node->len - node->pos));

	return clone_node;
}

void reos_compoundlist_unshare(ReOS_CompoundList *l)
{
	if (l->impl->refs > 1) {
		l->impl->refs--;
		l->impl = reos_compoundlistimpl_clone(l->impl);
	}
	l->impl->refs = -1;
}

/**
 * Prettyprints a list.
 *
 * \param l The list to print
 *
 * \memberof ReOS_CompoundList
 */
void reos_compoundlist_dump(ReOS_CompoundList *l)
{
	ReOS_CompoundListImpl *impl = l->impl;

	printf("compoundlist %p: head %p... tail %p\n\n", l,
		   impl->head, impl->tail);

	ReOS_CompoundListNode *node = impl->head;
	while (node) {
		printf("prev %p <- this %p -> next %p\n", node->prev, node, node->next);
		printf("pos %d, len %d, max_len %d\n", node->pos, node->len,
			   CLIST_MAX_LEN(node));

		int i;
		for (i = node->pos; i < node->len; i++)
			printf("%d: %p\n", i, node->array[i]);
		printf("\n");

		node = node->next;
	}

	printf("free_head: %p\n", impl->free_head);

	node = impl->free_head;
	while (node) {
		printf("this %p, max_len %d -> next %p\n", node, CLIST_MAX_LEN(node),
			   node->next);
		node = node->next;
	}

	printf("\n");
}

/**
 * Calculates the number of elements in a list by iterating over all nodes and
 * adding their lengths together.
 *
 * \param l The list whose length to calculate
 * \returns The number of elements in the list
 *
 * \memberof ReOS_CompoundList
 */
long reos_compoundlist_length(ReOS_CompoundList *l)
{
	ReOS_CompoundListImpl *impl = l->impl;

	long length = 0;
	ReOS_CompoundListNode *node = impl->head;
	while (node) {
		length += node->len - node->pos;
		node = node->next;
	}

	return length;
}

/**
 * Prepends a node to the \c head of a list, or assigns it as such if there was
 * no \c head already. If \c free_head is not 0, a node is popped off of it to
 * use, regardless of \c array length. Otherwise, a new node is allocated with
 * an \c array twice the length of the node \c array most recently allocated.
 *
 * \param l The list to prepend a \c head node to
 *
 * \memberof ReOS_CompoundList
 */
static void compoundlist_push_head_node(ReOS_CompoundList *l)
{
	ReOS_CompoundListImpl *impl = l->impl;

	ReOS_CompoundListNode *prev;
#ifdef OPTIMIZE_FOR_SIZE
	if (impl->head->len == impl->head->max_len)
		prev = new_reos_compoundlistnode(impl->head->len*2);
	else
		prev = new_reos_compoundlistnode(impl->head->len > 2 ? impl->head->len : 2);
#else
	if (impl->free_head) {
		prev = impl->free_head;
		impl->free_head = impl->free_head->next;
		prev->prev = 0;
	}
	else
		prev = new_reos_compoundlistnode();
#endif

	prev->next = impl->head;
	prev->pos = CLIST_MAX_LEN(prev);
	prev->len = CLIST_MAX_LEN(prev);

	impl->head->prev = prev;
	impl->head = prev;
}

/**
 * Appends a node to the \c tail of a list, or assigns it as such if there was
 * no \c tail already. If \c free_head is not 0, a node is popped off of it to
 * use, regardless of \c array length. Otherwise, a new node is allocated with
 * an \c array twice the length of the node \c array most recently allocated.
 *
 * \param l The list to append a \c tail node to
 *
 * \memberof ReOS_CompoundList
 */
static void compoundlist_push_tail_node(ReOS_CompoundList *l)
{
	ReOS_CompoundListImpl *impl = l->impl;

	ReOS_CompoundListNode *next;
#ifdef OPTIMIZE_FOR_SIZE
	if (impl->tail->pos == 0)
		next = new_reos_compoundlistnode(impl->tail->len*2);
	else
		next = new_reos_compoundlistnode(impl->tail->len - impl->tail->pos > 2 ? impl->tail->len - impl->tail->pos : 2);
#else
	if (impl->free_head) {
		next = impl->free_head;
		impl->free_head = impl->free_head->next;
		next->next = 0;
	}
	else
		next = new_reos_compoundlistnode();
#endif

	next->prev = impl->tail;
	next->pos = 0;
	next->len = 0;

	impl->tail->next = next;
	impl->tail = next;
}

/**
 * Prepends \a element to the list \c head. If the list's reference count is
 * greater than 1, a clone of the list is assigned to \a l_ptr.
 *
 * \param l_ptr A pointer to the list to prepend \a element to
 * \param element Any pointer
 *
 * \sa reos_compoundlist_pop_head()
 *
 * \memberof ReOS_CompoundList
 */
void reos_compoundlist_push_head(ReOS_CompoundList *l, void *element)
{
	reos_compoundlist_detach(l);
	ReOS_CompoundListImpl *impl = l->impl;

	if (impl->head->pos == 0)
		compoundlist_push_head_node(l);

	impl->head->array[--impl->head->pos] = element;
}

/**
 * Appends \a element to the list \c head. If the list's reference count is
 * greater than 1, a clone of the list is assigned to \a l_ptr.
 *
 * \param l_ptr A pointer to the list to append \a element to
 * \param element Any pointer
 *
 * \sa reos_compoundlist_pop_tail()
 *
 * \memberof ReOS_CompoundList
 */
void reos_compoundlist_push_tail(ReOS_CompoundList *l, void *element)
{
	reos_compoundlist_detach(l);
	ReOS_CompoundListImpl *impl = l->impl;

	if (impl->tail->len == CLIST_MAX_LEN(impl->tail))
		compoundlist_push_tail_node(l);

	impl->tail->array[impl->tail->len++] = element;
}

/**
 * Replaces the element at \a offset from the list \c head with \a element. \a
 * offset must be less than the length of the list. This function does not modify the
 * current element at \a offset, and the list's \c destructor is not called on it. If the list's reference
 * count is greater than 1, a clone of the list is assigned to \a l_ptr.
 *
 * \param l_ptr A pointer to the list in which to insert \a element
 * \param offset An offset from the list \c head within the range [0, reos_compoundlist_length())
 * \param element Any pointer
 *
 * \memberof ReOS_CompoundList
 */
void reos_compoundlist_insert_from_head(ReOS_CompoundList *l, int offset, void *element)
{
	reos_compoundlist_detach(l);
	ReOS_CompoundListImpl *impl = l->impl;

	ReOS_CompoundListNode *node = impl->head;
	while (node->len - node->pos < offset) {
		offset -= node->len - node->pos;
		node = node->next;
	}

	node->array[offset] = element;
}

/**
 * Removes and returns an element from the list \c head. This function does not modify the
 * element, and the list's \c destructor is not called on it. If the list's reference count is
 * greater than 1, a clone of the list is assigned to \a l_ptr.
 *
 * \param l_ptr The list to remove an element from
 * \return The element at \c head
 *
 * \sa reos_compoundlist_push_head()
 *
 * \memberof ReOS_CompoundList
 */
void *reos_compoundlist_pop_head(ReOS_CompoundList *l)
{
	reos_compoundlist_detach(l);
	ReOS_CompoundListImpl *impl = l->impl;

	void *d = impl->head->array[impl->head->pos++];

	if (impl->head->pos == impl->head->len) {
		if (impl->head != impl->tail) {
#ifdef OPTIMIZE_FOR_SIZE
			impl->head = impl->head->next;
			free_reos_compoundlistnode(impl->head->prev, impl->destructor);
			impl->head->prev = 0;
#else
			list_free_head(ReOS_CompoundListNode, impl);
#endif
		}
		else {
			impl->head->pos = 0;
			impl->head->len = 0;
		}
	}

	return d;
}

/**
 * Removes and returns an element from the list \c tail. This function does not modify the
 * element, and the list's \c destructor is not called on it. If the list's reference count is
 * greater than 1, a clone of the list is assigned to \a l_ptr.
 *
 * \param l_ptr The list to remove an element from
 * \return The element at \c tail
 *
 * \sa reos_compoundlist_push_tail()
 *
 * \memberof ReOS_CompoundList
 */
void *reos_compoundlist_pop_tail(ReOS_CompoundList *l)
{
	reos_compoundlist_detach(l);
	ReOS_CompoundListImpl *impl = l->impl;

	void *d = impl->tail->array[--impl->tail->len];

	if (impl->tail->pos == impl->tail->len) {
		if (impl->head != impl->tail) {
#ifdef OPTIMIZE_FOR_SIZE
			impl->tail = impl->tail->prev;
			free_reos_compoundlistnode(impl->tail->next, impl->destructor);
			impl->tail->next = 0;
#else
			list_free_tail(ReOS_CompoundListNode, impl);
#endif
		}
		else {
			impl->head->pos = 0;
			impl->head->len = 0;
		}
	}

	return d;
}

/**
 * Returns the element at the list \c head without removing it from the list.
 *
 * \param l The list to peek
 * \return The element at \c head
 *
 * \memberof ReOS_CompoundList
 */
void *reos_compoundlist_peek_head(ReOS_CompoundList *l)
{
	ReOS_CompoundListImpl *impl = l->impl;
	return impl->head->array[impl->head->pos];
}

/**
 * Returns the element at the list \c tail without removing it from the list.
 *
 * \param l The list to peek
 * \return The element at \c tail
 *
 * \memberof ReOS_CompoundList
 */
void *reos_compoundlist_peek_tail(ReOS_CompoundList *l)
{
	ReOS_CompoundListImpl *impl = l->impl;
	return impl->tail->array[impl->tail->len-1];
}

/**
 * Initializes a pre-allocated iterator. It is recommended that \a iter be
 * allocated on the stack for speed. The results of modifying a list while
 * iterating over it are undefined. ReOS_CompoundListIter advances in
 * <tt>head</tt>-to-<tt>tail</tt> order.
 *
 * \param iter A pre-allocated iterator
 * \param l The list to iterate over
 *
 * \memberof ReOS_CompoundListIter
 *
 * \todo Report this doc bug that prevents naming a function both a member of
 * and relating to different classes.
 */
void reos_compoundlistiter_init(ReOS_CompoundListIter *iter, ReOS_CompoundList *l)
{
	ReOS_CompoundListImpl *impl = l->impl;

	iter->node = impl->head;
	iter->node_pos = impl->head->pos;
}

/**
 * Tests whether there is another element after the current iterator position.
 *
 * \param iter The iterator to test
 *
 * \memberof ReOS_CompoundListIter
 * \relatesalso ReOS_CompoundList
 */
int reos_compoundlistiter_has_next_func(ReOS_CompoundListIter *iter)
{
	return reos_compoundlistiter_has_next_body(iter);
}

/**
 * Returns the current element under the iterator and advances the iterator
 * position.
 *
 * \param iter The iterator to advance
 *
 * \memberof ReOS_CompoundListIter
 * \relatesalso ReOS_CompoundList
 */
void *reos_compoundlistiter_get_next(ReOS_CompoundListIter *iter)
{
	if (iter->node_pos == iter->node->len) {
		iter->node = iter->node->next;
		iter->node_pos = iter->node->pos;
	}

	return iter->node->array[iter->node_pos++];
}

/**
 * Returns the current element under the iterator without advancing the iterator
 * position.
 *
 * \param iter The iterator to peek
 *
 * \memberof ReOS_CompoundListIter
 * \relatesalso ReOS_CompoundList
 */
void *reos_compoundlistiter_peek_next(ReOS_CompoundListIter *iter)
{
	if (iter->node_pos == iter->node->len && iter->node->next)
		return iter->node->next->array[0];
	else
		return iter->node->array[iter->node_pos];
}

ReOS_JudyList *new_reos_judylist(VoidPtrFunc destructor, VoidPtrFunc deref_element,
					   CloneFunc clone_element, VoidPtrFunc ref_element)
{
	ReOS_JudyList *l = malloc(sizeof(ReOS_JudyList));
	l->impl = new_reos_judylistimpl(destructor, deref_element, clone_element,
							   ref_element);
	return l;
}

static ReOS_JudyListImpl *new_reos_judylistimpl(VoidPtrFunc destructor,
									  VoidPtrFunc deref_element,
									  CloneFunc clone_element,
									  VoidPtrFunc ref_element)
{
	ReOS_JudyListImpl *impl = malloc(sizeof(ReOS_JudyListImpl));
	impl->judy = 0;
	impl->refs = 1;
	impl->destructor = destructor;
	impl->deref_element = deref_element;
	impl->clone_element = clone_element;
	impl->ref_element = ref_element;
	return impl;
}

int free_reos_judylist(ReOS_JudyList *l)
{
	if (l) {
		ReOS_JudyListImpl *impl = l->impl;

		impl->refs--;
		if (impl->refs > 0) {
			reos_judylist_deref_elements(l);
			free(l);

			return impl->refs;
		}

		free_judy(l->impl->judy, l->impl->destructor);
		free(l->impl);
		free(l);
	}

	return 0;
}

/**
 * Convenience function to free a Judy array with a destructor. Iterates over
 * \a list and calls \a destructor on every element, then frees \a list.
 *
 * \param list The Judy array to free
 * \param destructor The function to call on every element before freeing
 *
 * \warning This function does not set \a list to 0, as the JLFA macro does.
 */
void free_judy(void *list, VoidPtrFunc destructor)
{
	if (destructor) {
		judy_iter_begin(void, data, list) {
			destructor(data);
			judy_iter_next(data, list);
		}
	}

	Word_t rc;
	JLFA(rc, list);
}

void reos_judylist_ref_elements(ReOS_JudyList *l)
{
	reos_judylist_iter_begin(void, e, l) {
		l->impl->ref_element(e);
		reos_judylist_iter_next(e, l);
	}
}

void reos_judylist_deref_elements(ReOS_JudyList *l)
{
	reos_judylist_iter_begin(void, e, l) {
		l->impl->deref_element(e);
		reos_judylist_iter_next(e, l);
	}
}

ReOS_JudyList *reos_judylist_clone(ReOS_JudyList *l)
{
	ReOS_JudyList *clone = malloc(sizeof(ReOS_JudyList));
	clone->impl = l->impl;
	clone->impl->refs++;

	if (clone->impl->ref_element)
		reos_judylist_ref_elements(l);

	return clone;
}

ReOS_JudyListImpl *reos_judylistimpl_clone(ReOS_JudyListImpl *impl)
{
	ReOS_JudyListImpl *clone_impl = new_reos_judylistimpl(impl->destructor,
												impl->deref_element,
												impl->clone_element,
												impl->ref_element);

	if (impl->clone_element) {
		judy_iter_begin(void, e, impl->judy) {
			judy_insert(clone_impl->judy, iter_e, impl->clone_element(e));
			judy_iter_next(e, impl->judy);
		}
	}
	else {
		judy_iter_begin(void, e, impl->judy) {
			judy_insert(clone_impl->judy, iter_e, e);
			judy_iter_next(e, impl->judy);
		}
	}

	return clone_impl;
}

void reos_judylist_push_tail(ReOS_JudyList *l, void *element)
{
	Word_t index;
	reos_judylist_first_absent_index(l, index);
	reos_judylist_insert(l, index, element);
}

void *reos_judylist_pop_tail(ReOS_JudyList *l)
{
	Word_t index;
	void *element;

	reos_judylist_last_index(l, index);
	reos_judylist_get(l, index, element);
	reos_judylist_delete(l, index);

	return element;
}
