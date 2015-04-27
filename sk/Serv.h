#ifndef SK_PROTO_H
#define SK_PROTO_H

#ifdef __cplusplus
extern	"C"	{
#endif

#include <string.h>
#include <malloc.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/file.h>

#include <net/if.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>

#ifndef SkLineData
#define SkLineData	void
#endif

/* now define types for communication */
typedef	unsigned int	s__u32;
typedef	int				s__32;

#if (! defined linux) || (__GNUC__ >= 3) || defined(__x86_64__)
#pragma pack ( push, 4 ) /* Windows oder gcc 3.x.y */
#endif

typedef struct _SkDgPacket
{
	char			*data;
	struct sockaddr	from;
	int				len;

} SkDgPacket;

typedef struct _SkPacket
{
	int				len;
	int				size;
	unsigned char	ptype;
	char			*data;
	short			port;

} SkPacket;

typedef struct _sskPacket
{
	struct _SkPacket	p;
	struct _sskPacket	*next;
} sskPacket;

typedef struct _SkCache
{
	char			*cache;
	int				size;
	int				fill;
	int				ptr;

} SkCache;

#define SK_CACHE_SIZE	(16*1024)

typedef struct _SkLine	PreSkLine;

typedef void	(*skLineCloseProc)(struct _SkLine* l);

typedef void	*SkTimerType;

typedef struct _SkLine
{
	struct _SkHandler		*handler;
	struct in_addr			*ip;
	struct _SkCache			*in;
	struct _SkCache			*out;
	struct _SkLine			*next;
	skLineCloseProc			close;
	SkLineData				*data;
	char					*hostname;
	int						fd;
	int						max_cache_pages;
	short					port;
	short					intid;
	char					blocked;		/* leitung ist voll & 160k im cache */
	char					virt;			/* virt. readable */
	time_t					freetime;
	unsigned long			watch_time;		/* in sec */
	SkTimerType				tid_sendwatch;
	SkTimerType				tid_recvwatch;
	char					other_v;		/* 1 = other side is new */
	int						close_at_empty;
	int						disabled;

} SkLine;

typedef	struct _SkSearchType
{
	char	*nodename;
	int		ipcnt;
	char	**iplist;

} SkSearchType;

/*
**	SkLine - flags
*/
#define SK_LISTEN		(1L<<0)
#define SK_DG_LISTEN	(1L<<1)
#define SK_CONNECT		(1L<<2)
#define SK_CLIENT		(1L<<3)
#define SK_XLINE		(1L<<4)

typedef void	(*SkSearchProc)( SkSearchType * val );
typedef void	(*SkSearchProc2)( SkSearchType * val, void *own);

typedef void	(*SkHProc)(struct _SkLine *line, int htype,
			void *own, void *sys);

typedef void	(*SkTimerProc)(SkTimerType t, void *param );

typedef struct _SkHandler
{
	struct _SkHandler	*next;
	void				*ownparam;
	SkHProc				proc;
	int					type;
	unsigned char		flags;

} SkHandler;

/*
** SkHandler - type (interal)
*/
#define	SK_H_READABLE		1
#define	SK_H_CLOSE			2
#define	SK_H_PACKET			3
#define	SK_SEARCH_PREF		4		/* no handler !!! */
#define	SK_H_WRITABLE		5
#define	SK_H_CONNECT		6
/*
** SkHandler - type (global)
*/
#define	SK_H_DGREQ			49
#define	SK_H_ADDCLIENT		50

/*
** SkHandler - type (free)
*/
#define	SK_H_USER1			51

/*
** SK_IDs
*/
#define SK_ID_HTTP			14

/*
** Times
*/
#define READ_SELECT_TIME	5000
#define WRITE_SELECT_TIME	10000
#define WRITE_COUNT			300

/*
** Stream - head info
*/
#define SK_NO_STREAM		0
#define SK_STREAM_OPEN		1
#define SK_STREAM_DATA		2
#define SK_STREAM_CLOSE		3

/*
** NextTimer - return
*/
#define SK_NO_TIMER			0
#define SK_HAS_TIMER		1

/*
** DoSomething loopcont - param
*/
#define SK_NOCONT			0
#define SK_CONTINUE			1

/* SK_H_CONNECT bekommt als sys einen int ! */
#define	ASY_CONNECTED		0x00
#define	ASY_ERR_PARAM		0x01
#define	ASY_ERR_HOST		0x02
#define	ASY_TIMEOUT			0x03
#define	ASY_FAILED			0x04

/* handler.c */
extern	void	_skFreeHandlerRec( SkHandler *l );
extern	int		skAddHandler(SkLine *h, int htype, SkHProc proc, void *own);
extern	int		skDoHandler( SkLine *h, int htype, void *sysparam);
extern	int		skRemoveHandler(SkLine *h, int htype, SkHProc proc, void *own);

/* line.c */
extern	SkLine	*skNewLine( void );
extern	void	_skDelLine( SkLine *l );
extern	void	_xskDelLine( SkLine *l );
extern	int		skDoSomething( struct timeval *tv, int loopcont );
extern	void	skStep( void );
extern	int		skTimeoutStep( unsigned long msec );
extern	void	skMainLoop( void );
extern	void	skInitLoop( void );
extern	SkLine	*skLineOfFd( int fd );
extern	SkLine	*skGetLinesRoot( void );
extern	void	skDisconnect( SkLine *l );
extern	void	skCloseAtEmpty( SkLine *l );
extern	void	skDisableLine( SkLine *l );
extern	void	skEnableLine( SkLine *l );
extern  void    skSetSkTerminate(int i );
extern  int     skGetSkTerminate( void );
extern	void	skMultiDisable( SkLine *woline, int timer_also );
extern	void	skMultiEnable( SkLine *woline, int timer_also );

/* listen.c */
extern	SkLine	*skListenService( char *service );
extern	SkLine	*skListenPort( short port );
extern	SkLine	*skListenDGService( char *service );
extern	SkLine	*skListenDGPort( short port );
extern	SkLine	*skSimpleListenService( char *service );

/* accept.c */
extern	SkLine	*skAcceptFd( int fd );

/* timer.c */
extern	SkTimerType skAddTimer( long ms, SkTimerProc proc, void *param );
extern	SkTimerType skAddSystemTimer( long ms, SkTimerProc proc, void *param );
extern	int		skNextTimer( struct timeval *tv );
extern	void	skDoValidTimers( void );
extern	void	skRemoveTimer( SkTimerType t );
extern	void	skEnableTimer( void );
extern	void	skDisableTimer( void );
extern	long	skGetTimerTime( SkTimerType t );
extern	long	skGetTimerRemain( SkTimerType t );

/* asyconnect.c */
extern	void	skAsyConnect( char *hostname,char *service, unsigned long tout,
					SkHProc proc, void *own );
#ifdef __cplusplus
}
#endif

#endif
