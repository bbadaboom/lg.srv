#include "Serv.h"

static	SkHandler *_GetHandlerRec( void )
{
	return( (SkHandler*)malloc(sizeof(SkHandler)) );
}

void	_skFreeHandlerRec( SkHandler *h )
{
	free( h );
}

int	skAddHandler( SkLine *h, int htype, SkHProc proc, void *ownparam )
{
	SkHandler	*hnd;
	SkHandler	*hndp;

	hnd = _GetHandlerRec();
	hnd->type = htype;
	hnd->proc = proc;
	hnd->flags = 0;
	hnd->next = 0;
	hnd->ownparam = ownparam;

	if ( !h->handler )
	{
		h->handler = hnd;
		return( 0 );
	}
	for( hndp = h->handler; hndp->next; hndp=hndp->next );
	hndp->next = hnd;
	return( 0 );
}

int	skDoHandler( SkLine *h, int htype, void *sysparam )
{
	SkHandler	*hnd;
	SkHandler	*hnext = 0;
	int			cnt=0;

	for( hnd=h->handler; hnd; hnd = hnext )
	{
		if ( h->disabled )
			return(cnt);
		hnext = hnd->next;
		if ( hnd->type == htype )
		{
			cnt++;
			hnd->proc( h, htype, hnd->ownparam, sysparam );
			if (h->freetime) return (cnt); /* line deleted inside handle */

			if ( hnext && ( hnext->flags != 0 ))	/* somthing is wrong ! */
				return( cnt );
		}
		if ( htype == SK_H_CLOSE )
			hnd->flags = 1;
	}
	return( cnt );
}

int	skRemoveHandler( SkLine *h, int htype, SkHProc proc, void *ownparam )
{
	SkHandler	*hnd;
	SkHandler	*hndp;
	SkHandler	*hnext;

	if ( !h ) return -1;

	for( hndp=0, hnd=h->handler; hnd; hnd = hnext )
	{
		hnext = hnd->next;
		if (( hnd->type == htype ) &&
			( hnd->proc == proc ) &&
			( hnd->ownparam == ownparam ))
		{
			if ( hndp )
			{
				hndp->next = hnd->next;
			}
			else
			{
				h->handler = hnd->next;
			}
			_skFreeHandlerRec( hnd );
			return( 0 );
		}
		hndp = hnd;
	}
	return -1;
}
