#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "mymem.h"
#include <time.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))


/* The main structure for implementing memory allocation.
 * You may change this to fit your implementation.
 */

struct memoryList {
    // doubly-linked list
    struct memoryList *last;
    struct memoryList *next;

    size_t size;            // How many bytes in this block?
    char alloc;          // 1 if this block is allocated,
    // 0 if this block is free.
    void *ptr;           // location of block in memory pool.
};

strategies myStrategy = NotSet;    // Current strategy


size_t mySize;
void *myMemory = NULL;

static struct memoryList *head;
static struct memoryList *next;


/* initmem must be called prior to mymalloc and myfree.

   initmem may be called more than once in a given exeuction;
   when this occurs, all memory you previously malloc'ed  *must* be freed,
   including any existing bookkeeping data.

   strategy must be one of the following:
		- "best" (best-fit)
		- "worst" (worst-fit)
		- "first" (first-fit)
		- "next" (next-fit)
   sz specifies the number of bytes that will be available, in total, for all mymalloc requests.
*/

/**
 * Recursively release the entire linked list
 * @param current
 */
void release_all_rec(struct memoryList *current) {
    if (current->next != head) {
        release_all_rec(current->next);
    }
    free(current);
}

/**
 * Initialise the memoy into one block in the linked list
 * @param strategy
 * @param sz
 */
void initmem(strategies strategy, size_t sz) {
    myStrategy = strategy;

    /* all implementations will need an actual block of memory to use */
    mySize = sz;

    if (myMemory != NULL) {
        free(myMemory); /* in case this is not the first time initmem is called */
        release_all_rec(head);
    }

    myMemory = malloc(sz);

    struct memoryList *new = (struct memoryList *) malloc(sizeof(struct memoryList));
    new->next = new;
    new->last = new;
    new->ptr = myMemory;
    new->size = sz;
    new->alloc = 0;
    head = new;
    next = new;
}

/**
 * Split one block of the linked list into 2, marking the start as allocated. If the allocated size fits exactly will simply mark as alocated
 * and return NULL
 * @param current
 * @param size
 * @return
 */
struct memoryList *split_node(struct memoryList *current, size_t size) {
    current->alloc = 1;
    if (current->size != size) {
        struct memoryList *new = (struct memoryList *) malloc(sizeof(struct memoryList));
        new->size = current->size - size;
        current->size = size;
        new->alloc = 0;

        new->ptr = current->ptr + size;

        current->next->last = new;
        new->next = current->next;
        new->last = current;
        current->next = new;
        return new;
    }
    current->size = size;
    return NULL;
}

/**
 * allocates a block of size_t at the given location and returns the given location
 * @param current
 * @param size
 * @return
 */
struct memoryList *allocate_at(struct memoryList *current, size_t size) {
    split_node(current, size);
    return current;
}

/**
 * Finds the next chunk of free space
 * @param current
 * @param end
 * @param size
 * @return
 */
struct memoryList *get_next_free(struct memoryList *current, struct memoryList *end, size_t size) {
    if (current->alloc == 1 || current->size < size) {
        if (current->next == end) {
            return NULL;
        }
        return get_next_free(current->next, end, size);
    }
    return current;
}

/**
 * Finds the smallest chuck of free space that is larger than size
 * @param size
 * @return
 */
struct memoryList *get_best_free(size_t size) {
    struct memoryList *best = NULL;
    struct memoryList *current = head;
    do {
        if (current->alloc == 0 && current->size >= size && (best == NULL || current->size < best->size)) {
            best = current;
        }
        current = current->next;
    } while (current != head);
    return best;
}

/**
 * Finds the largest chunk of free space
 * @param size
 * @return
 */
struct memoryList *get_worst_free(size_t size) {
    struct memoryList *worst = NULL;
    struct memoryList *current = head;
    do {
        if (current->alloc == 0 && current->size >= size && (worst == NULL || current->size > worst->size)) {
            worst = current;
        }
        current = current->next;
    } while (current != head);
    return worst;
}

