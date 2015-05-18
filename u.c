#include <stdio.h>
#include <signal.h>
#include <stdarg.h>

#include "NetDrv.h"

/* passwd root: most9981 */

/* version histrory
 *
 * 13.08.2013   1.0    fx2 initial coding (first shot)
 * 13.08.2013   1.1    fx2 copy files from /usr/data/tmp /usr/bin
 * 13.08.2013   1.2    fx2 option -fg
 * 13.08.2013   1.3    fx2 create DESTPATH if not exist
 * 13.08.2013   1.4    fx2 use killall and system for restart
 * 15.08.2013   1.5    fx2 show cleaningrecord.stc , active dropbear
 * 19.08.2013   1.6    fx2 Cpu Idle+User+Sys in State page
 * 20.08.2013   1.7    fx2 Cpu usage without history
 * 21.08.2013   1.8    fx2 download map files
 * 29.08.2013   1.9    fx2 show map files (intern js)
 * 06.11.2013   1.10   fx2 syncline if more than 1000000 bytes
 * 14.01.2014   1.11   fx2 download to destpath /usr/tmp  (ramdisk)
 * 17.01.2014   1.12   fx2 first access for /dev/video0
 * 02.11.2014   1.13   fx2 access for /dev/video removed
 * 04.11.2014   1.14   fx2 timer page added
 * 05.11.2014   2.00   fx2 new webpages
 * 06.11.2014   2.01   fx2 upload finished, pages
 * 06.11.2014   2.02   fx2 show version of current running software
 * 19.12.2014   2.03   fx2 load cleaningrecord.scl 1x/minute
 * 08.01.2015   2.04   fx2 json.cgi - multiple bracket pairs '{ / }'
 * 15.01.2015   2.05   fx2 MONOTONIC-CLOCK, new web pages (statistic)
 * 17.01.2015   2.06   fx2 MultiDisable(), empty maps.html
 * 18.01.2015   2.07   fx2 bugfixes - uninitialized mem blkfile[]
 * 18.01.2015   2.08   fx2 bugfixes - vom bugfix
 * 18.01.2015   2.09   fx2 num_blk reset to 0
 * 20.01.2015   2.10   fx2 log-interface, show 3 maps
 * 22.01.2015   2.11   fx2 status.txt + status.html
 * 13.02.2015   2.12   fx2 Access-Control-Allow-Origin: *
 *                         status.txt converted to unix
 *                         add buttons for modes(zz,sb,spot)
 * 17.02.2015   2.13   fx2 accept mode at end of timer-string f.e.',ZZ'
 * 19.02.2015   2.14   fx2 upload/download Motion.xml+Navi.xml +Turbo +Repeat
 * 20.02.2015   2.15   fx2 Nickname CC, bugfix strlen(Navi.xml), Mode-button
 * 20.02.2015   2.16   fx2 Nickname CC on all pages
 * 12.04.2015   2.17   fx2 send mail after work (no config via web !)
 *                         log json=16, mail=2
 * 14.04.2015   2.18   fx2 setup mail via web interface
 * 14.04.2015   2.19   fx2 bug 'Enable=yes' in mail config fixed
 * 14.04.2015   2.20   fx2 allow 50 character len in mail config entries
 * 15.04.2015   2.21   fx2 smtp-password as '***' , enable as checkbox
 * 15.04.2015   2.22   fx2 pop3 function added
 * 17.04.2015   2.23   fx2 suppress bombing same message in log
 * 27.04.2015   2.24   fx2 fixup sourceforge
 * 28.04.2015   2.25   fx2 reconnect json if closed by remote
 * 28.04.2015   2.26   fx2 large file lead to frozen communication
 * 29.04.2015   2.27   aum (audimax) switch to advancend maps
 * 30.04.2015   2.28   fx2 accept additional user in smtp : 'from[,user]'
 * 04.05.2015   2.29   aum runtime fixes maps.html 
 * 13.05.2015   2.30   fx2/aum multiple mail receiver some fixes in map page
 * 15.05.2015   2.30a  aum edit xml values in webbrowser, system reboot
 * 17.05.2015   2.30b  aum error messeges for file loading refined
 * 17.05.2015   2.30c  aum added App.xml to editor
 * 18.05.2015   2.30d  aum added SLAM_control.xml to editor, on failed load restore defaults
*/

