/*
	This gives an example of how to use memcheck.h in a project with multiple .c files.
 	You're free to do whatever you want with this code.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MEMCHECK_IMPLEMENTATION
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

	/* Display statistics and cleanup internal structures (also releases /dev/null if was used) */
	memcheck_stats(NULL /* NULL -> memcheck_get_status_fp() */);
	memcheck_cleanup();

	printf("\nDone.\n");
	return 0;
}