#include "test_utils.h"

int main()
{
	slab_tests();
	print_str_literal("\n");
	btags_alloc_tests();
	print_str_literal("\n");
	klalloc_tests();
	return (0);
}