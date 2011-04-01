#include <ft2build.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "awchar.h"
#include "ft.h"

#include <harfbuzz-shaper.h>
#include <harfbuzz-global.h>
#include <harfbuzz-gpos.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

#define true 1
#define false 0

#define DBG if(1)

#define MIN(_a_,_b_) ( ( (_a_)<(_b_) ) ? _a_:_b_)
typedef unsigned long   	UINT32;

static HB_UChar32 getChar( const HB_UChar16 * string, hb_uint32 length, hb_uint32 * i )
{
	return string[*i];
}

static HB_Bool hb_stringToGlyphs( HB_Font font, const HB_UChar16 * string, hb_uint32 length, HB_Glyph * glyphs, hb_uint32 * numGlyphs, HB_Bool rightToLeft )
{
	FT_Face face = ( FT_Face ) font->faceData;
	if ( length > *numGlyphs )
		return false;

	int glyph_pos = 0;
	hb_uint32 i;
	for ( i = 0; i < length; ++i ) {
		glyphs[glyph_pos] = FT_Get_Char_Index( face, getChar( string, length, &i ) );
DBG		printf( "%04X -> %04X\r\n", getChar( string, length, &i ), glyphs[glyph_pos] );
		++glyph_pos;
	}

	*numGlyphs = glyph_pos;

	return true;
}

static void hb_getAdvances( HB_Font font, const HB_Glyph * glyphs, hb_uint32 numGlyphs, HB_Fixed * advances, int flags )
{
	hb_uint32 i;

	for ( i = 0; i < numGlyphs; ++i )
		advances[i] = 0;	// ### not tested right now
}

static HB_Bool hb_canRender( HB_Font font, const HB_UChar16 * string, hb_uint32 length )
{
	FT_Face face = ( FT_Face ) font->faceData;

	hb_uint32 i;
	for ( i = 0; i < length; ++i )
		if ( !FT_Get_Char_Index( face, getChar( string, length, &i ) ) )
			return false;

	return true;
}

static HB_Error hb_getSFntTable( void *font, HB_Tag tableTag, HB_Byte * buffer, HB_UInt * length )
{
	FT_Face face = ( FT_Face ) font;
	FT_ULong ftlen = *length;
	FT_Error error = 0;

	if ( !FT_IS_SFNT( face ) )
		return HB_Err_Invalid_Argument;

	error = FT_Load_Sfnt_Table( face, tableTag, 0, buffer, &ftlen );
	*length = ftlen;
	return ( HB_Error ) error;
}

HB_Error hb_getPointInOutline( HB_Font font, HB_Glyph glyph, int flags, hb_uint32 point, HB_Fixed * xpos, HB_Fixed * ypos, hb_uint32 * nPoints )
{
	HB_Error error = HB_Err_Ok;
	FT_Face face = ( FT_Face ) font->faceData;

	int load_flags = ( flags & HB_ShaperFlag_UseDesignMetrics ) ? FT_LOAD_NO_HINTING : FT_LOAD_DEFAULT;

DBG printf( "load %04X\r\n", glyph );
	if ( ( error = ( HB_Error ) FT_Load_Glyph( face, glyph, load_flags ) ) )
		return error;

	if ( face->glyph->format != ft_glyph_format_outline )
		return ( HB_Error ) HB_Err_Invalid_GPOS_SubTable;

	*nPoints = face->glyph->outline.n_points;
	if ( !( *nPoints ) )
		return HB_Err_Ok;

	if ( point > *nPoints )
		return ( HB_Error ) HB_Err_Invalid_GPOS_SubTable;

	*xpos = face->glyph->outline.points[point].x;
	*ypos = face->glyph->outline.points[point].y;

	return HB_Err_Ok;
}

void hb_getGlyphMetrics( HB_Font font, HB_Glyph glyph, HB_GlyphMetrics * metrics )
{
	// ###
	metrics->x = metrics->y = metrics->width = metrics->height = metrics->xOffset = metrics->yOffset = 0;
}

HB_Fixed hb_getFontMetric( HB_Font font, HB_FontMetric metric )
{
	return 0;		// ####
}

const HB_FontClass hb_fontClass = {
	hb_stringToGlyphs, 
	hb_getAdvances, 
	hb_canRender,
	hb_getPointInOutline, 
	hb_getGlyphMetrics, 
	hb_getFontMetric
};

typedef struct glyph_map_t
{
	FT_ULong uni;
	FT_UInt gi;
} glyph_map_t;

static glyph_map_t 	*glyph_map = NULL;
static int 		glyph_count = 0;
static HB_Face 		hbFace;
static HB_FontRec 	hbFont;

static int compare_glyph( const void *left, const void *right )
{
	return ((glyph_map_t*)left)->gi - ((glyph_map_t*)right)->gi;
}

