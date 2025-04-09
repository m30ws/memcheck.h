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
malloc/calloc/realloc/free calls. Make sure to also define `MEMCHECK_IMPLEMENTATION` in only ONE of your files to trigger adding source into it. At any point call `memcheck_stats()` to see a summary of your allocations.

You may define `-DMEMCHECK_IGNORE` to prevent all functionality; memcheck functions in your
code may remain since they will still be defined but as no-op versions of themselves.

Look at `example/` to see one way to use it, or look at the function declarations in the header to see all available features which should more-or-less be documented.

### TODO:
- Make threadsafe