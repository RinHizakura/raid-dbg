#ifndef UTILS_ASSERT_H
#define UTILS_ASSERT_H

#include <assert.h>
#include <stdio.h>

#ifndef NDEBUG
#define assert_op(a, b, op)                                                \
    for (; !((a) op(b)); assert((a) op(b))) {                              \
        fprintf(stderr,                                                    \
                "Assertion Failed: The left value is 0x%lx but the right " \
                "value is "                                                \
                "0x%lx\n",                                                 \
                (long) a, (long) b);                                       \
    }

#define assert_eq(a, b) assert_op(a, b, ==)
#define assert_ne(a, b) assert_op(a, b, !=)
#define assert_le(a, b) assert_op(a, b, <=)
#define assert_ge(a, b) assert_op(a, b, >=)
#else
#define assert_eq(a, b)
#define assert_ne(a, b)
#define assert_le(a, b)
#define assert_ge(a, b)
#endif

#endif /* UTILS_ASSERT_H */
