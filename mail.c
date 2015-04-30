#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <malloc.h>
#include <net/if.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdarg.h>
#include <unistd.h>

#include <openssl/ssl.h>

#include "NetDrv.h"

extern	JsonVars		json;
extern	MailVars		mail;

static	unsigned short	slport=0;
static	unsigned short	gport=0;

int sendMail( char *subject, char *text )
{
	int				fd;
	int				state=1;
	char			buffer[4096];
	struct sockaddr_in	insock;
	char			*ip, *p;
	char			*domain=0;
	struct hostent	*he;
	char			*bptr;
	int				x;
	int				use_ehlo=1;
	fd_set			rMask;
	struct timeval	tv;
	unsigned short	defport=465;
	SSL				*ssl=0;
	SSL_CTX			*ctx=0;
	SSL_METHOD		*method=0;
	char			luser[128];
	char			*auth_user=luser;

	if ( !mail.send.receiver || !mail.send.gateway ||
		!mail.send.user || !mail.send.pass )
			return -1;

	if ( strlen(mail.send.user) > 127 )
		return -1;

	strcpy(luser,mail.send.user);
	p=strchr(luser,',');
	if ( p )
	{
		*p=0;
		auth_user=p+1;
	}

	if ( !slport )
		slport=defport;

	domain = strchr(luser,'@');
	if (!domain)
		return -1;

	OpenSSL_add_all_algorithms();
	if(SSL_library_init() < 0)
		return -2;

	method = SSLv23_client_method();
	if ( method )
		ctx = SSL_CTX_new(method);
	if ( ctx )
	{
		SSL_CTX_set_options(ctx,SSL_OP_NO_SSLv2);
		ssl = SSL_new(ctx);
	}
	if ( !ssl )
		return -3;

	domain++;

	strcpy(buffer,mail.send.gateway);
	ip=strchr(buffer,':');
	if ( ip )
		*ip=0;
	he=gethostbyname( buffer );
	if ( !he )
		return -4;

	ip = malloc(sizeof(struct in_addr));
	memcpy(ip,he->h_addr,4);
	fd = socket( AF_INET, SOCK_STREAM, 0 );
	if ( fd == -1 )
		return -5;

	insock.sin_family = AF_INET;
	insock.sin_port = htons(slport);
	memcpy((char*)&insock.sin_addr,(char*)ip,sizeof(struct in_addr));

	if( connect(fd, (struct sockaddr*)&insock,
                 sizeof(struct sockaddr_in)) != 0 )
	{
		close(fd);
		return -6;
	}

	SSL_set_fd(ssl,fd);
	if ( SSL_connect(ssl)!=1)
	{
		close(fd);
		return -7;
	}

	bptr=buffer;
	while( 1 )
	{
		if ( bptr == buffer )
			memset(buffer,0,4096);

		x=SSL_read(ssl,bptr,1024);
		if ( x < 0 )
		{
			close(fd);
			SSL_free(ssl);
			return state >= 8 ? 0 : -8;
		}

		FD_ZERO(&rMask);
		FD_SET(fd,&rMask);
		tv.tv_sec=0;
		tv.tv_usec=100000;
		x=select( fd+1, &rMask, 0, 0, &tv );
		if ( x == 0 )
		{
			bptr=buffer;
		}
		else
		{
			bptr+=x;
			if ( bptr > buffer+3070 )
			{
				close(fd);
				SSL_free(ssl);
				return state >= 8 ? 0 : -9;
			}

			continue;
		}
		if ( *buffer == '5' )	/* 5xx */
		{
			Log(2,"sendMail: request not accepted [%d] (%s)\r\n",state,buffer);
			return -10;
		}

		switch( state )
		{
		case 1:
			if ( *buffer == '2' )	/* 220 */
			{
				if ( use_ehlo )
					sprintf(buffer,"EHLO %s\r\n",domain);
				else
					sprintf(buffer,"HELO %s\r\n",domain);

				SSL_write(ssl,buffer,strlen(buffer));
				state=2;
			}
			break;
		case 2 :
			if ( *buffer == '2' )	/* 250 */
			{
				SSL_write(ssl,"AUTH LOGIN\r\n",12);
				state=3;
			}
			break;
		case 3 :
			if ( !strncmp(buffer,"334 ",4) )
			{
				size_t	sz;
				char	tg[512];

				sz=base64_decode(buffer+4, (unsigned char*)tg, 512);
				if ( !sz )
					return -11;
				tg[sz]=0;
				if ( !strcasecmp("Username:",tg) )
				{
					if ( !base64_encode((unsigned char*)auth_user, strlen(auth_user), tg, 512) )
						return -12;

					SSL_write(ssl,tg,strlen(tg));
					SSL_write(ssl,"\r\n",2);
					break;
				}
				else if ( !strcasecmp("Password:",tg) )
				{
					if ( !base64_encode((unsigned char*)mail.send.pass, strlen(mail.send.pass), tg, 512) )
						return -13;

					SSL_write(ssl,tg,strlen(tg));
					SSL_write(ssl,"\r\n",2);
					break;
				}
				else
				{
					Log(2,"sendMail: unknown request (%s)\r\n",tg);
				}
			}
			else if ( *buffer == '2' )	/* 250 */
			{
				sprintf(buffer,"MAIL FROM:<%s>\r\n",luser);
				SSL_write(ssl,buffer,strlen(buffer));
				state=4;
			}
			else
			{
				Log(2,"sendMail: unknown auth-response :%s\r\n",buffer);
				return -14;
			}
			break;
		case 4 :
			if ( *buffer == '2' )	/* 250 */
			{
				sprintf(buffer,"RCPT TO:<%s>\r\n",mail.send.receiver);
				SSL_write(ssl,buffer,strlen(buffer));
				state=5;
			}
			break;
		case 5 :
			if ( *buffer == '2' )	/* 250 */
			{
				SSL_write(ssl,"DATA\r\n",6);
				state=7;
			}
			break;
		case 7 :
			if (( *buffer == '2' ) || ( *buffer == '3'))	/* 250,354 */
			{
				int		len;

				sprintf(buffer,"From: %s\r\nTo: %s\r\nSubject: %s\r\n\r\n",
					luser,mail.send.receiver,subject);
				SSL_write(ssl,buffer,strlen(buffer));
				len=strlen(text);
				SSL_write(ssl,text,len);
				SSL_write(ssl,"\r\n.\r\n",5);
				state=8;
			}
			break;
		case 8 :
			if ( *buffer == '2' )	/* 250 */
			{
				SSL_write(ssl,"QUIT\r\n",6);
				state=9;
			}
			break;
		case 9 :
			SSL_free(ssl);
			close(fd);
			return 0;
		}
	}
	return( 0 );
}

