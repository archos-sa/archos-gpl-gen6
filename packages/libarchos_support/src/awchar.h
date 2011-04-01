#ifndef AWCHAR_H
#define AWCHAR_H

#include <stddef.h>

typedef unsigned short int awchar;

#ifdef __cplusplus
extern "C"
#endif
const awchar *bidi( const awchar *arg );

#ifdef __cplusplus
extern "C"
#endif
awchar *awstrncpy(register awchar *dest, register const awchar *src, register size_t n);

#ifdef __cplusplus
extern "C"
#endif
size_t awstrlen(const awchar *string);

#ifdef __cplusplus
extern "C"
#endif
const char *w2c( const awchar *wide );

#endif // AWCHAR_H
