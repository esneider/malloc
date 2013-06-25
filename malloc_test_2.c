/**
 * malloc_test_2.h - C memory allocator
 *
 * Written in 2011 by Dario Sneidermanis (dariosn@gmail.com)
 *
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with this software. If not, see
 * http://creativecommons.org/publicdomain/zero/1.0/
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

