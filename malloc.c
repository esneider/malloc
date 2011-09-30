/**
 * @file malloc.c
 *
 * @author Dario Sneidermanis
 *
 * TODO: calloc, realloc, check_memory_corruption
 *       use a fenwick tree to optimize find_bin to log n
 *       use a trie/balanced tree in big enough bins to optimize find_chunk to log n
 */

#include "malloc.h"


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


#define BIN_NUMBER  ( sizeof( bin_sizes ) / sizeof( bin_sizes[0] ) )


#define FREE_STATUS  0
#define INUSE_STATUS 1


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
 * @return the bin position
 */
inline static size_t find_bin ( size_t size ) {

    assert( size < bin_sizes[ BIN_NUMBER - 1 ] );

    size_t min_bin = 0, max_bin = BIN_NUMBER, curr_bin;

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
 * Finds the first chunk if memory >= to a given size in a given bin
 *
 * @param bin   the bin to explore
 * @param size  the size of memory (in bytes)
 *
 * @return pointer to the chunk's fere header
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
 * @param size    memory buffer size (in bytes)
 */
inline static void add_free_chunk ( void* memory, size_t size ) {

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
 * Adds a new memory buffer
 *
 * @param memory  memory buffer
 * @param size    memory buffer size (in bytes)
 */
void add_malloc_buffer ( void* memory, size_t size ) {


    struct bound {

        struct inuse_header header;
        struct footer       footer;

    } *bound;

    assert( size >= 2 * sizeof( struct bound ) + sizeof( struct free_header ) +
                        sizeof( struct footer ) );

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
 * Initializes a new malloc context with the given memory buffer
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


// TODO !!!


/**
 * Splits a free chunk of memory in two chunks, the first of a given size, and
 * the second goes back to the set of free chunks
 *
 * @param chunk  the starting position of the chunk to split
 * @param size   the size of the desired chunk
 *
 * @return  a pointer to the first chunk (wich is setted "inuse")
 *
 */
static void* split ( size_t chunk, size_t size ) {

	size_t left_size = GET_FREE_HEADER( chunk )->size - size;

	if( left_size < FREE_HEADER_SIZE + FOOTER_SIZE ) {

		size += left_size;
		left_size = 0;
	}

	/* set headers and footers */
	struct inuse_header* first_header = GET_INUSE_HEADER( chunk );

	first_header->status = INUSE_STATUS;
	first_header->size = size;

	GET_FOOTER( chunk + size - FOOTER_SIZE )->size = size;

	if ( left_size > 0 ) {

		struct free_header* second_header = GET_FREE_HEADER( chunk + size );

		second_header->status = FREE_STATUS;
		second_header->size   = left_size;

		GET_FOOTER( chunk + size + left_size - FOOTER_SIZE )->size = left_size;

		/* set free */
		size_t upper_chunk = find_upper_chunk( find_bin( left_size ), left_size );

		second_header->prev_pos = GET_FREE_HEADER( upper_chunk )->prev_pos;
		second_header->next_pos = upper_chunk;

		GET_FREE_HEADER( second_header->prev_pos )->next_pos = chunk + size;
		GET_FREE_HEADER( second_header->next_pos )->prev_pos = chunk + size;

		last_chunk      = chunk + size;
		last_chunk_size = left_size;

	} else {

		last_chunk_size = 0;
	}

	free_memory -= size;

	return (char*)(first_header + 1);
}


/**
 * Allocs a chunk of memory of a given size.
 *
 * @param size  thesize trying to be allocated (in bytes)
 *
 * @return a pointer to the allocated memory, or NULL if an error ocurred
 *
 * For more info on the algorithm idea:
 * @see http://gee.cs.oswego.edu/dl/html/malloc.html
 *
 */
void* malloc ( size_t size ) {

	size += INUSE_HEADER_SIZE + FOOTER_SIZE;

	if ( size < FREE_HEADER_SIZE + FOOTER_SIZE )
		size = FREE_HEADER_SIZE + FOOTER_SIZE;

	if ( size > free_memory )
		return NULL;

	/* find the bin to use */

	size_t bin = find_bin(size);

	if ( bin == 0 )
		return NULL;

	while ( BIN_ISEMPTY( bin ) )
		if ( ++bin >= BIN_NUMBER )
			return NULL;

	/* find the chunk to use */

	size_t chunk = find_chunk( bin, size );

	if ( chunk == BIN_ABSOLUTE_POS( bin ) ) {

		do {
			if ( ++bin >= BIN_NUMBER )
				return NULL;
		} while ( BIN_ISEMPTY( bin ) );

		chunk = BINS[bin].next_pos;
	}
	/* heuristic to improve locallity */

	if ( GET_FREE_HEADER( chunk )->size > size && last_chunk_size >= size ) {

		chunk = last_chunk;
	}

	struct free_header *header = GET_FREE_HEADER( chunk );

	/* maintain linked list */
	GET_FREE_HEADER( header->prev_pos )->next_pos = header->next_pos;
	GET_FREE_HEADER( header->next_pos )->prev_pos = header->prev_pos;

	return split( chunk, size );
}


/**
 * Frees the previously allocated space.
 *
 * @param data  a pointer to the memory to be freed
 *
 */
void free ( void* data ) {

	struct free_header* header = NULL;

	if ( data == NULL )
		return;

	size_t pos = ((char*)data) - memory - INUSE_HEADER_SIZE;

	assert( pos >= MEMORY_START && pos < MEMORY_END );
	assert( pos + GET_INUSE_HEADER(pos)->size <= MEMORY_END );
	assert( GET_INUSE_HEADER(pos)->size ==
			GET_FOOTER( pos + GET_INUSE_HEADER(pos)->size - FOOTER_SIZE )->size );
	assert( GET_INUSE_HEADER(pos)->status == INUSE_STATUS );

	free_memory += GET_INUSE_HEADER( pos )->size;

	/* try to join with previous chunk */
	if ( pos > MEMORY_START ) {

		assert( pos - GET_FOOTER( pos - FOOTER_SIZE )->size >= MEMORY_START );
		assert( pos - GET_FOOTER( pos - FOOTER_SIZE )->size < MEMORY_END );

		header = GET_FREE_HEADER( pos - GET_FOOTER( pos - FOOTER_SIZE )->size );

		assert( header->size == GET_FOOTER( pos - FOOTER_SIZE )->size );

		if ( header->status == FREE_STATUS ) {

			GET_FREE_HEADER( header->prev_pos )->next_pos = header->next_pos;
			GET_FREE_HEADER( header->next_pos )->prev_pos = header->prev_pos;

			int size      = GET_INUSE_HEADER( pos )->size;
			pos          -= header->size;
			header->size += size;

		} else {

			header = NULL;
		}
	}

	if ( header == NULL ) {

		header         = GET_FREE_HEADER( pos );
		header->status = FREE_STATUS;
	}

	/* try to join with next chunk */
	if ( pos + header->size < MEMORY_END ) {

		struct free_header* next_header = GET_FREE_HEADER( pos + header->size );

		assert( pos + header->size + next_header->size <= MEMORY_END );
		assert( next_header->size ==
			GET_FOOTER( pos + header->size + next_header->size - FOOTER_SIZE )->size );

		if ( next_header->status == FREE_STATUS ) {

			if ( last_chunk == pos + header->size ) {

				last_chunk_size = 0;
            }

			GET_FREE_HEADER( next_header->prev_pos )->next_pos = next_header->next_pos;
			GET_FREE_HEADER( next_header->next_pos )->prev_pos = next_header->prev_pos;

			header->size += next_header->size;
		}
	}

	/* set footer */
	GET_FOOTER( pos + header->size - FOOTER_SIZE )->size = header->size;

	/* set free */
	size_t chunk = find_upper_chunk( find_bin( header->size ), header->size );

	header->prev_pos = GET_FREE_HEADER( chunk )->prev_pos;
	header->next_pos = chunk;

	GET_FREE_HEADER( header->prev_pos )->next_pos = pos;
	GET_FREE_HEADER( header->next_pos )->prev_pos = pos;
}


#include <stdio.h>

void print_memory_data () {

	printf( "\n" );
	printf( "BIN_NUMBER: %zu\n", BIN_NUMBER );
	printf( "BIN_TABLE_SIZE: %zu\n", BIN_TABLE_SIZE );
	printf( "BIN_TABLE_START: %d\n", BIN_TABLE_START );
	printf( "BIN_TABLE_END: %zu\n", BIN_TABLE_END );
	printf( "MEMORY_START: %zu\n", MEMORY_START );
	printf( "MEMORY_END: %zu\n", MEMORY_END );
	printf( "FREE_HEADER_SIZE: %zu\n", FREE_HEADER_SIZE );
	printf( "INUSE_HEADER_SIZE: %zu\n", INUSE_HEADER_SIZE );
	printf( "FOOTER_SIZE: %zu\n", FOOTER_SIZE );
	printf( "free_memory: %zu\n", free_memory );
	printf( "\n" );
	for ( size_t i = 0; i < BIN_NUMBER; i++ )
		printf( "%d|", !BIN_ISEMPTY(i) );
	printf( "\n\n" );
}

