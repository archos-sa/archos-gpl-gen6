#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "dmalloc.h"

#define DMALLOC_DEVICE	"/dev/dmalloc"
#define DMALLOC_FREE	1
#define DMALLOC_GETPHYS	2

#define DBG if (0)
#define ERR if (1)

#ifndef SIM

static int dmalloc_fd = -1;
static int dmalloc_fd_cached = -1;

int DMALLOC_init(void) 
{
	if (dmalloc_fd < 0)
		dmalloc_fd = open(DMALLOC_DEVICE, O_RDWR|O_SYNC);
	if (dmalloc_fd < 0) {
ERR		fprintf(stderr, "DMALLOC_init - failed: %s\n", strerror(errno));
		return -1;
	}
	fcntl(dmalloc_fd, F_SETFD, FD_CLOEXEC);
	
	if (dmalloc_fd_cached < 0)
		dmalloc_fd_cached = open(DMALLOC_DEVICE, O_RDWR);
	if (dmalloc_fd_cached < 0) {
ERR		fprintf(stderr, "DMALLOC_init - failed: %s\n", strerror(errno));
		return -1;
	}
	fcntl(dmalloc_fd_cached, F_SETFD, FD_CLOEXEC);
	
	return 0;
}

void* DMALLOC_alloc(size_t size)
{
	void* addr;
	
	if (dmalloc_fd <0) {
ERR		fprintf(stderr, "DMALLOC_alloc - not initialized\n");
		return 0;
	}
	
	addr = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, dmalloc_fd, 0);
	if (addr == MAP_FAILED) {
ERR		fprintf(stderr, "DMALLOC_alloc - cannot allocate %d byte: %s\n", size, strerror(errno));
		return NULL;
	}
	
DBG	fprintf(stderr, "DMALLOC_alloc - allocated %d byte at %p\n", size, addr);

	return addr;
}

int  DMALLOC_free(void* addr)
{
	if (dmalloc_fd < 0) {
DBG		fprintf(stderr, "DMALLOC_free - not initialized\n");
		return -1;
	}
	
	if (ioctl(dmalloc_fd, DMALLOC_FREE, (unsigned long)addr) < 0) {
ERR		fprintf(stderr, "DMALLOC_free - cannot free memory at %p: %s\n", addr, strerror(errno));
		return -1;
	}
	
DBG	fprintf(stderr, "DMALLOC_free - freed memory at %p\n", addr);
	return 0;
}

int   DMALLOC_exit(void)
{
	close(dmalloc_fd);
	dmalloc_fd = -1;
	return 0;
}

/* CMEM compatibility functions */
int CMEM_init(void)
{
	return DMALLOC_init();
}

int CMEM_exit(void)
{
	return DMALLOC_exit();
}

void* CMEM_alloc(size_t size)
{
	return DMALLOC_alloc(size);
}

int CMEM_free(void* buf)
{
	return DMALLOC_free(buf);
}

unsigned long CMEM_getPhys(void *ptr)
{
	unsigned long addr = (unsigned long)ptr;

	if (dmalloc_fd < 0) {
DBG		fprintf(stderr, "CMEM_getPhys - not initialized\n");
		return 0;
	}
	
	if (ioctl(dmalloc_fd, DMALLOC_GETPHYS, &addr) < 0) {
ERR		fprintf(stderr, "CMEM_getPhys - translation of %p failed\n", ptr);
		return 0;
	}
	
DBG	fprintf(stderr, "CMEM_getPhys - %p -> 0x%08X\n", ptr, addr);
	return addr;
}

//////////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////////
void* DMALLOC_alloc_cached(size_t size)
{
	void* addr;
	
	if (dmalloc_fd_cached <0) {
ERR		fprintf(stderr, "DMALLOC_alloc - not initialized\n");
		return 0;
	}
	
	addr = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, dmalloc_fd_cached, 0);
	if (addr == MAP_FAILED) {
ERR		fprintf(stderr, "DMALLOC_alloc - cannot allocate %d byte: %s\n", size, strerror(errno));
		return NULL;
	}
	
DBG	fprintf(stderr, "DMALLOC_alloc - allocated %d byte at %p\n", size, addr);

	return addr;
}

int  DMALLOC_free_cached(void* addr)
{
	if (dmalloc_fd_cached < 0) {
DBG		fprintf(stderr, "DMALLOC_free - not initialized\n");
		return -1;
	}
	
	if (ioctl(dmalloc_fd_cached, DMALLOC_FREE, (unsigned long)addr) < 0) {
ERR		fprintf(stderr, "DMALLOC_free - cannot free memory at %p: %s\n", addr, strerror(errno));
		return -1;
	}
	
DBG	fprintf(stderr, "DMALLOC_free - freed memory at %p\n", addr);
	return 0;
}

#else

int   DMALLOC_init(void)
{
	return 0;
}

int   DMALLOC_exit(void)
{
	return 0;
}

void* DMALLOC_alloc(size_t size)
{
	unsigned char *start = (unsigned char*)malloc(size + sizeof(unsigned int) + 32);
	
	if (!start)
		return NULL;

	unsigned char *ptr = (unsigned char *)(((unsigned long)start + 31) & ~0x1f);
	
	if (ptr == start)
		ptr += 32;
			
DBG	fprintf(stderr, "DMALLOC_alloc - allocated %d byte at real %p, %p\n", size, start, ptr);

	*((unsigned int*)(ptr-sizeof(unsigned int*))) = (unsigned int)start;

	return ptr;
}

int DMALLOC_free(void* addr)
{
	unsigned int start = *((unsigned int*)(((unsigned char*)addr)-sizeof(unsigned int)));
	
DBG	fprintf(stderr, "DMALLOC_free - freed memory at real %p, %p\n", start, addr);

	free(start);
	
	return 0;
}

void* DMALLOC_alloc_cached(size_t size)
{
	return malloc(size);
}

int   DMALLOC_free_cached(void* addr)
{
	free(addr);
	return 0;
}

#endif
