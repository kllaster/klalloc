#include "klalloc.h"
#include "klalloc_meta.h"

t_klalloc_meta *get_klalloc_meta()
{
	static t_klalloc_meta klalloc_meta = {};

	return &klalloc_meta;
}

static t_body_alloc_meta *allocate_new_body_alloc(size_t min_size)
{
	t_body_alloc_meta *body_alloc_meta = NULL;
	void *memory = NULL;
	size_t pages = 10;
	size_t body_alloc_size = getpagesize() * pages;

	if (min_size > body_alloc_size - sizeof(t_body_alloc_meta))
	{
		body_alloc_size = align(min_size + sizeof(t_body_alloc_meta), getpagesize());
		pages = 1;
	}

	do
	{
		memory = mmap(
			NULL, body_alloc_size,
			PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
		);
		body_alloc_size = getpagesize() * --pages;
	} while (!memory && pages && (min_size > body_alloc_size - sizeof(t_body_alloc_meta)));

	if (memory == NULL)
		return NULL;

	body_alloc_meta = memory;

	body_alloc_setup(
		body_alloc_meta,
		(char *)memory + sizeof(t_body_alloc_meta),
		body_alloc_size - sizeof(t_body_alloc_meta)
	);
	return body_alloc_meta;
}

static t_body_alloc_meta *find_body_alloc_with_right_min_size(
	const t_klalloc_meta *klalloc_meta,
	size_t min_size
)
{
	t_body_alloc_meta *suitable = NULL;
	t_body_alloc_meta *iter = klalloc_meta->list;

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
	t_body_alloc_meta *suitable = find_body_alloc_with_right_min_size(klalloc_meta, size);

	if (!suitable)
	{
		// if suitable is not find, allocate new body_alloc
		suitable = allocate_new_body_alloc(size);
		if (klalloc_meta->list == NULL)
			klalloc_meta->list = suitable;
		else
		{
			t_body_alloc_meta *last = klalloc_meta->list;
			while (last->next)
				last = last->next;
			last->next = suitable;
		}
	}
	return body_alloc(suitable, size);
}

static void *alloc_piece_of_mmap_align(t_klalloc_meta *klalloc_meta, size_t size, size_t align_to)
{
	void *mem = NULL;

	t_body_alloc_meta *ba_meta = klalloc_meta->list;
	while (ba_meta)
	{
		mem = body_alloc_align(ba_meta, size, align_to);
		if (mem)
			break;
		ba_meta = ba_meta->next;
	}

	if (!mem)
	{
		// if suitable is not find, allocate new body_alloc
		ba_meta = allocate_new_body_alloc(size);
		if (klalloc_meta->list == NULL)
			klalloc_meta->list = ba_meta;
		else
		{
			t_body_alloc_meta *last = klalloc_meta->list;
			while (last->next)
				last = last->next;
			last->next = ba_meta;
		}
		mem = body_alloc_align(ba_meta, size, align_to);
	}
	return mem;
}

static void *allocate_by_cache(t_slab_cache *cache, size_t cache_obj);

void *alloc_slab(size_t size)
{
	t_klalloc_meta *klalloc_meta = get_klalloc_meta();

	if (size == 4096 && klalloc_meta->cache[SLAB_CACHE_IDX_FOR_ALLOC_OTHER].object_size == 4096)
		return allocate_by_cache(&klalloc_meta->cache[SLAB_CACHE_IDX_FOR_ALLOC_OTHER], 4096);

	return alloc_piece_of_mmap_align(klalloc_meta, size, getpagesize());
}

void free_slab(void *slab, size_t size)
{
	t_klalloc_meta *klalloc_meta = get_klalloc_meta();

	if (size == 4096 && klalloc_meta->cache[SLAB_CACHE_IDX_FOR_ALLOC_OTHER].object_size == 4096)
	{
		cache_free(&klalloc_meta->cache[SLAB_CACHE_IDX_FOR_ALLOC_OTHER], slab);
		return;
	}

	t_body_alloc_meta *body_alloc = ptr_in_body_alloc_list(klalloc_meta->list, slab);
	if (body_alloc)
		body_alloc_free(body_alloc, slab);
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
			mem = allocate_by_cache(&klalloc_meta->cache[i], g_cache_obj_sizes[i]);
			return mem;
		}
	}
	mem = alloc_piece_of_mmap(klalloc_meta, align(size, sizeof(t_word)));
	return mem;
}

void klfree(void *ptr)
{
	// Check between what borders located ptr
	//
	// slab->start >= ptr < slab->start + slab->size
	//
	// body_alloc->start >= ptr < body_alloc->start + body_alloc->size
	t_klalloc_meta *klalloc_meta = get_klalloc_meta();

	bool in_cache = false;
	for (int i = 0; i < COUNT_SLAB_CACHE; i++)
	{
		if (ptr_in_cache(&klalloc_meta->cache[i], ptr))
		{
			cache_free(&klalloc_meta->cache[i], ptr);
			cache_shrink(&klalloc_meta->cache[i]);
			in_cache = true;
		}
	}

	if (!in_cache)
	{
		t_body_alloc_meta *body_alloc = ptr_in_body_alloc_list(klalloc_meta->list, ptr);
		if (body_alloc)
			body_alloc_free(body_alloc, ptr);
	}
}

//void *klrealloc(void *ptr, size_t size)
//{
//	//..
//}

static void
show_body_alloc_mem(const t_klalloc_meta *klalloc_meta, const t_body_alloc_meta *ba_meta)
{
	t_body_alloc_node *node = ba_meta->start;
	while (node)
	{
		bool in_cache = false;
		for (int i = 0; i < COUNT_SLAB_CACHE; i++)
		{
			if (!node->is_free
			    && ptr_in_cache(&klalloc_meta->cache[i], node + sizeof(t_body_alloc_node)))
			{
				in_cache = true;
				break;
			}
		}

		if (!node->is_free && in_cache == false)
		{
			print_str_literal("Body allocator node: ");
			print_num(node->size);
			print_str_literal(" bytes\n");

			print_ptr(node + sizeof(t_body_alloc_node));
			print_str_literal(" - ");
			print_ptr(node + sizeof(t_body_alloc_node) + node->size);
			print_str_literal("\n");
		}
		node = get_next_node(ba_meta, node);
	}
}

void show_alloc_mem()
{
	t_klalloc_meta *klalloc_meta = get_klalloc_meta();

	for (int i = 0; i < COUNT_SLAB_CACHE; i++)
		show_cache_mem(&klalloc_meta->cache[i]);

	for (t_body_alloc_meta *iter = klalloc_meta->list; iter != NULL; iter = iter->next)
		show_body_alloc_mem(klalloc_meta, iter);
}