#include <stdio.h>
#include <errno.h>
#include "NetDrv.h"

#define DEBUGLOOK		/* also set dede ! */

static	int	_fill_cache( SkLine *l, int num )
{
	fd_set	Mask;
	int				max = l->in->size - l->in->fill;
	struct timeval	tv = { 0, 5000 };
	int				x;
	int				stopper = 15;		/* dont hang me up */

	if ( max < num )
	{
		if ( l->in->ptr )
		{
			memmove(l->in->cache,
					l->in->cache+l->in->ptr,
					l->in->fill - l->in->ptr);
			l->in->fill -= l->in->ptr;
			l->in->ptr = 0;
		}
		max = l->in->size - l->in->fill;
		while ( max < num )
		{	/* cache-size-to-small */
#if 1
			fprintf(stderr,"Cache-size-Warning\n");
			fprintf(stderr,"  Skit : %s, %d\n",__FILE__,__LINE__);
			fprintf(stderr,"  cache size now %d bytes !!!\n",
						l->in->size + SK_CACHE_SIZE);
#endif
			l->in->cache = realloc(l->in->cache,l->in->size+SK_CACHE_SIZE);
			l->in->size += SK_CACHE_SIZE;
			max += SK_CACHE_SIZE;
		}
	}

	if ( l->fd == -1 )
		return( -1 );

	while(( num > 0 ) && ( stopper-- ))
	{
#ifdef _WINDOWS
		errno = 0;
		x = recv( l->fd, l->in->cache + l->in->fill, max, 0 );

		if ( x == -1 )
			errno = WSAGetLastError();

		if ((x == -1) && ( errno == WSAEWOULDBLOCK ))
		{
			return(0);
/*** virt-flag make a possible blind-date ***/
			FD_ZERO( &Mask );
			FD_SET( l->fd, &Mask );
			select( l->fd+1, &Mask, 0, 0, &tv );
			continue;
		}
#else
		x = read( l->fd, l->in->cache + l->in->fill, max );

		if (( x == -1 ) && ( errno == EAGAIN ))
		{
			if ( num <= 0 )
				return(0);
/*** virt-flag make a possible blind-date ***/
			FD_ZERO( &Mask );
			FD_SET( l->fd, &Mask );
			select( l->fd + 1, &Mask, 0, 0, &tv );
			continue;
		}
		if (( x == -1 ) && ( errno == EINTR ))
		{
			continue;
		}
#endif
		if (( x <= 0 ) || SkTerminate || ( l->fd == -1 ))
		{
			/* read-error */
			close( l->fd );
			l->fd = -1;
			return( -1 );
		}
		max -= x;
		num -= x;
		l->in->fill += x;
	}

	return( 1 );
}

/* push all other packets into a separate queue */
static void	_LookPackets( SkLine * l )
{
	unsigned char	*p;
	int				has;
	SkPacket		pck;

	if (( l->fd == -1 ) || SkTerminate )
		return;

	p = (unsigned char*)l->in->cache + l->in->ptr;
	has = l->in->fill - l->in->ptr;

	if ( !has )
		return;

	pck.size = 0;
	pck.len = has;
	pck.data = (char*)p;
	pck.ptype = 0;

	VskDoHandler( l, SK_H_PACKET, &pck );

	if (( l->fd == -1 ) || SkTerminate )
		return;

	l->in->ptr = 0;
	l->in->fill = 0;
}

void _HReadable( SkLine *l, int pt, void *own, void *sys )
{
	int		x;

	if ( !l->in )
	{
		l->in = (SkCache*)malloc(sizeof(SkCache));
		l->in->ptr = 0;
		l->in->fill = 0;
		l->in->size = SK_CACHE_SIZE;
		l->in->cache = (char*)malloc( SK_CACHE_SIZE );
	}

	x = _fill_cache( l, 1 );

	if ( x == -1 )
	{
		_VskDelLine( l );
		return;
	}

	_LookPackets( l );
}
