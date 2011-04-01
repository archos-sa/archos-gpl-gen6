/*
 * davinci-imgdma.h
 *
 * Copyright (c) 2006 Archos
 * Author: Matthias Welwarsky
 *
 */

#ifndef _MEDIA_DAVINCI_IMGDMA_H
#define _MEDIA_DAVINCI_IMGDMA_H

#define IMGDMA_START_TRANSFER	1
#define IMGDMA_WAIT_SYNC	2

typedef struct {
	unsigned int    size;
	unsigned long	src_addr;
	unsigned long	src_linestep;
	unsigned long	dst_addr;
	unsigned long	dst_linestep;
	unsigned long	width;
	unsigned long	height;
	int		vblsync;
} dma_transfer_t;

#endif
