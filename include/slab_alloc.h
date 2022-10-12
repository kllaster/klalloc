#ifndef KLSLAB_H
# define KLSLAB_H

# include <unistd.h>
# include <stdbool.h>

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
	void *(*allocate_slab)(size_t);
	void (*deallocate_slab)(void *, size_t);
	size_t object_size;
	size_t objects_in_slab;
	size_t slab_offset_to_header;
	size_t slab_size;
};

/**
 * Init slab allocator
 **/
void cache_setup(
	struct cache *cache,
	void *(*allocate_slab)(size_t),
	void (*deallocate_slab)(void *, size_t),
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

/**
 * Check if pointer is in cache
 **/
bool ptr_in_cache(struct cache *cache, void *ptr);

#endif