/**
 * @file malloc_test_2.c
 *
 * @author Dario Sneidermanis
 */

#include <stdio.h>
#include <assert.h>
#include "malloc.h"


#define SIZE (1024*1024*32)


char mem1[ SIZE ], mem2[ SIZE/2 ], *ptr1, *ptr2, *ptr3;


int main ( void ) {

    init_malloc( mem1, SIZE );
    assert( !check_malloc() );

    add_malloc_buffer( mem2, SIZE/2 );
    assert( !check_malloc() );

    assert( ptr1 = malloc( SIZE/2 ) );
    assert( !check_malloc() );

    free( ptr1 );
    assert( !check_malloc() );

    assert( ptr1 = malloc( 3 * SIZE/4 ) );
    assert( !check_malloc() );

    assert( ptr2 = malloc( SIZE / 5 ) );
    assert( !check_malloc() );

    assert( ptr3 = malloc( SIZE / 5 ) );
    assert( !check_malloc() );

    free( ptr1 );
    assert( !check_malloc() );

    assert( ptr1 = malloc( SIZE / 5 ) );
    assert( !check_malloc() );

    free( ptr3 );
    assert( !check_malloc() );

    free( ptr1 );
    assert( !check_malloc() );

    free( ptr2 );
    assert( !check_malloc() );

    printf( "SUCCESSFUL RUN!\n" );
    return 0;
}