char *cstr = "lg.srv, V2.30d compiled 18.05.2015, by audimax";

int	debug = 0;		// increasing debug output (0 to 9)

static	SkLine	*listen_line=0;
static	SkLine	*cl_array[30];

static	int		noWrongSize=0;
static	int		maxlog=48;

	JsonVars	json;
	TimerVars	timer;
	MailVars	mail;

void	SetMaxLog( int nr )
{
	maxlog=nr;
}

static	void	_clientClose( SkLine *l, int pt, void *own, void *sys )
{
	int			i;
	int			numcl=0;

	free( l->data );
	l->data = 0;

	for( i=0; i < 30; i++ )
	{
		if ( cl_array[i] == l )
			cl_array[i] = 0;
		if ( cl_array[i] )
			numcl++;
	}
}

static	void	_clientData( SkLine *l, int pt, void *own, void *sys )
{
	SkPacket	*pck = sys;
	int			i;
	unsigned char	*data=(unsigned char*)pck->data;
#if 0
	unsigned int	ui;
	unsigned int	dst;
	unsigned short	us;
	unsigned long	ul;
	unsigned short	instr;
#endif

	if ( l->data->mode == MODE_UNKNOWN )
	{
//		if (( data[1] == 0x00 ) && ( data[2] == 0x00 ))
//			l->data->mode = MODE_LG;
//		else if ( !strncmp((char*)data,"GET /",5) )
		if ( !strncmp((char*)data,"GET /",5) )
			l->data->mode = MODE_HTTP;
		else if ( !strncmp((char*)data,"POST ",5) )
		{
			l->data->mode = MODE_HTTP;
			l->data->post_mode = 1;
		}
		else
			l->data->mode = MODE_LOG;
	}

	if ( l->data->mode == MODE_HTTP )
	{
		if ( !l->data->post_mode && !strncmp((char*)data,"POST ",5) )
			l->data->post_mode = 1;
		HttpPck( l, 0, own, sys );
		return;
	}
	if ( l->data->mode == MODE_LOG )
	{
		DoLogData( l, pck->data, pck->len );
		return;
	}

	HttpStatAddClData();

	Log(2,"_clientData: %d bytes (%d)\r\n",pck->len,l->data->cl_id);

	if ( pck->len > 20000 )
	{
		Log(2," # request too long ! - ignoring (wait for fault)\r\n");
		noWrongSize++;
		skDisconnect( l );
		return;
	}

	for( i=0; i<pck->len && (i<maxlog); i++ )
	{
		if ( !(i%16) && i )
			Log(2,"\r\n");
		Log(2,"%02x ",data[i]);
	}
	Log(2,"\r\n");

	l->data->num_wrong_frames++;
	return;
}

void	_clientConnected( SkLine *cl, int pt, void *own, void *sys )
{
	int		i;

	for( i=0; i < 30; i++ )
	{
		if ( cl_array[i] == 0 )
			break;
	}
	if ( i == 30 )
	{
		/* too many clients : disconnect */
		Log(1,"too many clients - disconnect client\r\n");
		skDisconnect( cl );
		return;
	}
	cl_array[i] = cl;

	skAddHandler( cl, SK_H_READABLE, _HReadable, 0 );
	skAddHandler( cl, SK_H_PACKET, _clientData, 0 );
	skAddHandler( cl, SK_H_CLOSE, _clientClose, 0 );

	cl->data = malloc( sizeof(struct _ClientData) );
	memset(cl->data,0, sizeof(struct _ClientData) );

	cl->data->cl_id = i;
	Log(1,"new connection : clients (%d)\r\n",i);
}

static	char	**g_av=0;
static	int		g_ac=0;

/*
 * ------------------------------------------------------------------------
 */
