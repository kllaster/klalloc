#ifndef KL_BODY_ALLOC_H
# define KL_BODY_ALLOC_H

# include <limits.h>
# include <unistd.h>
# include <stdbool.h>

# define MIN_ALLOCTAED_BYTE_MEM 16

typedef struct s_body_alloc_node
{
	size_t size;
	bool is_free;
} t_body_alloc_node;

typedef struct s_body_alloc_meta t_body_alloc_meta;

struct s_body_alloc_meta
{
	t_body_alloc_meta *next;
	void *start;
	void *end;
	size_t size;
	size_t max_node_size;
};

void body_alloc_setup(t_body_alloc_meta *body_alloc_meta, void *buf, size_t size);
void *body_alloc(t_body_alloc_meta *body_alloc_meta, size_t size);
void body_alloc_free(t_body_alloc_meta *body_alloc_meta, void *ptr);

#endif