#include <stdlib.h>

#include "../memcheck.h"

#include "some_module.h"

static int module_counter = 0;

int* testing_func_new()
{
#ifdef __cplusplus
	int* val = (int*) malloc(sizeof(*val));
#else
	int* val = malloc(sizeof(*val));
#endif
	*val = ++module_counter;
	return val;
}

void testing_func_destroy(int* ptr)
{
	free(ptr);
}
