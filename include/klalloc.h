#ifndef KLALLOC_H
# define KLALLOC_H

# include <unistd.h>

void klfree(void *ptr);
void *klalloc(size_t size);
void *klrealloc(void *ptr, size_t size);

void show_alloc_mem();

#endif