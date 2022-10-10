#include "test_utils.h"

int main()
{
	slab_tests();
	print_str_literal("\n");
	body_alloc_tests();
	return (0);
}