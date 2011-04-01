#ifndef CAMEL_IMAP_STORE_PRIV_H
#define CAMEL_IMAP_STORE_PRIV_H 1
/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* camel-imap-store-priv.h : class for an imap store */

/*
 * Authors: Jose Dapena Paz <jdapena@igalia.com>
 *
 * Copyright (C) 2000 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#include <camel/camel-object.h>

G_BEGIN_DECLS

void _camel_imap_store_current_folder_finalize (CamelObject *stream, gpointer event_data, gpointer user_data);
void _camel_imap_store_old_folder_finalize (CamelObject *stream, gpointer event_data, gpointer user_data);
void _camel_imap_store_last_folder_finalize (CamelObject *stream, gpointer event_data, gpointer user_data);

G_END_DECLS

#endif /* CAMEL_IMAP_STORE_PRIV_H */
