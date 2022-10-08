#ifndef KL_ALLOC_UTILS_H
# define KL_ALLOC_UTILS_H

# include <unistd.h>

/**
 * Machine word size. Depending on the architecture,
 * can be 4 or 8 bytes.
 */
typedef intptr_t t_word;

size_t align(size_t n);

#endif