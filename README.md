# memcheck.h

A simple C/C++ single-file header library for checking memory leaks in your app.

## (Very) Basic usage:
```c
#define MEMCHECK_IMPLEMENTATION
#include "memcheck.h"

/* A lot of code... */

memcheck_stats();
memcheck_cleanup();
```

Simply include `memcheck.h` into any of your .c/.cpp files where you want to track
`malloc`/`calloc`/`realloc`/`free` calls. Make sure to also define `MEMCHECK_IMPLEMENTATION` in only ONE of your files to trigger adding source into it. At any point call `memcheck_stats()` to see a summary of your allocations.

You may define `-DMEMCHECK_IGNORE` to prevent all functionality; memcheck functions in your
code may remain since they will still be defined but as no-op versions of themselves.

Look at `example/` to see one way to use it, or look at the function declarations in the header to see all available features which should more-or-less be documented.

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

  > 000001E3FD16FCB0 {n=58570} :: FROM: ./src/prog.c ; L2888  (first 20 bytes...  |<!DOCTYPE html PUBLI|)

-=[ Memcheck elements over. ]=-
```

### Logged output
If you don't have logging disabled during execution (you should if you have a lot of output, or even better you should redirect it to a file), you may see info similar to this:
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

In the first case memcheck will try to pretend as if the allocation had actually been valid and behave like there was a `malloc()` with size 0 sometime before. If the assumption was valid, all should be good and stats() should return valid values.
Of course, if double-free is what actually happened the program will simply segfault (since `memcheck_free()` just forwards the call to C `free()` afterwards). Such a segfault is visible in the given example.
```
[FREE   ] [!!] TRYING TO USE FREE ON NONEXISTENT ELEMENT (000001A25F3CBFA0); RAW MALLOC/REALLOC USED SOMEWHERE?
          [!!] MIGHT CAUSE SEGFAULT (CONTINUING ANYWAY...)
[FREE   ] 000001A25F3CBFA0 {n=0} @ ./src/prog.c L76
Segmentation fault.
```

## TODO:
- Make threadsafe
- Improve output formats
