#ifndef TNY_GTK_PIXBUF_STREAM_H
#define TNY_GTK_PIXBUF_STREAM_H

/* libtinymailui-gtk - The Tiny Mail UI library for Gtk+
 * Copyright (C) 2006-2007 Philip Van Hoof <pvanhoof@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with self library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <glib.h>
#include <gtk/gtk.h>
#include <glib-object.h>

#include <tny-stream.h>

G_BEGIN_DECLS

#define TNY_TYPE_GTK_PIXBUF_STREAM             (tny_gtk_pixbuf_stream_get_type ())
#define TNY_GTK_PIXBUF_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_GTK_PIXBUF_STREAM, TnyGtkPixbufStream))
#define TNY_GTK_PIXBUF_STREAM_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_GTK_PIXBUF_STREAM, TnyGtkPixbufStreamClass))
#define TNY_IS_GTK_PIXBUF_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_GTK_PIXBUF_STREAM))
#define TNY_IS_GTK_PIXBUF_STREAM_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_GTK_PIXBUF_STREAM))
#define TNY_GTK_PIXBUF_STREAM_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_GTK_PIXBUF_STREAM, TnyGtkPixbufStreamClass))

typedef struct _TnyGtkPixbufStream TnyGtkPixbufStream;
typedef struct _TnyGtkPixbufStreamClass TnyGtkPixbufStreamClass;

struct _TnyGtkPixbufStream
{
	GObject parent;
};

struct _TnyGtkPixbufStreamClass 
{
	GObjectClass parent;

	/* virtual methods */
	gssize (*read) (TnyStream *self, char *buffer, gsize n);
	gssize (*write) (TnyStream *self, const char *buffer, gsize n);
	gint (*flush) (TnyStream *self);
	gint (*close) (TnyStream *self);
	gboolean (*is_eos) (TnyStream *self);
	gint (*reset) (TnyStream *self);
	gssize (*write_to_stream) (TnyStream *self, TnyStream *output);
};

GType tny_gtk_pixbuf_stream_get_type (void);
TnyStream* tny_gtk_pixbuf_stream_new (const gchar *mime_type);

GdkPixbuf *tny_gtk_pixbuf_stream_get_pixbuf (TnyGtkPixbufStream *self);

G_END_DECLS

#endif

