#include "body_alloc.h"

static inline t_body_alloc_node *move_pointer_node(const t_body_alloc_node *node, size_t size)
{
	return (t_body_alloc_node *)((char *)node + size);
}

void body_alloc_setup(t_body_alloc_meta *body_alloc_meta, void *buf, size_t size)
{
	t_body_alloc_node *node;

	body_alloc_meta->next = NULL;
	if (buf == NULL || size == 0)
	{
		body_alloc_meta->start = NULL;
		body_alloc_meta->end = NULL;
		body_alloc_meta->size = 0;
		body_alloc_meta->max_node_size = 0;
		return;
	}
	body_alloc_meta->start = buf;
	body_alloc_meta->end = (char *)buf + size;
	body_alloc_meta->size = size;

	// First meta info
	node = buf;
	node->is_free = true;
	node->size = size - sizeof(t_body_alloc_node) * 2;

	// Last meta info
	node = move_pointer_node(node, sizeof(t_body_alloc_node) + node->size);
	node->is_free = true;
	node->size = size - sizeof(t_body_alloc_node) * 2;

	body_alloc_meta->max_node_size = node->size;
}

static size_t get_max_node_size(const t_body_alloc_meta *body_alloc_meta)
{
	size_t max_node_size = 0;
	t_body_alloc_node *node = body_alloc_meta->start;
	while ((void *)node < body_alloc_meta->end)
	{
		if (node->is_free && node->size > max_node_size)
		{
			max_node_size = node->size;
		}
		node = move_pointer_node(node, node->size + sizeof(t_body_alloc_node) * 2);
	}
	return max_node_size;
}

static void body_alloc_split_node(t_body_alloc_node *node, size_t old_size, size_t new_size, bool first_free)
{
	if (old_size > new_size + MIN_ALLOCTAED_BYTE_MEM + sizeof(t_body_alloc_node) * 2)
	{
		node->size = new_size;
		if (first_free)
			node->is_free = true;
		else
			node->is_free = false;

		node = move_pointer_node(node, new_size + sizeof(t_body_alloc_node));
		node->size = new_size;
		if (first_free)
			node->is_free = true;
		else
			node->is_free = false;

		size_t split_size = old_size - new_size - sizeof(t_body_alloc_node) * 2;
		node = move_pointer_node(node, sizeof(t_body_alloc_node));
		node->size = split_size;
		if (!first_free)
			node->is_free = true;
		else
			node->is_free = false;

		node = move_pointer_node(node, node->size + sizeof(t_body_alloc_node));
		node->size = split_size;
		if (!first_free)
			node->is_free = true;
		else
			node->is_free = false;
	}
}

void *body_alloc(t_body_alloc_meta *body_alloc_meta, size_t size)
{
	t_body_alloc_node *res = NULL;
	size_t res_size = 0;
	t_body_alloc_node *node;

	if (body_alloc_meta->start == NULL)
		return NULL;

	if (size > body_alloc_meta->max_node_size)
		return NULL;

	node = body_alloc_meta->start;
	while ((void *)node < body_alloc_meta->end)
	{
		if (node->is_free && node->size >= size
		    && (res_size == 0 || res_size > node->size))
		{
			res = node;
			res_size = node->size;
		}
		node = move_pointer_node(node, node->size + sizeof(t_body_alloc_node) * 2);
	}

	if (res == NULL)
		return NULL;

	body_alloc_split_node(res, res_size, size, false);

	if (res_size == body_alloc_meta->max_node_size)
		body_alloc_meta->max_node_size = get_max_node_size(body_alloc_meta);
	return (char *)res + sizeof(t_body_alloc_node);
}

