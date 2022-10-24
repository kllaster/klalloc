#ifndef KL_BOUNDARIES_TAGS_ALLOC_H
# define KL_BOUNDARIES_TAGS_ALLOC_H

# include <limits.h>
# include <unistd.h>
# include <stdbool.h>
# include "utils.h"

# define MIN_ALLOCTAED_BYTE_MEM 16

typedef struct s_btags_alloc_node
{
	size_t size;
	bool is_free;
} t_btags_alloc_node;

typedef struct s_btags_alloc_meta t_btags_alloc_meta;

struct s_btags_alloc_meta
{
	t_btags_alloc_meta *next;
	void *start;
	void *end;
	size_t max_node_size;
};

void btags_alloc_setup(t_btags_alloc_meta *ba_meta, void *buf, size_t size);
void *btags_alloc(t_btags_alloc_meta *ba_meta, size_t size);
void *btags_alloc_align(t_btags_alloc_meta *ba_meta, size_t size, size_t align_to);
void btags_alloc_free(t_btags_alloc_meta *ba_meta, void *ptr);

t_btags_alloc_meta *ptr_in_btags_alloc_list(const t_btags_alloc_meta *ba_meta, void *ptr);
t_btags_alloc_node *get_next_node(const t_btags_alloc_meta *ba_meta, const t_btags_alloc_node *node);

#endif