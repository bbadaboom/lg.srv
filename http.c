#include <NetDrv.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/stat.h>

#define	DESTPATH	"/usr/tmp/"
#define DESTLEN     9	/* strlen(DESTPATH) */

extern  char		    *cstr;
extern	int				debug;
extern	JsonVars		json;
extern	TimerVars		timer;
extern	MailVars		mail;

extern int	__t_open( char *fname );
extern unsigned long	__t_getsize( int fd );
extern int	__t_read( int fd, char *to, int sz );
extern int	__t_close( int fd );

static int  http_requests=0;
static int	sum_cmd=0;

#define hdr_response "HTTP/1.1 200 OK\r\n\
Server: luigi internal V1.0 (Linux)\r\n\
Connection: Keep-Alive\r\n\
Access-Control-Allow-Origin: *\r\n\
Keep-Alive: timeout=2, max=2000\r\n\
Content-Type: text/html\r\n\
\r\n"

#define hdr "HTTP/1.1 200 OK\r\n\
Server: luigi internal V1.0 (Linux)\r\n\
Connection: Keep-Alive\r\n\
Access-Control-Allow-Origin: *\r\n\
Keep-Alive: timeout=2, max=2000\r\n\
Content-Type: text/html\r\n\
\r\n\
<html><title>VR6260</title>"

#if 1
#define hdr_html_nsize "HTTP/1.1 200 OK\r\n\
Server: luigi internal V1.0 (Linux)\r\n\
Connection: Keep-Alive\r\n\
Access-Control-Allow-Origin: *\r\n\
Keep-Alive: timeout=2, max=2000\r\n\
Content-Type: text/html\r\n\
\r\n"
#endif

#define hdr_html "HTTP/1.1 200 OK\r\n\
Server: luigi internal V1.0 (Linux)\r\n\
Connection: Keep-Alive\r\n\
Access-Control-Allow-Origin: *\r\n\
Keep-Alive: timeout=2, max=2000\r\n\
Content-Type: text/html\r\n\
Content-Length: %ld\r\n\
\r\n"

#define hdr_js "HTTP/1.1 200 OK\r\n\
Server: luigi internal V1.0 (Linux)\r\n\
Connection: Keep-Alive\r\n\
Access-Control-Allow-Origin: *\r\n\
Keep-Alive: timeout=2, max=2000\r\n\
Content-Type: text/javascript\r\n\
Content-Length: %ld\r\n\
\r\n"

#define hdr_css "HTTP/1.1 200 OK\r\n\
Server: luigi internal V1.0 (Linux)\r\n\
Connection: Keep-Alive\r\n\
Access-Control-Allow-Origin: *\r\n\
Keep-Alive: timeout=2, max=2000\r\n\
Content-Type: text/css\r\n\
Content-Length: %ld\r\n\
\r\n"

#define hdr_png "HTTP/1.1 200 OK\r\n\
Server: luigi internal V1.0 (Linux)\r\n\
Connection: Keep-Alive\r\n\
Access-Control-Allow-Origin: *\r\n\
Keep-Alive: timeout=2, max=2000\r\n\
Content-Type: image/png\r\n\
Content-Length: %ld\r\n\
\r\n"

#define hdr_xpm "HTTP/1.1 200 OK\r\n\
Server: luigi internal V1.0 (Linux)\r\n\
Connection: Keep-Alive\r\n\
Access-Control-Allow-Origin: *\r\n\
Keep-Alive: timeout=2, max=2000\r\n\
Content-Type: image/xpm\r\n\
Content-Length: %ld\r\n\
\r\n"

#define hdr_gif "HTTP/1.1 200 OK\r\n\
Server: luigi internal V1.0 (Linux)\r\n\
Connection: Keep-Alive\r\n\
Access-Control-Allow-Origin: *\r\n\
Keep-Alive: timeout=2, max=2000\r\n\
Content-Type: image/gif\r\n\
Content-Length: %ld\r\n\
\r\n"

#define hdr_jpeg "HTTP/1.1 200 OK\r\n\
Server: luigi internal V1.0 (Linux)\r\n\
Connection: Keep-Alive\r\n\
Access-Control-Allow-Origin: *\r\n\
Keep-Alive: timeout=2, max=2000\r\n\
Content-Type: image/jpeg\r\n\
Content-Length: %ld\r\n\
\r\n"

#define hdr_txt "HTTP/1.1 200 OK\r\n\
Server: luigi internal V1.0 (Linux)\r\n\
Connection: Keep-Alive\r\n\
Access-Control-Allow-Origin: *\r\n\
Keep-Alive: timeout=2, max=2000\r\n\
Content-Type: text/plain\r\n\
Content-Length: %ld\r\n\
\r\n"

#define hdr_binary "HTTP/1.1 200 OK\r\n\
Server: luigi internal V1.0 (Linux)\r\n\
Connection: Keep-Alive\r\n\
Access-Control-Allow-Origin: *\r\n\
Keep-Alive: timeout=2, max=2000\r\n\
Content-Type: application/octet-stream\r\n\
Content-Length: %ld\r\n\
\r\n"

#define hdr_tgz "HTTP/1.1 200 OK\r\n\
Server: luigi internal V1.0 (Linux)\r\n\
Connection: Keep-Alive\r\n\
Access-Control-Allow-Origin: *\r\n\
Keep-Alive: timeout=2, max=2000\r\n\
Content-Type: application/octet-stream\r\n\
\r\n"

static	void	WriteSimpleHttp(SkLine *l, char *fmt, ...)
{
	va_list	args;
	char	buffer[1024];

	va_start(args,fmt);
	vsnprintf(buffer,1024,fmt,args);
	va_end(args);
	buffer[1023]=0;
	_WriteString(l,buffer);
}

static	struct timeval	ltv={0,0};

#if 0
static	void	WriteFile( SkLine *l, char *fname )
{
	char	buff[1024];
	char	t[16];
	FILE	*in;
	char	*p;

	in=fopen(fname,"r");
	if ( !in )
		return;
	WriteSimpleHttp(l,"<pre><code>\n");
	t[1]=0;
	while( fgets( buff, 1024, in ) )
	{
		p=strchr(buff,0x0a); if ( p ) *p=0;
		p=strchr(buff,0x0d); if ( p ) *p=0;
		for( p=buff; *p; p++ )
		{
			if ( *p == '<' )
				WriteSimpleHttp(l,"&lt");
			else if (*p == '>' )
				WriteSimpleHttp(l,"&gt");
			else if (*p == '&' )
				WriteSimpleHttp(l,"&amp");
			else
			{
				*t=*p;
				WriteSimpleHttp(l,t);
			}
		}
		WriteSimpleHttp(l,"\n");
	}
	fclose(in);
	WriteSimpleHttp(l,"</code></pre><br>\n");
}

