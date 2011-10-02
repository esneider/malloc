/**
 * @file malloc_test_2.c
 *
 * @author Dario Sneidermanis
 */

#include <stdio.h>
#include "malloc.h"


#define SIZE (1024*1024*32)


char mem1[ SIZE ], mem2[ SIZE/2 ], *ptr1, *ptr2, *ptr3;


int main ( void ) {

    init_malloc( mem1, SIZE );

    printf( "chk1 = %p\n", check_malloc() );

    add_malloc_buffer( mem2, SIZE/2 );

    printf( "chk2 = %p\n", check_malloc() );

    ptr1 = malloc( SIZE/2 );

    printf( "ptr1 = %p\n", ptr1 );
    printf( "chk3 = %p\n", check_malloc() );

    free( ptr1 );
    printf( "chk4 = %p\n", check_malloc() );

    ptr1 = malloc( 3 * SIZE/4 );

    printf( "ptr1 = %p\n", ptr1 );
    printf( "chk5 = %p\n", check_malloc() );

    ptr2 = malloc( SIZE / 5 );

    printf( "ptr2 = %p\n", ptr1 );
    printf( "chk6 = %p\n", check_malloc() );

    ptr3 = malloc( SIZE / 5 );

    printf( "ptr3 = %p\n", ptr1 );
    printf( "chk7 = %p\n", check_malloc() );

    free( ptr1 );

    printf( "chk8 = %p\n", check_malloc() );

    ptr1 = malloc( SIZE / 5 );

    printf( "ptr1 = %p\n", ptr1 );
    printf( "chk9 = %p\n", check_malloc() );

    free( ptr3 );

    printf( "chk10 = %p\n", check_malloc() );

    free( ptr1 );

    printf( "chk11 = %p\n", check_malloc() );

    free( ptr2 );

    printf( "chk12 = %p\n", check_malloc() );

    return 0;
}