#define MAXLEN	2048
static	char		*FNAME="/usr/data/htdocs/mail.cfg";
static	char		oplog[MAXLEN];
static	int			oplog_len=0;
static	SkTimerType	snd_tid=0;

static	void	_sendTimer( SkTimerType tid, void *own )
{
	char	subject[512];
	int		x;

	snd_tid=0;
	if ( !mail.send.enable )
		return;

	sprintf(subject,"%s: operation log",json.nickname?json.nickname:"LuiGi");
	Log(2,"sendMail: send a mail to %s, subject='%s'\r\n",mail.send.receiver?mail.send.receiver:"?",subject);
	x=sendMail( subject, oplog );

	Log(2,"sendMail: return code %d\r\n",x);

	oplog_len=0;
}

static	int		cur_state=0;

static struct
{
	char	*state;
	int		id;
} StateTypes[] = {
{ "UNKNOWN",			0	},
{ "BACKMOVING",			1	},
{ "BACKMOVING_INIT",	2	},
{ "WORKING",			3	},
{ "HOMING",				4	},
{ "DOCKING",			5	},
{ "CHARGING",			10	},	/* do not change this number ! */
{ 0,					99  } };

void	SndMailAddLog( int to_response, char *fmt, ... )
{
	va_list			args;
	char			out[1024];
	char			buff[1024];
	struct tm		tm;
	struct timeval	tv;
	int				txtlen;

	va_start( args, fmt );
	vsprintf( out, fmt, args );
	va_end( args );

	gettimeofday(&tv,0);
	localtime_r(&tv.tv_sec,&tm);
	sprintf(buff,"%d/%02d/%02d %d:%02d:%02d : %s\r\n",
			tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,
			tm.tm_hour,tm.tm_min,tm.tm_sec,out);
	txtlen=strlen(buff);
	Log(2,"sendMail: [%2d] %s",cur_state,buff);

	if ( to_response )
	{
		if ( mail.response )
			free(mail.response);
		mail.response = strdup(buff);
	}

	if (( mail.send.enable ) && ( txtlen < MAXLEN-1-oplog_len ))
	{
		strcpy(oplog+oplog_len,buff);
		oplog_len+=txtlen;
	}
}