static	void	SendDownloadMaps( SkLine *l )
{
	char			*bbl[ 20 ];
	char			*blk[ 20 ];
	int				num_bbl=0;
	int				num_blk=0;
	int				i;
	struct dirent	*dex;
	DIR				*d;
	int				nlen;

	d=opendir("/usr/data/blackbox");
	if ( !d )
	{
		WriteSimpleHttp( l, hdr "<body><H1>Maps</H1>" );

		WriteSimpleHttp( l, "<h4>no files found...</h4>" );
		return;
	}
	for( dex=readdir(d); dex; dex=readdir(d) )
	{
		nlen=strlen(dex->d_name);
		if ( nlen < 5 )
			continue;
		if ( (num_bbl < 20) && !strcmp(dex->d_name+nlen-4,".bbl") )
			bbl[num_bbl++]=strdup(dex->d_name);
		else if ( (num_blk < 20) && !strcmp(dex->d_name+nlen-4,".blk") )
			blk[num_blk++]=strdup(dex->d_name);
	}
	closedir(d);

	WriteSimpleHttp(l,hdr "<body><table><tr><th colspan=\"2\" allign=\"center\"><H1>Maps</H1></th></tr>");
	WriteSimpleHttp(l,"<tr><td width=\"110%\"><h1>bbl</h1></td>"
					  "<td width=\"110%\"><h1>blk</h1></td></tr>");
	for( i=0; i<20; i++ )
	{
		if (( i>=num_bbl ) && ( i>=num_blk ))
			break;
		WriteSimpleHttp(l, "<tr><td>" );
		if ( i<num_bbl )
		{
			WriteSimpleHttp(l,"<a href=\".../usr/data/blackbox/%s\">%s</a><br>",
				bbl[i],bbl[i] );
			free( bbl[i] );
		}
		WriteSimpleHttp(l, "</td><td>" );
		if ( i<num_blk )
		{
			WriteSimpleHttp(l,"<a href=\".../usr/data/blackbox/%s\">%s</a><br>",
				blk[i],blk[i] );
			free( blk[i] );
		}
		WriteSimpleHttp(l, "</td></tr>" );
	}
	WriteSimpleHttp(l,"</table>");
}

static	void	ShowMaps( SkLine *l )
{
	char			*bbl[ 20 ];
	char			*blk[ 20 ];
	int				num_bbl=0;
	int				num_blk=0;
	int				i;
	struct dirent	*dex;
	DIR				*d;
	int				nlen;

	d=opendir("/usr/data/blackbox");
	if ( !d )
	{
		WriteSimpleHttp( l, hdr "<body><H1>Blk-Maps</H1>" );

		WriteSimpleHttp( l, "<h4>no files found...</h4>" );
		return;
	}
	for( dex=readdir(d); dex; dex=readdir(d) )
	{
		nlen=strlen(dex->d_name);
		if ( nlen < 5 )
			continue;
		if ( 0 && (num_bbl < 20) && !strcmp(dex->d_name+nlen-4,".bbl") )
			bbl[num_bbl++]=strdup(dex->d_name);
		else if ( (num_blk < 20) && !strcmp(dex->d_name+nlen-4,".blk") )
			blk[num_blk++]=strdup(dex->d_name);
	}
	closedir(d);

	WriteSimpleHttp(l,hdr "<body><script type=\"text/JavaScript\" src=\"js/blkshow.js\"></script>\n<script>\nfunction loadall()\n{\n");



	for( i=0; i<20 && i<num_blk; i++ )
	{
		WriteSimpleHttp(l,"\tBlkShow('%s','BlkCanvas%d');\n", blk[i],i );
	}

	WriteSimpleHttp(l,"}\n\nwindow.onload = new Function(\"loadall();\");\n</script>\n");
	WriteSimpleHttp(l,"<H1>Maps</H1>");
	for( i=0; i<20 && i<num_blk; i++ )
	{
		WriteSimpleHttp(l,"<canvas id=\"BlkCanvas%d\" width=\"200\" height=\"100\" style=\"border:1px solid #000000;\"></canvas>",i);
		free( blk[i] );
	}
	WriteSimpleHttp( l, "</body></html>");
}
#endif

static	char		dropb_version[512] = { 0,0 };
static	float		cpu_idle=0;
static	float		cpu_user=0;
static	float		cpu_sys=0;
static	float		cpu_nice=0;
static	float		sum_cmd_per_sec=0;
static	struct tm	tm;

static	void	PrepareVars( void )
{
	struct timeval tv;
	char		buff[1024];
	int			a_user, a_sys, a_nice, a_idle;

	gettimeofday(&tv,0);

	jsonSend(0);
	localtime_r(&tv.tv_sec,&tm);

	sum_cmd_per_sec = 0;

	if ( ltv.tv_sec )
	{
		float	secs=tv.tv_sec - ltv.tv_sec;
		int		usecs=0;
		if ( tv.tv_usec < ltv.tv_usec )
		{
			secs--;
			usecs=tv.tv_usec+1000000-ltv.tv_usec;
		}
		else
			usecs=tv.tv_usec-ltv.tv_usec;

		secs += (float)usecs/1000000;
		if ( secs )
			sum_cmd_per_sec = (float)sum_cmd/secs;
	}
	else
	{
		memcpy(&ltv,&tv,sizeof(tv));
		sum_cmd = 0;
	}

	if ( ! *dropb_version )
	{
		FILE	*fp;

		sprintf(buff," -version 2>&1");
		fp=popen("/usr/sbin/dropbearmulti -version 2>&1","r");
		if ( fp )
		{
			*dropb_version=0;
			fgets(dropb_version,512,fp);
			pclose(fp);
			if ( !strlen(buff) || !strstr(buff,"Dropbear") )
				*dropb_version='#';
		}
	}

	ReadProcStat( &a_user, &a_sys, &a_nice, &a_idle );

	cpu_idle = (float)a_idle/100;
	cpu_user = (float)a_user/100;
	cpu_sys = (float)a_sys/100;
	cpu_nice = (float)a_nice/100;

	skTimeoutStep(10);
}

#if 0
static	void	SendInfo( SkLine *l )
{
	WriteSimpleHttp( l, hdr "<body><H1>State</H1>" );
	WriteSimpleHttp( l, "(Version : %s)<BR><BR>",cstr );

	_ShowStatistic( l, 1 );

	WriteSimpleHttp( l, "Client Commands : %d<BR>",sum_cmd );

	if ( sum_cmd_per_sec )
		WriteSimpleHttp( l, "Cmd per second : %.3f<BR>",sum_cmd_per_sec );

	WriteSimpleHttp( l, "Http requests : %d<BR><BR>",http_requests );

	if ( *dropb_version != '#')
	{
		WriteSimpleHttp(l,dropb_version);
		WriteSimpleHttp(l,"<BR><BR>");
	}

	WriteSimpleHttp( l, "CPU-Idle : %.2f%%<br>\n",cpu_idle);
	WriteSimpleHttp( l, "CPU-User : %.2f%%<br>\n",cpu_user);
	WriteSimpleHttp( l, "CPU-Sys : %.2f%%<br>\n",cpu_sys);
	WriteSimpleHttp( l, "CPU-Nice : %.2f%%<br>\n",cpu_nice);
	WriteSimpleHttp( l, "-------- <a href=\".../usr/data/blackbox/cleaningrecord.stc\">cleaningrecord.stc</a> -------<br>" );
	WriteFile( l, "/usr/data/blackbox/cleaningrecord.stc" );

	WriteSimpleHttp( l, "<P><a href=\"reset_err\">reset counter</a><BR>" );
	WriteSimpleHttp( l, "<BR><a href=\"restart?lg.srv \">restart lg.srv now</a><BR>" );
}
#endif

