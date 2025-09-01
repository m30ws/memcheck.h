/*
	memcheck.h
	
	Copyright (C) 2025 m30ws MIT license
	
	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:
	
	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
	
	----------------------------------------
	
	A simple C89/C++98 single-file header library for checking memory leaks in your app.

	(Very) Basic usage:
	```
	  #define MEMCHECK_IMPLEMENTATION
	  #include "memcheck.h"

	  // A lot of code... //
	
	  memcheck_stats(NULL);
	  memcheck_cleanup();
	```
	  
	Simply include memcheck.h into any of your .c/.cpp files where you want to track
	  malloc/calloc/realloc/free calls. Make sure to also define MEMCHECK_IMPLEMENTATION
	  in only ONE of your files to trigger adding source into it.
	  At any point call memcheck_stats() to see a summary of your allocations.
	
	You may define -DMEMCHECK_IGNORE to prevent all functionality; memcheck functions in your
	  code may remain since they will still be defined but as no-op versions of themselves.

	Also available:
	  - MEMCHECK_NO_OUTPUT - disable all "debug" output. this overrides memcheck_set_status_fp() (memcheck_stats() will still work as normal when called)
	  - MEMCHECK_PURGE_ON_CLEANUP - when memcheck_cleanup() is called also try to free the remaining memory blocks (if any)
	  - MEMCHECK_ENABLE_THREADSAFETY - enables global mutex and locking when accessing global memcheck resources (TODO: consider making opt-out instead of opt-in?)
	  - MEMCHECK_NO_CRITICAL_OUTPUT - normally, realloc() and free() call attempts on non-tracked memory address will output warning message even if debug output is disabled; this option prevents it

	Look at example/ to see one way to use it, or look at the function declarations
	  further down to see all available features.

	TODO:
	  - instead of removing freed/realloc'd addresses from storage, move them to "already-freed" to detect double-free or use-after-free
	  - add something like memcheck_*alloc_alright() to tell memcheck to still keep track of that allocation, but not yell if it's not freed at the end(and/or even dealloc them automatically?) (ex. for some long-standing allocations which don't make sense if they are not valid for the entire duration of the program) ?
	  - Improve output formats
*/

#ifndef _MEMCHECK_H_
#define _MEMCHECK_H_

#pragma message ("-- Memcheck active.")

#if defined(MEMCHECK_ENABLE_THREADSAFETY) && _POSIX_C_SOURCE < 200809L
#pragma message "You should probably define _POSIX_C_SOURCE to at least 200809L for pthreads recursive mutex feature (will get removed in the future)"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef MEMCHECK_ENABLE_THREADSAFETY
#ifdef _WIN32
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
#else
	#include <pthread.h>
#endif
#endif


/********** EMBED TOU_LLIST_T IMPL (extracted from tou.h) **********/
/* Define _memcheck_tou_llist_t before others for get_memblocks()*/
typedef struct _memcheck_tou_llist_s
{
	struct _memcheck_tou_llist_s* prev;     /**< previous element                     */
	struct _memcheck_tou_llist_s* next;     /**< next element                         */
	void* dat1;                 /**< useful data                          */
	void* dat2;                 /**< useful data                          */
	char destroy_dat1/* : 1*/;  /**< automatically deallocate this data ? */
	char destroy_dat2/* : 1*/;  /**< automatically deallocate this data ? */
} _memcheck_tou_llist_t;
/********** END TOU_LLIST_T IMPL **********/

/********** EMBED TOU_THREAD_MUTEX_T IMPL (extracted from tou.h) **********/
#ifdef MEMCHECK_ENABLE_THREADSAFETY
#ifdef _WIN32
	typedef HANDLE          _memcheck_tou_thread_mutex_t;
#else
	typedef pthread_mutex_t _memcheck_tou_thread_mutex_t;
#endif
#endif
/********** END TOU_THREAD_MUTEX_T IMPL **********/

/********** EMBED TOU PRINT MACROS (extracted from tou.h) **********/
/* size_t (%zu) */
#ifndef _MEMCHECK_TOU_PRIuZ
#	ifdef _WIN32
#		ifdef _MSC_VER
#		define _MEMCHECK_TOU_PRIuZ "zu"
#		else /* non-msvc */
#		define _MEMCHECK_TOU_PRIuZ "llu"
#		endif
#	else
#		define _MEMCHECK_TOU_PRIuZ "zu"
#	endif
#endif
/* ssize_t (%zd) */
#ifndef _MEMCHECK_TOU_PRIdZ
#	ifdef _WIN32
#		ifdef _MSC_VER
#		define _MEMCHECK_TOU_PRIdZ "zd"
#		else /* non-msvc */
#		define _MEMCHECK_TOU_PRIdZ "lld"
#		endif
#	else
#		define _MEMCHECK_TOU_PRIdZ "zd"
#	endif
#endif
/* [s]size_t as hex (%zx) */
#ifndef _MEMCHECK_TOU_PRIxZ
#	ifdef _WIN32
#		ifdef _MSC_VER
#		define _MEMCHECK_TOU_PRIxZ "zx"
#		else /* non-msvc */
#		define _MEMCHECK_TOU_PRIxZ "llx"
#		endif
#	else
#		define _MEMCHECK_TOU_PRIxZ "zx"
#	endif
#endif
/********** END TOU PRINT MACROS **********/