void	NewState( char *stext )
{
	int				i;
	int				xtime=0;
	int				old_state=cur_state;

	for( i=1; StateTypes[i].state; i++ )
	{
		if ( !strcmp(StateTypes[i].state,stext) )
			break;
	}

	if (( StateTypes[i].id != 99 ) &&
		StateTypes[i].id == cur_state )
			return;
	cur_state = StateTypes[i].id;

	SndMailAddLog( 0, stext );

	if ( cur_state == 99 )
		xtime=180*1000;
	else if (( cur_state == 10 ) && old_state )
		xtime=10;
	else if (( cur_state != 99 ) && ( old_state == 99 ) && snd_tid )
	{
		Log(2,"sendMail: send timer deactivated\r\n");
		skRemoveTimer( snd_tid );
		snd_tid=0;
	}

	if ( xtime )
	{
		if ( snd_tid )
			skRemoveTimer( snd_tid );
		Log(2,"sendMail: send timer activated (%d msec)\r\n",xtime);
		snd_tid=skAddTimer( xtime, _sendTimer, 0 );
		return;
	}
}

static	int	WriteMailCfgToFile( void )
{
	FILE	*fp;

	fp=fopen(FNAME,"w");
	if ( !fp )
		return -1;

	if( mail.send.receiver && strlen(mail.send.receiver)) fprintf(fp,"RECEIVER=%s\r\n",mail.send.receiver);
	if( mail.send.gateway && strlen(mail.send.gateway)) fprintf(fp,"GATEWAY=%s\r\n",mail.send.gateway);
	if( mail.send.user && strlen(mail.send.user)) fprintf(fp,"USER=%s\r\n",mail.send.user);
	if( mail.send.pass && strlen(mail.send.pass)) fprintf(fp,"PASS=%s\r\n",mail.send.pass);
	fprintf(fp,"SENDLOG=%s\r\n",mail.send.enable?"yes":"no");
	if( mail.get.gateway && strlen(mail.get.gateway)) fprintf(fp,"P3SERVER=%s\r\n",mail.get.gateway);
	if( mail.get.user && strlen(mail.get.user)) fprintf(fp,"P3USER=%s\r\n",mail.get.user);
	if( mail.get.pass && strlen(mail.get.pass)) fprintf(fp,"P3PASS=%s\r\n",mail.get.pass);
	if( mail.get.sign && strlen(mail.get.sign)) fprintf(fp,"P3SIGN=%s\r\n",mail.get.sign);
	fprintf(fp,"P3ENABLE=%s\r\n",mail.get.enable?"yes":"no");
	fprintf(fp,"P3CYCLE=%d\r\n",mail.get.cycle);
	fclose(fp);

	return 0;
}

static struct
{
	int		rc;
	char	*text;
} errtrans[] = {
{   0, "successful" },
{  -1, "err: invalid config" },
{  -2, "err: init ssl-lib" },
{  -3, "err: set ssl options" },
{  -4, "err: resolve hostname" },
{  -5, "err: create socket" },
{  -6, "err: connect smtp/pop server" },
{  -7, "err: setup ssl on socket" },
{  -8, "err: read from smtp server" },
{  -9, "err: answer too long" },
{ -10, "err: received code 5xx" },
{ -11, "err: decode auth request" },
{ -12, "err: encode user string" },
{ -13, "err: encode pass string" },
{ -14, "err: unknown auth" },
{ -15, "err: initial +OK expected" },
{ -16, "err: user +OK expected" },
{ -17, "err: pass +OK expected" },
{ -18, "err: list +OK expected" },
{ -19, "err: retr +OK expected" },
{ -20, "err: dele +OK expected" },
{ -21, "err: too many mails" },
{   0, 0 } };

