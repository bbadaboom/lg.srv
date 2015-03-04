#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include "Serv.h"

static	SkLine		*root = 0;
static	SkLine		*hash = 0;

#define FreeValid(a)	if(a){free(a); a=0;}

static	SkLine *_GetLineRec( void )
{
	SkLine	*l;

	if ( !hash )
	{
		return( (SkLine*)malloc(sizeof(SkLine)) );
	}

	l = hash;
	hash = hash->next;

	return( l );
}

static	void	_FreeLineCache( SkLine *l )
{
	if ( l->in )
		FreeValid( l->in->cache );
	FreeValid( l->in );

	if ( l->out )
		FreeValid( l->out->cache );
	FreeValid( l->out );
}

static	void	_FreeLineRec( SkLine *l )
{
	_FreeLineCache(l);

	FreeValid( l->ip );

	l->close = 0;
}

static	void	_HashLineRec( SkLine *l )
{
	SkLine		*p;

	if ( root == l )
	{
		root = l->next;
	}
	else
	{
		for( p=root; p->next; p = p->next )
		{
			if ( p->next == l )
			{
				p->next = l->next;
				break;
			}
		}
	}
	l->next = hash;
	hash = l;
}

SkLine	*skNewLine( void )
{
	SkLine	*h;
static	int	first = 1;

	if ( first )
	{
		signal(SIGPIPE,SIG_IGN);
		first=0;
	}

	h = _GetLineRec();
	if ( !h )
		return( h );

	memset(h,0,sizeof(SkLine));

	h->max_cache_pages    = 10;
	h->fd                 = -1;
	h->other_v      	  = 1;

	h->next = root;
	root    = h;

	return( h );
}

void	_xskDelLine( SkLine *l )
{
	SkHandler		*h;
	SkHandler		*hn;
	struct timeval	now;

	if ( !l )
		return;

	gettimeofday(&now,0);

	if ( l->freetime )
	{
		if ( l->freetime < now.tv_sec )
			_HashLineRec( l );
		return;
	}

	l->freetime = now.tv_sec + 8;		/* 8 sec */

	skDoHandler( l, SK_H_CLOSE, 0 );

	if ( l->close )
		l->close( l );

	if ( l->tid_sendwatch )
		skRemoveTimer( l->tid_sendwatch );
	if ( l->tid_recvwatch )
		skRemoveTimer( l->tid_recvwatch );

	l->watch_time = 0;
	l->tid_sendwatch = 0;
	l->tid_recvwatch = 0;

	l->close = 0;
	l->data = 0;

	for( h = l->handler; h; h = hn )
	{
		hn = h->next;

		_skFreeHandlerRec( h );
	}

	l->handler = 0;

	FreeValid( l->hostname );

	_FreeLineRec( l );
}

void	_skDelLine( SkLine *l )
{
	if ( !l )
		return;

	l->intid = -1;

	if ( l->tid_sendwatch )
		skRemoveTimer( l->tid_sendwatch );
	if ( l->tid_recvwatch )
		skRemoveTimer( l->tid_recvwatch );

	l->watch_time = 0;
	l->tid_sendwatch = 0;
	l->tid_recvwatch = 0;

	if ( l->fd != -1 )
	{
		close( l->fd );
		l->fd = -1;
	}
}

static	SkTimerType		vsk_cl_tid = 0;

static void _clearDeadLines( SkTimerType tid, void *own )
{
	SkLine			*l;
	SkLine			*n;

	for( l=root; l; l=n )
	{
		n = l->next;

		if (( l->intid == -1 ) || ( l->fd == -1 ))
		{
			_xskDelLine( l );
		}
	}
	vsk_cl_tid = skAddSystemTimer( 1000, _clearDeadLines, 0 );
}

