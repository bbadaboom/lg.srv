#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "NetDrv.h"

extern	TimerVars	timer;

static	char	*FNAME="/usr/data/htdocs/timer.txt";

void	ReadTimerFromFile( void )
{
	FILE	*fp;
	char	buffer[512];
	char	*p;

	fp=fopen(FNAME,"r");

	if ( timer.mon )
		free(timer.mon);
	if ( timer.tue )
		free(timer.tue);
	if ( timer.wed )
		free(timer.wed);
	if ( timer.thu )
		free(timer.thu);
	if ( timer.fri )
		free(timer.fri);
	if ( timer.sat )
		free(timer.sat);
	if ( timer.sun )
		free(timer.sun);
	memset(&timer,0,sizeof(TimerVars));

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
		if ( !strcmp(buffer,"MONDAY") )
		{
			timer.mon=strdup(p+1);
		}
		else if ( !strcmp(buffer,"TUESDAY") )
		{
			timer.tue=strdup(p+1);
		}
		else if ( !strcmp(buffer,"WEDNESDAY") )
		{
			timer.wed=strdup(p+1);
		}
		else if ( !strcmp(buffer,"THURSDAY") )
		{
			timer.thu=strdup(p+1);
		}
		else if ( !strcmp(buffer,"FRIDAY") )
		{
			timer.fri=strdup(p+1);
		}
		else if ( !strcmp(buffer,"SATURDAY") )
		{
			timer.sat=strdup(p+1);
		}
		else if ( !strcmp(buffer,"SUNDAY") )
		{
			timer.sun=strdup(p+1);
		}
	}
	fclose(fp);
}

static	int	WriteTimerToFile( void )
{
	FILE	*fp;

	fp=fopen(FNAME,"w");
	if ( !fp )
		return -1;

	if( timer.mon && strlen(timer.mon)) fprintf(fp,"MONDAY=%s\r\n",timer.mon);
	if( timer.tue && strlen(timer.tue)) fprintf(fp,"TUESDAY=%s\r\n",timer.tue);
	if( timer.wed && strlen(timer.wed)) fprintf(fp,"WEDNESDAY=%s\r\n",timer.wed);
	if( timer.thu && strlen(timer.thu)) fprintf(fp,"THURSDAY=%s\r\n",timer.thu);
	if( timer.fri && strlen(timer.fri)) fprintf(fp,"FRIDAY=%s\r\n",timer.fri);
	if( timer.sat && strlen(timer.sat)) fprintf(fp,"SATURDAY=%s\r\n",timer.sat);
	if( timer.sun && strlen(timer.sun)) fprintf(fp,"SUNDAY=%s\r\n",timer.sun);
	fclose(fp);

	return 0;
}

void	RunTimerParam( char *param )
{
	char	*p, *p2;
	char	*pin=param;

	if ( timer.mon )
		free(timer.mon);
	if ( timer.tue )
		free(timer.tue);
	if ( timer.wed )
		free(timer.wed);
	if ( timer.thu )
		free(timer.thu);
	if ( timer.fri )
		free(timer.fri);
	if ( timer.sat )
		free(timer.sat);
	if ( timer.sun )
		free(timer.sun);
	memset(&timer,0,sizeof(TimerVars));

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
			if ( !strcmp(pin,"MONDAY") )
				timer.mon=strdup(p2+1);
			else if ( !strcmp(pin,"TUESDAY") )
				timer.tue=strdup(p2+1);
			else if ( !strcmp(pin,"WEDNESDAY") )
				timer.wed=strdup(p2+1);
			else if ( !strcmp(pin,"THURSDAY") )
				timer.thu=strdup(p2+1);
			else if ( !strcmp(pin,"FRIDAY") )
				timer.fri=strdup(p2+1);
			else if ( !strcmp(pin,"SATURDAY") )
				timer.sat=strdup(p2+1);
			else if ( !strcmp(pin,"SUNDAY") )
				timer.sun=strdup(p2+1);
		}
		if ( p )
		{
			pin=p+1;
			continue;
		}
		break;
	}
	WriteTimerToFile();
}