int main( int argc, char ** argv )
{
	int		i;
	int		fg=0;
	char	*cmdname;
	char	*port	= "6260";
#if 0
	int		novideo=1;
#endif


	cmdname=strrchr(*argv,'/');
	if ( cmdname )
		cmdname ++;
	else
		cmdname = *argv;

	g_av=argv;
	g_ac=argc;

	i=0;

	if (( argc>1 ) && !strcmp(argv[1],"-version"))
		i=1;

	memset( cl_array, 0, sizeof(cl_array) );

	for( i=1; i < argc; i++ )
	{
		if ( (i<argc-1) && !strcmp(argv[i],"-port") )
		{
			port = strdup(argv[++i]);
		}
		else if ( !strcmp(argv[i],"-debug") )
		{
			debug = 1;
		}
		else if ( !strncmp(argv[i],"-debug=",7) )
		{
			debug = atoi(argv[i]+7);
		}
		else if ( !strcmp(argv[i],"-fg") )
		{
			fg = 1;
		}
#if 0
		else if ( !strcmp(argv[i],"+video") )
		{
			novideo=0;
		}
#endif
		else if ( !strcmp(argv[i],"-version") )
		{
			printf("%s\n",cstr);
			exit(0);
		}
		else
		{
			printf("usage: %s [-port <nr>] [-debug=mask] [-fg]\n", *argv);
			printf("  debug-mask :  1 : quiet debug\n");
			printf("  debug-mask :  2 : send mail\n");
			printf("  debug-mask :  4 : timer\n");
			printf("  debug-mask :  8 : http traffic\n");
			printf("  debug-mask : 16 : json traffic\n");
			return 1;
		}
	}

	signal( SIGCHLD, SIG_IGN );
	signal( SIGPIPE, SIG_IGN );
	signal( SIGHUP, SIG_IGN );

	memset(&json,0,sizeof(JsonVars));
	memset(&timer,0,sizeof(TimerVars));
	memset(&mail,0,sizeof(MailVars));

	ReadTimerFromFile();
	ReadMailConfigFromFile();
	HttpLoadCleaningRecord(0,0);

	system("ifconfig lo up");

#if 0
	if ( !novideo )
		_v4l2_start();
	else
		HttpSetNoCam();
#endif

	if ( !fg && fork() )
		return 0;

	listen_line = _lgListen( atol(port) );

	StartTimer();

	jsonSend(0);

	skMainLoop();

	return( 0 );
}

static	void	WriteStat( SkLine *l, int ashtml, char *fmt, ... )
{
	va_list		args;
	char		out[2048];

	va_start( args, fmt );
	vsprintf( out, fmt, args );
	va_end( args );

	_WriteString(l,out);
	if ( ashtml )
		_WritePacket(l,(unsigned char*)"<BR>",4);
	else
		_WritePacket(l,(unsigned char*)"\r\n",2);
}

void	_ShowStatistic( SkLine *l, int ashtml )
{
	int		i;
	int		numcl=0;

	for( i=0; i < 30; i++ )
	{
		if ( cl_array[i] != 0 )
			numcl++;
	}
	WriteStat(l,ashtml,"Clients connected (all)   : %d",numcl);
	for( i=0; i<30; i++ )
	{

		if ( cl_array[i] == 0 )
			continue;
		switch( cl_array[i]->data->mode )
		{
		case MODE_UNKNOWN :
			WriteStat(l,ashtml," [%d] - protocol             : ?",i);
			break;
		case MODE_LOG :
			WriteStat(l,ashtml," [%d] - protocol             : LOG",i);
			break;
		case MODE_HTTP :
			WriteStat(l,ashtml," [%d] - protocol             : HTTP",i);
			break;
		}
	}
	WriteStat(l,ashtml,"noWrongSize (>20kBytes)   : %d",noWrongSize);
	WriteStat(l,ashtml,"MaxLog (def:48)           : %d",maxlog);
}

void	_ResetStatistik( void )
{
	int		i;

	noWrongSize=0;
	for( i=0; i < 30; i++ )
	{
		if ( cl_array[i] != 0 )
		{
			cl_array[i]->data->num_wrong_frames=0;
		}
	}
}

void	_RestartMe( void )
{
	unsigned short	port=0;

	if ( listen_line )
	{
		port=listen_line->port;
		skDisconnect( listen_line );
	}
	listen_line=0;

	execvp( "/usr/bin/lg.srv", g_av );

	listen_line = _lgListen( port ? port : 6260 );
}