static	void	SendUploaded( SkLine *l )
{
	char	*p;
	char	buff[512];
	int		wrong=1;

	WriteSimpleHttp( l, hdr_response "<h3>Upload finished</h3>" );
	p=l->data->filename + DESTLEN ;	/* behind /tmp/ */

	if (!strncmp(p,"lg.srv",6))
	{
		FILE	*fp;
		wrong=1;	

		sprintf(buff,"%s -version",l->data->filename);
		fp=popen(buff,"r");
		if ( fp )
		{
			*buff=0;
			fgets(buff,512,fp);
			pclose(fp);
			if ( strlen(buff) && strstr(buff,"lg.srv") )
			{
				WriteSimpleHttp(l,"<span id=\"fname\">%s</span> was uploaded.<br />",p);
				WriteSimpleHttp(l,"New software detected : %s<br />",buff);
				WriteSimpleHttp(l,"<input type=\"button\" id=\"install\" value=\"Install\"/>");
				wrong=0;
			}
		}
	}
	else if (!strncmp(p,"dropbearmulti",13))
	{
		FILE	*fp;
		wrong=1;	

		sprintf(buff,"%s -version 2>&1",l->data->filename);
		fp=popen(buff,"r");
		if ( fp )
		{
			*buff=0;
			fgets(buff,512,fp);
			pclose(fp);
			if ( strlen(buff) && strstr(buff,"Dropbear") )
			{
				WriteSimpleHttp(l,"<span id=\"fname\">%s</span> was uploaded.<br>",p);
				WriteSimpleHttp(l,"New software detected &gt %s",buff);
				WriteSimpleHttp(l,"<input type=\"button\" id=\"install\" value=\"Install\"/>");
				wrong=0;
			}
		}
	}
	else if (!strncmp(p,"status.html",11) ||
			!strncmp(p,"Motion.xml",10) ||
			!strncmp(p,"Navi.xml",8) ||
			!strncmp(p,"App.xml",7) ||
			!strncmp(p,"status.txt",10) ||
			!strncmp(p,"mail.cfg",8))
	{
		WriteSimpleHttp(l,"<span id=\"fname\">%s</span> was uploaded.<br>",p);
		WriteSimpleHttp(l,"<input type=\"button\" id=\"install\" value=\"Install\"/>");
		wrong=0;
	}

	if ( wrong )
		WriteSimpleHttp(l,"<span id=\"fname\">%s</span> was uploaded.<br><input type=\"button\" id=\"delete\" value=\"Delete\"/>",p);
}

static	void	SendUploadFail( SkLine *l )
{
	WriteSimpleHttp( l, hdr_response "<h3>Upload FAILED !</h3>" );
}

static	void	SendNoPage( SkLine *l )
{
	WriteSimpleHttp( l, hdr "<html><body>unknown REQUEST!</body></html>" );
}

static	int	movefile( char *from, char *to )
{
	int	fdi, fdo, x;
	char	buffer[4096];

	fdi=open( from,O_RDONLY );
	if ( fdi==-1 )
		return -1;
	fdo=open( to, O_WRONLY|O_CREAT, 0644 );
	if ( fdo==-1 )
	{
		close(fdi);
		return -1;
	}
	x=read( fdi, buffer, 4096 );
	while (  x > 0 )
	{
		x=write( fdo, buffer, x );
		if ( x < 0 )
		{
			close(fdi);
			close(fdo);
			return -1;
		}
		x=read( fdi, buffer, 4096 );
	}
	close(fdi);
	close(fdo);
	unlink( from );
	return 0;
}

static	int	DoActivate( SkLine *l, char *param )
{
	char	from[1024];
	int		failed=0;
	int		rc=0;
	int		reboot=0;

	sprintf(from,DESTPATH "/%s",param);

	if ( strstr(param,"lg.srv") )
	{
		char	*to="/usr/bin/lg.srv";
		rename(to,"/usr/bin/lg.srv.svd");
		if ( movefile( from, to ) )
			failed++;
		else if ( chmod( to, 0755 ) )
			failed++;
		if ( failed )
		{
			rename("/usr/bin/lg.srv.svd",to);
			unlink(from);
		}
		else
		{
			unlink("/usr/bin/lg.srv.svd");
			rc=1;
		}
	}
	else if ( strstr(param,"dropbear") )
	{
		char	*to="/usr/sbin/dropbearmulti";
		unlink(to);

		if ( movefile( from, to ) )
			failed++;
		else if ( chmod( to, 0755 ) )
			failed++;
		else
		{
			rc=2;
			system("rm /usr/bin/ssh; rm /usr/bin/scp; ln -s /usr/sbin/dropbearmulti /usr/bin/ssh; ln -s /usr/sbin/dropbearmulti /usr/bin/scp");
		}
	}
	else if ( strstr(param,"mail.cfg") )
	{
		char	*to="/usr/data/htdocs/mail.cfg";
		rename(to,"/usr/data/htdocs/mail.svd");
		if ( movefile( from, to ) )
		{
			failed++;
			rename("/usr/data/htdocs/mail.svd",to);
			unlink(from);
		}
		else
		{
			unlink("/usr/data/htdocs/mail.svd");
			rc=1;
		}
	}
	else if ( strstr(param,"status.txt") )
	{
		char	*to="/usr/data/htdocs/status.txt";
		rename(to,"/usr/data/htdocs/status.svd");
		if ( movefile( from, to ) )
		{
			failed++;
			rename("/usr/data/htdocs/status.svd",to);
			unlink(from);
		}
		else
		{
			unlink("/usr/data/htdocs/status.svd");
		}
	}
	else if ( strstr(param,"status.html") )
	{
		char	*to="/usr/data/htdocs/status.html";
		rename(to,"/usr/data/htdocs/status.svd");
		if ( movefile( from, to ) )
		{
			failed++;
			rename("/usr/data/htdocs/status.svd",to);
			unlink(from);
		}
		else
		{
			unlink("/usr/data/htdocs/status.svd");
		}
	}
	else if ( strstr(param,"Motion.xml") )
	{
		reboot++;
		char	*to="/usr/rcfg/Motion.xml";
		rename(to,"/usr/rcfg/Motion.svd");
		if ( movefile( from, to ) )
		{
			failed++;
			rename("/usr/rcfg/Motion.svd",to);
			unlink(from);
		}
		else
		{
			unlink("/usr/rcfg/Motion.svd");
		}
	}
	else if ( strstr(param,"Navi.xml") )
	{
		reboot++;
		char	*to="/usr/rcfg/Navi.xml";
		rename(to,"/usr/rcfg/Navi.svd");
		if ( movefile( from, to ) )
		{
			failed++;
			rename("/usr/rcfg/Navi.svd",to);
			unlink(from);
		}
		else
		{
			unlink("/usr/rcfg/Navi.svd");
		}
	}
	else if ( strstr(param,"App.xml") )
	{
		reboot++;
		char	*to="/usr/rcfg/App.xml";
		rename(to,"/usr/rcfg/App.svd");
		if ( movefile( from, to ) )
		{
			failed++;
			rename("/usr/rcfg/App.svd",to);
			unlink(from);
		}
		else
		{
			unlink("/usr/rcfg/App.svd");
		}
	}

	if ( failed ) {
		WriteSimpleHttp( l, hdr_response "<h3>Installation failed</h3>" );
	} else {
		if (reboot) {
		   WriteSimpleHttp( l, hdr_response "<h3>Installation successful</h3><br />You just uploaded a xml configuration file. <br />The file was copied to the correct location but your Hombot will only act to the new configuration after you reboot your system:<br /><input type=\"button\" id=\"reboot\" value=\"Reboot System\" />" );
      } else {
		   WriteSimpleHttp( l, hdr_response "<h3>Installation successful</h3><input type=\"button\" id=\"refresh\" value=\"Reload Page\" />" );
		}
   }
	return rc;
}

