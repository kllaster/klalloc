#include "slab_alloc.h"

void cache_setup(
	struct cache *cache,
	void *(*allocate_slab)(size_t),
	void (*deallocate_slab)(void *, size_t),
	size_t object_size
)
{
	cache->free_slabs = NULL;
	cache->partially_slabs = NULL;
	cache->full_slabs = NULL;
	cache->object_size = object_size;
	cache->allocate_slab = allocate_slab;
	cache->deallocate_slab = deallocate_slab;

	size_t slab_order;
	if (object_size > 4096)
		slab_order = (int)(object_size / 4096) * 2;
	else if (object_size > 2048)
		slab_order = 1;
	else
		slab_order = 0;

	cache->slab_size = 4096 * (size_t)(1UL << slab_order);
	size_t slab_size_without_header = cache->slab_size - sizeof(t_slab);

	if (slab_size_without_header < object_size)
	{
		slab_order += 1;
		cache->slab_size = 4096 * (size_t)(1 << slab_order);
	}

	cache->objects_in_slab = (cache->slab_size - sizeof(t_slab)) / object_size;
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

	mem = (char *)cache->allocate_slab(cache->slab_size);
	if (mem == NULL)
		return;
	slab = (t_slab * )(mem + cache->slab_offset_to_header);
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

static bool ptr_in_slabs(t_slab *slab, size_t slab_offset_to_header, void *ptr)
{
	while (slab)
	{
		if (ptr >= slab->start_mem && (char *)ptr < (char *)slab->start_mem + slab_offset_to_header)
			return true;
		slab = slab->next;
	}
	return false;
}

bool ptr_in_cache(struct cache *cache, void *ptr)
{
	if (cache->partially_slabs
	    && ptr_in_slabs(cache->partially_slabs, cache->slab_offset_to_header, ptr))
	{
		return true;
	}
	if (cache->full_slabs
	    && ptr_in_slabs(cache->full_slabs, cache->slab_offset_to_header, ptr))
	{
		return true;
	}
	return false;
}

void cache_free(struct cache *cache, void *ptr)
{
	t_slab *slab;
	char **new_obj;

	slab = (t_slab * )(
		(char *)((uint64_t)ptr & (~(cache->slab_size - 1)))
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
		cache->deallocate_slab(iter->start_mem, cache->slab_size);
		iter = iter->next;
	}
	cache->free_slabs = NULL;
}

static void free_slabs(void (*deallocate_slab)(void *, size_t), t_slab *slab_iter, int slab_size)
{
	t_slab *iter;
	void *slab;

	iter = slab_iter;
	while (iter)
	{
		slab = iter->start_mem;
		iter = iter->next;
		deallocate_slab(slab, slab_size);
	}
}

void cache_release(struct cache *cache)
{
	free_slabs(cache->deallocate_slab, cache->free_slabs, cache->slab_size);
	cache->free_slabs = NULL;
	free_slabs(cache->deallocate_slab, cache->partially_slabs, cache->slab_size);
	cache->partially_slabs = NULL;
	free_slabs(cache->deallocate_slab, cache->full_slabs, cache->slab_size);
	cache->full_slabs = NULL;
}

static void show_slabs_mem(t_slab *slab, size_t object_size, size_t objects_in_slab)
{
	size_t slab_bytes = object_size * objects_in_slab;
	while (slab)
	{
		if (slab->count_free != objects_in_slab)
		{
			print_ptr(slab->start_mem);
			print_str_literal(" - ");
			print_ptr((char *)slab->start_mem + slab_bytes);
			print_str_literal(" : ");
			print_num((objects_in_slab - slab->count_free) * object_size);
			print_str_literal(" bytes\n");
		}
		slab = slab->next;
	}
}

void show_cache_mem(struct cache *cache)
{
	if (!cache->partially_slabs && !cache->full_slabs)
		return;

	print_str_literal("Slabs with object size: ");
	print_num(cache->object_size);
	print_str_literal(" bytes\n");

	if (cache->partially_slabs)
	{
		show_slabs_mem(
			cache->partially_slabs,
			cache->object_size,
			cache->objects_in_slab
		);
	}

	if (cache->full_slabs)
	{
		show_slabs_mem(
			cache->full_slabs,
			cache->object_size,
			cache->objects_in_slab
		);
	}
}