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

	Look at example/ to see one way to use it or look at the function declarations
	  further down to see all available features.

	TODO:
	  - Make thread-safe
	  - Improve output formats
*/

#ifndef _MEMCHECK_H_
#define _MEMCHECK_H_

#pragma message "-- Memcheck active."

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* User funcs */
void  memcheck_stats(FILE* fp);          /* Displays numbers of allocations and releases, their byte amounts,
                                            and locations where they don't match (if any) up until this call.
                                            FILE* may be passed to output to specific stream; pass NULL to use
                                            stream set by memcheck_set_status_fp(). (Default: stdout) */
void  memcheck_set_status_fp(FILE* fp);  /* Set which FILE* to be used for immediate logging messages (allocations and
                                            releases; also used for memcheck_stats() if not overridden).
                                            Status_fp defaults to stdout but if NULL is passed to this function output will
                                            be redirected to /dev/null. This will cause memcheck itself to manage that FILE*.
                                            If you want to manage the FILE* yourself, open it using fopen() and pass it in here */
void  memcheck_set_track_mem(bool yn);   /* Whether to perform call tracking (dynamically turn memcheck on and off) */
void  memcheck_cleanup(void);            /* Destroys the internal memory blocks storage and closes status_fp if memcheck
                                            is managing it (Only a case when you let it through using memcheck_set_status_fp(NULL)).
                                            Note: you will NOT get memcheck warnings if you forget to call memcheck_cleanup()! */
void  memcheck_stats_reset(void);        /* Resets all statistics tracked to 0 */
FILE* memcheck_get_status_fp(void);      /* Returns the currently used status_fp inside memcheck. (Defaults to stdout) */

/* Internal (but may use explicitly) */
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
#if defined(MEMCHECK_IMPLEMENTATION_DONE)
#pragma error "MEMCHECK_IMPLEMENTATION already defined somewhere!"
#else

#define MEMCHECK_IMPLEMENTATION_DONE
#pragma message "-- Memcheck active (implementation)."