static	void	_checkTimer( SkTimerType tid, void *own )
{
	time_t		now;
	time_t		next=0;
	struct tm	tm;
	int			hour=0, min=0;
	char		rest[500];
	char		*text=0;
	char		*admode=0;
	char		**argv=0;
	int			argc=0;

	/* check current day */
	now=time(0);
	localtime_r(&now,&tm);
	*rest=0;
	switch(tm.tm_wday)
	{
	case 0 :
		text = timer.sun;
		Log(4,"_checkTimer: today=sun, text=%s\r\n",text?text:"-");
		break;
	case 1 :
		text = timer.mon;
		Log(4,"_checkTimer: today=mon, text=%s\r\n",text?text:"-");
		break;
	case 2 :
		text = timer.tue;
		Log(4,"_checkTimer: today=tue, text=%s\r\n",text?text:"-");
		break;
	case 3 :
		text = timer.wed;
		Log(4,"_checkTimer: today=wed, text=%s\r\n",text?text:"-");
		break;
	case 4 :
		text = timer.thu;
		Log(4,"_checkTimer: today=thu, text=%s\r\n",text?text:"-");
		break;
	case 5 :
		text = timer.fri;
		Log(4,"_checkTimer: today=fri, text=%s\r\n",text?text:"-");
		break;
	case 6 :
		text = timer.sat;
		Log(4,"_checkTimer: today=sat, text=%s\r\n",text?text:"-");
		break;
	}
	if ( !text )
	{
		skAddTimer( 1000*40, _checkTimer, 0 );
		return;
	}

	sscanf(text,"%d:%d%s",&hour,&min,rest);
	admode=strchr(rest,',');
	if(admode)
	{
		*admode=0;
		argv=M5sStrgCut( admode+1, &argc, 1 );
	}
		
	if ( strchr(rest,'p') || strchr(rest,'P'))
	{
		if ( hour != 12 )
			hour+=12;
	}
	else if ( strchr(rest,'a') || strchr(rest,'A'))
	{
		if ( hour == 12 )
			hour=0;
	}
	tm.tm_sec=0;
	tm.tm_hour=hour;
	tm.tm_min=min;
	next=mktime(&tm);

	if(LogActive(4))
	{
		Log(4,"translated timer : %02d:%02d",hour,min);
		if (( next < now+1 ) && ( next+60 > now ))
			Log(4," , arrived - start cleaning now\r\n");
		else if ( next < now )
			Log(4," , too late - wait for tomorrow\r\n");
		else
		{
			int	diff = next-now;
			Log(4," , diff = %d secs\r\n",diff);
		}
	}

	if (( next < now+1 ) && ( next+60 > now ))
	{	/* reached */
		int		i;

		skAddTimer( 1000*62, _checkTimer, 0 );
		jsonSend( 0 );
		for( i=0; i<argc; i++ )
		{
			if ( !strcasecmp(argv[i],"ZZ") ||
				!strcasecmp(argv[i],"SB") ||
				!strcasecmp(argv[i],"SPOT"))
			{
				char	cmd[64];
				Log(4,"switch to mode : %s\r\n",argv[i]);
				sprintf(cmd,"{\"COMMAND\":{\"CLEAN_MODE\":\"CLEAN_%s\"}}",argv[i]);
				jsonSend( cmd );
				jsonSend( 0 );
				skTimeoutStep(20);
			}
		}
		jsonSend( "{\"COMMAND\":\"CLEAN_START\"}" );
		skTimeoutStep(20);
		jsonSend( 0 );
		jsonSend( "{\"COMMAND\":\"CLEAN_START\"}" );
		if ( argv )
			free(argv);
		return;
	}
	if ( next < now )
		skAddTimer( 1000*40, _checkTimer, 0 );
	else
	{
		int	diff = next-now;
		if ( diff > 40 )
			diff=40;
		skAddTimer( diff*1000, _checkTimer, 0 );
	}
	if ( argv )
		free(argv);
}

void	StartTimer( void )
{
	_checkTimer(0,0);
}
