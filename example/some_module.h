#ifndef _TESTING_FUNC_H_
#define _TESTING_FUNC_H_
#ifdef __cplusplus
extern "C" {
#endif

int* testing_func_new();
void testing_func_destroy(int* ptr);

#ifdef __cplusplus
}
#endif
#endif