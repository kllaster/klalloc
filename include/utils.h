#ifndef KL_ALLOC_UTILS_H
# define KL_ALLOC_UTILS_H

# include <unistd.h>
# include "body_alloc.h"
# include "slab_alloc.h"

typedef struct s_klalloc_meta
{
	t_body_alloc_meta *list;
	struct cache small_objects;
	struct cache medium_objects;
} t_klalloc_meta;

t_klalloc_meta *get_klalloc_meta();

/**
 * Machine word size. Depending on the architecture,
 * can be 4 or 8 bytes.
 */
typedef intptr_t t_word;

size_t align(size_t n, size_t round_up_to);
void *align_addr(void *stack, uintptr_t align);

#endif