/**
 * creates a new block at the location given of size and returns a pointer to it
 * @param current
 * @param requested
 * @return
 */
void *get_ptr_new_block_at(struct memoryList *current, size_t requested) {
    if (current == NULL) return NULL;
    struct memoryList *allocated = allocate_at(current, requested);
    next = allocated->next;
    return allocated->ptr;
}

/* Allocate a block of memory with the requested size.
 *  If the requested block is not available, mymalloc returns NULL.
 *  Otherwise, it returns a pointer to the newly allocated block.
 *  Restriction: requested >= 1 
 */

void *mymalloc(size_t requested) {
    assert((int) myStrategy > 0);

    switch (myStrategy) {
        case NotSet:
            return NULL;
        case First:
            return get_ptr_new_block_at(get_next_free(head, head, requested), requested);
        case Best:
            return get_ptr_new_block_at(get_best_free(requested), requested);
        case Worst:
            return get_ptr_new_block_at(get_worst_free(requested), requested);
        case Next:
            return get_ptr_new_block_at(get_next_free(next, next, requested), requested);
    }
    return NULL;
}

/**
 * Removes a node from the linked list, freeing it and joining the previous and next node
 * @param node
 */
void remove_list_node(struct memoryList *node) {
    struct memoryList *tempNext = node->next;
    struct memoryList *tempLast = node->last;

    tempNext->last = node->last;
    tempLast->next = node->next;

    tempLast->size = tempLast->size + node->size;

    if (next == node) {
        next = tempNext;
    }

    free(node);
}

/**
 * traverses the linked list backwards stopping before reaching a used node or if it finds the head
 * @param current
 * @return
 */
struct memoryList *find_free_start_or_head(struct memoryList *current) {
    if (current->last->alloc == 1 || current == head) {
        return current;
    }
    return find_free_start_or_head(current->last);
}

/**
 * joins the list node after the current one to the current one if it's not allocated
 * @param current
 * @return
 */
struct memoryList *join_after(struct memoryList *current) {
    // join after
    while (current->next->alloc == 0 && current->next != head) {
        remove_list_node(current->next);
    }
    return current;
}

/**
 * Recursively frees a block and cleans the memory around it
 * @param block
 * @param current
 */
void myfree_rec(void *block, struct memoryList *current) {
    if (current->ptr == block) {
        if (current->alloc == 0) {
            fprintf(stderr, "Error, block already freed");
        }
        current->alloc = 0;
        join_after(find_free_start_or_head(current));
    } else {
        if (current->next == head) {
            fprintf(stderr, "Error, attempted to free block that does not exist");
        } else {
            myfree_rec(block, current->next);
        }
    }
}


/* Frees a block of memory previously allocated by mymalloc. */
void myfree(void *block) {
    myfree_rec(block, head);
    return;
}

/****** Memory status/property functions ******
 * Implement these functions.
 * Note that when we refer to "memory" here, we mean the 
 * memory pool this module manages via initmem/mymalloc/myfree. 
 */

int mem_holes_rec(struct memoryList *current) {
    int total = current->next == head ? 0 : mem_holes_rec(current->next);
    return total + (current->alloc == 0 ? 1 : 0);
}

/* Get the number of contiguous areas of free space in memory. */
int mem_holes() {
    return mem_holes_rec(head);
}

size_t mem_allocated_rec(struct memoryList *current) {
    size_t total = current->next == head ? 0 : mem_allocated_rec(current->next);
    return total + (current->alloc == 1 ? current->size : 0);
}

/* Get the number of bytes allocated */
int mem_allocated() {
    return mem_allocated_rec(head);
}

size_t mem_free_rec(struct memoryList *current) {
    size_t total = current->next == head ? 0 : mem_free_rec(current->next);
    return total + (current->alloc == 0 ? current->size : 0);
}