#ifdef __cplusplus
extern "C" {
#endif

/* User funcs */
int   memcheck_stats(FILE* fp);          /* Displays numbers of allocations and releases, their byte amounts,
                                             and locations where they don't match (if any) up until this call.
                                            The FILE* argument may be passed to output to specific stream; pass
                                             NULL to use the stream set by memcheck_set_status_fp(). (Default: stdout)
                                            Returns 0 if there are still some allocations up to now that weren't freed,
                                             otherwise returns 1.
                                         */
void  memcheck_set_status_fp(FILE* fp);  /* Set which FILE* to be used for immediate logging messages (allocations and
                                            releases; also used for memcheck_stats() if not overridden).
                                            Status_fp variable defaults to stdout but if NULL is passed to this function the output
                                            will be redirected to /dev/null. This will cause memcheck itself to manage that FILE*.
                                            If you want to manage the FILE* yourself, open it using fopen() and pass it in here */
FILE* memcheck_get_status_fp(void);      /* Returns the currently used status_fp inside memcheck. (Defaults to stdout) */
void  memcheck_set_tracking(int yn);     /* Whether to perform call tracking (dynamically turn memcheck on and off) (1 = yes, 0 = no) */
int   memcheck_is_tracking(void);        /* Retrieves current setting for controlling call tracking (1 = yes, 0 = no) */
void  memcheck_cleanup(void);            /* Destroys the internal memory blocks storage and closes status_fp if memcheck
                                            is managing it (Only a case when you let it do so using memcheck_set_status_fp(NULL)).
                                            Does NOT attempt to free the remaining memory blocks unless MEMCHECK_PURGE_ON_CLEANUP is defined.
                                            Note: you will NOT get memcheck warnings if you forget to call memcheck_cleanup()! */
void  memcheck_stats_reset(void);        /* Resets all statistics tracked to 0 */
void  memcheck_purge_remaining(void);    /* Attempts to perform free() on all of the remaining memblocks that are being tracked */

/* Special */
_memcheck_tou_llist_t** memcheck_get_memblocks(void); /* Returns a reference to the internal memory blocks storage */

/* Internal (but may use explicitly) */
/* If MEMCHECK_IGNORE is defined these will simply pass their parameters to their stdlib counterparts ignoring file and line data */
void* memcheck_malloc(size_t size, const char* file, size_t line);
void* memcheck_calloc(size_t num, size_t size, const char* file, size_t line);
void* memcheck_realloc(void* ptr, size_t new_size, const char* file, size_t line);
void  memcheck_free(void* ptr, const char* file, size_t line);

#ifdef __cplusplus
}
#endif

#endif /* _MEMCHECK_H_ */



/*****************************************************************/

#ifdef MEMCHECK_IMPLEMENTATION
#ifdef MEMCHECK_IMPLEMENTATION_DONE
#warning "MEMCHECK_IMPLEMENTATION already defined somewhere! Skipping..."
#else
#define MEMCHECK_IMPLEMENTATION_DONE
#undef MEMCHECK_IMPLEMENTATION

#pragma message ("-- Memcheck active (implementation).")


#ifdef MEMCHECK_IGNORE
	int memcheck_stats(FILE* fp)
	{
		(void)fp;
		return 1;
	}
	void memcheck_stats_reset(void)
	{
		(void)0;
	}
	void memcheck_set_tracking(int yn)
	{
		(void)yn;
	}
	int memcheck_is_tracking(void)
	{
		return 0;
	}
	void memcheck_set_status_fp(FILE* fp)
	{
		(void)fp;
	}
	FILE* memcheck_get_status_fp(void)
	{
		return NULL;
	}
	void memcheck_cleanup(void)
	{
		(void)0;
	}
	void memcheck_purge_remaining(void)
	{
		(void)0;
	}
	_memcheck_tou_llist_t** memcheck_get_memblocks(void)
	{
		return NULL;
	}
	void* memcheck_malloc(size_t size, const char* file, size_t line)
	{
		(void)file; (void)line;
		return malloc(size);
	}
	void* memcheck_calloc(size_t num, size_t size, const char* file, size_t line)
	{
		(void)file; (void)line;
		return calloc(num, size);
	}
	void* memcheck_realloc(void* ptr, size_t new_size, const char* file, size_t line)
	{
		(void)file; (void)line;
		return realloc(ptr, new_size);
	}
	void memcheck_free(void* ptr, const char* file, size_t line)
	{
		(void)file; (void)line;
		free(ptr);
	}
#else


/********** EMBED TOU_LLIST IMPL (extracted from tou.h) **********/

