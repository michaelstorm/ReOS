#ifndef REOS_TYPES_H
#define REOS_TYPES_H

#include <time.h>

/**
 * \file
 *
 * Declares all core ReOS types.
 *
 */

#if !defined(OPTIMIZE_FOR_SIZE) && !defined(OPTIMIZE_FOR_SPEED)
#define OPTIMIZE_FOR_SIZE
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ReOS_Debugger ReOS_Debugger;
typedef struct ReOS_Pattern ReOS_Pattern;
typedef struct ReOS_TokenBuffer ReOS_TokenBuffer;
typedef struct ReOS_Input ReOS_Input;
typedef struct ReOS_Inst ReOS_Inst;
typedef struct ReOS_State ReOS_State;
typedef struct ReOS_Kernel ReOS_Kernel;
typedef struct ReOS_BackrefBuffer ReOS_BackrefBuffer;
typedef struct ReOS_SimpleList ReOS_SimpleList;
typedef struct ReOS_SimpleListIter ReOS_SimpleListIter;
typedef struct ReOS_SimpleListNode ReOS_SimpleListNode;
typedef struct ReOS_CompoundList ReOS_CompoundList;
typedef struct ReOS_CompoundListImpl ReOS_CompoundListImpl;
typedef struct ReOS_CompoundListNode ReOS_CompoundListNode;
typedef struct ReOS_CompoundListIter ReOS_CompoundListIter;
typedef struct ReOS_JudyList ReOS_JudyList;
typedef struct ReOS_JudyListImpl ReOS_JudyListImpl;
typedef struct ReOS_Capture ReOS_Capture;
typedef struct ReOS_JoinRoot ReOS_JoinRoot;
typedef struct ReOS_JoinRootImpl ReOS_JoinRootImpl;
typedef struct ReOS_CaptureSet ReOS_CaptureSet;
typedef struct ReOS_Thread ReOS_Thread;
typedef struct ReOS_ThreadList ReOS_ThreadList;
typedef struct ReOS_Branch ReOS_Branch;

typedef void (*VoidPtrFunc)(void *);
typedef void *(*CloneFunc)(void *);
typedef int (*StreamReadFunc)(void *, int, void *);
typedef int (*IndexedReadFunc)(void *, int, long, void *);
typedef int (*ExecuteInstFunc)(ReOS_Kernel *, ReOS_Thread *, ReOS_Inst *, int);
typedef int (*TestBackrefFunc)(ReOS_Kernel *, void *, void *);
typedef void (*DebugCallbackFunc)(ReOS_Debugger *, ReOS_Kernel *);
typedef void (*PrintInputFunc)(ReOS_Kernel *);
typedef void (*PrintInstFunc)(ReOS_Inst *);

struct ReOS_Pattern
{
	void *data;

	ReOS_Inst *(*get_inst)(ReOS_Pattern *, int);
	void (*set_inst)(ReOS_Pattern *, ReOS_Inst *, int);
};

struct ReOS_TokenBuffer
{
	int len;
	int pos;
	void *buf;
	ReOS_Input *input;
};

struct ReOS_Input
{
	StreamReadFunc stream_read;
	IndexedReadFunc indexed_read;
	VoidPtrFunc free_token;
	int token_size;
	int buffer_size;
	void *data;
};

enum
{
	ReOS_InstRetHalt = 1,
	ReOS_InstRetDrop = 2,
	ReOS_InstRetStep = 4,
	ReOS_InstRetConsume = 8,
	ReOS_InstRetMatch = 16,
	ReOS_InstRetBacktrack = 32
};

struct ReOS_Inst
{
	int opcode;
	void *args;
};

struct ReOS_State
{
	ReOS_ThreadList *current_thread_list;
	ReOS_ThreadList *next_thread_list;
};

struct ReOS_Kernel
{
	ReOS_Pattern *pattern;
	ReOS_TokenBuffer *token_buf;
	ReOS_State state;
	ReOS_CompoundList *free_captureset_list;
	ReOS_CompoundList *free_thread_list;
	ReOS_SimpleList *debuggers;
	ExecuteInstFunc execute_inst;
	long sp;
	void *current_token;
	void *data;

	int num_capturesets;
	int max_capturesets;
	ReOS_SimpleList *matches;

	TestBackrefFunc test_backref;
	int next_backref_id;
};

struct ReOS_BackrefBuffer
{
	long index;
	long length;
	void **tokens;
};

/**
 * A doubly-linked list implementation.
 */
