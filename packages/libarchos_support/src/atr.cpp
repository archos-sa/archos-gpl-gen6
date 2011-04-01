#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "atr.h"
#include "awchar.h"
#include "ft.h"

namespace archos {

struct KeyTr {
	int key;
	int tr;
};

}

using namespace archos;

static int tr_num = 0;
static char *tr_data = NULL;
static const char *tr_strings = NULL;
static KeyTr *tr_tab;


void archos::atr_unload( void )
{
	if( tr_data ) {
		free( tr_data );
		tr_data = NULL;
	}
	tr_num = 0;
}

int archos::atr_load( const char *file )
{
	int fd;
	struct stat st;
	
	atr_unload();
	
	fd = open( file, O_RDONLY );
	if( fd == -1 )
		return 1;
	
	if( fstat( fd, &st ) ) {
		close( fd );
		return 1;
	}	

	if( !(tr_data = (char*)malloc( st.st_size )) ) {
		close( fd );
		return 1;
	}
	
	read( fd, tr_data, st.st_size );
	close( fd );
	
	if( strncmp( tr_data, "TREX", 4 ) ) {
		free( tr_data );
		tr_data = NULL;
		return 1;
	} 

	tr_num = *((int*)(tr_data + 4));
printf("tr_num: %d\r\n", tr_num );	
	tr_tab = (KeyTr*)(tr_data + 8);	
	tr_strings = tr_data + 8 + (tr_num * 8);

 	FT_load_font_i18n( "/opt/usr/share/fonts/archos_world.ttf" );

	return 0;
}

static int find_key( const char *key )
{
	// a little binary search for the right key
	int top    = tr_num - 1; 
	int bottom = 0;

	while( top >= bottom ) {
		int middle = ( top + bottom ) / 2;
		const char *_key = tr_strings + tr_tab[middle].key;
		int c = strcmp( key, _key );

		if(c == 0)
			return( middle );
		else if(c < 0)
			top = middle - 1;
		else
			bottom = middle + 1;
	}
	
	return -1;
}

const char *archos::atr( const char *key )
{
	const char* result;
	int k = find_key( key );

	if( k == -1 )
		return key;
	
	result = tr_strings + tr_tab[k].tr;

	if( !strcmp( result, "_no_tr_" ) ) {
		return key;
	}

	return result;
}
