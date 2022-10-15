#ifndef KL_SLAB_ALLOC_H
# define KL_SLAB_ALLOC_H

# include <unistd.h>
# include <stdbool.h>
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
typedef struct slab_cache
{
	t_slab *free_slabs;
	t_slab *partially_slabs;
	t_slab *full_slabs;
	void *(*allocate_slab)(size_t);
	void (*deallocate_slab)(void *, size_t);
	size_t object_size;
	size_t objects_in_slab;
	size_t slab_offset_to_header;
	size_t slab_size;
} t_slab_cache;

/**
 * Init slab allocator
 **/
void cache_setup(
	t_slab_cache *cache,
	void *(*allocate_slab)(size_t),
	void (*deallocate_slab)(void *, size_t),
	size_t object_size
);

/**
 * Deallocate all slabs in cache
 **/
void cache_release(t_slab_cache *cache);

/**
 * Deallocate all free slabs in cache
 **/
void cache_shrink(t_slab_cache *cache);

/**
 * Get free object in slab
 **/
void *cache_alloc(t_slab_cache *cache);

/**
 * Free object in slab
 **/
void cache_free(t_slab_cache *cache, void *ptr);

/**
 * Check if pointer is in cache
 **/
bool ptr_in_cache(const t_slab_cache *cache, void *ptr);

/**
 * Print allocated memory in cache
 **/
void show_cache_mem(t_slab_cache *cache);

#endif