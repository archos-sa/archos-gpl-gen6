#ifndef __HARFBUZZ_H
#define __HARFBUZZ_H

#include "awchar.h"
#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef __cplusplus
extern "C"
#endif
int harfbuzz_arabic( awchar *text, int text_max );

#ifdef __cplusplus
extern "C"
#endif
int harfbuzz_init( FT_Face face );

#endif // __HARFBUZZ_H
