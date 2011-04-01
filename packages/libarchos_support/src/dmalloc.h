#ifndef _DMALLOC_H
#define _DMALLOC_H

#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif

extern int   DMALLOC_init(void);
extern void* DMALLOC_alloc(size_t size);
extern int   DMALLOC_free(void* addr);
extern int   DMALLOC_exit(void);

extern void* DMALLOC_alloc_cached(size_t size);
extern int   DMALLOC_free_cached(void* addr);

#ifdef __cplusplus
}
#endif

#endif
