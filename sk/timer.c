#include <stdio.h>
#include <dlfcn.h>
#include <errno.h>
#include "Serv.h"
#include "PQ.h"
#include "time.h"

typedef struct _Timer
{
	int				tid;
	int				sys;
	unsigned long	sec;
	unsigned long	usec;
	long			ms;			/* skAddTimer( ms , ... */
	SkTimerProc		proc;
	void			*param;
	struct _Timer	*next;

} Timer;

#ifndef offsetof
#define	offsetof(a,b)	((size_t)&((a*)0)->b)
#endif

static	Timer		*hash = 0;

static	int			blocktimer = 0;

static	Timer *_GetTimerRec( void )
{
	Timer	*t;

	if ( !hash )
		return( (Timer*)malloc(sizeof(Timer)) );

	t = hash;
	hash = hash->next;

	return( t );
}

static	void	_FreeTimerRec( Timer *t )
{
	t->tid = -1;
	t->next = hash;
	hash = t;
}

static PQueue *normroot = 0;
static PQueue *sysroot = 0;

#if !defined(CLOCK_MONOTONIC)
/* correct the '#if !defined' line for other architectures !! */
#error CLOCK_MONOTONIC NEEDED !!!
#endif

static  void    CurTime( struct timeval *tv )
{
    struct timespec tsp;
    clock_gettime(CLOCK_MONOTONIC, &tsp);
    tv->tv_sec=tsp.tv_sec;
    tv->tv_usec=tsp.tv_nsec / 1000;
}

static int _cmp_( const void *t1, const void *t2 )
{
	const Timer	*ti1 = t1;
	const Timer	*ti2 = t2;

	if ( ti1->sec < ti2->sec )
		return -1;
	if ( ti1->sec > ti2->sec )
		return 1;
	if ( ti1->usec < ti2->usec )
		return -1;
	if ( ti1->usec > ti2->usec )
		return 1;
	return 0;
}

static	void	AddMS( Timer *timer, long ms )
{
	timer->sec += ms/1000;
	timer->usec += (ms%1000)*1000;

	while( timer->usec > 1000000 )
	{
		timer->usec -= 1000000;
		timer->sec++;
	}
}

static	long	DiffMS( Timer *t1, Timer *t2 )
{
	long			ms;
	unsigned long	s;

	s=t1->sec - t2->sec;
	ms=s*1000;

	ms+=(t1->usec/1000);
	ms-=(t2->usec/1000);

	return ms;
}

static	SkTimerType addTimer( long ms, SkTimerProc proc, void *param, int sys )
{
	Timer			*timer;
	struct timeval	tv;

	CurTime( &tv );

	timer = _GetTimerRec();
	timer->ms = ms;
	timer->sec = tv.tv_sec;
	timer->usec = tv.tv_usec;
	AddMS( timer, ms );
	timer->proc = proc;
	timer->param = param;
	timer->sys = sys;
	timer->next = 0;

	if ( sys )
	{
		if ( !sysroot )
			sysroot = CreatePQueue( _cmp_, offsetof(Timer,tid), 50 );
		InsertPQueue( sysroot, timer );
	}
	else
	{
		if ( !normroot )
			normroot = CreatePQueue( _cmp_, offsetof(Timer,tid), 50 );
		InsertPQueue( normroot, timer );
	}

	return( (SkTimerType)timer );
}

SkTimerType skAddTimer( long ms, SkTimerProc proc, void *param )
{
	return addTimer( ms, proc, param, 0 );
}

SkTimerType skAddSystemTimer( long ms, SkTimerProc proc, void *param )
{
	return addTimer( ms, proc, param, 1 );
}

int	skNextTimer( struct timeval *tiv )
{
	Timer			*t;
	struct timeval	tv;
	Timer			now;
	long			n1;
	long			n2;
	long			n;

	if ( !normroot && !sysroot )
	{
		tiv->tv_sec = 5;
		tiv->tv_usec = 0;
		return( SK_NO_TIMER );
	}
	CurTime( &tv );
	now.sec = tv.tv_sec;
	now.usec = tv.tv_usec;

	n1 = 994499;
	n2 = 994499;

	if ( sysroot )
	{
		t = PeekPQFirst( sysroot );
		if ( t )
		{
			if ( _cmp_( t, &now ) > 0 )
				n1 = DiffMS( t, &now );
			else
				n1 = 1;
		}
	}
	if ( !blocktimer && normroot )
	{
		t = PeekPQFirst( normroot );
		if ( t )
		{
			if ( _cmp_( t, &now ) > 0 )
				n2 = DiffMS( t, &now );
			else
				n2 = 1;
		}
	}

	n = n1 < n2 ? n1 : n2;
	if ( n == 994499 )
	{
		tiv->tv_sec = 5;
		tiv->tv_usec = 0;
		return( SK_NO_TIMER );
	}

	tiv->tv_sec = n/1000;
	tiv->tv_usec = (n%1000)*1000;

	return( SK_HAS_TIMER );
}

void	skDisableTimer( void )

{
	blocktimer++;
}

void	skEnableTimer( void )

{
	blocktimer--;
	if ( blocktimer < 0 )
		blocktimer = 0;
}

void skDoValidTimers( void )
{
	Timer			*t;
	struct timeval	tv;
	Timer			now;
	void			*param;

	CurTime( &tv );
	now.sec = tv.tv_sec;
	now.usec = tv.tv_usec;

	if ( !blocktimer && normroot )
	{
		t = PeekPQFirst( normroot );

		while( t && (_cmp_( &now, t) > 0) )
		{
			t = ExtractPQFirst( normroot );		/* take it */
			param = t->param;
			_FreeTimerRec( t );
			t->proc( (SkTimerType)t, param );
			t = PeekPQFirst( normroot );
		}
	}
	if ( sysroot )
		t = PeekPQFirst( sysroot );
	else
		t=0;

	while( t && (_cmp_( &now, t) > 0) )
	{
		t = ExtractPQFirst( sysroot );		/* take it */
		param = t->param;
		_FreeTimerRec( t );
		t->proc( (SkTimerType)t, param );
		t = PeekPQFirst( sysroot );
	}
}

long	skGetTimerTime( SkTimerType t )
{
	Timer	*timer = t;

	if ( !t )
		return( 0 );
	return( timer->ms );
}

long	skGetTimerRemain( SkTimerType t )
{
	struct timeval	tv;
	Timer	now;
	Timer	*timer = t;

	if ( !t )
		return( 0 );

	CurTime( &tv );
	now.sec = tv.tv_sec;
	now.usec = tv.tv_usec;
	return( DiffMS( timer, &now ) );
}

void	skRemoveTimer( SkTimerType t )
{
	Timer	*timer = t;

	if ( !t )
		return;

	if ( timer->tid == -1 )
		return;

	if ( timer->sys )
		ExtractPQueueAt( sysroot, timer->tid );
	else
		ExtractPQueueAt( normroot, timer->tid );
	_FreeTimerRec( timer );
}
