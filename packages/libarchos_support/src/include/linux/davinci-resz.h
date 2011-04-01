/*
 * davinci_resz.h
 *
 * Copyright (c) 2006 Archos
 * Author: Matthias Welwarsky
 *
 */

#ifndef _MEDIA_DAVINCI_RESZ_H
#define _MEDIA_DAVINCI_RESZ_H

#define VPSSRSZ_CMD_PARAMBLT_ASYNC	3
#define VPSSRSZ_CMD_WAIT_SYNC		4

typedef struct {
	unsigned long	addr;
	unsigned int	linestep;
	unsigned int	width, height;
} vpssrsz_rect_t;

typedef struct {
	short coeff[32];
} vpssrsz_coeff_t;

typedef struct {
	int size;
	vpssrsz_rect_t src_rect;
	vpssrsz_rect_t dst_rect;
	vpssrsz_coeff_t h_coeff;
	vpssrsz_coeff_t v_coeff;
	
	int	vbl_sync;
	int format;
} vpssrsz_rsz_cmd_t;

typedef struct {
	int size;
	vpssrsz_rect_t src_rect;
	vpssrsz_rect_t dst_rect;
	unsigned int  h_rsz, v_rsz;
	
	vpssrsz_coeff_t h_coeff;
	vpssrsz_coeff_t v_coeff;
	
	int	vbl_sync;
	int	format;
} vpssrsz_param_cmd_t;

#endif
