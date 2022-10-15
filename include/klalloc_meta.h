#ifndef KL_ALLOC_META_H
# define KL_ALLOC_META_H

# include <sys/mman.h>
# include "body_alloc.h"
# include "slab_alloc.h"
# include "utils.h"

const static int COUNT_SLAB_CACHE = 9;
const static int SLAB_CACHE_IDX_FOR_ALLOC_OTHER = 8;

const static size_t g_cache_obj_sizes[COUNT_SLAB_CACHE] = {
	8,
	16,
	32,
	64,
	128,
	256,
	1024,
	2048,
	4096
};

typedef struct s_klalloc_meta
{
	t_body_alloc_meta *list;
	t_slab_cache cache[COUNT_SLAB_CACHE];
} t_klalloc_meta;

t_klalloc_meta *get_klalloc_meta();

#endif