#ifdef MEMCHECK_IGNORE
	void memcheck_stats(FILE* fp)
	{
		(void)fp;
	}
	void memcheck_stats_reset(void)
	{
		(void)0;
	}
	void memcheck_set_track_mem(bool yn)
	{
		(void)yn;
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
	void* memcheck_malloc(size_t size, const char* file, size_t line)
	{
		(void)size; (void)file; (void)line;
		return NULL;
	}
	void* memcheck_calloc(size_t num, size_t size, const char* file, size_t line)
	{
		(void)num; (void)size; (void)file; (void)line;
		return NULL;
	}
	void* memcheck_realloc(void* ptr, size_t new_size, const char* file, size_t line)
	{
		(void)ptr; (void)new_size; (void)file; (void)line;
		return NULL;
	}
	void memcheck_free(void* ptr, const char* file, size_t line)
	{
		(void)ptr; (void)file; (void)line;
	}
#else


/********** EMBED TOU_LLIST IMPL (extracted from tou.h) **********/

#ifdef __cplusplus
extern "C" {        /* Extern C for llist */
#endif

typedef struct _memcheck_tou_llist_s
{
	struct _memcheck_tou_llist_s* prev;     /**< previous element                     */
	struct _memcheck_tou_llist_s* next;     /**< next element                         */
	void* dat1;                 /**< useful data                          */
	void* dat2;                 /**< useful data                          */
	char destroy_dat1/* : 1*/;  /**< automatically deallocate this data ? */
	char destroy_dat2/* : 1*/;  /**< automatically deallocate this data ? */
} _memcheck_tou_llist;

static _memcheck_tou_llist* _memcheck_tou_llist_append(_memcheck_tou_llist** node_ref, void* dat1, void* dat2, char dat1_is_dynalloc, char dat2_is_dynalloc)
{
	if (node_ref == NULL)
		return NULL;

	/* Spawn new node */
	_memcheck_tou_llist* new_node = (_memcheck_tou_llist*) malloc(sizeof(*new_node));

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
	_memcheck_tou_llist* prev_node = (*node_ref);
	if (prev_node->next == NULL) {
		prev_node->next = new_node; /* Update passed node's .next */
		new_node->prev = prev_node; /* Update new node's .prev */
		*node_ref = new_node; /* Update passed reference to point to the new node */
		return new_node;
	}

	/* The given node is now somewhere inbetween (after head) */

	/* Setup new node links */
	_memcheck_tou_llist* previous_next = prev_node->next; /* Save previously-next node */
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

static _memcheck_tou_llist* _memcheck_tou_llist_get_tail(_memcheck_tou_llist* list)
{
	if (!list) return NULL;

	while (list->prev)
		list = list->prev;

	return list;
}

static _memcheck_tou_llist* _memcheck_tou_llist_get_oldest(_memcheck_tou_llist* list)
{
	return _memcheck_tou_llist_get_tail(list);
}

static _memcheck_tou_llist* _memcheck_tou_llist_get_newer(_memcheck_tou_llist* elem)
{
	if (elem == NULL) return NULL;
	return elem->next;
}

static _memcheck_tou_llist* _memcheck_tou_llist_find_exactone(_memcheck_tou_llist* list, void* dat1)
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

static unsigned char _memcheck_tou_llist_is_head(_memcheck_tou_llist* elem)
{
	return elem == NULL || elem->next == NULL;
}

static _memcheck_tou_llist* _memcheck_tou_llist_pop(_memcheck_tou_llist* elem)
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

static void _memcheck_tou_llist_free_element(_memcheck_tou_llist* elem)
{
	if (!elem) return;

	if (elem->destroy_dat1) free(elem->dat1);
	if (elem->destroy_dat2) free(elem->dat2);

	free(elem);
}

static _memcheck_tou_llist* _memcheck_tou_llist_remove(_memcheck_tou_llist* elem)
{	
	_memcheck_tou_llist* next = elem->next;
	_memcheck_tou_llist* prev = elem->prev;
	_memcheck_tou_llist_pop(elem);
	_memcheck_tou_llist_free_element(elem);

	if (next == NULL) /* This element was head */
		return prev; /* prev will be the new head */
	else
		return next; /* if assigned, next will be the new head */
}

static size_t _memcheck_tou_llist_len(_memcheck_tou_llist* list)
{
	if (list == NULL)
		return 0;

	size_t len = 0;

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

static void _memcheck_tou_llist_destroy(_memcheck_tou_llist* list)
{
	if (!list) return;

	if (list->next == NULL) { /* this is head. */
		_memcheck_tou_llist* prev;
		_memcheck_tou_llist* curr = list;

		while (curr != NULL) {
			prev = curr->prev;
			if (curr->destroy_dat1) free(curr->dat1);
			if (curr->destroy_dat2) free(curr->dat2);
			curr->prev = NULL;
			free(curr);
			curr = prev;
		}

	} else { /* this is tail (or inbetween) */
		_memcheck_tou_llist* next;
		_memcheck_tou_llist* curr = list;
		
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

static bool 				_memcheck_g_do_track_mem 		= true; /* Controls current tracking of allocations and releases */
static FILE* 				_memcheck_g_status_fp 			= NULL; /* FILE* that serves as log for allocations and releases */
static bool 				_memcheck_g_manages_devnull 	= false; /* Indicator whether this lib needs to keep track of g_status_fp and close it */
static _memcheck_tou_llist* _memcheck_g_memblocks 			= NULL; /* Main storage for tracking allocations, releases and their locations */
static _memcheck_stats_t 	_memcheck_g_stats 				= {0}; /* Statistics tracker for allocations and releases to be displayed at the end */


void memcheck_set_track_mem(bool yn)
{
	_memcheck_g_do_track_mem = yn;
}


/* Only if `fp` is explicitly NULL, system's /dev/null will be used */
void memcheck_set_status_fp(FILE* fp)
{
	if (_memcheck_g_manages_devnull && _memcheck_g_status_fp != NULL)
		fclose(_memcheck_g_status_fp);

	if (fp != NULL) {
		_memcheck_g_status_fp = fp;
		_memcheck_g_manages_devnull = false;
	} else {
#ifdef _WIN32
		_memcheck_g_status_fp = fopen("NUL:", "w");
#else
		_memcheck_g_status_fp = fopen("/dev/null", "w");
#endif
		_memcheck_g_manages_devnull = true;
	}
}


/* Default will be to output info to stdout */
FILE* memcheck_get_status_fp(void)
{
	if (_memcheck_g_status_fp == NULL)
		memcheck_set_status_fp(stdout);
	return _memcheck_g_status_fp;
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
	if (!_memcheck_g_do_track_mem) {
		return malloc(size);
	} else {

		memcheck_set_track_mem(false);

		void* new_ptr = malloc(size);
		fprintf(memcheck_get_status_fp(), "[MALLOC ] %p {n=%zu} @ %s L%zu\n", new_ptr, size, file, line);
		fflush(memcheck_get_status_fp());
		
		_memcheck_g_stats.n_mallocs += 1;
		_memcheck_g_stats.n_total_allocs += 1;
		_memcheck_g_stats.total_alloc_size += size;

		_memcheck_meta_t* meta = memcheck_new_meta(file, line, size);
		_memcheck_tou_llist_append(&_memcheck_g_memblocks, new_ptr, meta, 0,1);
		
		memcheck_set_track_mem(true);
		return new_ptr;
	}
}


void* memcheck_calloc(size_t num, size_t size, const char* file, size_t line)
{
	if (!_memcheck_g_do_track_mem) {
		return calloc(num, size);
	} else {

		memcheck_set_track_mem(false);

		void* new_ptr = calloc(num, size);

		size = num * size; /*calloc size */

		fprintf(memcheck_get_status_fp(), "[CALLOC ] %p {n=%zu} @ %s L%zu\n", new_ptr, size, file, line);
		fflush(memcheck_get_status_fp());
		
		_memcheck_g_stats.n_callocs += 1;
		_memcheck_g_stats.n_total_allocs += 1;
		_memcheck_g_stats.total_alloc_size += size;

		_memcheck_meta_t* meta = memcheck_new_meta(file, line, size);
		_memcheck_tou_llist_append(&_memcheck_g_memblocks, new_ptr, meta, 0,1);
		
		memcheck_set_track_mem(true);
		return new_ptr;
	}
}


void* memcheck_realloc(void* ptr, size_t new_size, const char* file, size_t line)
{
	if (!_memcheck_g_do_track_mem) {
		return realloc(ptr, new_size);
	} else {

		memcheck_set_track_mem(false);

		_memcheck_meta_t* meta;
		_memcheck_tou_llist* elem = _memcheck_tou_llist_find_exactone(_memcheck_g_memblocks, ptr);
		if (!elem) {
			if (ptr != NULL) {
				fprintf(memcheck_get_status_fp(), "[REALLOC] [!!] USING REALLOC ON NONEXISTENT ELEMENT (%p); RAW MALLOC/REALLOC USED SOMEWHERE?\n", ptr);
				fflush(memcheck_get_status_fp());
			}
			/* But patch it and continue anyways */
			meta = memcheck_new_meta(file, line, 0);
			elem = _memcheck_tou_llist_append(&_memcheck_g_memblocks, ptr, meta, 0,1);
		} else {
			meta = (_memcheck_meta_t*) elem->dat2;
		}

		void* new_ptr = realloc(ptr, new_size);
		fprintf(memcheck_get_status_fp(), "[REALLOC] %p {n=%zu} --> %p {n=%zu} @ %s L%zu\n", 
			ptr, meta->size, new_ptr, new_size, file, line);

		fflush(memcheck_get_status_fp());

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
		
		memcheck_set_track_mem(true);
		return new_ptr;
	}
}


void memcheck_free(void* ptr, const char* file, size_t line)
{
	if (!_memcheck_g_do_track_mem) {
		free(ptr);
	} else {

		memcheck_set_track_mem(false);

		_memcheck_meta_t* meta;
		_memcheck_tou_llist* elem = _memcheck_tou_llist_find_exactone(_memcheck_g_memblocks, ptr);
		if (!elem) {
			fprintf(memcheck_get_status_fp(), "[FREE   ] [!!] TRYING TO USE FREE ON NONEXISTENT ELEMENT (%p); RAW MALLOC/REALLOC USED SOMEWHERE?\n", ptr);
			fprintf(memcheck_get_status_fp(), "          [!!] MIGHT CAUSE SEGFAULT (CONTINUING ANYWAY...)\n");
			fflush(memcheck_get_status_fp());
			/* But patch it and try to continue anyways (just pretend we had a malloc() with size 0) */
			meta = memcheck_new_meta(file, line, 0);
			elem = _memcheck_tou_llist_append(&_memcheck_g_memblocks, ptr, meta, 0,1); 
			_memcheck_g_stats.n_mallocs += 1;
			_memcheck_g_stats.n_total_allocs += 1;
		} else {
			meta = (_memcheck_meta_t*) elem->dat2;
		}

		fprintf(memcheck_get_status_fp(), "[FREE   ] %p {n=%zu} @ %s L%zu\n", ptr, meta->size, file, line);
		fflush(memcheck_get_status_fp());
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

		memcheck_set_track_mem(true);
	}
}


void memcheck_stats(FILE* fp)
{
	if (!fp)
		fp = memcheck_get_status_fp();

	fprintf(fp, "\n------------------------------------------\n");
	fprintf(fp, " >      Displaying memcheck stats:      <\n");
	fprintf(fp, "------------------------------------------\n");
	fprintf(fp, "  - malloc()'s:             %zu\n", _memcheck_g_stats.n_mallocs);
	fprintf(fp, "  - calloc()'s:             %zu\n", _memcheck_g_stats.n_callocs);
	fprintf(fp, "  - realloc()'s:            %zu\n", _memcheck_g_stats.n_reallocs);
	fprintf(fp, "     Total acquiring calls: %zu\n", _memcheck_g_stats.n_total_allocs);
	fprintf(fp, "     Total freeing calls:   %zu\n", _memcheck_g_stats.n_frees);
	fprintf(fp, "------------------------------------------\n");
	if (_memcheck_g_stats.n_frees < _memcheck_g_stats.n_total_allocs) {
		fprintf(fp, " ===> MISSING: %zd free()'s \n", _memcheck_g_stats.n_total_allocs - _memcheck_g_stats.n_frees);
	} else if (_memcheck_g_stats.n_frees > _memcheck_g_stats.n_total_allocs) {
		fprintf(fp, " ===> SURPLUS: %zd allocation(s) \n", _memcheck_g_stats.n_frees - _memcheck_g_stats.n_total_allocs);
		fprintf(fp, " ===> THIS SHOULDN'T HAPPEN, CHECK LOGS \n");
	} else {
		fprintf(fp, "                   OK.                  \n");
	}
	fprintf(fp, "------------------------------------------\n");
	fprintf(fp, "  - Total alloc'd size:     %zu\n", _memcheck_g_stats.total_alloc_size);
	fprintf(fp, "  - Total free'd size:      %zu\n", _memcheck_g_stats.total_free_size);
	fprintf(fp, "------------------------------------------\n");
	if (_memcheck_g_stats.total_free_size < _memcheck_g_stats.total_alloc_size) {
		fprintf(fp, " ===> DIFF: %zd bytes (0x%zx) \n",
			_memcheck_g_stats.total_alloc_size - _memcheck_g_stats.total_free_size,
			_memcheck_g_stats.total_alloc_size - _memcheck_g_stats.total_free_size);
	} else if (_memcheck_g_stats.total_free_size > _memcheck_g_stats.total_alloc_size) {
		fprintf(fp, " ===> FREE() SURPLUS: %zd bytes (0x%zx) \n",
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
	if (!_memcheck_g_memblocks || _memcheck_tou_llist_len(_memcheck_g_memblocks) < 1)
		return;

	/* If unfreed allocation detected, display them. */
	_memcheck_tou_llist* elem = _memcheck_tou_llist_get_oldest(_memcheck_g_memblocks);
	fprintf(fp, "\n-=[ UNFREED ALLOCATIONS DETECTED. ]=-\n");
	fprintf(fp, "\n-=[ Displaying stored remaining elements: ]=-\n\n");
	fflush(fp);
	while (elem) {
		_memcheck_meta_t* meta = (_memcheck_meta_t*) elem->dat2;
		const int nbytes_default = 20;
		const int nbytes = (meta->size > nbytes_default) ? nbytes_default : meta->size;
		fprintf(fp, "  > %p {n=%zu} :: FROM: %s ; L%zu  (first %d bytes...  |%.*s|)\n",
			elem->dat1, meta->size, meta->file, meta->line, nbytes, nbytes, (char*)elem->dat1);
		elem = _memcheck_tou_llist_get_newer(elem);
	}
	fprintf(fp, "\n-=[ Memcheck elements over. ]=-\n\n");
}


void memcheck_stats_reset(void)
{
	memset(&_memcheck_g_stats, 0, sizeof(_memcheck_g_stats));
}


void memcheck_cleanup(void)
{
	if (_memcheck_g_manages_devnull)
		fclose(_memcheck_g_status_fp);
	if (!_memcheck_g_memblocks)
		return;
	_memcheck_meta_t* meta = (_memcheck_meta_t*) _memcheck_g_memblocks->dat2;
	_memcheck_tou_llist_destroy(_memcheck_g_memblocks);
	_memcheck_g_memblocks = NULL;
}


#endif /* MEMCHECK_IGNORE */

#endif /* MEMCHECK_IMPLEMENTATION_DONE */
#endif /* MEMCHECK_IMPLEMENTATION */


/*****************************************************************/
/*  These will always get defined (unless memcheck is disabled)  */

#ifndef MEMCHECK_IGNORE
	#define malloc(size) memcheck_malloc(size, __FILE__, __LINE__)
	#define calloc(num, size) memcheck_calloc(num, size, __FILE__, __LINE__)
	#define realloc(ptr, new_size) memcheck_realloc(ptr, new_size, __FILE__, __LINE__)
	#define free(ptr) memcheck_free(ptr, __FILE__, __LINE__)
#endif
