#include <ft2build.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "awchar.h"
#include "harfbuzz.h"

#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

#define FONT_MAX 1

typedef struct stat		STAT;
typedef unsigned char   	UCHAR;

static FT_Library ft_library;
static unsigned char *ft_buffer = NULL;

static FT_Face ft_face[FONT_MAX] = { 0 };
static int font_loaded      = 0;

#define FONT_14				14
#define FONT_16				16
#define FONT_22				22
#define FONT_24				24
#define FONT_28				28
#define FONT_36				36
#define FONT_40				40

typedef struct {
	int size;
	int base;
	int width;
	int height;
	int extend_height;
} FONT_INFO;

#define CONFIG_FONT_28
#define CONFIG_FONT_40

static FONT_INFO font_info[] = {
#ifdef CONFIG_FONT_14
	{FONT_14, 13, 14, 14, 15},
#endif
#ifdef CONFIG_FONT_16
	{FONT_16, 14, 16, 16, 16},
#endif
#ifdef CONFIG_FONT_22
	{FONT_22, 16, 22, 22, 22},
#endif
	{FONT_24, 18, 24, 24, 24},
#ifdef CONFIG_FONT_28
	{FONT_28, 22, 28, 28, 28},
#endif
	{FONT_36, 28, 36, 36, 36},
#ifdef CONFIG_FONT_40
	{FONT_40, 30, 40, 40, 40}
#endif
};

static FT_Face i18n_face[FONT_MAX] = { 0 };
	
static unsigned char *i18n_buffer = NULL;
static int font_i18n_loaded = 0;
static int _unload_font_i18n( void );

// ************************************************
//
//	_create_map
//
// ************************************************
static int _create_map( FT_Face *face, int base )
{
	int err;
	// Create character maps
	int n;
	for ( n = 0; n < (*face)->num_charmaps; n++ ) { 
		FT_CharMap charmap = (*face)->charmaps[n]; 
//printf("\t%d  enc %.4s  pf_id %d  enc_id %d\r\n", n, (char*)&charmap->encoding, charmap->platform_id, charmap->encoding_id ); 
		if( charmap->platform_id == 3 ) {
			if( FT_Set_Charmap( *face, charmap ) ) {
printf("error setting charmap %d\r\n", base);
			}
			break;
		}
	} 
	
	// Specify the character dimensions
	if( (err = FT_Set_Pixel_Sizes( *face, 0, base )) ) {
printf("error set pixel size %d: %d\r\n", base, err);
		return 1;
	}

	return 0;
}

// ************************************************
//
//	_create_mem_face
//
// ************************************************
static int _create_mem_face( FT_Face *face, unsigned char *buffer, int buffer_size, int base )
{
printf("_create_mem_face( %d )\r\n", base );
	// Create Font Face
	int err = FT_New_Memory_Face( ft_library, buffer, buffer_size, 0, face );
	if( err == FT_Err_Unknown_File_Format ) {
printf("unknown file format font %d\r\n", base);
		return 1;
	} else if ( err ) {
printf("error creating face %d: %d\r\n", base, err);
		return 1;
	}
	
	return _create_map( face, base );
}

// ************************************************
//
//	_create_file_face
//
// ************************************************
static int _create_file_face( FT_Face *face, const char *path, int base )
{
printf("_create_file_face( %s, %d )\r\n", path, base );
	// Create Font Face
	int err = FT_New_Face( ft_library, path, 0, face );
	if( err == FT_Err_Unknown_File_Format ) {
printf("unknown file format font %d\r\n", base);
		return 1;
	} else if ( err ) {
printf("error creating face %d: %d\r\n", base, err);
		return 1;
	}
	
	return _create_map( face, base );
}