void *body_alloc_align(t_body_alloc_meta *body_alloc_meta, size_t size, size_t align_to)
{
	t_body_alloc_node *res = NULL;
	size_t res_size = 0;
	t_body_alloc_node *node;

	if (body_alloc_meta->start == NULL)
		return NULL;

	if (size > body_alloc_meta->max_node_size)
		return NULL;

	size_t split_bytes = 0;
	node = body_alloc_meta->start;
	while ((void *)node < body_alloc_meta->end)
	{
		if (node->is_free && node->size >= size
		    && (res_size == 0 || res_size > node->size))
		{
			char *check_ptr = (char *)align((size_t)node + sizeof(t_body_alloc_node), align_to);

			char *end_mem = (char *)node + sizeof(t_body_alloc_node) + node->size;
			if (end_mem > check_ptr
			    && (size_t)(end_mem - check_ptr) >= size)
			{
				check_ptr -= sizeof(t_body_alloc_node) * 2;
				if ((char *)check_ptr > (char *)node + sizeof(t_body_alloc_node))
				{
					res = node;
					res_size = node->size;
					split_bytes = (size_t)check_ptr - (size_t)node - sizeof(t_body_alloc_node);
				}
			}
			else
			{
				res = node;
				res_size = node->size;
				split_bytes = 0;
			}
		}
		node = move_pointer_node(node, node->size + sizeof(t_body_alloc_node) * 2);
	}

	if (res == NULL)
		return NULL;

	if (split_bytes)
	{
		body_alloc_split_node(res, res_size, split_bytes, true);
		res = move_pointer_node(res, split_bytes + sizeof(t_body_alloc_node) * 2);
	}

	if (res_size == body_alloc_meta->max_node_size)
		body_alloc_meta->max_node_size = get_max_node_size(body_alloc_meta);
	return (char *)res + sizeof(t_body_alloc_node);
}

static void check_right_segment(t_body_alloc_meta *body_alloc_meta, t_body_alloc_node *node)
{
	t_body_alloc_node *right_node;

	right_node = move_pointer_node(node, node->size + sizeof(t_body_alloc_node) * 2);
	if ((char *)right_node + sizeof(t_body_alloc_node) > (char *)body_alloc_meta->end)
		return;
	if (right_node->is_free == false)
		return;
	node->size += sizeof(t_body_alloc_node) * 2 + right_node->size;

	right_node = move_pointer_node(
		right_node, right_node->size + sizeof(t_body_alloc_node)
	);
	right_node->is_free = true;
	right_node->size = node->size;
}

static t_body_alloc_node *
check_left_segment(t_body_alloc_meta *body_alloc_meta, t_body_alloc_node *node)
{
	t_body_alloc_node *left_node;

	left_node = move_pointer_node(node, -sizeof(t_body_alloc_node));
	if ((void *)left_node < body_alloc_meta->start)
		return node;
	if (left_node->is_free == false)
		return node;
	node = move_pointer_node(node, node->size + sizeof(t_body_alloc_node));
	node->size += sizeof(t_body_alloc_node) * 2 + left_node->size;

	left_node = move_pointer_node(
		left_node, -(left_node->size + sizeof(t_body_alloc_node))
	);
	left_node->size = node->size;
	return node;
}

void body_alloc_free(t_body_alloc_meta *body_alloc_meta, void *ptr)
{
	t_body_alloc_node *node, *node_end;

	if (ptr < body_alloc_meta->start || ptr > body_alloc_meta->end)
		return;
	node = move_pointer_node(ptr, -sizeof(t_body_alloc_node));
	node->is_free = true;

	node_end = move_pointer_node(node, node->size + sizeof(t_body_alloc_node));
	if ((char *)node_end + sizeof(t_body_alloc_node) > (char *)body_alloc_meta->end)
		return;
	node_end->is_free = true;

	check_right_segment(body_alloc_meta, node);
	if (node->size > body_alloc_meta->max_node_size)
		body_alloc_meta->max_node_size = node->size;

	node = check_left_segment(body_alloc_meta, node);
	if (node->size > body_alloc_meta->max_node_size)
		body_alloc_meta->max_node_size = node->size;
}

static inline bool ptr_in_body_alloc_single(const t_body_alloc_meta *body_alloc_meta, void *ptr)
{
	if (ptr >= body_alloc_meta->start && ptr < body_alloc_meta->end)
		return true;
	return false;
}

t_body_alloc_meta *ptr_in_body_alloc_list(const t_body_alloc_meta *body_alloc_meta, void *ptr)
{
	while (body_alloc_meta)
	{
		if (ptr_in_body_alloc_single(body_alloc_meta, ptr))
			return (t_body_alloc_meta *)body_alloc_meta;
		body_alloc_meta = body_alloc_meta->next;
	}
	return NULL;
}