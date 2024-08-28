#include "heap.h"

#include <stdlib.h>

/**
 * @brief A structure representing a dynamic heap.
 * 
 * This structure manages a dynamic array of pointers, allowing the addition
 * and retrieval of elements. The array is dynamically resized as new elements 
 * are added.
 */
typedef struct heap {
    /** Generic array of pointers. */
    int32_t **ptr;
    /** How many pointers there are currently in the array. */
    int32_t count;
} heap_t;

/**
 * @brief Initializes a new heap.
 * 
 * This function allocates memory for a new heap structure and initializes 
 * the pointer array to NULL and the count to zero.
 * 
 * @return A pointer to the initialized heap structure.
 */
heap_t *heap_init() {
    // The heap array is initially allocated to hold zero elements.
    heap_t *heap = malloc(sizeof(heap_t));
    heap->ptr = NULL;
    heap->count = 0;
    return heap;
}

/**
 * @brief Adds a new pointer to the heap.
 * 
 * This function resizes the internal pointer array to accommodate the new 
 * element, adds the new pointer to the end of the array, and increments the 
 * element count.
 * 
 * @param heap A pointer to the heap structure.
 * @param ptr The pointer to be added to the heap.
 * @return The index at which the new pointer was added.
 */
int32_t heap_add(heap_t *heap, int32_t *ptr) {
    heap->ptr = realloc(heap->ptr, (heap->count + 1) * sizeof(int32_t *));
    heap->ptr[heap->count] = ptr;
    int32_t temp = heap->count;
    heap->count += 1;
    return temp;
}

/**
 * @brief Retrieves a pointer from the heap by its index.
 * 
 * This function returns the pointer stored at the specified index in the heap.
 * 
 * @param heap A pointer to the heap structure.
 * @param ref The index of the pointer to retrieve.
 * @return The pointer stored at the specified index.
 */
int32_t *heap_get(heap_t *heap, int32_t ref) {
    return heap->ptr[ref];
}

/**
 * @brief Frees the memory allocated for the heap.
 * 
 * This function frees all pointers stored in the heap as well as the internal
 * pointer array and the heap structure itself.
 * 
 * @param heap A pointer to the heap structure to be freed.
 */
void heap_free(heap_t *heap) {
    for (int32_t i = 0; i < heap->count; i++) {
        free(heap->ptr[i]);
    }
    free(heap->ptr);
    free(heap);
}