#ifdef __cplusplus
extern "C" {        /* Extern C for llist */
#endif

static _memcheck_tou_llist_t* _memcheck_tou_llist_append(_memcheck_tou_llist_t** node_ref, void* dat1, void* dat2, char dat1_is_dynalloc, char dat2_is_dynalloc)
{
	_memcheck_tou_llist_t* new_node;
	_memcheck_tou_llist_t* prev_node;
	_memcheck_tou_llist_t* previous_next;

	if (node_ref == NULL)
		return NULL;

	/* Spawn new node */
	new_node = (_memcheck_tou_llist_t*) malloc(sizeof(*new_node));

	new_node->prev = NULL;
	new_node->next = NULL;
	
	new_node->dat1 = dat1;
	new_node->destroy_dat1 = dat1_is_dynalloc;
	new_node->dat2 = dat2;
	new_node->destroy_dat2 = dat2_is_dynalloc;

	/* Given node is empty list */
	if (*node_ref == NULL) {
		*node_ref = new_node; /* Update passed reference to point to the new node */
		return new_node;
	}
	/* Given node/list already has something and this node is head */
	prev_node = (*node_ref);
	if (prev_node->next == NULL) {
		prev_node->next = new_node; /* Update passed node's .next */
		new_node->prev = prev_node; /* Update new node's .prev */
		*node_ref = new_node; /* Update passed reference to point to the new node */
		return new_node;
	}

	/* The given node is now somewhere inbetween (after head) */

	/* Setup new node links */
	previous_next = prev_node->next; /* Save previously-next node */
	new_node->prev = prev_node; /* Update new node's .prev to point to the passed node */
	new_node->next = previous_next; /* Update new node's .next to */
									/* point to the previously-next node */

	/* Setup previous (current) node links */
	prev_node->next = new_node; /* Update previously-next node to point to newly created node */
	if (previous_next != NULL)
		previous_next->prev = new_node; /* If previously-next actually exists, */
										/* make its .prev point to newly created node */

	return new_node;
}

static _memcheck_tou_llist_t* _memcheck_tou_llist_get_tail(_memcheck_tou_llist_t* list)
{
	if (!list) return NULL;

	while (list->prev)
		list = list->prev;

	return list;
}

static _memcheck_tou_llist_t* _memcheck_tou_llist_get_head(_memcheck_tou_llist_t* list)
{
	if (!list) return NULL;

	while (list->next)
		list = list->next;
	
	return list;
}

static _memcheck_tou_llist_t* _memcheck_tou_llist_get_oldest(_memcheck_tou_llist_t* list)
{
	return _memcheck_tou_llist_get_tail(list);
}

static _memcheck_tou_llist_t* _memcheck_tou_llist_get_older(_memcheck_tou_llist_t* elem)
{
	if (elem == NULL) return NULL;
	return elem->prev;
}

static _memcheck_tou_llist_t* _memcheck_tou_llist_get_newest(_memcheck_tou_llist_t* list)
{
	return _memcheck_tou_llist_get_head(list);
}

static _memcheck_tou_llist_t* _memcheck_tou_llist_get_newer(_memcheck_tou_llist_t* elem)
{
	if (elem == NULL) return NULL;
	return elem->next;
}

static _memcheck_tou_llist_t* _memcheck_tou_llist_find_exact_one(_memcheck_tou_llist_t* list, void* dat1)
{
	if (list == NULL)
		return NULL;

	while (list) {
		if (list->dat1 == dat1)
			return list;
		list = list->prev;
	}

	return NULL;
}

static unsigned char _memcheck_tou_llist_is_head(_memcheck_tou_llist_t* elem)
{
	return elem == NULL || elem->next == NULL;
}

static _memcheck_tou_llist_t* _memcheck_tou_llist_pop(_memcheck_tou_llist_t* elem)
{
	if (!elem) return NULL;

	if (elem->prev) {
		elem->prev->next = elem->next;
	}
	if (elem->next) {
		elem->next->prev = elem->prev;
	}

	return elem;
}

static void _memcheck_tou_llist_free_element(_memcheck_tou_llist_t* elem)
{
	if (!elem) return;

	if (elem->destroy_dat1) free(elem->dat1);
	if (elem->destroy_dat2) free(elem->dat2);

	free(elem);
}

static _memcheck_tou_llist_t* _memcheck_tou_llist_remove(_memcheck_tou_llist_t* elem)
{	
	_memcheck_tou_llist_t* next = elem->next;
	_memcheck_tou_llist_t* prev = elem->prev;
	_memcheck_tou_llist_pop(elem);
	_memcheck_tou_llist_free_element(elem);

	if (next == NULL) /* This element was head */
		return prev; /* prev will be the new head */
	else
		return next; /* if assigned, next will be the new head */
}

static size_t _memcheck_tou_llist_len(_memcheck_tou_llist_t* list)
{
	size_t len = 0;

	if (list == NULL)
		return 0;

	if (list->next == NULL) { /* this is head. */
		while (list) {
			len++;
			list = list->prev;
		}

	} else { /* this is tail (or inbetween) */
		while (list) {
			len++;
			list = list->next;
		}
	}

	return len;
}

static void _memcheck_tou_llist_destroy(_memcheck_tou_llist_t* list)
{
	if (!list) return;

	if (list->next == NULL) { /* this is head. */
		_memcheck_tou_llist_t* prev;
		_memcheck_tou_llist_t* curr = list;

		while (curr != NULL) {
			prev = curr->prev;
			if (curr->destroy_dat1) free(curr->dat1);
			if (curr->destroy_dat2) free(curr->dat2);
			curr->prev = NULL;
			free(curr);
			curr = prev;
		}

	} else { /* this is tail (or inbetween) */
		_memcheck_tou_llist_t* next;
		_memcheck_tou_llist_t* curr = list;
		
		while (curr != NULL) {
			next = curr->next;
			if (curr->destroy_dat1) free(curr->dat1);
			if (curr->destroy_dat2) free(curr->dat2);
			curr->next = NULL;
			free(curr);
			curr = next;
		}
	}
}

#ifdef __cplusplus
}
#endif

/********** END TOU_LLIST IMPL **********/


/********** EMBED TOU_THREAD_MUTEX IMPL (extracted from tou.h) **********/

#ifdef MEMCHECK_ENABLE_THREADSAFETY