int	skDoSomething( struct timeval *tv, int loopcont )
{
	fd_set			rMask;
	fd_set			wMask;
	SkLine			*l;
	SkLine			*ln;
	int				x;
	unsigned int	hi;

	if ( !tv->tv_usec && !tv->tv_sec )
		return(0);

	if ( !vsk_cl_tid )
		vsk_cl_tid = skAddSystemTimer( 2000, _clearDeadLines, 0 );

	while( 1 )
	{
		hi = 0;
		FD_ZERO( &rMask );
		FD_ZERO( &wMask );

		for( l=root; l; l=ln )
		{
			ln=l->next;

			if ( l->disabled )
				continue;

			if ( l->fd != -1 )
			{
				if ( !hi )
					hi = 1;

				if ( l->fd > hi )
					hi = l->fd;
				if ( l->virt != 99 )
				{
					FD_SET( l->fd, &rMask );
				}
				if ( l->out && (l->out->fill != l->out->ptr ))
					FD_SET( l->fd, &wMask );
				if ( l->close_at_empty )
				{
					if ( !l->out || (l->out->fill == l->out->ptr ))
					{
						skDisconnect( l );
						continue;
					}
				}
				if ( l->virt == 99 )
					FD_SET( l->fd, &wMask );
			}
		}
		x = select( hi? hi+1 : 0, &rMask, &wMask, 0, tv );
		if ( x == -1 )
		{
			if ( errno == EINTR )
				return( 0 );

			return( -1 );
		}
		for( l=root; l; l=l->next )
		{
			if ( l->fd == -1 )
				continue;
			if ( l->virt == 99 )		/* wait for connect */
				continue;
			if ( l->virt )
			{
				FD_SET( l->fd, &rMask );
				x++;
			}
		}
		if ( x == 0 )
		{
			return( 0 );
		}

		for( l=root; l; l=ln )
		{
			ln = l->next;

			if ( l->disabled )
				continue;

			if ( (l->fd != -1 ) && FD_ISSET(l->fd, &wMask ) )
			{
				if ( l->virt == 99 )	/* wait for connect */
				{
					l->virt = 0;
					skDoHandler( l, SK_H_WRITABLE, 0 );
				}

				if ( l->close_at_empty )
				{
					if ( !l->out || (l->out->fill == l->out->ptr ))
					{
						skDisconnect( l );
						continue;
					}
				}
			}
			if ( (l->fd != -1 ) && FD_ISSET(l->fd, &rMask ) )
			{
				skDoHandler( l, SK_H_READABLE, 0 );
				l->virt = 0;
			}
		}

		return( 1 );
	}

	return( 0 );
}

void	skCloseAtEmpty( SkLine *l )
{
	if ( !l->out || (l->out->fill == l->out->ptr ))
	{
		skDisconnect( l );
		return;
	}

	l->close_at_empty = 1;
}

SkLine	*skLineOfFd( int fd )
{
	SkLine	*l;

	for( l=root; l; l=l->next )
	{
		if ( l->fd == fd )
			return( l );
	}
	return( 0 );
}

void	skStep( void )
{
	struct timeval	tv;
	int				x;

	skNextTimer( &tv );
	x = skDoSomething( &tv, SK_NOCONT );
	if ( !x )
		skDoValidTimers();
}

static	void	_stepTO( SkTimerType tid, void *own )
{
	int		*donot = own;

	*donot = 1;
}

int	skTimeoutStep( unsigned long msec )
{
	struct timeval	tv;
	int				x = 0;
	int				donot = 0;
	SkTimerType		tid=0;

	if ( msec )
		tid=skAddSystemTimer( msec, _stepTO, &donot );

	while( !donot )
	{
		if ( msec )
		{
			skNextTimer( &tv );
		}
		else
		{
			tv.tv_sec = 0;
			tv.tv_usec = 1000;		// 1ms
		}
		x = skDoSomething( &tv, SK_NOCONT );
		skDoValidTimers();

		if ( !msec && !x )
			break;
	}
	if ( !donot && tid )
		skRemoveTimer(tid);

	return x;
}

void	skMainLoop( void )
{
static int			highcnt = 20;
	SkLine			*l;
	SkLine			*n;
	int				cnt = 0;
	struct timeval	now = { 0, 0 };

	while( 1 )
	{
		skStep();

		highcnt--;
		if ( !highcnt )
		{
			skDoValidTimers();
			highcnt = 20;
		}
		cnt++;
		if ( cnt > 50 )
		{
			now.tv_sec = 0;
			for( l=root, n=0; l; l=n )
			{
				n = l->next;
				if ( l->freetime )
				{
					if ( !now.tv_sec )
						gettimeofday( &now, 0 );
					if ( l->freetime < (time_t)now.tv_sec )
					{
						_HashLineRec( l );
					}
				}
			}
			cnt = 0;
		}
	}

	while( root )
	{
		_skDelLine( root );
		_xskDelLine( root );
		_HashLineRec( root );
	}
}

void	skDisconnect( SkLine *l )
{
	_skDelLine( l );
	_xskDelLine( l );
}

SkLine	*skGetLinesRoot( void )
{
	return( root );
}

void	skDisableLine( SkLine *l )
{
	if ( l )
		l->disabled++;
}

void	skEnableLine( SkLine *l )
{
	if ( l && l->disabled )
		l->disabled--;
}

void	skMultiDisable( SkLine *woline, int timer_also )
{
	SkLine 	*l;
	for( l=root; l; l=l->next )
	{
		if ( l == woline )
			continue;
		l->disabled++;
	}
	if ( timer_also )
		skDisableTimer();
}

void	skMultiEnable( SkLine *woline, int timer_also )
{
	SkLine 	*l;
	for( l=root; l; l=l->next )
	{
		if ( l == woline )
			continue;
		if ( l->disabled )
			l->disabled--;
	}
	if ( timer_also )
		skEnableTimer();
}
