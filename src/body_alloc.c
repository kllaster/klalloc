#include "body_alloc.h"
#include <printf.h>

static t_body_alloc_node *move_pointer_node(t_body_alloc_node *node, size_t size)
{
	return (t_body_alloc_node *)((char *)node + size);
}

void body_alloc_setup(t_body_alloc_meta *body_alloc_meta, void *buf, size_t size)
{
	t_body_alloc_node *node;

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

static size_t get_max_node_size(const t_body_alloc_meta* body_alloc_meta)
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

void *body_alloc(t_body_alloc_meta *body_alloc_meta, size_t size)
{
	void *res = NULL;
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

	if (!res)
	{
		return NULL;
	}

	node = res;
	node->is_free = false;
	if (res_size > size + MIN_ALLOCTAED_BYTE_MEM + sizeof(t_body_alloc_node) * 2)
	{
		node->size = size;

		node = move_pointer_node(node, size + sizeof(t_body_alloc_node));
		node->size = size;
		node->is_free = false;

		node = move_pointer_node(node, sizeof(t_body_alloc_node));
		node->size = res_size - size - sizeof(t_body_alloc_node) * 2;
		node->is_free = true;

		node = move_pointer_node(node, node->size + sizeof(t_body_alloc_node));
		node->size = res_size - size - sizeof(t_body_alloc_node) * 2;
		node->is_free = true;
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

static t_body_alloc_node *check_left_segment(t_body_alloc_meta *body_alloc_meta, t_body_alloc_node *node)
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

	node = move_pointer_node(ptr, -sizeof(t_body_alloc_node));
	if ((void *)node < body_alloc_meta->start || ptr > body_alloc_meta->end)
		return;
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