typedef struct _CRVar
{
	char	*name;
	int		sz;
	char	*value;
	struct _CRVar	*next;
} CRVar;

static	CRVar	*crv_root=0;
static	int 	num_open=0;
#define MAX_B_FILES	64
static	char	*bbl[ MAX_B_FILES ];
static	char	*blk[ MAX_B_FILES ];
static	int		num_bbl=0;
static	int		num_blk=0;

static int cstring_cmp(const void *a, const void *b) 
{ 
const char **ia = (const char **)a;
const char **ib = (const char **)b;
if ( !ib || !*ib )
	return 1;
if ( !ia || !*ia )
	return -1;
return strcmp(*ib, *ia);
}

static void LoadBlkFilenames( void )
{
	int				i;
	struct dirent	*dex;
	DIR				*d;
	int				nlen;

	d=opendir("/usr/data/blackbox");
	if ( !d )
		return;

	for( i=0; i<MAX_B_FILES; i++ )
	{
		if (( i < num_blk ) && blk[i] )
			free( blk[i] );
		if (( i < num_bbl ) && bbl[i] )
			free( bbl[i] );
		blk[i] = 0;
		bbl[i] = 0;
	}

	num_blk=0;
	num_bbl=0;

	for( dex=readdir(d); dex; dex=readdir(d) )
	{
		nlen=strlen(dex->d_name);
		if ( nlen < 5 )
			continue;
		if ( (num_bbl < MAX_B_FILES) && !strcmp(dex->d_name+nlen-4,".bbl") )
			bbl[num_bbl++]=strdup(dex->d_name);
		else if ( (num_blk < MAX_B_FILES) && !strcmp(dex->d_name+nlen-4,".blk") )
			blk[num_blk++]=strdup(dex->d_name);
	}
	closedir(d);

	if(num_blk)
		qsort(blk,num_blk,sizeof(char*),cstring_cmp);
	if(num_bbl)
		qsort(bbl,num_bbl,sizeof(char*),cstring_cmp);
}

void	HttpLoadCleaningRecord( SkTimerType tid, void *own )
{
	CRVar	*v;
	CRVar	*lastv=0;
	FILE	*fp;
	char	buff[512];	/* a line */
	char	*p;
	char	*tok=0;
	char	*name=0;
	char	*val=0;
	int		len;

	skAddTimer(28000,HttpLoadCleaningRecord,0);	/* try again in 1 minute */

	if ( num_open )
		return;

	Log(1,"refresh blk/bll file list\r\n");

	LoadBlkFilenames();

	Log(1,"refresh statistic values\r\n");

	fp=fopen("/usr/data/blackbox/cleaningrecord.stc","r");
	if ( !fp )
		return;
	/* clean up old records */
	for( v=crv_root; v; v=lastv )
	{
		lastv=v->next;
		free(v->name);
		free(v->value);
		free(v);
	}
	lastv=0;
	crv_root=0;
	while( fgets(buff,512,fp) )
	{
		p=strchr(buff,'=');
		if ( !p )
			continue;
		p=strchr(buff,'\n'); if(p)*p=0;
		p=strchr(buff,'\r'); if(p)*p=0;
		/* run to token */
		p=strchr(buff,'<');
		if ( !p )
			continue;
		tok=p+1;
		len=strlen(tok);
		if (( *(tok+len-2) != '/' ) && ( *(tok+len-2) != '>' ))
			continue;
		*(tok+len-2)=0;
/* search 1st. ' ' */
		for( p=tok; *p; p++ )
		{
			if (( *p == ' ' ) || ( *p == '\t' ))
			{
				*p=0;
				p++;
				break;
			}
		}
		while( *p )
		{
			if ( !*p  )
				break;
/* search 1st.  nospace */
			for( ; *p; p++ )
			{
				if (( *p == ' ' ) || ( *p == '\t' ))
					continue;
				break;
			}

			name=p;
			p=strchr(name,'=');
			if ( !p )
				break;
			*p=0;
			val=p+1;
			if ( *val == '\"' )
			{
				val++;
				for( p=val; *p; p++ )
				{
					if ( *p == '\"' )
						break;
					if ( *p == '\\' )
						p++;
				}
				if ( *p == '\"' )
				{
					char	vname[256];

					*p=0; p++;
					sprintf(vname,"CLREC:%s.%s",tok,name);
					len=strlen(vname);

					v=malloc(sizeof(CRVar));
					memset(v,0,sizeof(CRVar));
					v->name=strdup(vname);
					v->sz = len;
					v->value=strdup(val);
					if( lastv )
						lastv->next = v;
					else
						crv_root = v;
					lastv=v;
				}
			}
		}
	}
	fclose(fp);
}

static	void	FillCleaningVar( char* to, char *code, int sz)
{
	CRVar	*v;

	for( v=crv_root; v; v=v->next )
	{
		if (( sz == v->sz ) && !strncmp(code,v->name,sz))
		{
			sprintf(to,"%s",v->value);
			break;
		}
	}
}

