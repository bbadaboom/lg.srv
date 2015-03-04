#include "Serv.h"
#include <errno.h>
#include <stdlib.h>

struct hostent	*skGetHostByName( char *hostname )
{
	char			*p;
	char			*p1;
	int				l=strlen(hostname);
	char			host2[ 20 ];
	int				ac=0;
static struct hostent	he;
static struct in_addr	in_addr;
static char				*addr_list[1];
	unsigned char	av[4];

	if ( l < 16 )
	{
		strcpy(host2,hostname);
		p1=host2;
		for( p=host2; *p; p++ )
		{
			if ( *p == '.' )
			{
				if ( ac == 3 )
					break;
				*p=0;
				av[ac]=atoi(p1);
				p1=p+1;
				*p='.';
				ac++;
				continue;
			}
			if ( *p < '0' )
				break;
			if ( *p > '9' )
				break;
		}
		if (( p1 != p ) && ( ac == 3 ))
		{
			av[ac]=atoi(p1);
			ac++;
		}
		if ( ! *p && (ac==4) )
		{
			memset(&in_addr,0,sizeof(struct in_addr));
			memcpy(&in_addr,av,4);
			he.h_addr_list = addr_list;
			addr_list[0] = (char*)&in_addr;
			he.h_length = 4;
			return &he;
		}
	}
	return 0;
}
static	void	conn_tout( SkTimerType tid, void *own )
{
	SkLine		*h = own;

	h->tid_sendwatch = 0;

	if ( !h->virt )
		return;

	if ( h->fd != -1 )
		close( h->fd );
	h->fd = -1;
	h->virt = 0;
	skDoHandler( h, SK_H_CONNECT, (void*)ASY_TIMEOUT );
	h->intid = -1;

	_skDelLine( h );
	_xskDelLine( h );
}

static	void	conn_done( SkLine *h, int pt, void *own, void *sys )
{
	int					x;
#ifdef _LINUX_TYPES_H
	int					y;		/* old */
#else
	unsigned int		y;
#endif

	if ( h->tid_sendwatch )
		skRemoveTimer( h->tid_sendwatch );
	h->tid_sendwatch = 0;

	if ( h->fd == -1 )
	{
		skDoHandler( h, SK_H_CONNECT, (void*)ASY_FAILED );
		return;
	}
	/* (dt) ...nur der x-Zeiger muss immer ein char* sein */
	y = sizeof(x);
	if ( (getsockopt( h->fd, SOL_SOCKET, SO_ERROR, (char *)&x, &y ) == -1) 
		 || x )
	{
		close( h->fd );
		h->fd = -1;
		skDoHandler( h, SK_H_CONNECT, (void*)ASY_FAILED );
		return;
	}

	skRemoveHandler( h, SK_H_WRITABLE, conn_done, 0 );
	skDoHandler( h, SK_H_CONNECT, (void*)ASY_CONNECTED );
	return;
}

static	int	_visAsyConnectIP( SkLine *h, unsigned long tout )
{
	struct	sockaddr_in	insock;

	h->fd = socket( AF_INET, SOCK_STREAM, 0 );
	if ( h->fd == -1 )
	{
		skDoHandler( h, SK_H_CONNECT, (void*)ASY_FAILED );
		return( -1 );
	}

	memcpy((char*)&insock.sin_addr,(char*)h->ip,sizeof(struct in_addr));
	insock.sin_family = AF_INET;
	insock.sin_port = htons(h->port);

	{
		long close_on_exec = FD_CLOEXEC;

		fcntl( h->fd, F_SETFD, close_on_exec );
	}

#ifdef SO_KEEPALIVE
	{
		int		t = 1;

		setsockopt( h->fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&t, sizeof(int) );
	}
#endif
#if 0
#ifdef SO_LINGER
	{
		int	linger[2] = { 1, 1 };

		setsockopt (h->fd, SOL_SOCKET, SO_LINGER, (char *)linger, sizeof(linger));
	}
#endif
#endif

	/*
	 * set fd to nonblocking IO
	 */
	fcntl( h->fd, F_SETFL, O_NONBLOCK );
	errno = EAGAIN;

	h->virt = 0;

	while( errno == EAGAIN )
	{
		if ( connect( h->fd, (struct sockaddr*)&insock,
					  sizeof(struct sockaddr_in)) < 0 )
		{
			if ( errno == EINTR )
			{
				errno = EAGAIN;
				continue;
			}
			if ( errno == EAGAIN )
				continue;
			if (( errno != EINPROGRESS ) && ( errno != EALREADY ))
			{
				close( h->fd );
				h->fd = -1;
				skDoHandler( h, SK_H_CONNECT, (void*)ASY_FAILED );
				return( -1 );
			}
			h->virt = 99;		/* in progress */
		}
		break;
	}

	if ( !h->virt )
	{
		skDoHandler( h, SK_H_CONNECT, (void*)ASY_CONNECTED );
		return 0;
	}

	if ( tout )
		h->tid_sendwatch = skAddTimer( tout, conn_tout, h );
	skAddHandler( h, SK_H_WRITABLE, conn_done, 0 );
	return 0;
}

void	skAsyConnect( char *hostname,char *service, unsigned long tout,
			SkHProc proc, void *own )
{
	SkLine			*l;
	struct hostent	*he;

	if ( !hostname || !service )
	{
		if ( proc )
			proc( 0, SK_H_CONNECT, own, (void*)ASY_ERR_PARAM );
		return;
	}
	he = skGetHostByName( hostname );
	if ( !he )
	{
		if ( proc )
			proc( 0, SK_H_CONNECT, own, (void*)ASY_ERR_HOST );
		return;
	}

	l = skNewLine( );
	l->port = atol( service );
	l->hostname = strdup( hostname );

	if ( !l->ip )
		l->ip = malloc(sizeof(struct in_addr));
	memcpy((char*)l->ip,(char*)he->h_addr,sizeof(struct in_addr));
	l->watch_time = 0;
	skAddHandler( l, SK_H_CONNECT, proc, own );
	if ( _visAsyConnectIP( l, tout ) != 0 )
	{
		_skDelLine( l );
		_xskDelLine( l );
	}

	return;
}
