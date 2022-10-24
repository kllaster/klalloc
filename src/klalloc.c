#include "klalloc.h"
#include "klalloc_meta.h"

t_klalloc_meta *get_klalloc_meta()
{
	static t_klalloc_meta klalloc_meta = {};

	return &klalloc_meta;
}

static t_btags_alloc_meta *allocate_new_btags_alloc(size_t min_size)
{
	t_btags_alloc_meta *btags_alloc_meta = NULL;
	void *memory = NULL;
	size_t pages = 10;
	size_t btags_alloc_size = getpagesize() * pages;

	if (min_size > btags_alloc_size - sizeof(t_btags_alloc_meta))
	{
		btags_alloc_size = align(min_size + sizeof(t_btags_alloc_meta), getpagesize());
		pages = 1;
	}

	do
	{
		memory = mmap(
			NULL, btags_alloc_size,
			PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
		);
		btags_alloc_size = getpagesize() * --pages;
	} while (!memory && pages && (min_size > btags_alloc_size - sizeof(t_btags_alloc_meta)));

	if (memory == NULL)
		return NULL;

	btags_alloc_meta = memory;

	btags_alloc_setup(
		btags_alloc_meta,
		(char *)memory + sizeof(t_btags_alloc_meta),
		btags_alloc_size - sizeof(t_btags_alloc_meta)
	);
	return btags_alloc_meta;
}

static t_btags_alloc_meta *find_btags_alloc_with_right_min_size(
	const t_klalloc_meta *klalloc_meta,
	size_t min_size
)
{
	t_btags_alloc_meta *suitable = NULL;
	t_btags_alloc_meta *iter = klalloc_meta->list;

	while (iter)
	{
		if ((suitable && iter->max_node_size < suitable->max_node_size)
		    || (!suitable && iter->max_node_size >= min_size))
		{
			suitable = iter;
		}
		iter = iter->next;
	}
	return suitable;
}

static void *alloc_piece_of_mmap(t_klalloc_meta *klalloc_meta, size_t size)
{
	t_btags_alloc_meta *suitable = find_btags_alloc_with_right_min_size(klalloc_meta, size);

	if (!suitable)
	{
		// if suitable is not find, allocate new btags_alloc
		suitable = allocate_new_btags_alloc(size);
		if (klalloc_meta->list == NULL)
			klalloc_meta->list = suitable;
		else
		{
			t_btags_alloc_meta *last = klalloc_meta->list;
			while (last->next)
				last = last->next;
			last->next = suitable;
		}
	}
	return btags_alloc(suitable, size);
}

static void *alloc_piece_of_mmap_align(t_klalloc_meta *klalloc_meta, size_t size, size_t align_to)
{
	void *mem = NULL;

	t_btags_alloc_meta *ba_meta = klalloc_meta->list;
	while (ba_meta)
	{
		mem = btags_alloc_align(ba_meta, size, align_to);
		if (mem)
			break;
		ba_meta = ba_meta->next;
	}

	if (!mem)
	{
		// if suitable is not find, allocate new btags_alloc
		ba_meta = allocate_new_btags_alloc(size);
		if (klalloc_meta->list == NULL)
			klalloc_meta->list = ba_meta;
		else
		{
			t_btags_alloc_meta *last = klalloc_meta->list;
			while (last->next)
				last = last->next;
			last->next = ba_meta;
		}
		mem = btags_alloc_align(ba_meta, size, align_to);
	}
	return mem;
}

static void *allocate_by_cache(t_slab_cache *cache, size_t cache_obj);

void free_slab(void *slab, size_t size)
{
	t_klalloc_meta *klalloc_meta = get_klalloc_meta();

	t_btags_alloc_meta *btags_alloc = ptr_in_btags_alloc_list(klalloc_meta->list, slab);
	if (btags_alloc)
		btags_alloc_free(btags_alloc, slab);
	(void)size;
}

void *alloc_slab(size_t size)
{
	t_klalloc_meta *klalloc_meta = get_klalloc_meta();

	return alloc_piece_of_mmap_align(klalloc_meta, size, getpagesize());
}

static void *allocate_by_cache(t_slab_cache *cache, size_t cache_obj)
{
	if (cache->object_size == 0)
	{
		cache_setup(
			cache,
			alloc_slab,
			free_slab,
			cache_obj
		);
	}
	return cache_alloc(cache);
}

void *klalloc(size_t size)
{
	void *mem;
	t_klalloc_meta *klalloc_meta = get_klalloc_meta();

	if (size == 0)
		return NULL;
	size = align(size, sizeof(t_word));

	for (int i = 0; i < COUNT_SLAB_CACHE; i++)
	{
		if (size <= g_cache_obj_sizes[i])
		{
			pthread_mutex_lock(&g_klalloc_mutex);
			mem = allocate_by_cache(&klalloc_meta->cache[i], g_cache_obj_sizes[i]);
			pthread_mutex_unlock(&g_klalloc_mutex);
			return mem;
		}
	}
	pthread_mutex_lock(&g_klalloc_mutex);
	mem = alloc_piece_of_mmap(klalloc_meta, align(size, sizeof(t_word)));
	pthread_mutex_unlock(&g_klalloc_mutex);
	return mem;
}