static	char	*GetVar( char *code, int sz )
{
static	char	retbuf[512];
	*retbuf=0;
	if ( (sz == 12) && !strncmp(code,"JSON:VERSION",sz) )
		sprintf(retbuf,"%s",json.version?json.version:"");
	else if ( (sz == 16) && !strncmp(code,"JSON:ROBOT_STATE",sz) )
		sprintf(retbuf,"%s",json.robot_state?json.robot_state:"");
	else if ( (sz == 10) && !strncmp(code,"JSON:TURBO",sz) )
		sprintf(retbuf,"%s",json.turbo?json.turbo:"");
	else if ( (sz == 11) && !strncmp(code,"JSON:REPEAT",sz) )
		sprintf(retbuf,"%s",json.repeat?json.repeat:"");
	else if ( (sz == 9) && !strncmp(code,"JSON:BATT",sz) )
		sprintf(retbuf,"%s",json.batt?json.batt:"");
	else if ( (sz == 13) && !strncmp(code,"JSON:BATTPERC",sz) )
		sprintf(retbuf,"%s",json.battperc?json.battperc:"");
	else if ( (sz == 9) && !strncmp(code,"JSON:MODE",sz) )
		sprintf(retbuf,"%s",json.mode?json.mode:"");
	else if ( (sz == 13) && !strncmp(code,"JSON:NICKNAME",sz) )
		sprintf(retbuf,"%s",json.nickname?json.nickname:"");
	else if ( (sz == 8) && !strncmp(code,"CPU:IDLE",sz) )
		sprintf(retbuf,"%.2f",cpu_idle);
	else if ( (sz == 8) && !strncmp(code,"CPU:USER",sz) )
		sprintf(retbuf,"%.2f",cpu_user);
	else if ( (sz == 8) && !strncmp(code,"CPU:NICE",sz) )
		sprintf(retbuf,"%.2f",cpu_nice);
	else if ( (sz == 7) && !strncmp(code,"CPU:SYS",sz) )
		sprintf(retbuf,"%.2f",cpu_sys);
	else if ( (sz == 13) && !strncmp(code,"LGSRV:VERSION",sz) )
		sprintf(retbuf,"%s",cstr);
	else if ( (sz == 12) && !strncmp(code,"LGSRV:SUMCMD",sz) )
		sprintf(retbuf,"%d",sum_cmd);
	else if ( (sz == 15) && !strncmp(code,"LGSRV:SUMCMDSEC",sz) )
		sprintf(retbuf,"%f",sum_cmd_per_sec);
	else if ( (sz == 13) && !strncmp(code,"LGSRV:NUMHTTP",sz) )
		sprintf(retbuf,"%d",http_requests);
	else if ( (sz == 13) && !strncmp(code,"SYS:DROPBEARV",sz) )
		sprintf(retbuf,"%s",*dropb_version!='#' ? dropb_version:"-?-");
	else if ( (sz == 12) && !strncmp(code,"TIMER:MONDAY",sz) )
		sprintf(retbuf,"%s",timer.mon ? timer.mon:"");
	else if ( (sz == 13) && !strncmp(code,"TIMER:TUESDAY",sz) )
		sprintf(retbuf,"%s",timer.tue ? timer.tue:"");
	else if ( (sz == 15) && !strncmp(code,"TIMER:WEDNESDAY",sz) )
		sprintf(retbuf,"%s",timer.wed ? timer.wed:"");
	else if ( (sz == 14) && !strncmp(code,"TIMER:THURSDAY",sz) )
		sprintf(retbuf,"%s",timer.thu ? timer.thu:"");
	else if ( (sz == 12) && !strncmp(code,"TIMER:FRIDAY",sz) )
		sprintf(retbuf,"%s",timer.fri ? timer.fri:"");
	else if ( (sz == 14) && !strncmp(code,"TIMER:SATURDAY",sz) )
		sprintf(retbuf,"%s",timer.sat ? timer.sat:"");
	else if ( (sz == 12) && !strncmp(code,"TIMER:SUNDAY",sz) )
		sprintf(retbuf,"%s",timer.sun ? timer.sun:"");
	else if ( (sz == 13) && !strncmp(code,"MAIL:RECEIVER",sz) )
		sprintf(retbuf,"%s",mail.send.receiver ? mail.send.receiver:"");
	else if ( (sz == 13) && !strncmp(code,"MAIL:RESPONSE",sz) )
		sprintf(retbuf,"%s",mail.response ? mail.response:"");
	else if ( (sz == 12) && !strncmp(code,"MAIL:GATEWAY",sz) )
		sprintf(retbuf,"%s",mail.send.gateway ? mail.send.gateway:"");
	else if ( (sz == 9) && !strncmp(code,"MAIL:USER",sz) )
		sprintf(retbuf,"%s",mail.send.user ? mail.send.user:"");
	else if ( (sz == 9) && !strncmp(code,"MAIL:PASS",sz) )
		sprintf(retbuf,"%s",mail.send.pass ? mail.send.pass:"");
	else if ( (sz == 13) && !strncmp(code,"MAIL:SLENABLE",sz) )
		sprintf(retbuf,"%s",mail.send.enable ? "checked":"");
	else if ( (sz == 13) && !strncmp(code,"MAIL:P3SERVER",sz) )
		sprintf(retbuf,"%s",mail.get.gateway ? mail.get.gateway:"");
	else if ( (sz == 11) && !strncmp(code,"MAIL:P3USER",sz) )
		sprintf(retbuf,"%s",mail.get.user ? mail.get.user:"");
	else if ( (sz == 11) && !strncmp(code,"MAIL:P3PASS",sz) )
		sprintf(retbuf,"%s",mail.get.pass ? mail.get.pass:"");
	else if ( (sz == 13) && !strncmp(code,"MAIL:P3ENABLE",sz) )
		sprintf(retbuf,"%s",mail.get.enable ? "checked":"");
	else if ( (sz == 11) && !strncmp(code,"MAIL:P3SIGN",sz) )
		sprintf(retbuf,"%s",mail.get.sign ? mail.get.sign:"");
	else if ( (sz == 12) && !strncmp(code,"MAIL:P3CYCLE",sz) )
		sprintf(retbuf,"%d",mail.get.cycle);
	else if ( (sz == 12) && !strncmp(code,"SYS:TIME-H:M",sz) )
		sprintf(retbuf,"%d:%02d",tm.tm_hour,tm.tm_min);
	else if ( (sz == 14) && !strncmp(code,"SYS:TIME-H:M:S",sz) )
		sprintf(retbuf,"%d:%02d:%02d",tm.tm_hour,tm.tm_min,tm.tm_sec);
	else if ( (sz == 14) && !strncmp(code,"SYS:TIME-H12:M",sz) )
		sprintf(retbuf,"%d:%02d %s",tm.tm_hour>12?tm.tm_hour%12:tm.tm_hour,tm.tm_min,tm.tm_hour>11?"PM":"AM");
	else if ( (sz == 16) && !strncmp(code,"SYS:TIME-H12:M:S",sz) )
		sprintf(retbuf,"%02d:%02d:%02d %s",tm.tm_hour>12?tm.tm_hour%12:tm.tm_hour,tm.tm_min,tm.tm_sec,tm.tm_hour>11?"PM":"AM");
	else if ( (sz == 14) && !strncmp(code,"SYS:DATE-D.M.Y",sz) )
		sprintf(retbuf,"%02d.%02d.%d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900);
	else if ( (sz == 14) && !strncmp(code,"SYS:DATE-M.D.Y",sz) )
		sprintf(retbuf,"%02d.%02d.%d",tm.tm_mon+1,tm.tm_mday,tm.tm_year+1900);
	else if ( (sz == 14) && !strncmp(code,"SYS:DATE-Y.M.D",sz) )
		sprintf(retbuf,"%d.%02d.%02d",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday);
	else if ( (sz == 14) && !strncmp(code,"SYS:DATE-D/M/Y",sz) )
		sprintf(retbuf,"%02d/%02d/%d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900);
	else if ( (sz == 14) && !strncmp(code,"SYS:DATE-M/D/Y",sz) )
		sprintf(retbuf,"%02d/%02d/%d",tm.tm_mon+1,tm.tm_mday,tm.tm_year+1900);
	else if ( (sz == 14) && !strncmp(code,"SYS:DATE-Y/M/D",sz) )
		sprintf(retbuf,"%d/%02d/%02d",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday);
	else if ( (sz > 6) && !strncmp(code,"CLREC:",6) )
		FillCleaningVar(retbuf,code,sz);
	else if ( (sz > 8) && !strncmp(code,"BLKFILE:",8) )
	{
		int  idx=atoi(code+8);
		if (( idx >=0 ) && ( idx < num_blk ))
			sprintf(retbuf,"%s",blk[idx]);
		else
			strcpy(retbuf," ");
	}
	else if ( (sz > 8) && !strncmp(code,"BBLFILE:",8) )
	{
		int  idx=atoi(code+8);
		if (( idx >=0 ) && ( idx < num_bbl ))
			sprintf(retbuf,"%s",bbl[idx]);
		else
			strcpy(retbuf," ");
	}

	return retbuf;
}

static	int	HtmlReplace( char *in, int sz )
{
	char	*p;
	int		new_sz=sz;
	int		done=0;
	int		rm_size,ins_size;
	char	*ins;

	in[sz]=0;	/* end of string */
	p=strchr(in,'$');
	while( p )
	{
		while(1)
		{
			char	*bcl=0;
			if ( *(p+1) != '{' )
				break;
			bcl=strchr(p+1,'}');
			if ( !bcl )
			{
				done=1;
				break;
			}
			rm_size = bcl-p+1;
			ins=GetVar(p+2,rm_size-3);
			ins_size=strlen(ins);
			if ( new_sz + ins_size - rm_size > 4999 )
			{
				done=1;
				break;
			}
			if ( ins_size != rm_size )
				memmove(p+ins_size,p+rm_size,new_sz-(p-in)-rm_size);
			if ( ins_size )
			{
				memcpy(p,ins,ins_size);
				new_sz += ins_size;
			}
			new_sz -= rm_size;
			break;
		}
		if ( done )
			break;
		p=strchr(p+2,'$');
	}
	return new_sz;
}

static	void	SendFile( SkLine *l, char *fname )
{
	char		*p;
	struct stat	st;
	char		buff[5008];
	int			x=0;
	int			intern_file=0;
	FILE		*fp=0;
	int			fd=-1;
	int			is_html=0;

	memset(buff,0,5008);

	p=strchr(fname,' ');
	if ( !p )
	{
		WriteSimpleHttp( l, hdr "<body>unknown REQUEST!<br />" );
		WriteSimpleHttp( l, "SendFile(%s)",fname );
		WriteSimpleHttp( l, "</body></html>");
		skCloseAtEmpty(l);
		return;
	}
	*p=0;
	num_open++;

	if ( !strncmp(fname,".../",4) )
	{
		fname += 3;
		fp=fopen(fname,"r");
		if ( !fp )
		{
//printf("file not found : %s\n",fname);
			WriteSimpleHttp( l, hdr "<body>file not found : 401<br />" );
			WriteSimpleHttp( l, "</body></html>");
			skCloseAtEmpty(l);
			num_open--;
			return;
		}
		fstat(fileno(fp),&st);
	}
	else
	{
#define HTML_WITH_SIZE
#ifdef HTML_WITH_SIZE
		p=strrchr(fname,'.');
		if ( p )
		{
			if ( !strcmp(p,".html") ||	// HTML
				 !strcmp(p,".htm") ||	// HTML
				 !strcmp(fname,"status.txt") )	// HTML
			{
				if ( num_open == 1)
					PrepareVars();
				is_html=1;
			}
		}
#endif
/* first try to find file on /usr/data/htdocs */
		sprintf(buff,"/usr/data/htdocs/%s",fname);
		fp=fopen(buff,"r");
		if ( fp )
		{
#ifdef HTML_WITH_SIZE
			if ( !is_html )
#endif
				fstat(fileno(fp),&st);
#ifdef HTML_WITH_SIZE
			else
			{
				int		xb;
				st.st_size=0;
				xb=fread(buff,1,5000,fp);
				while( xb>0 )
				{
					x=HtmlReplace(buff,xb);
					st.st_size += x;
					if ( xb < 5000 )
						break;
					xb=fread(buff,1,5000,fp);
				}
				fclose(fp);
				sprintf(buff,"/usr/data/htdocs/%s",fname);
				fp=fopen(buff,"r");
			}
#endif
		}
		else
		{
			intern_file=1;
			fd=__t_open( fname );
			if ( fd == -1 )
			{
				WriteSimpleHttp( l, hdr "<body>file not found : 401<br />" );
				WriteSimpleHttp( l, "</body></html>");
				skCloseAtEmpty(l);
				num_open--;
				return;
			}
#ifdef HTML_WITH_SIZE
			if ( !is_html )
#endif
				st.st_size = __t_getsize(fd);
#ifdef HTML_WITH_SIZE
			else
			{
				int		xb;
				st.st_size = 0;
				xb=__t_read(fd,buff,5000);
				while( xb>0 )
				{
					x=HtmlReplace(buff,xb);
					st.st_size += x;
					if ( xb < 5000 )
						break;
					xb=__t_read(fd,buff,5000);
				}
				__t_close(fd);
				fd=__t_open( fname );
			}
#endif
		}
	}
	p=strrchr(fname,'.');
	if ( !p )
	{
		WriteSimpleHttp( l, hdr_binary, st.st_size );
	}
	else
	{
		if ( !strcmp(p,".js") )	// js
		{
			Log(8,"> JS : size= %ld\r\n",(unsigned long)st.st_size);
			WriteSimpleHttp(l,hdr_js,st.st_size);
		}
		else if ( !strcmp(p,".png") )	// PNG
		{
			Log(8,"> PNG : size= %ld\r\n",(unsigned long)st.st_size);
			WriteSimpleHttp(l,hdr_png,st.st_size);
		}
		else if ( !strcmp(p,".gif") )	// GIF
		{
			Log(8,"> GIF : size= %ld\r\n",(unsigned long)st.st_size);
			WriteSimpleHttp(l,hdr_gif,st.st_size);
		}
		else if ( !strcmp(p,".html") )	// HTML
		{
#ifdef HTML_WITH_SIZE
			Log(8,"> HTML : size= %ld\r\n",(unsigned long)st.st_size);
			WriteSimpleHttp(l,hdr_html,st.st_size);
#else
			PrepareVars();
			WriteSimpleHttp(l,hdr_html_nsize);
#endif
			is_html=1;
		}
		else if ( !strcmp(p,".htm") )	// HTML
		{
#ifdef HTML_WITH_SIZE
			Log(8,"> HTML : size= %ld\r\n",(unsigned long)st.st_size);
			WriteSimpleHttp(l,hdr_html,st.st_size);
#else
			PrepareVars();
			WriteSimpleHttp(l,hdr_html_nsize);
#endif
			is_html=1;
		}
		else if ( !strcmp(p,".jpg") )	// JPEG
		{
			Log(8,"> JPEG : size= %ld\r\n",(unsigned long)st.st_size);
			WriteSimpleHttp(l,hdr_jpeg,st.st_size);
		}
		else if ( !strcmp(p,".jpeg") )	// JPEG
		{
			Log(8,"> JPEG : size= %ld\r\n",(unsigned long)st.st_size);
			WriteSimpleHttp(l,hdr_jpeg,st.st_size);
		}
		else if ( !strcmp(p,".txt") )	// TXT
		{
			Log(8,"> TXT : size= %ld\r\n",(unsigned long)st.st_size);
			WriteSimpleHttp(l,hdr_txt,st.st_size);
		}
		else if ( !strcmp(p,".css") )	// css
		{
			Log(8,"> CSS : size= %ld\r\n",(unsigned long)st.st_size);
			WriteSimpleHttp(l,hdr_css,st.st_size);
		}
		else
		{
			Log(8,"> %p, RAW : size= %ld\r\n",l,(unsigned long)st.st_size);
			WriteSimpleHttp( l, hdr_binary, st.st_size );
		}
	}
	skMultiDisable(l,0);
	if ( !intern_file )
	{
		int		xb;
		int		xsum=0;
		xb=fread(buff,1,5000,fp);
		while( xb>0 )
		{
			if ( is_html )
				x=HtmlReplace(buff,xb);
			else
				x=xb;
			_WritePacket(l,(unsigned char*)buff,x);
			xsum+=x;
			if ( l->out && ( l->out->fill - l->out->ptr > 100000 ))
				_SyncLine(l);
			if ( xb < 5000 )
				break;
			xb=fread(buff,1,5000,fp);
		}
		fclose(fp);
		Log(8,"%p file: %d bytes written\r\n",l,xsum);
	}
	else
	{
		int		xb;
		int		xsum=0;
		xb=__t_read(fd,buff,5000);
		while( xb>0 )
		{
			if ( is_html )
				x=HtmlReplace(buff,xb);
			else
				x=xb;
			_WritePacket(l,(unsigned char*)buff,x);
			xsum+=x;
			if ( l->out && ( l->out->fill - l->out->ptr > 100000 ))
			{
				_SyncLine(l);
			}
			if ( xb < 5000 )
				break;
			xb=__t_read(fd,buff,5000);
		}
		__t_close(fd);
		Log(8,"ifile: %d bytes written\r\n",xsum);
	}
	_SyncLine(l);
	skMultiEnable(l,0);
	num_open--;
}

static	void	DoPostData( SkLine *l, char *data, int len )
{
	char	*p=0;
	char	*val;
	int		off;
	int		sz;
	int		num;
	char	xfname[512];

	switch( l->data->post_mode )
	{
	case 1 :
		p=strstr(data,"boundary=");
		if ( p )
		{
			val=p+9;
			p=strchr(val,'\r');
			if( p )
			{
				*p=0;
				l->data->boundary=strlen(val);
				*p='\r';
			}
		}
		/* content-length */
		p=strstr(data,"Content-Length:");
		if ( !p )
			return;
		val=p+15;
		p=strchr(val,'\r');
		if ( !p )
			return;
		*p=0;
		l->data->size = atoi(val);
		l->data->post_mode=2;
		*p='\r';	/* put back for next jump */
		/* now jump to \r\n\r\n */
		p = strstr(p-1,"\r\n\r\n");
		if ( !p )
			return;
		p+=4;
		sz=(int)(p-data);
		data=p;
		len-=sz;
		if ( len < 10 )
		{
			l->data->written=len;
			return;
		}
	case 2 :
		p=strstr(data,"filename=");
		if ( !p )
			return;
		val=p+10;
		p=strchr(val,'"');
		if ( !p )
			return;
		*p=0;
		sprintf(xfname,DESTPATH"%s",val);
		l->data->filename=strdup(xfname);
		/* now jump to \r\n\r\n */
		p = strstr(p+1,"\r\n\r\n");
		if ( !p )
			return;
		l->data->post_mode=3;
		p+=4;
		num=(int)(p-data);
		l->data->written += num;
		if ( l->data->boundary )
		{
			l->data->written += l->data->boundary + 8;
		}

		sprintf(xfname,DESTPATH"%p",l);
		l->data->fp = fopen(xfname,"w");
		if ( !l->data->fp )
		{
			mkdir( DESTPATH, 0755 );
			l->data->fp = fopen(xfname,"w");
		}
		num=len-(int)(p-data);
		if ( num )
		{
			off=l->data->size - l->data->written;
			sz=off < num ? off : num;
			if ( l->data->fp )
				fwrite( p, 1, sz, l->data->fp );
			l->data->written += sz;
			num-=sz;
		}
		if ( l->data->written == l->data->size )
		{
			if ( l->data->fp )
			{
				fclose(l->data->fp);
				l->data->fp=0;
				rename( xfname, l->data->filename );
				chmod(l->data->filename,0755);
				SendUploaded(l);
			}
			else
			{
				SendUploadFail(l);
			}
			skCloseAtEmpty(l);
			return;
		}
		break;
	case 3 :
		if ( !l->data->fp )
			return;
		off=l->data->size - l->data->written;
		sz=off < len ? off : len;
		if (( sz > 0 ) && l->data->fp )
			fwrite( data, 1, sz, l->data->fp );
		l->data->written += sz;
		if ( l->data->written == l->data->size )
		{
			if ( l->data->fp )
			{
				fclose(l->data->fp);
				l->data->fp=0;
				sprintf(xfname,DESTPATH "%p",l);
				rename( xfname, l->data->filename );
				chmod(l->data->filename,0755);
				SendUploaded(l);
			}
			else
			{
				strcpy(xfname,"upfail.html ");
				SendFile( l, xfname );
			}
			skCloseAtEmpty(l);
			return;
		}
		break;
	}
}

#if 0
static	int		http_snapshot_avail=0;
static	int		http_snapshot_running=0;
static	int		http_snapshot_busy=0;
static	int		http_snapshot_failed=1;
static	int		http_no_cam=1;
#endif

extern	void	RestartMe( void );

void	HttpPck( SkLine *l, int pt, void *own, void *sys )
{
	SkPacket   	*pck=sys;
	char		*data=pck->data;

	if ( l->data->post_mode )
	{
		DoPostData( l, data, pck->len );
		return;
	}

	http_requests++;

	if ( debug&8 )
	{
		if ( !strncmp(data,"GET ",4) )
		{
			char  txt[512];
			char *p=strstr(data," HTTP");
			if ( p )
			{
				int n=p-data;
				if ( n > 511) n=511;
				memcpy(txt,data,n);
				txt[n]=0;
				printf("http.c: %s\n",txt);
			}
		}
	}

	if ( !strncmp(data,"GET /activate?",14) )
	{
		char	*p=strchr(data+14,' ');
		char	buff[1024];
		char	*f=data+14, *t=buff;
		int		rc;

		if (p)
			*p=0;
		for( ; *f; f++, t++ )
		{
			if ( *f == '%' )
			{
				char  			hx[3];
				unsigned int	it;
				memcpy(hx,f+1,2);
				hx[2]=0;
				sscanf(hx,"%x",&it);
				*t=it;
				f+=2;
				continue;
			}
			*t=*f;
		}
		*t=*f;
		rc=DoActivate(l,buff);
		skCloseAtEmpty(l);
		if ( rc == 1 )
		{
			skTimeoutStep(1000);
			_RestartMe();
			return;
		}
		if ( rc == 2 )
		{
			char	cmd[1024];
			sprintf(cmd,"killall %s; sleep 1; %s",buff,buff);
			system(cmd);
		}
		return;
	}
	else if ( !strncmp(data,"GET /remove?",12) )
	{
		char	*p=strchr(data+12,' ');
		char	buff[1024];
		char	*f=data+12, *t=buff+DESTLEN;

		if (p)
			*p=0;
		strcpy(buff,DESTPATH);
		for( ; *f; f++, t++ )
		{
			if ( *f == '%' )
			{
				char  			hx[3];
				unsigned int	it;
				memcpy(hx,f+1,2);
				hx[2]=0;
				sscanf(hx,"%x",&it);
				*t=it;
				f+=2;
				continue;
			}
			*t=*f;
		}
		*t=*f;
		unlink(buff);
		WriteSimpleHttp( l, hdr_response );
		WriteSimpleHttp( l, buff+DESTLEN );
		WriteSimpleHttp( l, " removed" );
		skCloseAtEmpty(l);
		return;
	}
	else if ( !strncmp(data,"GET /restart?",13) )
	{
		char	*p=strchr(data+13,' ');
		char	*f=data+13;
		char	buff[0124];

		if (p)
			*p=0;

		strcpy(buff,f);
		WriteSimpleHttp( l, hdr "<body><h4>restart of %s inititiated</h4>",f);
		WriteSimpleHttp( l, "</body></html>");
		skCloseAtEmpty(l);

		if ( !strcmp(buff,"lg.srv") )
		{
			skTimeoutStep(1000);
			_RestartMe();
			return;
		}
		sprintf(buff,"killall %s; sleep 1; %s",f,f);
		system(buff);
		return;
	}
	else if ( !strncmp(data,"GET /reboot?",12) )
	{
		char	buff[0124];

		strcpy(buff,"reboot");
		WriteSimpleHttp( l, hdr "<b>reboot of hombot inititiated, please wait ...</b>");
		skCloseAtEmpty(l);

		system(buff);
		return;
	}
	else if ( !strncmp(data,"GET /json.cgi?",14) )
	{
		int				rc;
		int				f=0;
		char			locdata[512];
		char			*p, *q;
		char  			hx[3];
		unsigned int	b;
		int				obr=0;

		data+=14;

		for( p=data, q=locdata; *p; f++, p++, q++ )
		{
			if ( f == 510 )
				break;
			if ( *p == ' ' )
				break;
			b=*p;
			if ( *p == '%' )
			{
				memcpy(hx,p+1,2);
				hx[2]=0;
				sscanf(hx,"%x",&b);
				p+=2;
			}
			if ( b == '{' )
				obr++;
			else if ( b == '}' )
			{
				*q=b;
				if(obr)
					obr--;
				if ( !obr )
				{
					q++;
					break;
				}
			}
			*q=b;
		}
		*q=0;
		if ( !strcmp(locdata,"turbo") )
		{
			int		is_turbo=json.turbo && !strcasecmp(json.turbo,"true") ? 1 : 0;
			sprintf(locdata,"{\"COMMAND\":{\"TURBO\":\"%s\"}}",is_turbo?"false":"true");
		}
		else if ( !strcmp(locdata,"mode") )
		{
			char	*modes[]={"ZZ","SB","SPOT"};
static		int		cur_mode=-1;

			if ( cur_mode == -1 )
			{
				for( cur_mode=0; cur_mode < 3; cur_mode++ )
				{
					if ( !strcasecmp(modes[cur_mode],json.mode) )
						break;
				}
			}
			if ( cur_mode > 1 )
				cur_mode=0;
			else
				cur_mode += 1;

			sprintf(locdata,"{\"COMMAND\":{\"CLEAN_MODE\":\"CLEAN_%s\"}}",modes[cur_mode]);
		}
		if ( !strcmp(locdata,"repeat") )
		{
			int		is_repeat=json.repeat && !strcasecmp(json.repeat,"true") ? 1 : 0;
			sprintf(locdata,"{\"COMMAND\":{\"REPEAT\":\"%s\"}}",is_repeat?"false":"true");
		}
		rc=jsonSend( locdata );
		WriteSimpleHttp( l, hdr "<body><h4> %s inititiated</h4>",locdata);

		WriteSimpleHttp( l, "retcode : %d<br>",rc);
		WriteSimpleHttp( l, "</body></html>");
		skCloseAtEmpty(l);
		return;
	}
	else if ( !strncmp(data,"GET /sites/schedule.html?",25) )
	{
		char			def[32]="sites/schedule.html ";
		char			locdata[512];
		char			*p, *q;
		char  			hx[3];
		unsigned int	b;
		int				f=0;

		for( p=data+24, q=locdata; *p; f++, p++, q++ )
		{
			if ( f == 510 )
				break;
			if ( *p == ' ' )
				break;
			if ( *p != '%' )
			{
				*q=*p;
				continue;
			}
			memcpy(hx,p+1,2);
			hx[2]=0;
			sscanf(hx,"%x",&b);
			*q=b;
			p+=2;
			continue;
		}
		*q=0;
		RunTimerParam(locdata);
		SendFile(l,def);
		skCloseAtEmpty(l);
		return;
	}
	else if ( !strncmp(data,"GET /sites/mailcfg.html?",24) )
	{
		char			def[32]="sites/mailcfg.html ";
		char			locdata[1024];
		char			*p, *q;
		char  			hx[3];
		unsigned int	b;
		int				f=0;

		for( p=data+24, q=locdata; *p; f++, p++, q++ )
		{
			if ( f == 510 )
				break;
			if ( *p == ' ' )
				break;
			if ( *p != '%' )
			{
				*q=*p;
				continue;
			}
			memcpy(hx,p+1,2);
			hx[2]=0;
			sscanf(hx,"%x",&b);
			*q=b;
			p+=2;
			continue;
		}
		*q=0;
		RunMailCfgParam(locdata);
		SendFile(l,def);
		skCloseAtEmpty(l);
		return;
	}
	else if ( !strncmp(data,"GET / HTTP",10) )
	{
		char	def[16]="index.html ";
		SendFile(l,def);
#ifdef HTML_WITH_SIZE
		_SyncLine(l);
		Log(8,"*** done (%s)\r\n",def);
#else
		skCloseAtEmpty(l);
#endif
		return;
	}
	else if ( !strncmp(data,"GET /blackbox.tgz HTTP",22) )
	{
		int		xb, x;
		FILE	*fp;
		char	buff[5008];

		fp=popen("tar cf - /usr/data/blackbox | gzip","r");
		if ( !fp )
		{
			SendNoPage(l);
			WriteSimpleHttp(l,data);
			return;
		}

		WriteSimpleHttp( l, hdr_tgz );

		xb=fread(buff,1,5000,fp);
		while( xb>0 )
		{
			x=xb;
			_WritePacket(l,(unsigned char*)buff,x);
			if ( l->out && ( l->out->fill - l->out->ptr > 100000 ))
				_SyncLine(l);
			if ( xb < 5000 )
				break;
			xb=fread(buff,1,5000,fp);
		}
		pclose(fp);
		skCloseAtEmpty(l);
		return;
	}
	else if ( !strncmp(data,"GET /",5) )
	{
		char	*fname=malloc(pck->len);
		char	*p;

		memcpy(fname,data+5,pck->len-5);
		*(fname+pck->len-5)=0;
		p=strchr(fname,'?');
		if(p)
			*p=' ';
		SendFile(l,fname);
#ifdef HTML_WITH_SIZE
		_SyncLine(l);
		Log(8,"*** done (%s)\r\n",fname);
#else
		skCloseAtEmpty(l);
#endif
		free(fname);
		return;
	}
	else
	{
		SendNoPage(l);
		WriteSimpleHttp(l,data);
	}

	WriteSimpleHttp( l, "</body></html>");
	skCloseAtEmpty(l);
	Log(8,"*** done - closed\r\n");
}

void HttpStatAddClData( void )
{
	sum_cmd++;
}

#if 0
void HttpPictureDone( int rc )
{
	http_snapshot_failed=1;
	if ( !rc )
	{
		rename( "/usr/tmp/v0.jpg", "/usr/tmp/v1.jpg" );
		http_snapshot_failed=0;
	}
	http_snapshot_avail=1;
}

int HttpNeedPicture( void )
{
	return http_snapshot_running ? 1 :0;	/* no */
}

void	HttpSetNoCam( void )
{
	http_no_cam=1;
}
#endif
