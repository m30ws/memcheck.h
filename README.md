# memcheck.h

A simple C89/C++98 single-file header library for checking memory leaks in your app.

## (Very) Basic usage:
```c
#define MEMCHECK_IMPLEMENTATION
#include "memcheck.h"

/* A lot of code... */

memcheck_stats(NULL); /* NULL for auto (default: stdout) or FILE* */
memcheck_cleanup();
```

Simply include `memcheck.h` into any of your .c/.cpp files where you want to track
`malloc`/`calloc`/`realloc`/`free` calls. Make sure to also define `MEMCHECK_IMPLEMENTATION` in only ONE of your files to trigger adding source into it. At any point call `memcheck_stats()` to see a summary of your allocations.

You may define `-DMEMCHECK_IGNORE` to prevent all functionality; memcheck functions in your
code may remain since they will still be defined but as no-op versions of themselves.
<br>
If you are defining it, make sure to define it globally accessible to wherever you use memcheck (for instance through compiler options like shown).

Also available:
- `MEMCHECK_NO_OUTPUT` - disable all "debug" output. this overrides `memcheck_set_status_fp()` (`memcheck_stats()` will still work as normal when called)
- `MEMCHECK_PURGE_ON_CLEANUP` - when `memcheck_cleanup()` is called also try to free the remaining memory blocks (if any)
- `MEMCHECK_ENABLE_THREADSAFETY` - enables global mutex and locking when accessing global memcheck resources (TODO: consider making opt-out instead of opt-in?)
- `MEMCHECK_NO_CRITICAL_OUTPUT` - normally, `realloc()` and `free()` call attempts on non-tracked memory address will output warning message even if debug output is disabled; this option prevents it

Look at `example/` to see one way to use it, or look at the function declarations to see all available features which should more-or-less be documented.

```c
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
```

## Preview

### Success
When all allocations were successfully freed `memcheck_stats()` call looks like:
```
------------------------------------------
 >      Displaying memcheck stats:      <
------------------------------------------
  - malloc()'s:             13213
  - calloc()'s:             1227
  - realloc()'s:            1664
     Total acquiring calls: 14751
     Total freeing calls:   14751
------------------------------------------
                   OK.
------------------------------------------
  - Total alloc'd size:     398250
  - Total free'd size:      398250
------------------------------------------
                   OK.
------------------------------------------
```

### Errors
If allocations that weren't matched by calls to `free()` were encountered output might look similar to this:
```
------------------------------------------
 >      Displaying memcheck stats:      <
------------------------------------------
  - malloc()'s:             13213
  - calloc()'s:             1227
  - realloc()'s:            1664
     Total acquiring calls: 14751
     Total freeing calls:   14750
------------------------------------------
 ===>  MISSING: 1 free()'s
------------------------------------------
  - Total alloc'd size:     398250
  - Total free'd size:      339680
------------------------------------------
 ===>  DIFF: 58570 bytes (0xe4ca)
------------------------------------------


-=[ UNFREED ALLOCATIONS DETECTED. ]=-

-=[ Displaying stored remaining elements: ]=-
  > 000001E3FD16FCB0 {n=58570 (0xe4ca)} :: FROM: ./src/prog.c ; L2888  (first 20 bytes...  |<!DOCTYPE html PUBLI|)
-=[ Memcheck elements over. ]=-
```

### Logged output
If you don't have logging disabled during execution (you should have if you have a lot of output, or even better you should redirect it to a file), you may see info similar to this:
```
[MALLOC ] 000001DF36F3C980 {n=17} @ ./src/prog.c L2056
[REALLOC] 0000000000000000 {n=0} --> 000001DF36F2E480 {n=5} @ ./src/prog.c L2727
[REALLOC] 000001DF36F2E480 {n=5} --> 000001DF36F3C420 {n=14} @ ./src/prog.c L2727
[REALLOC] 000001DF36F3C420 {n=14} --> 000001DF36F3C6A0 {n=15} @ ./src/prog.c L2745
[REALLOC] 000001DF36F3C6A0 {n=15} --> 000001DF36F3C540 {n=15} @ ./src/prog.c L2763
[CALLOC ] 000001DF36F3C420 {n=24} @ ./src/prog_module.c L31
[MALLOC ] 000001DF36F2E470 {n=1} @ ./src/prog.c L2039
[FREE   ] 000001DF36F3C980 {n=17} @ ./src/prog_module.c L297
[MALLOC ] 000001DF36F39290 {n=40} @ ./src/prog.c L3018
```

### Double-free / freeing unmanaged memory
You may get output similar to the one in this example in one of two cases:
- if you try to double-free memory
- if you use `free()` in a part of the code where `memcheck` tracking is active, but the original memory was allocated before the tracking was active

In the first case memcheck will try to pretend as if the allocation had actually been valid and behave like there was a `malloc()` with size 0 sometime before. If the assumption was valid, all should be good and `memcheck_stats()` should return valid values.
Of course, if double-free is what actually happened the program will simply segfault (since `memcheck_free()` just forwards the call to C `free()` afterwards). Such a segfault is visible in the given example.
```
[FREE   ] [!!] TRYING TO USE FREE ON NONEXISTENT ELEMENT (000001A25F3CBFA0); RAW MALLOC/REALLOC/CALLOC USED SOMEWHERE?
          [!!] MIGHT CAUSE SEGFAULT (CONTINUING ANYWAY...)
[FREE   ] 000001A25F3CBFA0 {n=0} @ ./src/prog.c L76
Segmentation fault.
```

## Downsides
Since this uses `__FILE__` and `__LINE__` macros unfortunately you won't be able to see the full stacktrace. However, you will still be able to get an idea of whether there are any memory issues and where they come from.

## TODO:
- Improve output formats
