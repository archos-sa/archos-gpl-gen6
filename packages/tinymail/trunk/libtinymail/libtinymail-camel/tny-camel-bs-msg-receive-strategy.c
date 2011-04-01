/* libtinymail-camel - The Tiny Mail base library for Camel
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

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include <tny-camel-bs-msg-receive-strategy.h>

#include <tny-camel-folder.h>
#include <tny-error.h>
#include <tny-msg.h>
#include <tny-header.h>
#include <tny-camel-msg.h>

#include <camel/camel-folder.h>
#include <camel/camel.h>
#include <camel/camel-session.h>
#include <camel/camel-service.h>
#include <camel/camel-store.h>

#include <tny-session-camel.h>
#include <tny-account.h>
#include <tny-camel-account.h>

#include <tny-status.h>
#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-common-priv.h"
#include "tny-camel-common-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API

#include <tny-camel-bs-msg.h>
#include <tny-camel-bs-mime-part.h>

#include <tny-fs-stream.h>

#include "tny-camel-account-priv.h"
#include "tny-camel-folder-priv.h"
#include "tny-camel-msg-header-priv.h"
#include "tny-camel-bs-msg-priv.h"
#include "tny-camel-bs-mime-part-priv.h"

static GObjectClass *parent_class = NULL;

static TnyMsg *
tny_camel_bs_msg_receive_strategy_perform_get_msg (TnyMsgReceiveStrategy *self, TnyFolder *folder, TnyHeader *header, GError **err)
{
	return TNY_CAMEL_BS_MSG_RECEIVE_STRATEGY_GET_CLASS (self)->perform_get_msg(self, folder, header, err);
}


TnyStream *
tny_camel_bs_msg_receive_strategy_start_receiving_part (TnyCamelBsMsgReceiveStrategy *self, TnyFolder *folder, TnyCamelBsMimePart *part, gboolean *binary, GError **err)
{
	TnyCamelBsMimePartPriv *ppriv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (part);
	gchar *uid = ppriv->uid;
	bodystruct_t *bodystruct = ppriv->bodystructure;
	gchar *part_spec = bodystruct->part_spec;
	TnyStream *retval = NULL;
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;
	char *filename;

	if (part_spec) 
	{
		CamelFolder *cfolder = _tny_camel_folder_get_camel_folder (TNY_CAMEL_FOLDER (folder));

		/* TODO: play with IMAP's CONVERT here ... */

		filename = camel_folder_fetch (cfolder, uid, part_spec?part_spec:"1", binary, &ex);

		if (camel_exception_is_set (&ex)) {
			_tny_camel_exception_to_tny_error (&ex, err);
			camel_exception_clear (&ex);
			retval = NULL;
		} else {
			int fd = open (filename, 0);

			if (fd == -1) {
				g_set_error (err, TNY_ERROR_DOMAIN,
					TNY_IO_ERROR_READ,
					"Can't open %s for reading", filename);
				retval = NULL;
			} else
				retval = tny_fs_stream_new (fd);
		}

		if (filename)
			g_free (filename);
	}

	return retval;
}

static TnyMsg *
tny_camel_bs_msg_receive_strategy_perform_get_msg_default (TnyMsgReceiveStrategy *self, TnyFolder *folder, TnyHeader *header, GError **err)
{
	TnyMsg *message = NULL;
	gchar *uid;
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;
	bodystruct_t *bodystructure = NULL;
	gchar *structure_str;
	GError *parse_err = NULL;
	CamelFolder *cfolder = _tny_camel_folder_get_camel_folder (TNY_CAMEL_FOLDER (folder));

	uid = tny_header_dup_uid (TNY_HEADER (header));
	structure_str = camel_folder_fetch_structure (cfolder, (const char *) uid, &ex);

	if (camel_exception_is_set (&ex)) {
		_tny_camel_exception_to_tny_error (&ex, err);
		camel_exception_clear (&ex);
		if (structure_str)
			g_free (structure_str);
		g_free (uid);
		return NULL;
	}

	if (structure_str) {
		bodystructure = bodystruct_parse ((guchar *) structure_str, strlen (structure_str),
			&parse_err);
		if (parse_err) {
			g_propagate_error (err, parse_err);
			bodystructure = NULL;
		}
		g_free (structure_str);
	}

	if (bodystructure) {
		message = _tny_camel_bs_msg_new (bodystructure, uid, NULL);

		_tny_camel_bs_msg_set_header (TNY_CAMEL_BS_MSG (message), header);
		_tny_camel_bs_mime_part_set_folder (TNY_CAMEL_BS_MIME_PART (message), folder);
		_tny_camel_bs_mime_part_set_strat (TNY_CAMEL_BS_MIME_PART (message), 
			TNY_CAMEL_BS_MSG_RECEIVE_STRATEGY (self));

	} else
		message = NULL;

	g_free (uid);

	return message;
}

static void
tny_camel_bs_msg_receive_strategy_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
tny_msg_receive_strategy_init (TnyMsgReceiveStrategyIface *klass)
{
	klass->perform_get_msg= tny_camel_bs_msg_receive_strategy_perform_get_msg;
}


/**
 * tny_camel_bs_msg_receive_strategy_new:
 * 
 * A message receiver that fetches bs messages
 *
 * Return value: A new #TnyMsgReceiveStrategy instance implemented for Camel
 **/
TnyMsgReceiveStrategy* 
tny_camel_bs_msg_receive_strategy_new (void)
{
	TnyCamelBsMsgReceiveStrategy *self = g_object_new (TNY_TYPE_CAMEL_BS_MSG_RECEIVE_STRATEGY, NULL);

	return TNY_MSG_RECEIVE_STRATEGY (self);
}


static void
tny_camel_bs_msg_receive_strategy_class_init (TnyCamelBsMsgReceiveStrategyClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;

	klass->perform_get_msg= tny_camel_bs_msg_receive_strategy_perform_get_msg_default;

	object_class->finalize = tny_camel_bs_msg_receive_strategy_finalize;
}

static void 
tny_camel_bs_msg_receive_strategy_instance_init (GTypeInstance *instance, gpointer g_class)
{
	return;
}


static gpointer
tny_camel_bs_msg_receive_strategy_register_type (gpointer notused)
{
	GType type = 0;
	static const GTypeInfo info = 
		{
			sizeof (TnyCamelBsMsgReceiveStrategyClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_bs_msg_receive_strategy_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelBsMsgReceiveStrategy),
			0,      /* n_preallocs */
			tny_camel_bs_msg_receive_strategy_instance_init,    /* instance_init */
			NULL
		};


	static const GInterfaceInfo tny_msg_receive_strategy_info = 
		{
			(GInterfaceInitFunc) tny_msg_receive_strategy_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyCamelBsMsgReceiveStrategy",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_MSG_RECEIVE_STRATEGY,
				     &tny_msg_receive_strategy_info);
	
	return GUINT_TO_POINTER (type);
}

GType
tny_camel_bs_msg_receive_strategy_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_camel_bs_msg_receive_strategy_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
