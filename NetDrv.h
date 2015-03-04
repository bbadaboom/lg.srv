#ifndef NETDRV_H
#define NETDRV_H

#ifndef SkLineData
#define SkLineData		struct _ClientData
#endif

#if 0
#define SWAP4(a)	htonl(a)
#define SWAP2(a)	htons(a)
#else
#define SWAP4(a)	(a)
#define SWAP2(a)	(a)
#endif

#include <Serv.h>
#include <linux/if_ether.h>
#include <time.h>

#define ETH_H_PACKET	SK_H_USER1

typedef struct _EthPacket
{
	struct sockaddr	from;
	struct ethhdr	ethhdr;
	unsigned char	*data;
	int				len;

} EthPacket;

typedef struct _ClientData
{
	int				cl_id;
	int				modify_state;
	int				mode;
	int				active_log;
	int				num_wrong_frames;

/* http data */
	int				post_mode;
	char			*filename;
	int				boundary;
	int				size;
	int				written;
	FILE			*fp;
	char			logger;
	int				nlines;
	SkTimerType 	rcv_timer;
	int				last_x;

} ClientData;

#define	MODE_UNKNOWN	0
#define	MODE_HTTP		1
#define	MODE_LOG		3

/* vinter.c */
extern	char **M5sStrgCut( char *in, int *rargc, char beautify );
#ifdef NOINTERFACES
#define Log		if(0)printf
#else
extern	void	Log( int lv, char *fmt, ... );
#endif

/* rd2.c */
extern	void	_HReadable( SkLine *l, int pt, void *own, void *sys );

/* write.c */
extern	int	_WritePacket( SkLine * l, unsigned char * from, int cnt );
extern	int	_WriteString( SkLine * l, char * strg );
extern	int	_FlushLine( SkLine *l );
extern	int	_SyncLine( SkLine *l );
extern	void _AsySyncLine( SkLine *l );

/* u.c */
extern	void	_clientPacket( SkLine *l, int pt, void *own, void *sys );
extern	void	_clientConnected( SkLine *l, int pt, void *own, void *sys );
extern	void	_ShowStatistic( SkLine *l, int ashtml );
extern	void    _ResetStatistik( void );
extern	void	SetMaxLog( int nr );
extern	void	_RestartMe( void );

/* listen.c */
extern	SkLine	*_lgListen( unsigned short port );

/* http.c */
extern	void	HttpStatAddClData( void );
extern	void	HttpPck( SkLine *l, int pt, void *own, void *sys );
extern	int		HttpNeedPicture( void );
extern	void	HttpPictureDone( int rc );
extern	void	HttpSetNoCam( void );
extern	void	HttpLoadCleaningRecord( SkTimerType tid, void *own );

/* vinter.c */
extern	void	DoLogData( SkLine *l, char *data, int len );
extern	int		LogActive( int lv );

/* control.c */
extern	void	RunControlFrame( unsigned char *in );
extern	int		FillControlFrame( SkLine *l, unsigned char *out );

/* ident.c */
extern	void	FillIdentFrame( SkLine *l, unsigned char *out );

/* param.c */
extern	void	FillParamFrame( SkLine *l, unsigned char *out );

/* data.c */
extern	void	FillDataFrame( SkLine *l, unsigned char *out );

/* proc.c */
extern	int	ReadProcStat( int *a_user, int *a_sys, int *a_nice, int *a_idle );

#if 0
/* v4l2.c */
extern	void	_v4l2_start( void );

/* jpeg.c */
extern	int		GenerateJpegFile( unsigned char *img, char *filename );
#endif

/* json.c */
typedef struct _JsonVars
{
	char	*robot_state;
	char	*turbo;
	char	*repeat;
	char	*batt;
	char	*mode;
	char	*nickname;
	char	*version;
	char	*battperc;
} JsonVars;

extern	int	jsonSend( char *command );

/* json.c */
typedef struct _TimerVars
{
	char	*mon;
	char	*tue;
	char	*wed;
	char	*thu;
	char	*fri;
	char	*sat;
	char	*sun;
} TimerVars;

extern	void	ReadTimerFromFile( void );
extern	void	RunTimerParam( char *param );
extern	void	StartTimer( void );

#endif
