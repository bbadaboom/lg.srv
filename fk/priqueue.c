#include "PQ.h"
#include <stdlib.h>
#include <stdio.h>

PQueue *CreatePQueue(

	PQueueCmpFunc cmp,
	int ival_offset,
	int initial_size )
{
	PQueue *r = (PQueue*)malloc( sizeof(PQueue)) ;

	r -> heap = malloc( (1+initial_size) * sizeof( void * ) ) ;
	/*	Heap - Implementierungen sind mit Base-1-Arrays einfacher */

	r -> count = 0 ;
	r -> size = initial_size ;
	r -> cmp = cmp ;
	r -> offs = ival_offset ;

	return r ;
}

void DestroyPQueue( PQueue *P )
{
	free( P->heap ) ;
	free(P) ;
}

static void * ResizePQueue( PQueue *that , int size )
{
	if ( ( size < that->size && size > that->count ) 
		|| size > that->size  )
	{
		void **new_heap ;
		new_heap = realloc( that->heap , (1+size)*sizeof(void*) ) ;
		if ( new_heap )
		{
			that->size = size ;
			that->heap = new_heap ;
		}
		else
		{
			fprintf(stderr,"ResizePQueue %p failed , old=%d , new=%d  // %s,%d\n",
				(void*)that , that->size , size, __FILE__,__LINE__ ) ;
			return 0 ;
		}
	}
	return that ;
}

void InsertPQueue( PQueue *that , void *entry )
{
	int i,j ; 
	void *hlp , **n ;
	if ( that->count >= that->size )
		if ( !ResizePQueue( that , (int)(that->size * 1.618034 + 1 )) )
			if ( !ResizePQueue( that , (int)(that->size *1.1 + 3) ) )
				return ;
	/*
		Das heisst : Die Größe wird skaliert , die Speicherverschwendung 
		betraegt maximal 60 Prozent .
		Wenn der Platz knapp sein sollte , versucht er 	eine geringe Erweiterung.
	*/

	n = that->heap ;
	i = ++that->count ;
	n[i] = entry ;
	for (;;)
	{
		if ( i == 1 )
			break ;
		j = i/2 ;
		if ( that->cmp( n[j] , n[i] ) <= 0 )
			break ;
		/* if ( that->ref_cb ) that->ref_cb( n[i] , i ) ; */
		hlp = n[j] ;
		n[j] = n[i] ;
		n[i] = hlp ;
		i = j ;
	}
	for ( j = that->count ; j >= i ; j /= 2 )
		*((int*)((char*)n[j] + that->offs)) = j;
}


void * ExtractPQueueAt( PQueue *that , int i )
{
	int c , lo ;
	void *hlp ;
	void **n , *result ;

	if ( that->count <= 0 || that->count < i || i <= 0 )
		return 0 ;
	/* printf("Xtr at %d %d\n" , i , that->count ) ; */
	n = that->heap ;
	lo = i ;
	result = n[i]  ;
	n[i] = n[ that->count-- ] ;
	for (;;)
	{
		c = 2*i ;
		if ( c > that->count )
			break ;

		if ( c+1 <= that->count && that->cmp( n[c+1] , n[c] ) < 0 )
			c++ ;
		if ( that->cmp( n[i] , n[c] ) <= 0  )
			break ;
		hlp = n[c] ;
		n[c] = n[i] ;
		n[i] = hlp ;
		i = c ;
	}

	for ( i = c/2 ; i >= lo ; i /= 2 )
		*((int*)((char*)n[i] + that->offs)) = i;
	*((int*)((char*)result + that->offs)) = 0;

	i = lo ;
	for (;;)
	{
		if ( i == 1 )
			break ;
		c = i/2 ;
		if ( that->cmp( n[c] , n[i] ) <= 0 )
			break ;
		hlp = n[c] ;
		n[c] = n[i] ;
		n[i] = hlp ;
		i = c ;
	}
	for ( c = lo ; c >= i ; c /= 2 )
		*((int*)((char*)n[c] + that->offs)) = c;

	return result ;
}

void InvarPQueue( PQueue *that )
{
	int i = 1 ;
	for(;;)
	{
		if ( 2*i <= that->count )
		{
			/* printf("%d %ld %ld\n" , i , *(long*)(that->heap[i]) , 
							*(long*)(that->heap[2*i] ) ) ; */
			if ( that->cmp( that->heap[i] , that->heap[2*i] ) > 0 )
			{
				fprintf(stderr,"Invariant Failure\n");
				abort() ;
			}
		}
		else
			break ;
		if ( 2*i+1 <= that->count )
		{
			/* printf("%d %ld %ld\n" , i , *(long*)(that->heap[i]) , 
							*(long*)(that->heap[2*i+1] ) ) ; */
			if ( that->cmp( that->heap[i] , that->heap[2*i+1] ) > 0 )
			{
				fprintf(stderr,"Invariant Failure\n");
				abort() ;
			}
		}
		i++ ;
	}
}
