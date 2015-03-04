#ifndef _PQ_H
#define _PQ_H

#ifdef __cplusplus
extern	"C"	{
#endif


#define	Bool			int
#define	True			1
#define	False			0

typedef struct _PQueue
{
	int count ;
	int	size ;
	int	(*cmp)( const void * , const void * ) ;	
	int offs;
	void **heap ;
} PQueue ;

typedef int	(*PQueueCmpFunc)( const void *, const void * );

extern	PQueue *CreatePQueue(
	PQueueCmpFunc cmp,
	int ival_offset,
	int initial_size );

extern void DestroyPQueue( PQueue *P );

extern void InsertPQueue( PQueue *that , void *entry );

extern void InvarPQueue( PQueue *that );
/* TEST - auf Konsitenz */

extern void * ExtractPQueueAt( PQueue *that , int i );
/*	Achtung :
	Fuer i > 1 ist das extrahierte Element im allgemeinen nicht minimal .
*/

/* extern inline void * PriorityQueueExtractMin( PriorityQueue *that ) ; */
#define ExtractPQFirst(that) ExtractPQueueAt( (that) , 1 ) 

/* inline extern void * PriorityQueuePeekMin( PriorityQueue *that ) ; */
#define PeekPQFirst(that) \
	(( (that)->count <= 0 ) ? (void*)0 : (that)->heap[1] )

/* extern inline int PriorityQueueCount( PriorityQueue *that ) ; */
#define GetPQueueCount(that) ( (that)->count )

#ifdef __cplusplus
}
#endif

#endif /* VIS_FUNC_H */
