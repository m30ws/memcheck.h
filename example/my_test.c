/*
	This gives an example of how to use memcheck.h in a project with multiple .c files.
 	You're free to do whatever you want with this code.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MEMCHECK_IMPLEMENTATION
/* #define MEMCHECK_NO_OUTPUT */
/* #define MEMCHECK_PURGE_ON_CLEANUP */
#include "../memcheck.h"

#include "some_module.h"

typedef struct {
	int a;
	char b;
} teststruct;

/* class TestClass {
public:
	int abc;
	TestClass(int def) {
		this->abc = def;
	}
	~TestClass() {
		printf("Class done. (%d)\n", this->abc);
	}
}; */

void func2(char** ptr)
{
	free(*ptr);
}


int main(int argc, char const* argv[])
{
	/* memcheck_set_status_fp(NULL); */                           /* This will be interpreted as /dev/null */
	/* memcheck_set_status_fp(fopen("memcheck_log.txt", "w"));*/  /* Some other FILE* */
	printf("\n");

	/* TestClass cls = TestClass(123456); */

	/*  */
	char* ptr = (char*) malloc(1024+1);
	teststruct* ts = (teststruct*) calloc(1, sizeof(*ts));
	ptr = (char*) realloc(ptr, 2048+1);

	char** ptr2 = &ptr;
	func2(ptr2);

	/* Memcheck will yell about this but continue execution (probably into segfault) */
	/* free(ptr2); */

	free(ts);

	/* Call "module" function that allocates some more memory random amount of times */
	/* This "module" (.c+.h) also contains a #include "memcheck.h" which enables its allocations to be tracked */
	srand(time(NULL));
	int n_calls = rand() % 20;
	printf("Calling module function %d times...\n", n_calls);
	
	int i;
	for (i = 0; i < n_calls; i++)
		testing_func_destroy(testing_func_new()); /* Module contains a counter how many times _new() has been called */
	
	/* Retrieve another reading from it */
	int* val = testing_func_new();
	printf("Value from module :: %d\n", *val);
	testing_func_destroy(val);

	/* #### --Manual memblock access-- #### */
#ifndef MEMCHECK_IGNORE
	/* (void) malloc(2222); */
	_memcheck_tou_llist_t* lst = *memcheck_get_memblocks();
	while (lst) {
		void* ptr = lst->dat1;
		_memcheck_meta_t* meta = (_memcheck_meta_t*)(lst->dat2);
		printf("#### Memblock :: %p, f=%s, l=%zu, s=%zu\n",
			ptr, meta->file, meta->line, meta->size);
		lst = lst->prev;
	}
#endif

	/* Display statistics */
	memcheck_stats(NULL /* NULL -> memcheck_get_status_fp() */);

	/* Allocate once more to test purging */
	
	printf("==============\nTesting purge:\n==============\n\n");
	(void) malloc(5555);
	memcheck_stats(stdout); // Always outputs to stdout //

	memcheck_purge_remaining(); // free() all remaining unfreed allocations //

	printf("\n====================\nStats after purging:\n====================\n");
	memcheck_stats(stdout);
	

	/* Cleanup internal structures (also releases /dev/null if was used) */
	memcheck_cleanup();

	printf("\nDone.\n");
	return 0;
}