static char *MErrText( int rc )
{
	int	i;
	for( i=0; errtrans[i].text; i++ )
	{
		if ( errtrans[i].rc == rc )
			return errtrans[i].text;
	}
	return "?";
}

static SkTimerType tid_start=0;
static void _timedStart( SkTimerType tid, void *own )
{
	tid_start=0;
	jsonSend( "{\"COMMAND\":\"CLEAN_START\"}" );
}

static	int getMail( int bg  )
{
	int				fd;
	int				state=1;
	char			buffer[4096];
	struct sockaddr_in	insock;
	char			*ip;
	struct hostent	*he;
	char			*bptr;
	int				x;
	int				msglist[ 100 ];
	int				nummsglist=0;
	int				curmsg=0;
	int				wait4sub=1;
	SSL				*ssl=0;
	SSL_CTX			*ctx=0;
	SSL_METHOD		*method=0;
	int				dodelete=0;
	char			*subject_value=0;
	char			*from_value=0;
	unsigned short	defport=995;

	if ( !mail.get.sign || !mail.get.gateway ||
		!mail.get.user || !mail.get.pass )
			return -1;

	if ( !gport )
		gport=defport;

	OpenSSL_add_all_algorithms();
	if(SSL_library_init() < 0)
		return -2;

	method = SSLv23_client_method();
	if ( method )
		ctx = SSL_CTX_new(method);
	if ( ctx )
	{
		SSL_CTX_set_options(ctx,SSL_OP_NO_SSLv2);
		ssl = SSL_new(ctx);
	}
	if ( !ssl )
		return -3;

	strcpy(buffer,mail.get.gateway);
	ip=strchr(buffer,':');
	if ( ip )
		*ip=0;
	he=gethostbyname( buffer );
	if ( !he )
		return -4;

	ip = malloc(sizeof(struct in_addr));
	memcpy(ip,he->h_addr,4);
	fd = socket( AF_INET, SOCK_STREAM, 0 );
	if ( fd == -1 )
		return -5;

	insock.sin_family = AF_INET;
	insock.sin_port = htons(gport);
	memcpy((char*)&insock.sin_addr,(char*)ip,sizeof(struct in_addr));

	if( connect(fd, (struct sockaddr*)&insock,
				 sizeof(struct sockaddr_in)) != 0 )
	{
		close(fd);
		return -6;
	}
	SSL_set_fd(ssl,fd);
	if(SSL_connect(ssl) != 1)
	{
		close(fd);
		return -7;
	}

	bptr=buffer;

	while( 1 )
	{
		if ( state == 100 )
			break;
		if ( bptr == buffer )
			memset(buffer,0,4096);

		x=SSL_read(ssl,bptr,1);
		if ( x < 0 )
		{
			close(fd);
			SSL_free(ssl);
			return state == 8 ? 0 : -1;
		}
		if ( *bptr == 0x0a )
			bptr=buffer;
		else
		{
			bptr++;
			continue;
		}
		skTimeoutStep(10);

		switch( state )
		{
		case 1:
			if ( !strncmp(buffer,"+OK",3) ) /*  */
			{
				sprintf(buffer,"USER %s\r\n",mail.get.user);
				SSL_write(ssl,buffer,strlen(buffer));
				state=2;
			}
			else
			{
				close(fd);
				SSL_free(ssl);
				return -15;
			}
			break;
		case 2 :
			if ( strncmp(buffer,"+OK",3) ) 	/*  */
			{
				close(fd);
				SSL_free(ssl);
				return -16;
			}

			sprintf(buffer,"PASS %s\r\n",mail.get.pass);
			SSL_write(ssl,buffer,strlen(buffer));
			state=3;
			break;
		case 3 :
			if ( strncmp(buffer,"+OK",3) ) 	/*  */
			{
				close(fd);
				SSL_free(ssl);
				return -17;
			}
			strcpy(buffer,"LIST\r\n");
			SSL_write(ssl,buffer,strlen(buffer));
			state=4;
			break;
		case 4 :
			if ( strncmp(buffer,"+OK",3) ) 	/*  */
			{
				close(fd);
				SSL_free(ssl);
				return -18;
			}
			state=5;
			break;
		case 5:	/* read list lines */
			if ( strcmp(buffer,".\r\n"))	/* done */
			{
				char	*p;
				p = strchr(buffer,' ');
				if ( p )
					*p=0;
				msglist[ nummsglist++ ] = atol(buffer);
				if ( nummsglist == 100 )
				{
					close(fd);
					SSL_free(ssl);
					return -21;
				}
				break;
			}
			if ( !nummsglist )	/* nothing there */
			{
				strcpy(buffer,"QUIT\r\n");
				SSL_write(ssl,buffer,strlen(buffer));
				state=99;
				break;
			}
			curmsg=0;
			sprintf(buffer,"RETR %d\r\n",msglist[ curmsg ]);
			SSL_write(ssl,buffer,strlen(buffer));
			state=6;
			break;
		case 6:
			if ( strncmp(buffer,"+OK",3) ) 	/*  */
			{
				close(fd);
				SSL_free(ssl);
				return -19;
			}
			state=7;
			break;
		case 7:	/* read file lines */
			if ( !strcmp(buffer,".\r\n"))	/* done */
			{
				wait4sub=1;
				if ( subject_value )
				{
					char	*pt;
					SndMailAddLog( 0, "MAIL received\r\n%s%s",subject_value,from_value?from_value:"" );
					pt=strstr(subject_value,"command=");
					if ( pt )
					{
						pt+=8;
						if ( !strncasecmp(pt,"START",5) )
						{
							int			time=0;
							int			rc;
							char		subject[512];
							char		xlog[1024];

							SkTimerType	tid;
							if ( *(pt+5) == ',' )
								time+=atoi(pt+6)*60000;
							tid=skAddTimer( time?time:10, _timedStart, 0 );
							if ( !tid_start )
								tid_start=tid;

							sprintf(subject,"%s: START command response",json.nickname?json.nickname:"LuiGi");
							Log(2,"sendMail: send a mail to %s, subject='%s'\r\n",mail.send.receiver?mail.send.receiver:"?",subject);
							sprintf(xlog,"cleaning initiated via email\r\n%s%s",subject_value,from_value?from_value:"");
							rc=sendMail( subject, xlog );
							SndMailAddLog( 0, "send response mail: rc = %d  (%s)", rc,MErrText(rc) );
						}
					}
					free(subject_value);
					subject_value=0;
				}
				if ( from_value )
					free(from_value);
				from_value=0;
				if ( dodelete )
				{
					sprintf(buffer,"DELE %d\r\n",msglist[ curmsg ]);
					SSL_write(ssl,buffer,strlen(buffer));
					state=8;
					dodelete=0;
					break;
				}
				dodelete=0;
				curmsg++;
				if ( curmsg == nummsglist )	/* thats all */
				{
					strcpy(buffer,"QUIT\r\n");
					SSL_write(ssl,buffer,strlen(buffer));
					state=99;
					break;
				}
				sprintf(buffer,"RETR %d\r\n",msglist[ curmsg ]);
				SSL_write(ssl,buffer,strlen(buffer));
				state=6;
			}
			if ( !strcmp(buffer,"\r\n") )
			{
				wait4sub=0;
			}
			if ( mail.get.sign && wait4sub && !strncmp(buffer,"Subject:",8) )
			{
				char	*pt = strstr(buffer,mail.get.sign);
				wait4sub=0;
				if ( pt )
				{
					dodelete=1;
					subject_value=strdup(buffer);
				}
			}
			else if ( !from_value && !strncmp(buffer,"From:",5) )
			{
				from_value=strdup(buffer);
			}
			break;
		case 8:
			if ( strncmp(buffer,"+OK",3) ) 	/* delete retcode */
			{
				close(fd);
				SSL_free(ssl);
				return -20;
			}
			curmsg++;

			if ( curmsg == nummsglist )	/* thats all */
			{
				strcpy(buffer,"QUIT\r\n");
				SSL_write(ssl,buffer,strlen(buffer));
				state=99;
				break;
			}
			sprintf(buffer,"RETR %d\r\n",msglist[ curmsg ]);
			SSL_write(ssl,buffer,strlen(buffer));
			state=6;
			break;
		case 99 :
			SSL_free(ssl);
			close(fd);
			state++;
			break;
		}
	}

	return( 0 );
}

