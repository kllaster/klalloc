#ifndef KLSLAB_H
# define KLSLAB_H

# include <unistd.h>
# include "utils.h"

typedef struct s_slab t_slab;

struct s_slab
{
	char *first_free_obj;
	void *start_mem;
	t_slab *next;
	t_slab *prev;
	unsigned int count_free;
};

typedef enum
{
	FULL,
	PARTIALLY,
	FREE
} t_slab_list;

/**
 * This struct is slab allocator for object_size
 **/
struct cache
{
	t_slab *free_slabs;
	t_slab *partially_slabs;
	t_slab *full_slabs;
	// allocate_slab - allocate memory where size = getpagesize() * 2 ^ order
	void *(*allocate_slab)(int);
	void (*deallocate_slab)(void *, int);
	size_t object_size;
	size_t objects_in_slab;
	size_t slab_offset_to_header;

	// for allocate_slab() / deallocate_slab()
	int slab_order;
};

/**
 * Init slab allocator
 **/
void cache_setup(
	struct cache *cache,
	void *(*allocate_slab)(int),
	void (*deallocate_slab)(void *, int),
	size_t object_size
);

/**
 * Deallocate all slabs in cache
 **/
void cache_release(struct cache *cache);

/**
 * Deallocate all free slabs in cache
 **/
void cache_shrink(struct cache *cache);

/**
 * Get free object in slab
 **/
void *cache_alloc(struct cache *cache);

/**
 * Free object in slab
 **/
void cache_free(struct cache *cache, void *ptr);

#endif