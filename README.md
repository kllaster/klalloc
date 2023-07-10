# klalloc

## Description
klalloc is a heap memory allocator using sequential fit and slab algorithms.

## Features
+ Using sequential fit and slab algorithms for allocation
+ Calculation and display of memory reserved `show_alloc_mem()`
+ Provides the standard functionality of the `malloc`, `free`, and `realloc` functions

## Usage
1. Cloning this repository
    ```shell
    git clone https://github.com/kllaster/klalloc.git
    ```
2. `cd` into the root directory
    ```shell
    cd klalloc
    ```
3. Creating lib from source code
    ```shell
    make
    ```
4. Take the library from the `bin/` project directory and include the `include/klalloc.h` file in your project

## References
+ [wiki: Slab allocation](https://en.wikipedia.org/wiki/Slab_allocation)