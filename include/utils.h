#ifndef KL_ALLOC_UTILS_H
# define KL_ALLOC_UTILS_H

# include <unistd.h>

#define print_str_literal(str) \
        write(STDIN_FILENO, (str), sizeof(str) - 1); \

/**
 * Machine word size. Depending on the architecture,
 * can be 4 or 8 bytes.
 */
typedef intptr_t t_word;

size_t align(size_t n, size_t round_up_to);
void *align_addr(const void *addr, uintptr_t align);

void print_ptr(const void *ptr);
void print_num(size_t num);
void print_num_base(size_t num, size_t base, size_t width);

#endif