// ************************************************
//
//	_create_bitmap_face
//
// ************************************************
static int _create_bitmap_face( FT_Face *face, const char *path )
{
printf("_create_bitmap_face( %s )\r\n", path );
	// Create Font Face
	int err = FT_New_Face( ft_library, path, 0, face );
	if( err == FT_Err_Unknown_File_Format ) {
printf("unknown file format font %s\r\n", path);
		return 1;
	} else if ( err ) {
printf("error creating face %s: %d\r\n", path, err);
		return 1;
	}
/*
printf("\tnum_faces      : %d\r\n", (*face)->num_faces);
printf("\tface_index     : %d\r\n", (*face)->face_index);
printf("\tface_flags     : %08X\r\n", (*face)->face_flags);
printf("\tstyle_flags    : %08X\r\n",(* face)->style_flags);
printf("\tnum_glyphs     : %d\r\n", (*face)->num_glyphs);
printf("\tfamily_name    : %s\r\n",(*face)->family_name );
printf("\tstyle_name     : %s\r\n", (*face)->style_name);
printf("\tnum_fixed_sizes: %d\r\n", (*face)->num_fixed_sizes);
printf("\t    height     : %d\r\n", (*face)->available_sizes[0].height);
printf("\t    width      : %d\r\n", (*face)->available_sizes[0].width);
printf("\t    size       : %d\r\n", (*face)->available_sizes[0].size);
printf("\tnum_charmaps   : %d\r\n", (*face)->num_charmaps);
*/
	// Create character maps
	int n;
	for ( n = 0; n < (*face)->num_charmaps; n++ ) { 
		FT_CharMap charmap = (*face)->charmaps[n]; 
//printf("\t%d  enc %.4s  pf_id %d  enc_id %d\r\n", n, (char*)&charmap->encoding, charmap->platform_id, charmap->encoding_id ); 
		if( charmap->platform_id == 3 ) {
			if( FT_Set_Charmap( *face, charmap ) ) {
printf("error setting charmap 1\r\n");
			}
			break;
		}
	} 
	
	// Specify the character dimensions
	if( (err = FT_Set_Pixel_Sizes( *face, 0, (*face)->available_sizes[0].height )) ) {
/*#ifdef SIM
		// with never versions of freetype the above call does not always work
		// in this case we just select the first strike.
		if( (err = FT_Select_Size( *face, 0 )) ) {
printf("error FT_Select_Size( %d ): %d\r\n", 0, err);
			return 1;
		}
#else	*/	
printf("error FT_Set_Pixel_Sizes %d: %d\r\n", (*face)->available_sizes[0].height, err);
		return 1;
//#endif
	}
	return 0;
}
//#endif

// ************************************************
//
//	_unload_font
//
// ************************************************
static int _unload_font( void )
{
	if( font_loaded ) {
		int i;
		for( i = 0; i < FONT_MAX; i ++ ) {
			if( ft_face[i] ) {
				FT_Done_Face( ft_face[i] ); 
				ft_face[i] = NULL;
			}
		} 
		if( ft_buffer ) {
			free( ft_buffer );
			ft_buffer = NULL;
		}
		font_loaded = 0;
	}
	return 0;
}

// ************************************************
//
//	_load_font
//
// ************************************************
static int _load_font( const char *font_name )
{
	int fh;
	STAT st;
	
/*#ifdef CONFIG_FT_CJK
#ifndef CONFIG_ALL_FONTS
	_unload_font_cjk();
#endif
#endif	
#ifdef CONFIG_FT_I18N
#ifndef CONFIG_ALL_FONTS
	_unload_font_i18n();
#endif
#endif	*/
	if( font_loaded ) {
		return 0;
	}
printf("FT_load_font( %s )\r\n", font_name );

	// init FreeType library
	if ( FT_Init_FreeType( &ft_library ) ) {
printf("error in freetype init\r\n");	 
		return 1;
	}

	// open font file
	if ( (fh = open( font_name, O_RDONLY, 40 )) == -1 ) {
printf("cannot open font\r\n" );	
		return 1;
	}
	fstat( fh, &st );
	
	if( !(ft_buffer = (UCHAR*)malloc( st.st_size )) ) {
printf("cannot alloc font mem\r\n" );	
		close( fh );
		return 1;
	}
	
	if( read( fh, ft_buffer, st.st_size ) != st.st_size ) {
printf("cannot read font\r\n" );	
		close( fh );
		return 1;
	}
	close( fh );

	// create font faces
	int i;
	for( i = 0; i < FONT_MAX; i ++ ) {
		if( _create_mem_face( &ft_face[i], ft_buffer, st.st_size, font_info[i].base ) ) {
printf("error creating font 1\r\n");	 
			return 1;
		}
	} 

	// done!
	font_loaded = 1;
	return 0;
}

