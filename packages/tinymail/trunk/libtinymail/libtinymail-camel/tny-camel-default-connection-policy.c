/* Your copyright here */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include <tny-camel-default-connection-policy.h>

static GObjectClass *parent_class = NULL;

static void
tny_camel_default_connection_policy_set_current (TnyConnectionPolicy *self, TnyAccount *account, TnyFolder *folder)
{
	return;
}

static void
tny_camel_default_connection_policy_on_connect (TnyConnectionPolicy *self, TnyAccount *account)
{
	return;
}

static void
tny_camel_default_connection_policy_on_connection_broken (TnyConnectionPolicy *self, TnyAccount *account)
{
	return;
}

static void
tny_camel_default_connection_policy_on_disconnect (TnyConnectionPolicy *self, TnyAccount *account)
{
	return;
}

static void
tny_camel_default_connection_policy_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
tny_camel_default_connection_policy_instance_init (GTypeInstance *instance, gpointer g_class)
{
}

static void
tny_connection_policy_init (TnyConnectionPolicyIface *klass)
{
	klass->on_connect= tny_camel_default_connection_policy_on_connect;
	klass->on_connection_broken= tny_camel_default_connection_policy_on_connection_broken;
	klass->on_disconnect= tny_camel_default_connection_policy_on_disconnect;
	klass->set_current= tny_camel_default_connection_policy_set_current;
}

static void
tny_camel_default_connection_policy_class_init (TnyCamelDefaultConnectionPolicyClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	object_class->finalize = tny_camel_default_connection_policy_finalize;
}



/**
 * tny_camel_default_connection_policy_new:
 * 
 * A connection policy that does nothing special
 *
 * Return value: A new #TnyConnectionPolicy instance 
 **/
TnyConnectionPolicy*
tny_camel_default_connection_policy_new (void)
{
	return TNY_CONNECTION_POLICY (g_object_new (TNY_TYPE_CAMEL_DEFAULT_CONNECTION_POLICY, NULL));
}

static gpointer
tny_camel_default_connection_policy_register_type (gpointer notused)
{
	GType type = 0;
	static const GTypeInfo info = 
		{
			sizeof (TnyCamelDefaultConnectionPolicyClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_default_connection_policy_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelDefaultConnectionPolicy),
			0,      /* n_preallocs */
			tny_camel_default_connection_policy_instance_init,    /* instance_init */
			NULL
		};
	
	
	static const GInterfaceInfo tny_connection_policy_info = 
		{
			(GInterfaceInitFunc) tny_connection_policy_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyCamelDefaultConnectionPolicy",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_CONNECTION_POLICY,
				     &tny_connection_policy_info);
	
	return GUINT_TO_POINTER (type);
}

GType
tny_camel_default_connection_policy_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_camel_default_connection_policy_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
