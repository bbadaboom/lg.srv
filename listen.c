#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32_WCE
#include <signal.h>
#endif //_WIN32_WCE
#include <stdarg.h>

#include "NetDrv.h"

static	void	li_close( SkLine *h )
{
}

static	void	_add_client( SkLine *h, int type, void *own, void *sys )
{
	SkLine	*l;

	l = skAcceptFd( h->fd );

	if ( !l )
		return;

	l->intid = SK_ID_HTTP;
	l->other_v = 1;

	if ( l->tid_sendwatch )
			skRemoveTimer( l->tid_sendwatch );
	l->tid_sendwatch = 0;

	_clientConnected( l, 0, 0, 0 );
}

static	int	__listen( SkLine *h )
{
	struct sockaddr_in	insock;
	int					close_on_exec = 1;

	h->fd = socket( AF_INET, SOCK_STREAM, 0 );
	if ( h->fd == -1 )
		return -1;

	insock.sin_family = AF_INET;
	insock.sin_port = htons(h->port);
	insock.sin_addr.s_addr = htonl( INADDR_ANY );

#ifdef SO_REUSEADDR
	{
		int xxx = 1;

		setsockopt(h->fd, SOL_SOCKET, SO_REUSEADDR, (char*)&xxx, sizeof(int));
	}
#endif

	if ( bind(h->fd, (struct sockaddr*)&insock, sizeof(insock)) )
	{
		close( h->fd );
		h->fd = -1;
		return( -1 );
	}

	if ( listen( h->fd, 5 ) )
	{
		close( h->fd );
		h->fd = -1;
		return( -1 );
	}

	skAddHandler( h, SK_H_READABLE, _add_client, 0 );

#if !defined(_WINDOWS) && !defined(_WIN32_WCE)
	fcntl( h->fd, F_SETFD, &close_on_exec );
#endif

	return( 0 );
}

SkLine	*_lgListen( unsigned short port )
{
	SkLine				*l;

	l = skNewLine();
	l->port = port;
	l->intid = SK_ID_HTTP;
	l->close = li_close;

	if ( __listen( l ) == -1 )
	{
		_skDelLine( l );
		_xskDelLine( l );
		l=0;
	}

	return l;
}
