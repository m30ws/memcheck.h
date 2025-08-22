/*
	This gives an example of how to use memcheck.h in a project with multiple .c files.
 	You're free to do whatever you want with this code.
*/

#ifndef _TESTING_FUNC_H_
#define _TESTING_FUNC_H_
#ifdef __cplusplus
extern "C" {
#endif

int* testing_func_new(void);
void testing_func_destroy(int* ptr);

#ifdef __cplusplus
}
#endif
#endif
