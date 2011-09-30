/**
 * @file malloc.c
 *
 * @author Dario Sneidermanis
 *
 * TODO: split, malloc, free
 *       calloc, realloc, check_memory_corruption
 *       use a fenwick tree to optimize find_bin to log n
 *       use a trie/balanced tree in big enough bins to optimize find_chunk to log n
 */

#include "malloc.h"
#include <assert.h>


/*
 * The main algorithm idea was taken from here:
 *
 * http://gee.cs.oswego.edu/dl/html/malloc.html
 *
 *
 * The idea is to have chunks of memory delimited by headers and footers.
 * An allocated memory chunk looks like this:
 *
 *  _____________________
 * | STATUS (inuse)      |
 * |_____________________|
 * | SIZE (in bytes)     |
 * |_____________________|
 * |        DATA         |
 * |        ...          |
 * |_____________________|
 * | SIZE (in bytes)     |
 * |_____________________|
 *
 * And a free chunk looks like this:
 *
 *  ______________________
 * | STATUS (free)        |
 * |______________________|
 * | SIZE (in bytes)      |
 * |______________________|
 * | PREVIOUS FREE CHUNCK |
 * |______________________|
 * | NEXT FREE CHUNK      |
 * |______________________|
 * |      GARBAGE         |
 * |        ...           |
 * |______________________|
 * | SIZE (in bytes)      |
 * |______________________|
 *
 * Besides, we have an array of "bins", each with a list of free chunks of
 * a specific size (really a range of sizes)
 *
 */


/*
 * Bin sizes from 16 bytes to 2 GB
 *
 * Note: bin_sizes[0] is never used, since a free_header must fit in a free
 *       chunk
 */

#define BIN_NUMBER  ( sizeof( bin_sizes ) / sizeof( bin_sizes[0] ) )

static const size_t bin_sizes[] = {

	     8,    16,    24,    32,    40,    48,    56,    64,    72,    80,
	    88,    96,   104,   112,   120,   128,   136,   144,   152,   160,
	   168,   176,   184,   192,   200,   208,   216,   224,   232,   240,
	   248,   256,   264,   272,   280,   288,   296,   304,   312,   320,
	   328,   336,   344,   352,   360,   368,   376,   384,   392,   400,
	   408,   416,   424,   432,   440,   448,   456,   464,   472,   480,
	   488,   496,   504,   512,   576,   640,   768,  1024,  2048,  4096,
	     0x2000,     0x4000,     0x8000,    0x10000,    0x20000,    0x40000,
	    0x80000,   0x100000,   0x200000,   0x400000,   0x800000,  0x1000000,
	  0x2000000,  0x4000000,  0x8000000, 0x10000000, 0x20000000, 0x40000000,
	 0x80000000
};


struct free_header {

	unsigned int status : 1;
	size_t size : 31;

	struct free_header* prev_pos;
	struct free_header* next_pos;
};


struct inuse_header {

	unsigned int status : 1;
	size_t size : 31;
};


struct footer {

	size_t size : 32;
};


#define FREE_STATUS  0
#define INUSE_STATUS 1


struct memory_context {

    size_t memory_size;
    size_t free_memory;
    size_t last_chunk_size;

    struct free_header* last_chunk;
    struct free_header  bins[ BIN_NUMBER ];
};


/**
 * Global current memory context
 */
static struct memory_context* context;


/**
 * Performs a binary search to find the first bin of size >= to a given size
 *
 * @param size  the size of the memory (in bytes)
 *
 * @return the bin index
 */
static size_t find_bin ( size_t size ) {

    size_t min_bin = 0, max_bin = BIN_NUMBER, curr_bin;

    assert( size < bin_sizes[ BIN_NUMBER - 1 ] );

    while ( min_bin + 1 != max_bin ) {

        curr_bin = ( min_bin + max_bin ) / 2;

        if ( bin_sizes[ curr_bin ] <= size )

            min_bin = curr_bin;
        else
            max_bin = curr_bin;
    }

    return min_bin;
}


/**
 * Finds the first chunk of memory >= to a given size in a given bin
 *
 * @param bin   the bin to explore
 * @param size  the size of memory (in bytes)
 *
 * @return pointer to the chunk's free header
 */
inline static struct free_header* find_chunk ( size_t bin, size_t size ) {

    struct free_header* chunk = context->bins + bin;

    do {

        chunk = chunk->next;

    } while ( chunk != context->bins + bin && chunk->size < size );

    return chunk;
}


/**
 * Finds the first chunk of memory > to a given size in a given bin
 *
 * Note the use of > instead of >=, like in find_chunk. This version
 * is used to implement a tie breaking least-recently-used strategy,
 * which mantains (for some reason) low memory fragmentation
 *
 * @param bin   the bin to explore
 * @param size  the size of memory (in bytes)
 *
 * @return pointer to the chunk's free header
 */
inline static struct free_header* find_upper_chunk ( size_t bin, size_t size ) {

    struct free_header* chunk = context->bins + bin;

    do {

        chunk = chunk->next;

    } while ( chunk != context->bins + bin && chunk->size <= size );

    return chunk;
}


/**
 * Adds a chunk of memory to the bins
 *
 * @param memory  free memory buffer
 * @param size    free memory buffer size (in bytes)
 */
static void add_free_chunk ( void* memory, size_t size ) {

    struct free_header* header;
    struct footer*      footer;

    assert( size >= sizeof( struct free_header ) + sizeof( struct footer ) );

    header = memory;

    header->status = FREE_STATUS;
    header->size   = size;

    header->next_pos = find_upper_chunk( find_bin( memory, size ), size );
    header->prev_pos = header->next_pos->prev_pos;

    header->next_pos->prev_pos = header;
    header->prev_pos->next_pos = header;

    footer = (void*)( (char*)memory + size - sizeof( struct footer ) );

    footer->size = size;
}


/**
 * Adds a new memory area for allocations to the current memory context
 *
 * @param memory  memory buffer
 * @param size    memory buffer size (in bytes)
 */
void add_malloc_buffer ( void* memory, size_t size ) {


    struct bound {

        struct inuse_header header;
        struct footer       footer;

    } *bound;

    if ( size < sizeof( struct bound ) * 2 + sizeof( struct free_header ) +
                sizeof( struct footer ) )
    {
        return;
    }

    bound  = memory;

    bound->header.status = INUSE_STATUS;
    bound->header.size   = sizeof( struct bound );
    bound->footer.size   = sizeof( struct bound );

    bound = (void*)( (char*)memory + size - sizeof( struct bound ) );

    bound->header.status = INUSE_STATUS;
    bound->header.size   = sizeof( struct bound );
    bound->footer.size   = sizeof( struct bound );

    memory += sizeof( struct bound );
    size   -= sizeof( struct bound ) * 2;

    add_free_chunk( memory, size );
}


/**
 * Creates a new malloc context in the given memory buffer. Uses the remaining
 * memory for allocations
 *
 * Must be called before any malloc or free (unless a memory context has been
 * set manually)
 *
 * @param memory  memory buffer
 * @param size    memory buffer size (in bytes)
 */
void init_malloc ( void* memory, size_t size ) {

    assert( size >= sizeof( struct memory_context ) );

    context = (struct memory_context*)memory;
    memory += sizeof( struct memory_context );
    size   -= sizeof( struct memory_context );


    context->free_memory = context->memory_size = context->last_chunk_size = 0;

    for ( struct free_header* bin = context->bins; bin < memory; bin++ ) {

        bin->size     = sizeof( struct free_header );
        bin->next_pos = bin;
        bin->prev_pos = bin;
    }

    add_malloc_buffer( memory, size );
}


