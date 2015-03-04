
// #include "headers/mainClass.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include "NetDrv.h"

extern	char	*cstr;
extern	int		debug;

/*
** M5sStrgCut - imported from mon5k-project
*/
/* deli = [ \t], also a string can bound into "" or '' */
/* beautify : remove bounds : ' or " */
char **M5sStrgCut( char *in, int *rargc, char beautify ) {
	char	*argv[ 1024 ];
	char	**rargv;
	int		argc=0;
	char	*curpos=in;
	char	step=0;
	char	bound=0;

	while( *curpos ) {
		switch( step ) {
			case 0 : // run over [ \t]
				if (( *curpos == '\r') || ( *curpos == '\n')) {
					*curpos=0;
					break;
				}

				if (( *curpos == ' ' ) || ( *curpos == '\t' )) {
					curpos++;
					break;
				}
				if (( *curpos == '\'') || ( *curpos == '"' )) {
					bound=*curpos;
					if ( beautify )
						curpos++;
					step=1;	// will added to 2
				}
				argv[argc++]=curpos;
				step++;
				curpos++;
				break;
			case 2 : /* search next bound */
				if ( *curpos == '\\' ) {
					curpos++;
					if ( *curpos == 0 )
						break;
					curpos++;
					break;
				}
				if ( *curpos == bound ) {
					if (( *(curpos+1) == ' ' ) ||
							( *(curpos+1) == '\t') ||
							( *(curpos+1) == 0) ||
							( *(curpos+1) == '\r') ||
							( *(curpos+1) == '\n') ) {
						step=0;
						if ( beautify ) {
							*curpos=0;
							curpos++;
							break;
						}
						curpos++;
						if ( *curpos == 0 ) // eos
							break;
						*curpos=0;
						curpos++;
						break;
					}
				}
				curpos++;
				break;
			case 1 : /* search next space */
				if ( *curpos == '\\' ) {
					curpos++;
					if ( *curpos == 0 )
						break;
					curpos++;
					break;
				}
				if (( *curpos == ' ' ) ||
						( *curpos == '\t') ||
						( *curpos == 0) ||
						( *curpos == '\r') ||
						( *curpos == '\n') ) {
					step=0;
					if ( *curpos == 0 ) // eos
						break;
					*curpos=0;
				}
				curpos++;
				break;
		}
	}
	argv[argc++]=0;
	rargv=malloc(sizeof(char*)*argc);
	memcpy(rargv,argv,sizeof(char*)*argc);
	*rargc=argc-1;
	return rargv;
}

void  DoLogData( SkLine *l, char *data, int len )
{
	char	**argv;
	int		argc;
	char	*cmd_copy=0;
	char	buffer[ 1024 ];

	cmd_copy=strdup(data);

	argv=M5sStrgCut( cmd_copy, &argc, 1 );

	if ( !argv ) {
		free( cmd_copy );
		return;
	}
	if ( !argv[0] ) {
		free( cmd_copy );
		free( argv );
		return;
	}
	if ( !strcmp(argv[0],"version") )
	{
		sprintf(buffer,"%s\r\n",cstr);
		_WriteString(l,buffer);

	}
	else if ( !strcmp(argv[0],"state") )
	{
		_WriteString(l,"*state of lg.srv*\r\n" );
		_ShowStatistic(l,0);
	}
	else if ( !strcmp(argv[0],"help") )
	{
		_WriteString(l,"*command list*\r\n" );
		_WriteString(l,"version          : show lg.srv version\r\n");
		_WriteString(l,"state            : show state of lg.srv\r\n");
		_WriteString(l,"quit | exit      : hangup this connection\r\n");
		_WriteString(l,"log <mask>       : toggle logging of mask(4=timer,8=http)\r\n");
		_WriteString(l,"maxlog <num>     : set more bytes to log\r\n");
	}
	else if ( (argc>1) && !strcmp(argv[0],"log") )
	{
		int		mask=atoi(argv[1]);
		char	buff[128];
		if ( !strcmp(argv[1],"timer") )
			mask=4;
		else if ( !strcmp(argv[1],"http") )
			mask=8;
		l->data->active_log ^= mask;
		if ( l->data->active_log )
			sprintf(buff,"log on : mask=0x%02x\r\n",l->data->active_log);
		else
			strcpy(buff,"log off\r\n");
		_WriteString(l,buff);
	}
	else if ( (argc==1) && !strcmp(argv[0],"log") )
	{
		l->data->active_log ^= 7;
		_WriteString(l,l->data->active_log ? "log on \r\n":"log off\r\n");
	}
	else if ( (argc>1) && !strcmp(argv[0],"maxlog") )
	{
		SetMaxLog( atoi( argv[1]) );
	}
	else if ( !strcmp(argv[0],"quit") || !strcmp(argv[0],"exit"))
	{
		skDisconnect( l );
		free( cmd_copy );
		free( argv );
		return;
	}
	free( cmd_copy );
	free( argv );
}

void	Log( int lv, char *fmt, ... )
{
	va_list		args;
	char		out[2048];
	SkLine		*l;
	SkLine		*n;

	va_start( args, fmt );
	vsprintf( out, fmt, args );
	va_end( args );

	if ( debug & lv )
		printf("%s",out);

	for( l=skGetLinesRoot(); l; l=n )
	{
		n=l->next;
		if ( l->other_v != 1 )
			continue;
		if ( l->intid != 1 )
			continue;
		if ( l->fd == -1 )
			continue;
		if ( !l->data )
			continue;
		if ( !l->data->active_log )
			continue;
		if ( !(l->data->active_log & lv ))
			continue;
//		if ( l->data->mode == MODE_HTTP )
//			HttpLog(l,out);
		if ( l->data->mode == MODE_LOG )
			_WriteString(l,out);
	}
}

int	LogActive( int lv )
{
	SkLine		*l;
	SkLine		*n;
	if ( debug & lv )
		return 1;
	for( l=skGetLinesRoot(); l; l=n )
	{
		n=l->next;
		if ( l->other_v != 1 )
			continue;
		if ( l->intid != 1 )
			continue;
		if ( l->fd == -1 )
			continue;
		if ( !l->data )
			continue;
		if ( !l->data->active_log )
			continue;
		if ( !(l->data->active_log & lv ))
			continue;
		if ( l->data->mode == MODE_LOG )
			return 1;
	}
	return 0;
}
