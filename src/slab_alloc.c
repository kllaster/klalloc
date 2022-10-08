#include "slab_alloc.h"

void cache_setup(
	struct cache *cache,
	void *(*allocate_slab)(int),
	void (*deallocate_slab)(void *, int),
	size_t object_size
)
{
	object_size = align(object_size);
	cache->free_slabs = NULL;
	cache->partially_slabs = NULL;
	cache->full_slabs = NULL;
	cache->object_size = object_size;
	cache->allocate_slab = allocate_slab;
	cache->deallocate_slab = deallocate_slab;

	if (object_size > 4096)
		cache->slab_order = (int)(object_size / 4096) * 2;
	else if (object_size > 2048)
		cache->slab_order = 1;
	else
		cache->slab_order = 0;

	size_t slab_size = 4096 * (size_t)(1 << cache->slab_order);
	size_t slab_size_without_header = slab_size - sizeof(t_slab);

	if (slab_size_without_header < object_size)
	{
		cache->slab_order += 1;
		slab_size = 4096 * (size_t)(1 << cache->slab_order);
	}
	cache->objects_in_slab = (slab_size - sizeof(t_slab)) / object_size;
	cache->slab_offset_to_header = cache->objects_in_slab * object_size;
}

static void slab_move_list(t_slab *slab, t_slab **move_from, t_slab **move_to)
{
	t_slab *next_slab;
	t_slab *prev_slab;

	// Delete slab in your list
	next_slab = slab->next;
	prev_slab = slab->prev;
	if (prev_slab == NULL)
	{
		if (next_slab)
		{
			next_slab->prev = NULL;
			*move_from = next_slab;
		}
		else
			*move_from = NULL;
	}
	else
	{
		prev_slab->next = next_slab;
		if (next_slab)
			next_slab->prev = prev_slab;
	}

	// Add slab in new list
	slab->next = *move_to;
	slab->prev = NULL;
	if (*move_to)
		(*move_to)->prev = slab;
	*move_to = slab;
}

static void check_slab_obj_and_move(struct cache *cache, t_slab_list slab_list, t_slab *slab)
{
	if (slab == NULL)
		return;
	if (slab->count_free == 0)
	{
		// move_slab_to_full_list
		if (slab_list == FULL)
			return;
		else if (slab_list == PARTIALLY)
			slab_move_list(slab, &cache->partially_slabs, &cache->full_slabs);
		else // (slab_list == FREE)
			slab_move_list(slab, &cache->free_slabs, &cache->full_slabs);
	}
	else if (slab->count_free == cache->objects_in_slab)
	{
		// move_slab_to_free_list
		if (slab_list == FREE)
			return;
		else if (slab_list == FULL)
			slab_move_list(slab, &cache->full_slabs, &cache->free_slabs);
		else // (slab_list == PARTIALLY)
			slab_move_list(slab, &cache->partially_slabs, &cache->free_slabs);
	}
	else
	{
		// move_slab_to_partially_list
		if (slab_list == PARTIALLY)
			return;
		else if (slab_list == FULL)
			slab_move_list(slab, &cache->full_slabs, &cache->partially_slabs);
		else // (slab_list == FREE)
			slab_move_list(slab, &cache->free_slabs, &cache->partially_slabs);
	}
}

static void *get_new_obj(t_slab *slab)
{
	void *obj;

	if (slab == NULL)
		return NULL;
	obj = slab->first_free_obj;
	slab->count_free--;
	if (slab->count_free > 0)
		slab->first_free_obj = *((char **)obj);
	return obj;
}

static void init_slab_objs(struct cache *cache, t_slab *slab)
{
	unsigned int i;
	char **obj;
	char *next_obj;

	i = 0;
	obj = (char **)slab->start_mem;
	while (i < slab->count_free)
	{
		next_obj = (char *)obj + cache->object_size;
		*obj = next_obj;
		obj = (char **)next_obj;
		i++;
	}
	obj = NULL;
}

static void alloc_new_slab(struct cache *cache)
{
	char *mem;
	t_slab *slab;

	mem = (char *)cache->allocate_slab(cache->slab_order);
	if (mem == NULL)
		return;
	slab = (t_slab *)(mem + cache->slab_offset_to_header);
	slab->start_mem = mem;
	slab->count_free = cache->objects_in_slab;
	slab->next = NULL;
	slab->prev = NULL;
	init_slab_objs(cache, slab);
	slab->first_free_obj = mem;
	cache->free_slabs = slab;
}

void *cache_alloc(struct cache *cache)
{
	void *obj;

	if (cache->partially_slabs)
	{
		// get object and
		// check if slab don't have free obj
		obj = get_new_obj(cache->partially_slabs);
		check_slab_obj_and_move(cache, PARTIALLY, cache->partially_slabs);
		return obj;
	}
	if (cache->free_slabs == NULL)
		alloc_new_slab(cache);

	// get object and move slab to partially_slabs
	// and move to partially
	// or check if slab don't have free obj move to full
	obj = get_new_obj(cache->free_slabs);
	check_slab_obj_and_move(cache, FREE, cache->free_slabs);
	return obj;
}

void cache_free(struct cache *cache, void *ptr)
{
	t_slab *slab;
	char **new_obj;

	slab = (t_slab *)(
		(char *)((uint64_t)ptr & (~(4096 * (1UL << cache->slab_order) - 1)))
		+ cache->slab_offset_to_header
	);
	new_obj = (char **)ptr;
	*new_obj = slab->first_free_obj;
	slab->first_free_obj = (char *)new_obj;

	if (slab->count_free == 0)
	{
		slab->count_free++;
		check_slab_obj_and_move(cache, FULL, slab);
	}
	else if (slab->count_free == cache->objects_in_slab)
	{
		// it's error or double free
		return;
	}
	else
	{
		slab->count_free++;
		check_slab_obj_and_move(cache, PARTIALLY, slab);
	}
}

void cache_shrink(struct cache *cache)
{
	t_slab *iter;

	iter = cache->free_slabs;
	while (iter)
	{
		cache->deallocate_slab(iter->start_mem, cache->slab_order);
		iter = iter->next;
	}
	cache->free_slabs = NULL;
}

static void free_slabs(void (*deallocate_slab)(void *, int), t_slab *slab_iter, int slab_order)
{
	t_slab *iter;
	void *slab;

	iter = slab_iter;
	while (iter)
	{
		slab = iter->start_mem;
		iter = iter->next;
		deallocate_slab(slab, slab_order);
	}
}

void cache_release(struct cache *cache)
{
	free_slabs(cache->deallocate_slab, cache->free_slabs, cache->slab_order);
	cache->free_slabs = NULL;
	free_slabs(cache->deallocate_slab, cache->partially_slabs, cache->slab_order);
	cache->partially_slabs = NULL;
	free_slabs(cache->deallocate_slab, cache->full_slabs, cache->slab_order);
	cache->full_slabs = NULL;
}