#ifndef KL_ALLOC_META_H
# define KL_ALLOC_META_H

# include <sys/mman.h>
# include "body_alloc.h"
# include "slab_alloc.h"
# include "utils.h"

const static int COUNT_SLAB_CACHE = 6;

const static size_t g_cache_obj_sizes[COUNT_SLAB_CACHE] = {
	8,
	16,
	32,
	64,
	128,
	256,
};

typedef struct s_klalloc_meta
{
	t_body_alloc_meta *list;
	t_slab_cache cache[COUNT_SLAB_CACHE];
} t_klalloc_meta;

t_klalloc_meta *get_klalloc_meta();

#endif