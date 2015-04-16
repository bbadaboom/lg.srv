#include <stdio.h>
#include <signal.h>
#include <stdarg.h>

#include "NetDrv.h"

int	ReadProcStat( int *a_user, int *a_sys, int *a_nice, int *a_idle )
{
	int 	fd;
	char	tmp[2048];
	char	*p;
	char	**av;
	int		ac;
	int		i, l;
static	long	ouser=0;
static	long	osys=0;
static	long	onice=0;
static	long	oidle=0;
	long	user, nice, sys, idle;
	long	udiff, ndiff, sdiff, idiff;
	long	jiffies=0;

	fd=open( "/proc/stat", O_RDONLY );
	if ( fd == -1 )
		return -1;

	*tmp=0;
	if ( read( fd, tmp, 2048 ) );
	close(fd);
	tmp[512]=0;
	p=strchr(tmp,'\n');
	if ( p ) *p=0;
	av=M5sStrgCut(tmp,&ac,1);
	if ( !av )
		return -1;
	if ( ac<5 )
	{
		free( av );
		return -1;
	}
	for( i=1;i<5;i++ )
	{
		l=strlen(av[i]);
		if ( l>6 )
			av[i]=av[i]+l-6;
	}
	user=atoi(av[1]);
	nice=atoi(av[2]);
	sys=atoi(av[3]);
	idle=atoi(av[4]);
	udiff = user < ouser ? 1000000+user-ouser : user-ouser;
	sdiff = sys < osys ? 1000000+sys-osys : sys-osys;
	idiff = idle < oidle ? 1000000+idle-oidle : idle-oidle;
	ndiff = nice < onice ? 1000000+nice-onice : nice-onice;
	jiffies = udiff+sdiff+idiff+ndiff;
	if ( jiffies )
	{
		*a_user = udiff>jiffies?10000:udiff*10000/jiffies;
		*a_sys = sdiff>jiffies?10000:sdiff*10000/jiffies;
		*a_nice = ndiff>jiffies?10000:ndiff*10000/jiffies;
		*a_idle = idiff>jiffies?10000:idiff*10000/jiffies;
	}
	else
	{
		*a_user = 0;
		*a_sys = 0;
		*a_nice = 0;
		*a_idle = 0;
	}
	free( av );
	ouser=user;
	osys=sys;
	onice=nice;
	oidle=idle;

	return 0;
}
