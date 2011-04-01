#ifndef TNY_GTK_ACCOUNT_LIST_MODEL_H
#define TNY_GTK_ACCOUNT_LIST_MODEL_H

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
#include <tny-account.h>

G_BEGIN_DECLS

#define TNY_TYPE_GTK_ACCOUNT_LIST_MODEL             (tny_gtk_account_list_model_get_type ())
#define TNY_GTK_ACCOUNT_LIST_MODEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_GTK_ACCOUNT_LIST_MODEL, TnyGtkAccountListModel))
#define TNY_GTK_ACCOUNT_LIST_MODEL_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_GTK_ACCOUNT_LIST_MODEL, TnyGtkAccountListModelClass))
#define TNY_IS_GTK_ACCOUNT_LIST_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_GTK_ACCOUNT_LIST_MODEL))
#define TNY_IS_GTK_ACCOUNT_LIST_MODEL_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_GTK_ACCOUNT_LIST_MODEL))
#define TNY_GTK_ACCOUNT_LIST_MODEL_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_GTK_ACCOUNT_LIST_MODEL, TnyGtkAccountListModelClass))

typedef struct _TnyGtkAccountListModel TnyGtkAccountListModel;
typedef struct _TnyGtkAccountListModelClass TnyGtkAccountListModelClass;

#define TNY_TYPE_GTK_ACCOUNT_LIST_MODEL_COLUMN (tny_gtk_account_list_model_column_get_type())

enum _TnyGtkAccountListModelColumn
{
	TNY_GTK_ACCOUNT_LIST_MODEL_NAME_COLUMN,
	TNY_GTK_ACCOUNT_LIST_MODEL_INSTANCE_COLUMN,
	TNY_GTK_ACCOUNT_LIST_MODEL_N_COLUMNS
};

typedef enum _TnyGtkAccountListModelColumn TnyGtkAccountListModelColumn;

struct _TnyGtkAccountListModel
{
	GtkListStore parent;
	GMutex *iterator_lock;
 	GList *first;
	gboolean first_needs_unref;
};

struct _TnyGtkAccountListModelClass
{
	GtkListStoreClass parent_class;
};


GType tny_gtk_account_list_model_get_type (void);
GType tny_gtk_account_list_model_column_get_type (void);
GtkTreeModel* tny_gtk_account_list_model_new (void);

G_END_DECLS

#endif
