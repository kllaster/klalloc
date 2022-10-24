#include "btags_alloc.h"

static inline t_btags_alloc_node *move_pointer_node(const t_btags_alloc_node *node, size_t size)
{
	return (t_btags_alloc_node *)((char *)node + size);
}

void btags_alloc_setup(t_btags_alloc_meta *ba_meta, void *buf, size_t size)
{
	t_btags_alloc_node *node;

	ba_meta->next = NULL;
	if (buf == NULL || size == 0)
	{
		ba_meta->start = NULL;
		ba_meta->end = NULL;
		ba_meta->max_node_size = 0;
		return;
	}
	ba_meta->start = buf;
	ba_meta->end = (char *)buf + size;

	// First meta info
	node = buf;
	node->is_free = true;
	node->size = size - sizeof(t_btags_alloc_node) * 2;

	// Last meta info
	node = move_pointer_node(node, sizeof(t_btags_alloc_node) + node->size);
	node->is_free = true;
	node->size = size - sizeof(t_btags_alloc_node) * 2;

	ba_meta->max_node_size = node->size;
}

t_btags_alloc_node *get_next_node(const t_btags_alloc_meta *ba_meta, const t_btags_alloc_node *node)
{
	node = move_pointer_node(node, node->size + sizeof(t_btags_alloc_node) * 2);
	if ((void *)node >= ba_meta->end)
		return NULL;
	return (t_btags_alloc_node *)node;
}

static size_t get_max_node_size(const t_btags_alloc_meta *ba_meta)
{
	size_t max_node_size = 0;
	t_btags_alloc_node *node = ba_meta->start;
	while (node != NULL)
	{
		if (node->is_free && node->size > max_node_size)
			max_node_size = node->size;
		node = get_next_node(ba_meta, node);
	}
	return max_node_size;
}

static void
btags_alloc_split_node(t_btags_alloc_node *node, size_t old_size, size_t new_size, bool first_free)
{
	if (old_size > new_size + MIN_ALLOCTAED_BYTE_MEM + sizeof(t_btags_alloc_node) * 2)
	{
		node->size = new_size;
		if (first_free)
			node->is_free = true;
		else
			node->is_free = false;

		node = move_pointer_node(node, new_size + sizeof(t_btags_alloc_node));
		node->size = new_size;
		if (first_free)
			node->is_free = true;
		else
			node->is_free = false;

		size_t split_size = old_size - new_size - sizeof(t_btags_alloc_node) * 2;
		node = move_pointer_node(node, sizeof(t_btags_alloc_node));
		node->size = split_size;
		if (!first_free)
			node->is_free = true;
		else
			node->is_free = false;

		node = move_pointer_node(node, node->size + sizeof(t_btags_alloc_node));
		node->size = split_size;
		if (!first_free)
			node->is_free = true;
		else
			node->is_free = false;
	}
}

void *btags_alloc(t_btags_alloc_meta *ba_meta, size_t size)
{
	t_btags_alloc_node *res = NULL;
	size_t res_size = 0;
	t_btags_alloc_node *node;

	if (ba_meta->start == NULL)
		return NULL;

	if (size > ba_meta->max_node_size)
		return NULL;

	node = ba_meta->start;
	while (node != NULL)
	{
		if (node->is_free && node->size >= size
		    && (res_size == 0 || res_size > node->size))
		{
			res = node;
			res_size = node->size;
		}
		node = get_next_node(ba_meta, node);
	}

	if (res == NULL)
		return NULL;

	btags_alloc_split_node(res, res_size, size, false);

	if (res_size == ba_meta->max_node_size)
		ba_meta->max_node_size = get_max_node_size(ba_meta);
	return (char *)res + sizeof(t_btags_alloc_node);
}