static	SkTimerType	tid_recv=0;

static void _cycleGet( SkTimerType tid, void *own )
{
	int rc;
static int lastrc=0;

	tid_recv=0;

	if ( !mail.get.enable )
		return;

	if ( mail.get.cycle )
		tid_recv=skAddTimer( mail.get.cycle*60000, _cycleGet, 0 );

	rc= getMail( 0 );

	if ( rc != lastrc )
		SndMailAddLog( 0, "  rc = %d  (%s)", rc,MErrText(rc) );

	lastrc=rc;
}

void	ReadMailConfigFromFile( void )
{
	FILE	*fp;
	char	buffer[512];
	char	*p;
	char	*old_response=mail.response;
	int		old_cycle=mail.get.cycle;

	fp=fopen(FNAME,"r");

	if ( mail.send.receiver )
		free(mail.send.receiver);
	if ( mail.send.user )
		free(mail.send.user);
	if ( mail.send.pass )
		free(mail.send.pass);
	if ( mail.send.gateway )
		free(mail.send.gateway);

	if ( mail.get.user )
		free(mail.get.user);
	if ( mail.get.pass )
		free(mail.get.pass);
	if ( mail.get.gateway )
		free(mail.get.gateway);
	if ( mail.get.sign )
		free(mail.get.sign);

	memset(&mail,0,sizeof(MailVars));
	slport=0;
	gport=0;
	mail.response = old_response;

	if ( !fp )
	{
		strcpy(buffer,FNAME);
		p=strrchr(buffer,'/');
		if ( p )
		{
			*p=0;
			mkdir(buffer,0777);
		}
		return;
	}


	while(fgets(buffer,512,fp))
	{
		if ( *buffer == '#' )
			continue;
		p=strchr(buffer,'\r');
		if ( p ) *p=0;
		p=strchr(buffer,'\n');
		if ( p ) *p=0;
		p=strchr(buffer,'=');
		if ( !p )
			continue;
		*p=0;
		if ( !strcmp(buffer,"RECEIVER") )
			mail.send.receiver=strdup(p+1);
		else if ( !strcmp(buffer,"GATEWAY") )
			mail.send.gateway=strdup(p+1);
		else if ( !strcmp(buffer,"USER") )
			mail.send.user=strdup(p+1);
		else if ( !strcmp(buffer,"PASS") )
			mail.send.pass=strdup(p+1);
		else if ( !strcmp(buffer,"SENDLOG") )
			mail.send.enable=strcasecmp(p+1,"YES") ? 0 : 1;
		else if ( !strcmp(buffer,"P3SERVER") )
			mail.get.gateway=strdup(p+1);
		else if ( !strcmp(buffer,"P3USER") )
			mail.get.user=strdup(p+1);
		else if ( !strcmp(buffer,"P3PASS") )
			mail.get.pass=strdup(p+1);
		else if ( !strcmp(buffer,"P3SIGN") )
			mail.get.sign=strdup(p+1);
		else if ( !strcmp(buffer,"P3ENABLE") )
			mail.get.enable=strcasecmp(p+1,"YES") ? 0 : 1;
		else if ( !strcmp(buffer,"P3CYCLE") )
			mail.get.cycle=atoi(p+1);
	}
	fclose(fp);

	if ( mail.send.gateway )
	{
		char *p=strchr(mail.send.gateway,':');
		if ( p )
		{
			slport=atoi(p+1);
		}
	}
	if ( mail.get.gateway )
	{
		char *p=strchr(mail.get.gateway,':');
		if ( p )
		{
			gport=atoi(p+1);
		}
	}
	if ( mail.get.cycle != old_cycle )
	{
		if ( tid_recv )
			skRemoveTimer( tid_recv );
		tid_recv=0;
		if ( mail.get.cycle && mail.get.enable )
			tid_recv=skAddTimer( mail.get.cycle*60000, _cycleGet, 0 );
	}
}