// ************************************************
//
//	create_glyph_map
//
//	for harfbuzz we need a reverse mapping between glyph
//	indices and unicode points, build this map here
//
// ************************************************
static int create_glyph_map( FT_Face face )
{
	glyph_count = 0;

	FT_UInt  gindex;
	FT_ULong charcode = FT_Get_First_Char( face, &gindex );
	while ( gindex != 0 ) {
		glyph_count++;
		charcode = FT_Get_Next_Char( face, charcode, &gindex );
	}
	
	glyph_map = ( glyph_map_t * ) malloc( glyph_count * sizeof( glyph_map_t ) );
	if ( !glyph_map ) {
		glyph_count = 0;
		return 1;
	}

	glyph_map_t *gm = glyph_map;
	charcode = FT_Get_First_Char( face, &gindex );
	while ( gindex != 0 ) {
		gm->uni = charcode;
		gm->gi = gindex;
		gm++;
		charcode = FT_Get_Next_Char( face, charcode, &gindex );
	}

	// sort the list by glyph index
       	qsort(glyph_map, glyph_count, sizeof( glyph_map_t ), compare_glyph );
/*
 	int i; for( i = 0; i < glyph_count; i ++ ) {
printf("%08X -> %04X\r\n", glyph_map[i].uni, glyph_map[i].gi);
	}
*/
DBG printf( "glyph map: %d\r\n", glyph_count );
	return 0;
}

// ************************************************
//
//	free_glyph_map
//
// ************************************************
static void free_glyph_map( void )
{
	if( glyph_map )
		free( glyph_map );
	glyph_map   = NULL;
	glyph_count = 0;
}

// ************************************************
//
//	query_glyph_map
//
// ************************************************
static FT_ULong query_glyph_map( FT_UInt gi )
{
	// a little binary search for the right glyph index
	int top    = glyph_count - 1; 
	int bottom = 0;

	while( top >= bottom ) {
		int middle = ( top + bottom ) / 2;
		int c = gi - glyph_map[middle].gi;

		if(c == 0)
			return glyph_map[middle].uni;
		else if(c < 0)
			top = middle - 1;
		else
			bottom = middle + 1;
	}
	
	return 0;
}


static int shaping( awchar *text, int text_max, HB_Script script )
{
DBG printf( "\r\n---------------------------------------------------------------------\r\n\r\n" );
	#define MAX_GLYPHS 256
	HB_Glyph 		hb_glyphs     [MAX_GLYPHS];
	HB_GlyphAttributes 	hb_attributes [MAX_GLYPHS];
	HB_Fixed 		hb_advances   [MAX_GLYPHS];
	HB_FixedPoint 		hb_offsets    [MAX_GLYPHS];
	static unsigned short	hb_logClusters[MAX_GLYPHS];

	HB_ShaperItem shaper_item;

	shaper_item.kerning_applied     = false;
	shaper_item.string              = text;
	shaper_item.stringLength        = MIN( awstrlen( text ), MAX_GLYPHS );
	shaper_item.item.script         = script;
	shaper_item.item.pos            = 0;
	shaper_item.item.length         = shaper_item.stringLength;
	shaper_item.item.bidiLevel      = 2;	// ###
	shaper_item.shaperFlags         = 0;
	shaper_item.font                = &hbFont;
	shaper_item.face                = hbFace;
	shaper_item.num_glyphs          = shaper_item.item.length;
	shaper_item.glyphIndicesPresent = false;
	shaper_item.initialGlyphCount   = 0;

	while ( 1 ) {
DBG printf( "shaper_item.num_glyphs: %d\r\n", shaper_item.num_glyphs );

		memset( hb_glyphs,     0, MAX_GLYPHS * sizeof( HB_Glyph ) );
		memset( hb_attributes, 0, MAX_GLYPHS * sizeof( HB_GlyphAttributes ) );
		memset( hb_advances,   0, MAX_GLYPHS * sizeof( HB_Fixed ) );
		memset( hb_offsets,    0, MAX_GLYPHS * sizeof( HB_FixedPoint ) );

		shaper_item.glyphs       = hb_glyphs;
		shaper_item.attributes   = hb_attributes;
		shaper_item.advances     = hb_advances;
		shaper_item.offsets      = hb_offsets;
		shaper_item.log_clusters = hb_logClusters;

		if ( HB_ShapeItem( &shaper_item ) )
			break;
	}

DBG printf( "shaper_item.num_glyphs: %d\r\n", shaper_item.num_glyphs );
	
	unsigned int i;
	for ( i = 0; i < shaper_item.num_glyphs; ++i ) {
		// we need to get back the unicode point from the glyph index,
		// hopefully this works!
		UINT32 uni = query_glyph_map( shaper_item.glyphs[i] );
		if( uni && text_max-- > 0 ) {
			*text ++ = uni;
		}
DBG printf( "%2d  glyph %04X  uni %04X  attr %08X  adv %d  offs %d\r\n",
		i, shaper_item.glyphs[i], ( unsigned int ) uni, ( shaper_item.attributes[i] ), shaper_item.advances[i], ( shaper_item.offsets[i] ) );
	}
	*text = 0;
	
	return 0;
}

static int no_harfbuzz = 0;

int harfbuzz_arabic( awchar *text, int text_max )
{
	if( no_harfbuzz )
		return 0;
	if( !glyph_map )
		return 0;
		
	return shaping( text, text_max, HB_Script_Arabic );
}

int harfbuzz_init( FT_Face face )
{

	if( create_glyph_map( face ) )
		return 1;

	hbFace = HB_NewFace( face, hb_getSFntTable );

	hbFont.klass    = &hb_fontClass;
	hbFont.userData = 0;
	hbFont.faceData = face;
	hbFont.x_ppem   = face->size->metrics.x_ppem;
	hbFont.y_ppem   = face->size->metrics.y_ppem;
	hbFont.x_scale  = face->size->metrics.x_scale;
	hbFont.y_scale  = face->size->metrics.y_scale;


	return 0;
}

int harfbuzz_exit( void )
{
	HB_FreeFace( hbFace );
	
	free_glyph_map();
	
	return 0;
}

