#include <stdio.h>
#include <errno.h>

#include "NetDrv.h"

static int	__Sk_rwrite( SkLine * l, int num )
{
	int				x;
	fd_set			Mask;
	int				max = l->out->fill - l->out->ptr;
	struct timeval	tv = { 0, 500000 };
	char			f = 1;
	int				cxi = 50;

	if ( l->fd == -1 )
		return( -1 );

	if ( max > 1024 * 4 )
		max = 1024 * 4;

	if ( !max )
		return( 0 );

	while( (num > 0) || (f == 1) )
	{
		if  ( l->fd == -1 )
			return( -1 );

		f = 0;
#ifdef _WINDOWS
		errno = 0;
		x = send( l->fd, l->out->cache+l->out->ptr, max, 0 );

		if ( x == -1 )
			errno = WSAGetLastError();

		if (( x == -1 ) && ( errno == WSAEWOULDBLOCK ))
		{
			if ( num > 0 )
			{
				if ( ! cxi-- )
				{
					close(l->fd);
					l->fd = -1;
					return( -1 );
				}
				FD_ZERO( &Mask );
				FD_SET( l->fd, &Mask );
				select( l->fd+1, 0, &Mask, 0, &tv );
				tv.tv_sec = 0;
				tv.tv_usec = 100000;
			}
			continue;
		}
#else
		x = write( l->fd, l->out->cache+l->out->ptr, max );

		if (!x || (( x == -1 ) && ( errno == EAGAIN )))
		{
			if ( num > 0 )
			{
				if ( ! cxi-- )
				{
					close(l->fd);
					l->fd = -1;
					return( -1 );
				}
				FD_ZERO( &Mask );
				FD_SET( l->fd, &Mask );
				x = select( l->fd+1, 0, &Mask, 0, &tv );
				tv.tv_sec = 0;
				tv.tv_usec = 100000;
				if ( x < 0 )
					perror("select");
			}
			continue;
		}
		if (( x == -1 ) && ( errno == EINTR ))
		{
			x = 0;
		}
#endif
		if ( x < 0 )
		{
			close(l->fd);
			l->fd = -1;
			return( -1 );
		}

		num -= x;
		l->out->ptr += x;
		max = l->out->fill - l->out->ptr;
		if ( max > 1024 * 4 )
			max = 1024 * 4;
	}

	if ( l->out->ptr == l->out->fill )
	{
		l->out->ptr = 0;
		l->out->fill = 0;
	}

	return( 0 );
}

/*
**
	_WritePacket
--
	best possible 'unblocked' write, block if needed (max-cache used) !!!
**
*/
int	_WritePacket( SkLine * l, unsigned char * from, int cnt )
{
	int		x;
	int		max;
	int		max2;
	int		bw = 0;

	if ( l->fd == -1 )
		return(-1);

	if ( !l->out )
	{
		l->out = (SkCache*)malloc(sizeof(SkCache));
		l->out->ptr = 0;
		l->out->fill = 0;
		l->out->size = SK_CACHE_SIZE;
		l->out->cache = (char*)malloc( SK_CACHE_SIZE );
	}
	max = l->out->size - l->out->fill;
	max2 = max + l->out->ptr;
	if ( cnt > max2 )
	{
		if ( (l->out->fill - l->out->ptr > 0 ) && l->out->ptr )
		{
			memmove(l->out->cache,l->out->cache+l->out->ptr,
					l->out->fill - l->out->ptr);
		}
		l->out->fill -= l->out->ptr;
		l->out->ptr = 0;
/*
** Cache will be bigger
*/
		if ( l->out->size < l->max_cache_pages * SK_CACHE_SIZE )
		{
			char	*nc;

/*			l->out->size = SK_CACHE_SIZE; */
#if 0
fprintf(stderr,"realloc write-cache to size %d\n",
l->out->size + SK_CACHE_SIZE);
#endif
			nc = realloc( l->out->cache, l->out->size + SK_CACHE_SIZE );
			if ( nc )
			{
				bw = 0;
				l->out->cache = nc;
				l->out->size += SK_CACHE_SIZE;
				max += SK_CACHE_SIZE;
				max2 += SK_CACHE_SIZE;
			}
		}
		if ( bw )
		{
			l->blocked = 1;
#if 0
fprintf(stderr,"blocked write of %d bytes\n",cnt - max2);
#endif
			x = __Sk_rwrite( l, cnt - max2 );	/* blocked write */
			if ( x == -1 )
			{
				_skDelLine( l );
				return( -1 );
			}
			if ( (l->out->fill - l->out->ptr > 0 ) && l->out->ptr )
			{
				memmove(l->out->cache,l->out->cache+l->out->ptr,
						l->out->fill - l->out->ptr);
			}
			l->out->fill -= l->out->ptr;
			l->out->ptr = 0;
		}
	} else if ( cnt > max )
	{
		if ( (l->out->fill-l->out->ptr > 0 ) && l->out->ptr )
		{
			memmove(l->out->cache,l->out->cache+l->out->ptr,
					l->out->fill - l->out->ptr);
		}
		l->out->fill -= l->out->ptr;
		l->out->ptr = 0;
	}

	if ( cnt )
	{
		memcpy(l->out->cache+l->out->fill,from,cnt);
		l->out->fill += cnt;
	}

	x = __Sk_rwrite( l, 0 );	/* unblocked write */
	if ( x == -1 )
	{
		_skDelLine( l );
		return( -1 );
	}

	return( cnt );
}

int	_WriteString( SkLine *l, char *data )
{
	return _WritePacket(l,(unsigned char*)data,strlen(data));
}


/*
**
	_FlushLine
--
	'unblocked' flush output buffers
**
*/
int	skFlushLine( SkLine *l )
{
	int		x;

	if ( !l->out )
		return( 0 );

	x = __Sk_rwrite( l, 0 );	/* unblocked write */
	if ( x == -1 )
	{
		_skDelLine( l );
	}
	return( x );
}

/*
**
	_SyncLine
--
	'blocked' flush output buffers
**
*/
int	_SyncLine( SkLine *l )
{
	int		x = 0;

	if ( !l->out )
		return( 0 );

	while ( l->out->fill > l->out->ptr )
	{
			x = __Sk_rwrite( l, l->out->fill - l->out->ptr );
			if ( x == -1 )
			{
				_skDelLine( l );
				return( x );
			}
	}

	return( x );
}