void RunMailCfgParam( char *param)
{
	char	*p, *p2;
	char	*pin=param;
	char	*old_response=mail.response;
	int		action=0;
	int		old_cycle=mail.get.cycle;

	if ( mail.send.receiver )
		free(mail.send.receiver);
	if ( mail.send.user )
		free(mail.send.user);
	if ( mail.send.pass )
		free(mail.send.pass);
	if ( mail.send.gateway )
		free(mail.send.gateway);

	if ( mail.get.user )
		free(mail.get.user);
	if ( mail.get.pass )
		free(mail.get.pass);
	if ( mail.get.gateway )
		free(mail.get.gateway);
	if ( mail.get.sign )
		free(mail.get.sign);

	memset(&mail,0,sizeof(MailVars));
	slport=0;
	gport=0;
	mail.response = old_response;

	while(1)
	{
		p=strchr(pin,'&');
		if ( p )
			*p=0;
		p2=strchr(pin,'=');
		if ( !p2 )
		{
			if ( p )
			{
				pin=p+1;
				continue;
			}
			break;
		}
		*p2=0;
		if ( strlen(p2+1) )
		{
			if ( !strcmp(pin,"RECEIVER") )
				mail.send.receiver=strdup(p2+1);
			else if ( !strcmp(pin,"GATEWAY") )
				mail.send.gateway=strdup(p2+1);
			else if ( !strcmp(pin,"USER") )
				mail.send.user=strdup(p2+1);
			else if ( !strcmp(pin,"PASS") )
				mail.send.pass=strdup(p2+1);
			else if ( !strcmp(pin,"SLENABLE") )
				mail.send.enable=strcasecmp(p2+1,"YES") ? 0 : 1;
			else if ( !strcmp(pin,"ACTION") )
			{
				action=0;
				if ( !strcasecmp(p2+1,"SAVE") )
					action=1;
				else if ( !strcasecmp(p2+1,"RECV") )
					action=2;
			}
			else if ( !strcmp(pin,"P3SERVER") )
				mail.get.gateway=strdup(p2+1);
			else if ( !strcmp(pin,"P3USER") )
				mail.get.user=strdup(p2+1);
			else if ( !strcmp(pin,"P3PASS") )
				mail.get.pass=strdup(p2+1);
			else if ( !strcmp(pin,"P3SIGN") )
				mail.get.sign=strdup(p2+1);
			else if ( !strcmp(pin,"P3ENABLE") )
				mail.get.enable=strcasecmp(p2+1,"YES") ? 0 : 1;
			else if ( !strcmp(pin,"P3CYCLE") )
				mail.get.cycle=atoi(p2+1);
		}
		if ( p )
		{
			pin=p+1;
			continue;
		}
		break;
	}
	if ( !mail.get.cycle )
		mail.get.enable=0;
	if ( mail.send.gateway )
	{
		char *p=strchr(mail.send.gateway,':');
		if ( p )
		{
			slport=atoi(p+1);
		}
	}
	if ( mail.get.gateway )
	{
		char *p=strchr(mail.get.gateway,':');
		if ( p )
		{
			gport=atoi(p+1);
		}
	}

	if ( action == 1 )
	{
		if ( mail.response )
			free ( mail.response );
		mail.response=0;
		WriteMailCfgToFile();
		if ( mail.get.cycle != old_cycle )
		{
			if ( tid_recv )
				skRemoveTimer( tid_recv );
			tid_recv=0;
			if ( mail.get.cycle && mail.get.enable )
				tid_recv=skAddTimer( mail.get.cycle*60000, _cycleGet, 0 );
		}
	}
	else if ( action == 0 )
	{
		int	rc;

		mail.get.cycle = old_cycle;
		rc= sendMail( "lg.srv control-center : Testmail", "successful\r\n" );

		SndMailAddLog( 1, "send test : rc = %d  (%s)", rc,MErrText(rc) );
	}
	else if ( action == 2 )
	{
		int	rc;

		mail.get.cycle = old_cycle;
		rc= getMail( 0 );

		SndMailAddLog( 1, "read mails : rc = %d  (%s)", rc,MErrText(rc) );
	}
}
