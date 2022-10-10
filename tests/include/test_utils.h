#ifndef KL_TEST_ALLOC_UTILS_H
# define KL_TEST_ALLOC_UTILS_H

# include <printf.h>
# include <stdlib.h>
# include <assert.h>
# include <printf.h>
# include <unistd.h>

#define assert_print_ptr(expr, ptr1, ptr2) \
        if (!(expr)) { \
            dprintf(STDERR_FILENO, "ptr1: %p, ptr2: %p\n", ptr1, ptr2); \
            assert((expr)); \
        } \

#define assert_print_size_t(expr, val1, val2) \
        if (!(expr)) { \
            dprintf(STDERR_FILENO, "current: %zu, expected: %zu\n", val1, val2); \
            assert((expr)); \
        } \

#define print_str_literal(str) \
        write(1, (str), sizeof(str) - 1); \

void slab_tests();
void body_alloc_tests();

#endif