struct ReOS_SimpleList
{
	/*!
	 *  The head of the list. When a ReOS_SimpleList object is first created, its \c
	 *  head and \c tail are 0. They are only allocated when elements are added
	 *  to the list.
	 */
	ReOS_SimpleListNode *head;

	/*!
	 *  The tail of the list. When a ReOS_SimpleList object is first created, its \c
	 *  head and \c tail are 0. They are only allocated when elements are added
	 *  to the list.
	 */
	ReOS_SimpleListNode *tail;

	/*!
	 *  The head of a chain of free nodes, most recently freed first.
	 */
	ReOS_SimpleListNode *free_head;

	/*!
	 *  The function that will be called on every element when the list is
	 *  freed.
	 */
	VoidPtrFunc destructor;
};

struct ReOS_SimpleListNode
{
	ReOS_SimpleListNode *next;
	ReOS_SimpleListNode *prev;
	void *data;
};

struct ReOS_SimpleListIter
{
	ReOS_SimpleListNode *current;
};

/**
 * Represents a capture interval.
 */
struct ReOS_Capture
{
	long start; //!< Interval start index, inclusive
	long end; //!< Interval end index, inclusive
	int partial; //!< True if partial match, false if end has matched
};

struct ReOS_CaptureSet
{
	int refs;
	long version;

	ReOS_CompoundList *free_list;
	ReOS_JudyList *captures;
};

struct ReOS_CompoundList
{
	ReOS_CompoundListImpl *impl;
};

struct ReOS_CompoundListImpl
{
	int refs;
	ReOS_CompoundListNode *head;
	ReOS_CompoundListNode *tail;
	ReOS_CompoundListNode *free_head;
	VoidPtrFunc destructor;
	CloneFunc clone_element;
};

struct ReOS_CompoundListNode
{
	int pos;
	int len;
#ifdef OPTIMIZE_FOR_SIZE
	int max_len;
#endif
	void **array;
	ReOS_CompoundListNode *next;
	ReOS_CompoundListNode *prev;
};

struct ReOS_CompoundListIter
{
	ReOS_CompoundListNode *node;
	int node_pos;
};

struct ReOS_JudyList
{
	ReOS_JudyListImpl *impl;
};

struct ReOS_JudyListImpl
{
	int refs;
	void *judy;
	VoidPtrFunc destructor;
	VoidPtrFunc deref_element;
	CloneFunc clone_element;
	VoidPtrFunc ref_element;
};

struct ReOS_Branch
{
	int strong_refs;
	int weak_refs;
	int num_threads;
	int matched;
	int negated;
	int marked;
	ReOS_CompoundList *matches;
};

struct Call
{
	int num;
	long return_pc;
};

struct ReOS_Thread
{
	int pc;
	ReOS_BackrefBuffer *backref_buffer;
	ReOS_CaptureSet *capture_set;
	ReOS_CompoundList *call_stack;
	ReOS_CompoundList *free_thread_list;

	ReOS_Branch *ref;
	ReOS_CompoundList *deps;
};

struct ReOS_ThreadList
{
	ReOS_CompoundList *list;
	void *pc_table;
	int backtrack_captures;
	int gen;
};

struct ReOS_Debugger
{
	DebugCallbackFunc start;
	DebugCallbackFunc before_token;
	DebugCallbackFunc before_inst;
	DebugCallbackFunc after_inst;
	DebugCallbackFunc after_token;
	DebugCallbackFunc match;
	DebugCallbackFunc failure;
	DebugCallbackFunc end;

	void *data;
};

#define reos_simplelistiter_has_next_body(iter) \
	((iter)->current ? 1 : 0)

#define reos_compoundlistiter_has_next_body(iter) \
	((iter)->node && (((iter)->node_pos < (iter)->node->len) \
					  || ((iter)->node->next && (iter)->node->next->len)))

#ifdef INLINE_LISTS
#define reos_simplelistiter_has_next reos_simplelistiter_has_next_body
#define reos_compoundlistiter_has_next reos_compoundlistiter_has_next_body
#else
#define reos_simplelistiter_has_next reos_simplelistiter_has_next_func
#define reos_compoundlistiter_has_next reos_compoundlistiter_has_next_func
#endif

#ifdef BRANCH_DEBUG
#define BRANCH_DEBUG_DO(A) A
#else
#define BRANCH_DEBUG_DO(A)
#endif

#ifdef __cplusplus
}
#endif

#endif
