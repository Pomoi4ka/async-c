#ifndef DA_H_
#define DA_H_

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define ARRAY_LEN(x) (sizeof(x)/sizeof(*(x)))

#define da_delete(da, p)                                                \
    do {                                                                \
        ptrdiff_t i = (p) - (da)->items;                                \
        assert(0 <= i && i < (ptrdiff_t) (da)->count);                  \
        assert(sizeof(*(p)) == sizeof(*(da)->items));                   \
        memcpy(p, (p) + 1, ((da)->count-- - i)*sizeof(*(da)->items));   \
    } while (0)

#define da_alloc(da, cnt)                                               \
    do {                                                                \
        while ((da)->count + (cnt) >= (da)->capacity) {                 \
            (da)->capacity = (da)->capacity ? (da)->capacity*2 : 1;     \
            (da)->items = realloc((da)->items, sizeof(*(da)->items)*(da)->capacity); \
            assert((da)->items && "Realloc failed :(");                 \
        }                                                               \
    } while (0)

#define da_append(da, x)                        \
    do {                                        \
        da_alloc(da, 1);                        \
        (da)->items[(da)->count++] = x;         \
    } while (0)

#define da_append_many(da, xs, cnt)                 \
    do {                                            \
        size_t __c = (size_t) (cnt);                \
        da_alloc(da, __c);                          \
        memcpy(&(da)->items[(da)->count], xs, __c); \
        (da)->count += __c;                         \
    } while (0)

#endif
