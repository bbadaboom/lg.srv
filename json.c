#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>

#define SkLineData		struct _JsonData

#include "NetDrv.h"

extern	JsonVars	json;

typedef struct _JsonData
{
	int			*f1;
} JsonData;

static void makeHeader(unsigned char* paramArrayOfByte, int paramInt1,
	unsigned char paramByte,
	short paramShort1, short paramShort2, short paramShort3, int paramInt2)
{
paramArrayOfByte[0] = 13;
paramArrayOfByte[1] = (unsigned char)paramInt1;
paramArrayOfByte[2] = paramByte;
paramArrayOfByte[3] = (unsigned char)paramInt2;
//paramShort1=htons(paramShort1);
//paramShort2=htons(paramShort2);
//paramShort3=htons(paramShort3);
memcpy(paramArrayOfByte+4,&paramShort1,2);
memcpy(paramArrayOfByte+6,&paramShort2,2);
memcpy(paramArrayOfByte+8,&paramShort3,2);
}

static void sendJSONPacket(SkLine *l, char *paramString )
{
int paramInt=strlen(paramString);
static int id=0;
unsigned char arrayOfByte[512];

	memset(arrayOfByte,0,12);
	makeHeader(arrayOfByte, 4, id, (short)0, (short)1, (short)paramInt, 0);
	memcpy(arrayOfByte+12,paramString,paramInt);
	id++;
	if(id == 255)
		id = 0;
#if 0
{
int i;
unsigned char *p;
printf(">> %p\n",l);
for(i=0, p=arrayOfByte; i<paramInt+12; i++, p++)
printf("%02x ",*p);
printf("\n");
}
#endif
	_WritePacket( l, arrayOfByte, paramInt+12 );
}

static	SkLine	*cmd_line=0;

static	void	_clientClose( SkLine *l, int pt, void *own, void *sys )
{
	cmd_line=0;
	if ( l->data )
	{
		*(l->data->f1) = 0;	/* mark for loop */
		free( l->data );
		l->data=0;
	}
}

static	void	GetJson( char *in, char **dest, char *property )
{
	char	*p = strstr(in,property);
	char	*pe;
	int		sl;

	if ( !p )
		return;
	p += strlen(property)+1;	/* go to value */
	pe = strchr(p,'"');
	if ( *dest )
		free( *dest );
	sl=pe-p;
	*dest = malloc(sl+1);
	memcpy(*dest,p,sl);
	memset((*dest)+sl,0,1);
}

static	void	_clientData( SkLine *l, int pt, void *own, void *sys )
{
	SkPacket		*pck=sys;
	char			*data=pck->data;
	int				size=pck->len;
	short			len;
	int				i=0;

	while( size > 12 )
	{
		memcpy(&len,data+8,2);
		if ( size < len+12 )
			break;
		data[ len +12 -1 ] = 0;	/* replace last char with 0 */

		GetJson( data+12, &json.robot_state, "\"ROBOT_STATE\":" );
		GetJson( data+12, &json.turbo, "\"TURBO\":" );
		GetJson( data+12, &json.repeat, "\"REPEAT\":" );
		GetJson( data+12, &json.batt, "\"BATT\":" );
		GetJson( data+12, &json.mode, "\"MODE\":" );
		GetJson( data+12, &json.nickname, "\"NICKNAME\":" );
		GetJson( data+12, &json.version, "\"VERSION\":" );

		if ( l && l->data && (*(l->data->f1)>0) )
			*(l->data->f1) -= 1;

		data += len+12;
		size -= (len+12);
		i++;
	}

	if ( json.batt )
	{
		char	tmp[128];
		int		batt = atoi(json.batt);
		sprintf(tmp,"%d",batt*20);
		json.battperc=strdup(tmp);
	}

#if 0
	printf("_clientData: %d bytes\r\n",pck->len);

	for( i=0, data=pck->data; i<pck->len; i++ )
	{
		if ( !(i%16) && i )
			printf("\r\n");
		printf("%02x ",data[i]);
	}
	printf("\r\n");
#endif
	return;
}

static void _asyConn( SkLine *l, int pt, void *own, void *sys )
{
	SkLine	**rl=own;
	int	k=(int)sys;
	rl[0] = l;
	if ( k == ASY_CONNECTED )
		rl[1]=l;
	else
		rl[1]=0;
}

static SkLine	*RawConnect( char *service, char *host )
{
	SkLine *l[2]={0,0};

	skAsyConnect( host, service, 500, _asyConn, l );

	while( l[0] == 0 )
		skTimeoutStep(100);

	return l[1];
}

#ifdef JSON_SIMUL
static void json_simu( void )
{
	char		data[]="____________{\"CONNECT_INIT\":[{\"ROBOT_STATE\":\"CHARGING\"},{\"TURBO\":\"true\"},{\"REPEAT\":\"false\"},{\"BATT\":\"4\"},{\"MODE\":\"ZZ\"},{\"RESERVATION\":{\"RESP_RSVSTATE\":\"false\"}},{\"NICKNAME\":\"HOMBOT\"},{\"VERSION\":\"11128\"},{\"BLACKBOX\":{\"RECENT_ABS\":\"false\"}},{\"DIAGNOSIS\":{\"RECENT\":\"false\"}},{\"VOICEMODE\":\"MALE\"}]}";
	SkPacket	pck;
	short		len;

	pck.data = data;
	pck.len = strlen(data);
	len = pck.len - 12;
	data[8] = len%256;
	data[9] = len/256;
	_clientData( 0, 0, 0, &pck );
}
#endif

int	jsonSend( char *command )
{
	int		clen=command ? strlen(command) : 0;
	SkLine	*l=0;
	int		f1=0;
	int		cnt=10;

	if ( clen > 512 )
		return -1;

#ifdef JSON_SIMUL
	json_simu();
	return 0;
#endif

	if ( cmd_line )
	{
		l=cmd_line;
	}
	else
	{
		l=RawConnect("4000","127.0.0.1");
//		l=RawConnect("4000","192.168.28.112");
		if (!l )
			return -2;
		cmd_line=l;
		f1++;
		skAddHandler( l, SK_H_READABLE, _HReadable, 0 );
		skAddHandler( l, SK_H_PACKET, _clientData, 0 );
		skAddHandler( l, SK_H_CLOSE, _clientClose, 0 );

		l->data = malloc(sizeof(JsonData));
		l->data->f1 = &f1;
	}

	if ( clen )
	{
		f1++;
		sendJSONPacket(l, command );
	}

	while( f1 && --cnt )
		skTimeoutStep(30);

	return 0;
}