// ************************************************
//
//	_unload_font_i18n
//
// ************************************************
static int _unload_font_i18n( void )
{
	if( font_i18n_loaded ) {
printf("_unload_font_i18n\r\n");
		
/*#ifdef CONFIG_HARFBUZZ
		harfbuzz_exit();
#endif		*/
		int i;
		for( i = 0; i < FONT_MAX; i ++ ) {
			if( i18n_face[i] ) {
				FT_Done_Face( i18n_face[i] ); 
				i18n_face[i] = NULL;
			}
		} 

		if( i18n_buffer ) {
			free( i18n_buffer );
			i18n_buffer = NULL;
		}

		font_i18n_loaded = 0;
	}
	return 0;
}

#define FT_CACHE_MAX	199
#define MAX_FONT_HEIGHT		42
#define FT_BITMAP_MAX_ROWS	42

typedef struct {
	awchar 	keycode;
	int 	height;
	int 	advance;
	int 	rsb_delta;
	int 	lsb_delta;
	UCHAR 	data[FT_BITMAP_MAX_ROWS * MAX_FONT_HEIGHT];
} FT_CACHE_ENTRY;

static FT_CACHE_ENTRY cache[FT_CACHE_MAX];

// ************************************************
//
//	_reset_cache
//
// ************************************************
static void _reset_cache( void )
{
	memset( cache, 0, sizeof(cache) );
}

// ************************************************
//
//	FT_load_font_i18n
//
// ************************************************
int FT_load_font_i18n( const char *font )
{
      	if ( FT_Init_FreeType( &ft_library ) )
		return 1;

printf("load I18 font( %s )\r\n", font  );
	if( font_i18n_loaded ) {
printf("already loaded\r\n");
		return 0;	
	}

	_reset_cache();

#define CONFIG_I18N_HDD

#ifdef CONFIG_I18N_HDD	
	// create font faces
	int i;
	for( i = 0; i < FONT_MAX; i ++ ) {
		if( _create_file_face( &i18n_face[i], font, font_info[i].base ) ) {
printf("error creating font: %d, %s  base %d\r\n", i, font, font_info[i].base );	 
			return 1;
		}
	} 
#else
	int fh;
	STAT st;

	// open 2nd font file for I18N chars
	if ( (fh = open( font, O_RDONLY, 40 )) == -1 ) {
printf("cannot open font\r\n" );	
		return 1;
	}
	fstat( fh, &st );
	
	if( !(i18n_buffer = (UCHAR*)malloc( st.st_size )) ) {
printf("cannot alloc font mem\r\n" );	
		close( fh );
		return 1;
	}
	
	if( read( fh, i18n_buffer, st.st_size ) != st.st_size ) {
printf("cannot read font\r\n" );	
		close( fh );
		return 1;
	}
	close( fh );

	// create font faces
	int i;
	for( i = 0; i < FONT_MAX; i ++ ) {
		if( _create_mem_face( &i18n_face[i], i18n_buffer, st.st_size, font_info[i].base ) ) {
printf("error creating font: %d, base %d\r\n", i, font_info[i].base );	 
			return 1;
		}
	} 
#endif

	harfbuzz_init( i18n_face[0] );

	font_i18n_loaded = 1;
	return 0;
}	