void klfree(void *ptr)
{
	// Check between what borders located ptr
	//
	// slab->start >= ptr < slab->start + slab->size
	//
	// btags_alloc->start >= ptr < btags_alloc->start + btags_alloc->size
	t_klalloc_meta *klalloc_meta = get_klalloc_meta();

	bool in_cache = false;
	pthread_mutex_lock(&g_klalloc_mutex);
	for (int i = 0; i < COUNT_SLAB_CACHE; i++)
	{
		if (ptr_in_cache(&klalloc_meta->cache[i], ptr))
		{
			cache_free(&klalloc_meta->cache[i], ptr);
			cache_shrink(&klalloc_meta->cache[i]);
			in_cache = true;
			break;
		}
	}

	if (!in_cache)
	{
		t_btags_alloc_meta *btags_alloc = ptr_in_btags_alloc_list(klalloc_meta->list, ptr);
		if (btags_alloc)
			btags_alloc_free(btags_alloc, ptr);
	}
	pthread_mutex_unlock(&g_klalloc_mutex);
}

void *klrealloc(void *ptr, size_t size)
{
	int in_cache_idx = -1;
	size_t ptr_size = 0;
	t_klalloc_meta *klalloc_meta;

	if (ptr == NULL)
		return klalloc(size);
	else if (size == 0)
	{
		klfree(ptr);
		return NULL;
	}

	pthread_mutex_lock(&g_klalloc_mutex);
	klalloc_meta = get_klalloc_meta();
	for (int i = 0; i < COUNT_SLAB_CACHE; i++)
	{
		if (ptr_in_cache(&klalloc_meta->cache[i], ptr))
		{
			if (klalloc_meta->cache[i].object_size == size)
				return ptr;
			ptr_size = klalloc_meta->cache[i].object_size;
			in_cache_idx = i;
			break;
		}
	}

	t_btags_alloc_meta *ba_meta;
	if (in_cache_idx == -1)
	{
		ba_meta = ptr_in_btags_alloc_list(klalloc_meta->list, ptr);
		if (!ba_meta)
		{
			// if the pointer is not found in the cache and btags_alloc
			return NULL;
		}

		t_btags_alloc_node *node = ba_meta->start;
		while (node != NULL)
		{
			if (ptr == (char *)node + sizeof(t_btags_alloc_node))
			{
				ptr_size = node->size;
				break;
			}
			node = get_next_node(ba_meta, node);
		}

		if (ptr_size == 0)
		{
			// This happens if ptr is invalid
			return NULL;
		}
	}
	pthread_mutex_unlock(&g_klalloc_mutex);

	void *mem = klalloc(size);
	if (mem)
		memmove(mem, ptr, ptr_size);

	pthread_mutex_lock(&g_klalloc_mutex);
	if (in_cache_idx != -1)
	{
		cache_free(&klalloc_meta->cache[in_cache_idx], ptr);
		cache_shrink(&klalloc_meta->cache[in_cache_idx]);
	}
	else
		btags_alloc_free(ba_meta, ptr);
	pthread_mutex_unlock(&g_klalloc_mutex);

	return mem;
}

static void
show_btags_alloc_mem(const t_klalloc_meta *klalloc_meta, const t_btags_alloc_meta *ba_meta)
{
	t_btags_alloc_node *node = ba_meta->start;
	while (node)
	{
		bool in_cache = false;
		for (int i = 0; i < COUNT_SLAB_CACHE; i++)
		{
			if (!node->is_free
			    && ptr_in_cache(&klalloc_meta->cache[i], node + sizeof(t_btags_alloc_node)))
			{
				in_cache = true;
				break;
			}
		}

		if (!node->is_free && in_cache == false)
		{
			print_str_literal("Btags allocator node: ");
			print_num(node->size);
			print_str_literal(" bytes\n");

			print_ptr(node + sizeof(t_btags_alloc_node));
			print_str_literal(" - ");
			print_ptr(node + sizeof(t_btags_alloc_node) + node->size);
			print_str_literal("\n");
		}
		node = get_next_node(ba_meta, node);
	}
}

void show_alloc_mem()
{
	t_klalloc_meta *klalloc_meta = get_klalloc_meta();

	pthread_mutex_lock(&g_klalloc_mutex);
	for (int i = 0; i < COUNT_SLAB_CACHE; i++)
		show_cache_mem(&klalloc_meta->cache[i]);

	for (t_btags_alloc_meta *iter = klalloc_meta->list; iter != NULL; iter = iter->next)
		show_btags_alloc_mem(klalloc_meta, iter);
	pthread_mutex_unlock(&g_klalloc_mutex);
}