#ifdef __cplusplus
extern "C" {        /* Extern C for mutex */
#endif

/* Mutex will get initialized on the first use of lock() */
static _memcheck_tou_thread_mutex_t _memcheck_g_mutex;
static int                          _memcheck_g_mutex_init_successful = 0;
#ifdef _WIN32
static INIT_ONCE                    _memcheck_g_mutex_init_once = INIT_ONCE_STATIC_INIT;
#else
static pthread_once_t               _memcheck_g_mutex_init_once = PTHREAD_ONCE_INIT;
#endif

static int _memcheck_tou_thread_mutex_init(_memcheck_tou_thread_mutex_t* mutex)
{
#ifdef _WIN32
	/* Winapi mutexes should be recursive by default ... */
	*mutex = CreateMutexA(
		NULL,              /* default security attributes */
		0,                 /* initially not owned */
		NULL               /* unnamed mutex */
	);
	return (*mutex == NULL) ? GetLastError() : 0;
#else
	/* ... as opposed to pthreads where we have to specify explicity */
	pthread_mutexattr_t attr;
	memset(&attr, 0, sizeof(pthread_mutexattr_t));
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	return pthread_mutex_init(mutex, &attr);
#endif
}

static int _memcheck_tou_thread_mutex_destroy(_memcheck_tou_thread_mutex_t* mutex)
{
#ifdef _WIN32
	return CloseHandle(*mutex) != 0;
#else
	return pthread_mutex_destroy(mutex);
#endif
}

#ifdef _WIN32
static /*bool*/int CALLBACK _memcheck_init_once_callback(INIT_ONCE* once, void* userdata, void** ctx)
{
	(void)once; (void)userdata; (void)ctx;
	
	if (_memcheck_tou_thread_mutex_init(&_memcheck_g_mutex) != 0)
		return 0;
	_memcheck_g_mutex_init_successful = 1;
	return 1;
}
#else
static void _memcheck_init_once_callback(void)
{
	if (_memcheck_tou_thread_mutex_init(&_memcheck_g_mutex) == 0)
	    _memcheck_g_mutex_init_successful = 1;
}
#endif

static int _memcheck_tou_thread_mutex_lock(_memcheck_tou_thread_mutex_t* mutex)
{
/* Since we wish to avoid initializing memcheck library itself,
   validity/initialization of mutex(es) will be checked here */
#ifdef _WIN32
	if (!InitOnceExecuteOnce(&_memcheck_g_mutex_init_once, _memcheck_init_once_callback, NULL, NULL))
		return GetLastError();
	return ! (WaitForSingleObject(*mutex, INFINITE) == WAIT_OBJECT_0);
#else
	int err;
	if ((err = pthread_once(&_memcheck_g_mutex_init_once, _memcheck_init_once_callback)) != 0)
		return err;
	return pthread_mutex_lock(mutex);
#endif
}

static int _memcheck_tou_thread_mutex_unlock(_memcheck_tou_thread_mutex_t* mutex)
{
#ifdef _WIN32
	return ReleaseMutex(*mutex) != 0;
#else
	return pthread_mutex_unlock(mutex);
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* MEMCHECK_ENABLE_THREADSAFETY */

/********** END TOU_THREAD_MUTEX_T IMPL **********/


typedef struct {
	const char* file;
	size_t line;
	size_t size;
} _memcheck_meta_t;

typedef struct {
	size_t n_mallocs;
	size_t n_callocs;
	size_t n_reallocs;
	size_t n_total_allocs;
	size_t n_frees;
	size_t total_alloc_size;
	size_t total_free_size;
} _memcheck_stats_t;

static int                          _memcheck_g_do_track_mem    = 1; /* Controls current tracking of allocations and releases */
static FILE*                        _memcheck_g_status_fp       = NULL; /* FILE* that serves as log for allocations and releases */
static int                          _memcheck_g_manages_devnull = 0; /* Indicator whether this lib needs to keep track of g_status_fp and close it */
static _memcheck_tou_llist_t*       _memcheck_g_memblocks       = NULL; /* Main storage for tracking allocations, releases and their locations */
#ifdef __cplusplus
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
static _memcheck_stats_t 	        _memcheck_g_stats           = {0}; /* Statistics tracker for allocations and releases to be displayed at the end */
#ifdef __cplusplus
#pragma GCC diagnostic warning "-Wmissing-field-initializers"
#endif


void memcheck_set_tracking(int yn)
{
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	if (_memcheck_tou_thread_mutex_lock(&_memcheck_g_mutex) != 0) {
		fprintf(stderr, "[%s] Unexpected mutex lock failure\n", __func__);
		return;
	}
#endif
	_memcheck_g_do_track_mem = yn;
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	_memcheck_tou_thread_mutex_unlock(&_memcheck_g_mutex);
#endif
}


int memcheck_is_tracking(void)
{
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	int track;
	if (_memcheck_tou_thread_mutex_lock(&_memcheck_g_mutex) != 0) {
		fprintf(stderr, "[%s] Unexpected mutex lock failure\n", __func__);
		return 0;
	}
	track = _memcheck_g_do_track_mem;
	_memcheck_tou_thread_mutex_unlock(&_memcheck_g_mutex);
	return track;
#else
	return _memcheck_g_do_track_mem;
#endif
}


/* Only if `fp` is explicitly NULL, system's /dev/null will be used */
void memcheck_set_status_fp(FILE* fp)
{
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	if (_memcheck_tou_thread_mutex_lock(&_memcheck_g_mutex) != 0) {
		fprintf(stderr, "[%s] Unexpected mutex lock failure\n", __func__);
		return;	
	}
#endif

	if (_memcheck_g_status_fp != NULL) {
		fflush(_memcheck_g_status_fp);
		if (_memcheck_g_manages_devnull)
			fclose(_memcheck_g_status_fp);
	}

	if (fp != NULL) {
		_memcheck_g_status_fp = fp;
		_memcheck_g_manages_devnull = 0;
	} else {
#ifdef _WIN32
		_memcheck_g_status_fp = fopen("NUL:", "w");
#else
		_memcheck_g_status_fp = fopen("/dev/null", "w");
#endif
		_memcheck_g_manages_devnull = 1;
	}

#ifdef MEMCHECK_ENABLE_THREADSAFETY
	_memcheck_tou_thread_mutex_unlock(&_memcheck_g_mutex);
#endif
}


/* Default will be to output info to stdout */
FILE* memcheck_get_status_fp(void)
{
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	FILE* fp;
	if (_memcheck_tou_thread_mutex_lock(&_memcheck_g_mutex) != 0) {
		fprintf(stderr, "[%s] Unexpected mutex lock failure\n", __func__);
		return NULL;
	}
#endif

	if (_memcheck_g_status_fp == NULL)
		memcheck_set_status_fp(stdout);

#ifdef MEMCHECK_ENABLE_THREADSAFETY
	fp = _memcheck_g_status_fp;
	_memcheck_tou_thread_mutex_unlock(&_memcheck_g_mutex);
	return fp;
#else
	return _memcheck_g_status_fp;
#endif
}


_memcheck_meta_t* memcheck_new_meta(const char* file, size_t line, size_t size)
{
	_memcheck_meta_t* meta = (_memcheck_meta_t*) malloc(sizeof(*meta));
	meta->file = file;
	meta->line = line;
	meta->size = size;
	return meta;
}


void* memcheck_malloc(size_t size, const char* file, size_t line)
{
	void* new_ptr = NULL; /* Pointer to a new block of memory to be returned
	                         at the end after releasing mutex (if enabled)  */
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	if (_memcheck_tou_thread_mutex_lock(&_memcheck_g_mutex) != 0) {
		fprintf(stderr, "[%s] Unexpected mutex lock failure\n", __func__);
		return NULL;
	}
#endif

	if (!memcheck_is_tracking()) {
		new_ptr = malloc(size);		
	} else {
		memcheck_set_tracking(0);
		
		new_ptr = malloc(size);
		
#ifndef MEMCHECK_NO_OUTPUT
		fprintf(memcheck_get_status_fp(), "[MALLOC ] %p%s {n=%" _MEMCHECK_TOU_PRIuZ "} @ %s L%" _MEMCHECK_TOU_PRIuZ "\n",
			new_ptr, (new_ptr == NULL ? " <SKIPPING>" : ""), size, file, line);
		fflush(memcheck_get_status_fp());
#endif
		/* Since we don't want free() to bark at NULL frees, let's not add them in in the first place */
		if (new_ptr != NULL) {
			_memcheck_meta_t* meta = memcheck_new_meta(file, line, size);
			_memcheck_tou_llist_append(&_memcheck_g_memblocks, new_ptr, meta, 0,1);

			_memcheck_g_stats.n_mallocs += 1;
			_memcheck_g_stats.n_total_allocs += 1;
			_memcheck_g_stats.total_alloc_size += size;
		}

		memcheck_set_tracking(1);
	}
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	_memcheck_tou_thread_mutex_unlock(&_memcheck_g_mutex);
#endif
	return new_ptr;
}


void* memcheck_calloc(size_t num, size_t size, const char* file, size_t line)
{
	void* new_ptr = NULL; /* Pointer to a new block of memory to be returned
	                         at the end after releasing mutex (if enabled)  */
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	if (_memcheck_tou_thread_mutex_lock(&_memcheck_g_mutex) != 0) {
		fprintf(stderr, "[%s] Unexpected mutex lock failure\n", __func__);
		return NULL;
	}
#endif

	if (!memcheck_is_tracking()) {
		new_ptr = calloc(num, size);
	} else {
		memcheck_set_tracking(0);

		new_ptr = calloc(num, size);

		size = num * size; /*calloc size */

#ifndef MEMCHECK_NO_OUTPUT
		fprintf(memcheck_get_status_fp(), "[CALLOC ] %p%s {n=%" _MEMCHECK_TOU_PRIuZ "} @ %s L%" _MEMCHECK_TOU_PRIuZ "\n",
			new_ptr, (new_ptr == NULL ? " <SKIPPING>" : ""), size, file, line);
		fflush(memcheck_get_status_fp());
#endif
		if (new_ptr != NULL) {
			_memcheck_meta_t* meta = memcheck_new_meta(file, line, size);
			_memcheck_tou_llist_append(&_memcheck_g_memblocks, new_ptr, meta, 0,1);

			_memcheck_g_stats.n_callocs += 1;
			_memcheck_g_stats.n_total_allocs += 1;
			_memcheck_g_stats.total_alloc_size += size;
		}
		
		memcheck_set_tracking(1);
	}
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	_memcheck_tou_thread_mutex_unlock(&_memcheck_g_mutex);
#endif
	return new_ptr;
}


void* memcheck_realloc(void* ptr, size_t new_size, const char* file, size_t line)
{
	void* new_ptr = NULL; /* Pointer to a new block of memory to be returned
	                         at the end after releasing mutex (if enabled)  */
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	if (_memcheck_tou_thread_mutex_lock(&_memcheck_g_mutex) != 0) {
		fprintf(stderr, "[%s] Unexpected mutex lock failure\n", __func__);
		return NULL;
	}
#endif

	if (!memcheck_is_tracking()) {
		new_ptr = realloc(ptr, new_size);
	} else {
		_memcheck_meta_t* meta;
		_memcheck_tou_llist_t* elem;
		memcheck_set_tracking(0);

		elem = _memcheck_tou_llist_find_exact_one(_memcheck_g_memblocks, ptr);
		if (!elem) {
#ifndef MEMCHECK_NO_CRITICAL_OUTPUT
			if (ptr != NULL) {
				fprintf(stderr/*memcheck_get_status_fp()*/, "[REALLOC] [!!] USING REALLOC ON NONEXISTENT ELEMENT (%p); RAW MALLOC/REALLOC/CALLOC USED SOMEWHERE?\n", ptr);
				fflush(stderr/*memcheck_get_status_fp()*/);
			}
#endif
			/* But patch it and continue anyways */
			meta = memcheck_new_meta(file, line, 0);
			elem = _memcheck_tou_llist_append(&_memcheck_g_memblocks, ptr, meta, 0,1);
		} else {
			meta = (_memcheck_meta_t*) elem->dat2;
		}

		/* Do output in two parts because using ptr after realloc is UB */
#ifndef MEMCHECK_NO_OUTPUT
		fprintf(memcheck_get_status_fp(), "[REALLOC] %p {n=%" _MEMCHECK_TOU_PRIuZ "}",
			ptr, meta->size);
		fflush(memcheck_get_status_fp());
#endif

		new_ptr = realloc(ptr, new_size);

#ifndef MEMCHECK_NO_OUTPUT
		fprintf(memcheck_get_status_fp(), " --> %p {n=%" _MEMCHECK_TOU_PRIuZ "} @ %s L%" _MEMCHECK_TOU_PRIuZ "\n",
			new_ptr, new_size, file, line);
		fflush(memcheck_get_status_fp());
#endif

		_memcheck_g_stats.n_reallocs += 1;
		/* Change in total allocs only if requested size was 0 */
		/* -> means it's effectively just a malloc */
		if (meta->size == 0)
			_memcheck_g_stats.n_total_allocs += 1;

		_memcheck_g_stats.total_alloc_size += new_size - meta->size;

		meta->file = file;
		meta->line = line;
		meta->size = new_size;
		elem->dat1 = new_ptr;
		
		memcheck_set_tracking(1);
		
	}
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	_memcheck_tou_thread_mutex_unlock(&_memcheck_g_mutex);
#endif
	return new_ptr;
}


void memcheck_free(void* ptr, const char* file, size_t line)
{
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	if (_memcheck_tou_thread_mutex_lock(&_memcheck_g_mutex) != 0) {
		fprintf(stderr, "[%s] Unexpected mutex lock failure\n", __func__);
		return;
	}
#endif
	if (!memcheck_is_tracking()) {
		free(ptr);
	} else {
		_memcheck_meta_t* meta;
		_memcheck_tou_llist_t* elem;
		memcheck_set_tracking(0);

		elem = _memcheck_tou_llist_find_exact_one(_memcheck_g_memblocks, ptr);
		if (!elem) {
			if (ptr == NULL) {
				/* Do not bark at null pointers */
				memcheck_set_tracking(1);
#ifdef MEMCHECK_ENABLE_THREADSAFETY
				_memcheck_tou_thread_mutex_unlock(&_memcheck_g_mutex);
#endif
				return;
			}

#ifndef MEMCHECK_NO_CRITICAL_OUTPUT
			fprintf(stderr/*memcheck_get_status_fp()*/, "[FREE   ] [!!] TRYING TO USE FREE ON NONEXISTENT ELEMENT (%p); RAW MALLOC/REALLOC/CALLOC USED SOMEWHERE?\n"
			                                            "          [!!] MIGHT CAUSE SEGFAULT (CONTINUING ANYWAY...)\n", ptr);
			fflush(stderr/*memcheck_get_status_fp()*/);
#endif
			/* But patch it and try to continue anyways (just pretend we had a malloc() with size 0) */
			meta = memcheck_new_meta(file, line, 0);
			elem = _memcheck_tou_llist_append(&_memcheck_g_memblocks, ptr, meta, 0,1); 
			_memcheck_g_stats.n_mallocs += 1;
			_memcheck_g_stats.n_total_allocs += 1;
		} else {
			meta = (_memcheck_meta_t*) elem->dat2;
		}

#ifndef MEMCHECK_NO_OUTPUT
		fprintf(memcheck_get_status_fp(), "[FREE   ] %p {n=%" _MEMCHECK_TOU_PRIuZ "} @ %s L%" _MEMCHECK_TOU_PRIuZ "\n", ptr, meta->size, file, line);
		fflush(memcheck_get_status_fp());
#endif
		free(ptr);

		_memcheck_g_stats.n_frees += 1;
		_memcheck_g_stats.total_free_size += meta->size;
		
		/*
			NEED TO CHECK WHETHER ELEM WAS HEAD OR NOT (TO UPDATE _memcheck_g_memblocks)
		*/
		if (_memcheck_tou_llist_is_head(elem)) {
			_memcheck_g_memblocks = _memcheck_tou_llist_remove(elem);
		} else {
			_memcheck_tou_llist_remove(elem);
		}

		memcheck_set_tracking(1);
	}
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	_memcheck_tou_thread_mutex_unlock(&_memcheck_g_mutex);
#endif
}


int memcheck_stats(FILE* fp)
{
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	if (_memcheck_tou_thread_mutex_lock(&_memcheck_g_mutex) != 0) {
		fprintf(stderr, "[%s] Unexpected mutex lock failure\n", __func__);
		return 0;
	}
#endif
	if (!fp)
		fp = memcheck_get_status_fp();

	fprintf(fp, "\n------------------------------------------\n");
	fprintf(fp, " >      Displaying memcheck stats:      <\n");
	fprintf(fp, "------------------------------------------\n");
	fprintf(fp, "  - malloc()'s:             %" _MEMCHECK_TOU_PRIuZ "\n", _memcheck_g_stats.n_mallocs);
	fprintf(fp, "  - calloc()'s:             %" _MEMCHECK_TOU_PRIuZ "\n", _memcheck_g_stats.n_callocs);
	fprintf(fp, "  - realloc()'s:            %" _MEMCHECK_TOU_PRIuZ "\n", _memcheck_g_stats.n_reallocs);
	fprintf(fp, "     Total acquiring calls: %" _MEMCHECK_TOU_PRIuZ "\n", _memcheck_g_stats.n_total_allocs);
	fprintf(fp, "     Total freeing calls:   %" _MEMCHECK_TOU_PRIuZ "\n", _memcheck_g_stats.n_frees);
	fprintf(fp, "------------------------------------------\n");
	if (_memcheck_g_stats.n_frees < _memcheck_g_stats.n_total_allocs) {
		fprintf(fp, " ===> MISSING: %" _MEMCHECK_TOU_PRIdZ " free()'s \n", _memcheck_g_stats.n_total_allocs - _memcheck_g_stats.n_frees);
	} else if (_memcheck_g_stats.n_frees > _memcheck_g_stats.n_total_allocs) {
		fprintf(fp, " ===> SURPLUS: %" _MEMCHECK_TOU_PRIdZ " allocation(s) \n", _memcheck_g_stats.n_frees - _memcheck_g_stats.n_total_allocs);
		fprintf(fp, " ===> THIS SHOULDN'T HAPPEN, CHECK LOGS \n");
	} else {
		fprintf(fp, "                   OK.                  \n");
	}
	fprintf(fp, "------------------------------------------\n");
	fprintf(fp, "  - Total alloc'd size:     %" _MEMCHECK_TOU_PRIuZ "\n", _memcheck_g_stats.total_alloc_size);
	fprintf(fp, "  - Total free'd size:      %" _MEMCHECK_TOU_PRIuZ "\n", _memcheck_g_stats.total_free_size);
	fprintf(fp, "------------------------------------------\n");
	if (_memcheck_g_stats.total_free_size < _memcheck_g_stats.total_alloc_size) {
		fprintf(fp, " ===> DIFF: %" _MEMCHECK_TOU_PRIdZ " bytes (0x%" _MEMCHECK_TOU_PRIxZ ") \n",
			_memcheck_g_stats.total_alloc_size - _memcheck_g_stats.total_free_size,
			_memcheck_g_stats.total_alloc_size - _memcheck_g_stats.total_free_size);
	} else if (_memcheck_g_stats.total_free_size > _memcheck_g_stats.total_alloc_size) {
		fprintf(fp, " ===> FREE() SURPLUS: %" _MEMCHECK_TOU_PRIdZ " bytes (0x%" _MEMCHECK_TOU_PRIxZ ") \n",
			_memcheck_g_stats.total_free_size - _memcheck_g_stats.total_alloc_size,
			_memcheck_g_stats.total_free_size - _memcheck_g_stats.total_alloc_size);
		fprintf(fp, " ===> THIS SHOULDN'T HAPPEN, CHECK LOGS \n");
	} else {
		fprintf(fp, "                   OK.                  \n");
	}
	fprintf(fp, "------------------------------------------\n");
	fprintf(fp, "\n");
	fflush(fp);

	/* All is good, no unfreed elements */
	if (!_memcheck_g_memblocks || _memcheck_tou_llist_len(_memcheck_g_memblocks) < 1) {
#ifdef MEMCHECK_ENABLE_THREADSAFETY
		_memcheck_tou_thread_mutex_unlock(&_memcheck_g_mutex);
#endif
		return 1;
	}

	/* If unfreed allocation detected, display them. */
	{
		_memcheck_tou_llist_t* elem = _memcheck_tou_llist_get_oldest(_memcheck_g_memblocks);
		fprintf(fp, "\n-=[ UNFREED ALLOCATIONS DETECTED. ]=-\n");
		fprintf(fp, "\n-=[ Displaying stored remaining elements: ]=-\n");
		fflush(fp);
		while (elem) {
			_memcheck_meta_t* meta = (_memcheck_meta_t*) elem->dat2;
			const int nbytes_default = 20;
			int nbytes = ((int)meta->size > nbytes_default) ? nbytes_default : (int)meta->size;
			nbytes = (nbytes < 0) ? nbytes_default : nbytes;
			fprintf(fp, "  > %p {n=%" _MEMCHECK_TOU_PRIuZ " (0x%" _MEMCHECK_TOU_PRIxZ ")} :: FROM: %s ; L%" _MEMCHECK_TOU_PRIuZ "  (first %d bytes...  |%.*s|)\n",
				elem->dat1, meta->size, meta->size, meta->file, meta->line, nbytes, nbytes, (char*)elem->dat1);
			elem = _memcheck_tou_llist_get_newer(elem);
		}
		fprintf(fp, "-=[ Memcheck elements over. ]=-\n\n");
	}
	
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	_memcheck_tou_thread_mutex_unlock(&_memcheck_g_mutex);
#endif
	return 0;
}


void memcheck_stats_reset(void)
{
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	if (_memcheck_tou_thread_mutex_lock(&_memcheck_g_mutex) != 0) {
		fprintf(stderr, "[%s] Unexpected mutex lock failure\n", __func__);
		return;
	}
#endif
	memset(&_memcheck_g_stats, 0, sizeof(_memcheck_g_stats));
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	_memcheck_tou_thread_mutex_unlock(&_memcheck_g_mutex);
#endif
}


void memcheck_cleanup(void)
{
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	if (_memcheck_tou_thread_mutex_lock(&_memcheck_g_mutex) != 0) {
		fprintf(stderr, "[%s] Unexpected mutex lock failure\n", __func__);
		return;
	}
#endif
	if (_memcheck_g_manages_devnull)
		fclose(_memcheck_g_status_fp);
	
	if (!_memcheck_g_memblocks) {
#ifdef MEMCHECK_ENABLE_THREADSAFETY
		_memcheck_tou_thread_mutex_unlock(&_memcheck_g_mutex);
#endif
		return;
	}

#ifdef MEMCHECK_PURGE_ON_CLEANUP
	memcheck_purge_remaining();
#endif
	_memcheck_tou_llist_destroy(_memcheck_g_memblocks);
	_memcheck_g_memblocks = NULL;

#ifdef MEMCHECK_ENABLE_THREADSAFETY
	if (_memcheck_g_mutex_init_successful) {
		_memcheck_tou_thread_mutex_unlock(&_memcheck_g_mutex);
		_memcheck_tou_thread_mutex_destroy(&_memcheck_g_mutex);
	}
#endif
}


void memcheck_purge_remaining(void)
{
	_memcheck_tou_llist_t* elem;
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	if (_memcheck_tou_thread_mutex_lock(&_memcheck_g_mutex) != 0) {
		fprintf(stderr, "[%s] Unexpected mutex lock failure\n", __func__);
		return;
	}
#endif
	if (!_memcheck_g_memblocks) {
#ifdef MEMCHECK_ENABLE_THREADSAFETY
		_memcheck_tou_thread_mutex_unlock(&_memcheck_g_mutex);
#endif
		return;
	}

	elem = _memcheck_tou_llist_get_newest(_memcheck_g_memblocks);
#ifndef MEMCHECK_NO_OUTPUT
	fprintf(memcheck_get_status_fp(), "\n-=[! Purging remaining elements... !]=-\n");
#endif

	while (elem) {
		_memcheck_meta_t* meta = (_memcheck_meta_t*) elem->dat2;
		_memcheck_tou_llist_t* older;
	
#ifndef MEMCHECK_NO_OUTPUT
		const int nbytes_default = 20;
		int nbytes = ((int)meta->size > nbytes_default) ? nbytes_default : (int)meta->size;
		nbytes = (nbytes < 0) ? nbytes_default : nbytes;
		fprintf(memcheck_get_status_fp(), "  %% Freeing %p... {n=%" _MEMCHECK_TOU_PRIuZ "} :: FROM: %s ; L%" _MEMCHECK_TOU_PRIuZ "  (first %d bytes...  |%.*s|)\n",
			elem->dat1, meta->size, meta->file, meta->line, nbytes, nbytes, (char*)elem->dat1);
		fflush(memcheck_get_status_fp());
#endif
		free(elem->dat1);
		
		_memcheck_g_stats.n_frees += 1;
		_memcheck_g_stats.total_free_size += meta->size;

		/* Don't forget to destroy storage otherwise we might get double free's */
		older = _memcheck_tou_llist_get_older(elem);
		if (_memcheck_tou_llist_is_head(elem)) {
			_memcheck_g_memblocks = older;
		}
		_memcheck_tou_llist_remove(elem);
		elem = older;
	}

#ifndef MEMCHECK_NO_OUTPUT
	fprintf(memcheck_get_status_fp(), "-=[! Purge done. !]=-\n");
#endif
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	_memcheck_tou_thread_mutex_unlock(&_memcheck_g_mutex);
#endif
}


_memcheck_tou_llist_t** memcheck_get_memblocks(void)
{
	_memcheck_tou_llist_t** mblk;
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	if (_memcheck_tou_thread_mutex_lock(&_memcheck_g_mutex) != 0) {
		fprintf(stderr, "[%s] Unexpected mutex lock failure\n", __func__);
		return NULL;
	}
#endif
	mblk = &_memcheck_g_memblocks;
#ifdef MEMCHECK_ENABLE_THREADSAFETY
	_memcheck_tou_thread_mutex_unlock(&_memcheck_g_mutex);
#endif
	return mblk;
}


#endif /* MEMCHECK_IGNORE */

#endif /* MEMCHECK_IMPLEMENTATION_DONE */
#endif /* MEMCHECK_IMPLEMENTATION */


/**********************************************************************************************/
/*  These will always get defined (unless memcheck is disabled, and they are not overridden)  */
/**********************************************************************************************/
#ifndef MEMCHECK_IGNORE
#	ifndef malloc
#		define malloc(size)           memcheck_malloc(size, __FILE__, __LINE__)
#	endif
#	ifndef calloc
#		define calloc(num, size)      memcheck_calloc(num, size, __FILE__, __LINE__)
#	endif
#	ifndef realloc
#		define realloc(ptr, new_size) memcheck_realloc(ptr, new_size, __FILE__, __LINE__)
#	endif
#	ifndef free
#		define free(ptr)              memcheck_free(ptr, __FILE__, __LINE__)
#	endif
#endif
