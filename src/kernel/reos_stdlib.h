#ifndef REOS_STDLIB_H
#define REOS_STDLIB_H

#include <stdlib.h>

#ifdef __GNUC__
# define __MALLOC_P(args)       args __THROW
#else
# define __MALLOC_P(args)       args
#endif

#define HAS_MALLOC_USABLE_SIZE
extern size_t malloc_usable_size __MALLOC_P ((void *__ptr));

#endif