void *btags_alloc_align(t_btags_alloc_meta *ba_meta, size_t size, size_t align_to)
{
	t_btags_alloc_node *res = NULL;
	size_t res_size = 0;
	t_btags_alloc_node *node;

	if (ba_meta->start == NULL)
		return NULL;

	if (size > ba_meta->max_node_size)
		return NULL;

	size_t split_bytes = 0;
	node = ba_meta->start;
	while (node != NULL)
	{
		if (node->is_free && node->size >= size
		    && (res_size == 0 || res_size > node->size))
		{
			char *check_ptr = (char *)align((size_t)node + sizeof(t_btags_alloc_node), align_to);

			char *end_mem = (char *)node + sizeof(t_btags_alloc_node) + node->size;
			if (end_mem > check_ptr
			    && (size_t)(end_mem - check_ptr) >= size)
			{
				check_ptr -= sizeof(t_btags_alloc_node) * 2;
				if ((char *)check_ptr > (char *)node + sizeof(t_btags_alloc_node))
				{
					res = node;
					res_size = node->size;
					split_bytes = (size_t)check_ptr - (size_t)node - sizeof(t_btags_alloc_node);
				}
			}
			else
			{
				res = node;
				res_size = node->size;
				split_bytes = 0;
			}
		}
		node = get_next_node(ba_meta, node);
	}

	if (res == NULL)
		return NULL;

	if (split_bytes)
	{
		btags_alloc_split_node(res, res_size, split_bytes, true);
		res = move_pointer_node(res, split_bytes + sizeof(t_btags_alloc_node) * 2);
	}

	if (res_size == ba_meta->max_node_size)
		ba_meta->max_node_size = get_max_node_size(ba_meta);
	return (char *)res + sizeof(t_btags_alloc_node);
}

static void check_right_segment(t_btags_alloc_meta *ba_meta, t_btags_alloc_node *node)
{
	t_btags_alloc_node *right_node;

	right_node = move_pointer_node(node, node->size + sizeof(t_btags_alloc_node) * 2);
	if ((char *)right_node + sizeof(t_btags_alloc_node) > (char *)ba_meta->end)
		return;
	if (right_node->is_free == false)
		return;
	node->size += sizeof(t_btags_alloc_node) * 2 + right_node->size;

	right_node = move_pointer_node(
		right_node, right_node->size + sizeof(t_btags_alloc_node)
	);
	right_node->is_free = true;
	right_node->size = node->size;
}

static t_btags_alloc_node *
check_left_segment(t_btags_alloc_meta *ba_meta, t_btags_alloc_node *node)
{
	t_btags_alloc_node *left_node;

	left_node = move_pointer_node(node, -sizeof(t_btags_alloc_node));
	if ((void *)left_node < ba_meta->start)
		return node;
	if (left_node->is_free == false)
		return node;
	node = move_pointer_node(node, node->size + sizeof(t_btags_alloc_node));
	node->size += sizeof(t_btags_alloc_node) * 2 + left_node->size;

	left_node = move_pointer_node(
		left_node, -(left_node->size + sizeof(t_btags_alloc_node))
	);
	left_node->size = node->size;
	return node;
}

void btags_alloc_free(t_btags_alloc_meta *ba_meta, void *ptr)
{
	t_btags_alloc_node *node, *node_end;

	if (ptr < ba_meta->start || ptr > ba_meta->end)
		return;
	node = move_pointer_node(ptr, -sizeof(t_btags_alloc_node));
	node->is_free = true;

	node_end = move_pointer_node(node, node->size + sizeof(t_btags_alloc_node));
	if ((char *)node_end + sizeof(t_btags_alloc_node) > (char *)ba_meta->end)
		return;
	node_end->is_free = true;

	check_right_segment(ba_meta, node);
	if (node->size > ba_meta->max_node_size)
		ba_meta->max_node_size = node->size;

	node = check_left_segment(ba_meta, node);
	if (node->size > ba_meta->max_node_size)
		ba_meta->max_node_size = node->size;
}

static inline bool ptr_in_btags_alloc_single(const t_btags_alloc_meta *ba_meta, void *ptr)
{
	if (ptr >= ba_meta->start && ptr < ba_meta->end)
		return true;
	return false;
}

t_btags_alloc_meta *ptr_in_btags_alloc_list(const t_btags_alloc_meta *ba_meta, void *ptr)
{
	while (ba_meta)
	{
		if (ptr_in_btags_alloc_single(ba_meta, ptr))
			return (t_btags_alloc_meta *)ba_meta;
		ba_meta = ba_meta->next;
	}
	return NULL;
}