/* Number of non-allocated bytes */
int mem_free() {
    return mem_free_rec(head);
}

size_t mem_largest_free_rec(struct memoryList *current) {
    size_t currentBest = current->next == head ? 0 : mem_largest_free_rec(current->next);
    return current->alloc == 0 ? MAX(currentBest, current->size) : currentBest;
}

/* Number of bytes in the largest contiguous area of unallocated memory */
int mem_largest_free() {
    return mem_largest_free_rec(head);
}

int mem_small_free_rec(int size, struct memoryList *current) {
    return (current->next == head ? 0 : mem_small_free_rec(size, current->next)) +
           (current->alloc == 0 && current->size <= size ? 1 : 0);
}

/* Number of free blocks smaller than "size" bytes. */
int mem_small_free(int size) {
    return mem_small_free_rec(size, head);
}

char mem_is_alloc_rec(void *ptr, struct memoryList *current) {
    if (current->ptr == ptr) {
        return current->alloc;
    } else {
        if (current->next == head) {
            return 0;
        }
        return mem_is_alloc_rec(ptr, current->next);
    }
}

char mem_is_alloc(void *ptr) {
    return mem_is_alloc_rec(ptr, head);
}

/* 
 * Feel free to use these functions, but do not modify them.  
 * The test code uses them, but you may find them useful.
 */


//Returns a pointer to the memory pool.
void *mem_pool() {
    return myMemory;
}

// Returns the total number of bytes in the memory pool. */
int mem_total() {
    return mySize;
}


// Get string name for a strategy. 
char *strategy_name(strategies strategy) {
    switch (strategy) {
        case Best:
            return "best";
        case Worst:
            return "worst";
        case First:
            return "first";
        case Next:
            return "next";
        default:
            return "unknown";
    }
}

// Get strategy from name.
strategies strategyFromString(char *strategy) {
    if (!strcmp(strategy, "best")) {
        return Best;
    } else if (!strcmp(strategy, "worst")) {
        return Worst;
    } else if (!strcmp(strategy, "first")) {
        return First;
    } else if (!strcmp(strategy, "next")) {
        return Next;
    } else {
        return 0;
    }
}


/* 
 * These functions are for you to modify however you see fit.  These will not
 * be used in tests, but you may find them useful for debugging.
 */


void print_memory_rec_f(struct memoryList *current, int index) {
    printf("bytes: %zu, allocation status: %i, index: %i\n", current->size, current->alloc, index);
    if (current->next == head) return;
    print_memory_rec_f(current->next, index + 1);
}

/* Use this function to print out the current contents of memory. */
void print_memory() {
    printf("--memory content--\n");
    print_memory_rec_f(head, 0);
}

/* Use this function to track memory allocation performance.  
 * This function does not depend on your implementation, 
 * but on the functions you wrote above.
 */
void print_memory_status() {
    printf("%d out of %d bytes allocated.\n", mem_allocated(), mem_total());
    printf("%d bytes are free in %d holes; maximum allocatable block is %d bytes.\n", mem_free(), mem_holes(),
           mem_largest_free());
    printf("Average hole size is %f.\n\n", ((float) mem_free()) / mem_holes());
}

/* Use this function to see what happens when your malloc and free
 * implementations are called.  Run "mem -try <args>" to call this function.
 * We have given you a simple example to start.
 */
void try_mymem(int argc, char **argv) {
    strategies strat;
    void *a, *b, *c, *d, *e;
    if (argc > 1)
        strat = strategyFromString(argv[1]);
    else
        strat = First;


    /* A simple example.
       Each algorithm should produce a different layout. */

    initmem(strat, 500);

    a = mymalloc(100);
    b = mymalloc(100);
    c = mymalloc(100);
    myfree(b);
    d = mymalloc(50);
    myfree(a);
    e = mymalloc(25);

    print_memory();
    print_memory_status();

}
