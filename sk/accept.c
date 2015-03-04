#include <stdio.h>
#include "Serv.h"

#define XXV		0

SkLine	*skAcceptFd( int fd )
{
	struct sockaddr_in	from;
#ifdef _LINUX_TYPES_H
	int					from_len = sizeof(from);		/* old */
#else
	unsigned int		from_len = sizeof(from);
#endif
	int				nfd;
	SkLine				*l;
	int					close_on_exec = 1;
	char				host[ 64 ];
	unsigned char		*ip = (unsigned char*)&(from.sin_addr.s_addr);
	struct hostent		*he=0;

	nfd = accept( fd, (struct sockaddr*)&from, &from_len );

	if ( nfd == -1 )
	{
		return( 0 );
	}

//	he = gethostbyaddr( (char*)ip, 4, AF_INET );
	if ( he )
	{
		strcpy(host,he->h_name);
	}
	else
	{
		if (( ip[0] == 127 ) &&
			( ip[1] == 0 ) &&
			( ip[1] == 0 ) &&
			( ip[1] == 1 ))
		{
			strcpy(host,"localhost");
		}
		else
		{
			sprintf(host,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
		}
	}

#ifdef SO_KEEPALIVE
	{
		int		t = 1;

		setsockopt( nfd, SOL_SOCKET, SO_KEEPALIVE, (char*)&t, sizeof(int) );
	}
#endif
#if 0
#ifdef SO_LINGER
	{
		int	linger[2] = { 1, 1 };

		setsockopt (nfd, SOL_SOCKET, SO_LINGER, (char *)linger, sizeof(linger));
	}
#endif
#endif

	fcntl( nfd, F_SETFL, O_NONBLOCK );
	fcntl( nfd, F_SETFD, &close_on_exec );

	l = skNewLine( );
	l->fd = nfd;
	l->hostname = strdup(host);
	l->ip = malloc(sizeof(struct in_addr));
	memcpy((char*)l->ip,&from.sin_addr,sizeof(struct in_addr));

#if (XXV == 1)
	printf("accept from %s\n",host);
#endif

	if ( l->intid == 1 )
	{
		l->other_v = 1;
		return l;
	}
/* 
 * for old clients we need to send cyclic a life sign 
 */
	l->other_v = 0;
	l->tid_sendwatch = 0;

	return( l );
}
