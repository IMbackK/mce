/**
 * @file mce-gconf.c
 * Gconf handling code for the Mode Control Entity
 * <p>
 * Copyright © 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
 * <p>
 * @author David Weinehall <david.weinehall@nokia.com>
 * @author Jonathan Wilson <jfwfreo@tpgi.com.au>
 *
 * mce is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * mce is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with mce.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <glib.h>
#include <gconf/gconf-client.h>
#include "mce.h"
#include "mce-gconf.h"
#include "mce-log.h"

/** Pointer to the GConf client */
static GConfClient *gconf_client = NULL;
/** List of GConf notifiers */
static GSList *gconf_notifiers = NULL;

/**
 * Set an integer GConf key to the specified value
 *
 * @param key The GConf key to set the value of
 * @param value The value to set the key to
 * @return TRUE on success, FALSE on failure
 */
gboolean mce_gconf_set_int(const gchar *const key, const gint value)
{
	gboolean status = FALSE;

	if (gconf_client_set_int(gconf_client, key, value, NULL) == FALSE) {
		mce_log(LL_WARN, "Failed to write %s to GConf", key);
		goto EXIT;
	}

	/* synchronise if possible, ignore errors */
	gconf_client_suggest_sync(gconf_client, NULL);

	status = TRUE;

EXIT:
	return status;
}

/**
 * Return a boolean from the specified GConf key
 *
 * @param key The GConf key to get the value from
 * @param[out] value Will contain the value on return, if successful
 * @return TRUE on success, FALSE on failure
 */
gboolean mce_gconf_get_bool(const gchar *const key, gboolean *value)
{
	gboolean status = FALSE;
	GError *error = NULL;
	GConfValue *gcv;

	gcv = gconf_client_get(gconf_client, key, &error);

	if (gcv == NULL) {
		mce_log((error != NULL) ? LL_WARN : LL_INFO,
			"Could not retrieve %s from GConf; %s",
			key, (error != NULL) ? error->message : "Key not set");
		goto EXIT;
	}

	if (gcv->type != GCONF_VALUE_BOOL) {
		mce_log(LL_ERR,
			"GConf key %s should have type: %d, but has type: %d",
			key, GCONF_VALUE_BOOL, gcv->type);
		goto EXIT;
	}

	*value = gconf_value_get_bool(gcv);

	status = TRUE;

EXIT:
	g_clear_error(&error);

	return status;
}

/**
 * Return an integer from the specified GConf key
 *
 * @param key The GConf key to get the value from
 * @param[out] value Will contain the value on return
 * @return TRUE on success, FALSE on failure
 */
gboolean mce_gconf_get_int(const gchar *const key, gint *value)
{
	gboolean status = FALSE;
	GError *error = NULL;
	GConfValue *gcv;

	gcv = gconf_client_get(gconf_client, key, &error);

	if (gcv == NULL) {
		mce_log((error != NULL) ? LL_WARN : LL_INFO,
			"Could not retrieve %s from GConf; %s",
			key, (error != NULL) ? error->message : "Key not set");
		goto EXIT;
	}

	if (gcv->type != GCONF_VALUE_INT) {
		mce_log(LL_ERR,
			"GConf key %s should have type: %d, but has type: %d",
			key, GCONF_VALUE_INT, gcv->type);
		goto EXIT;
	}

	*value = gconf_value_get_int(gcv);

	status = TRUE;

EXIT:
	g_clear_error(&error);

	return status;
}

/**
 * Add a GConf notifier
 *
 * @param path The GConf directory to watch
 * @param key The GConf key to add the notifier for
 * @param callback The callback function
 * @param[out] cb_id Will contain the callback ID on return
 * @return TRUE on success, FALSE on failure
 */
gboolean mce_gconf_notifier_add(const gchar *path, const gchar *key,
				const GConfClientNotifyFunc callback,
				guint *cb_id)
{
	GError *error = NULL;
	gboolean status = FALSE;

	gconf_client_add_dir(gconf_client, path,
			     GCONF_CLIENT_PRELOAD_NONE, &error);

	if (error != NULL) {
		mce_log(LL_CRIT,
			"Could not add %s to directories watched by "
			"GConf client setting from GConf; %s",
			path, error->message);
		//goto EXIT;
	}

	g_clear_error(&error);

	*cb_id = gconf_client_notify_add(gconf_client, key, callback,
					 NULL, NULL, &error);
	if (error != NULL) {
		mce_log(LL_CRIT,
			"Could not register notifier for %s; %s",
			key, error->message);
		//goto EXIT;
	}

	gconf_notifiers = g_slist_prepend(gconf_notifiers,
					  GINT_TO_POINTER(*cb_id));
	status = TRUE;

//EXIT:
	g_clear_error(&error);

	return status;
}

/**
 * Remove a GConf notifier
 *
 * @param cb_id The ID of the notifier to remove
 * @param user_data Unused
 */
void mce_gconf_notifier_remove(gpointer cb_id, gpointer user_data)
{
	(void)user_data;

	gconf_client_notify_remove(gconf_client, GPOINTER_TO_INT(cb_id));
	gconf_notifiers = g_slist_remove(gconf_notifiers, cb_id);
}

/**
 * Init function for the mce-gconf component
 *
 * @return TRUE on success, FALSE on failure
 */
gboolean mce_gconf_init(void)
{
	gboolean status = FALSE;

	/* Get the default GConf client */
	if ((gconf_client = gconf_client_get_default()) == FALSE) {
		mce_log(LL_CRIT, "Could not get default GConf client");
		goto EXIT;
	}

	status = TRUE;

EXIT:
	return status;
}

/**
 * Exit function for the mce-gconf component
 */
void mce_gconf_exit(void)
{
	if (gconf_client != NULL) {
		/* Free the list of GConf notifiers */
		if (gconf_notifiers != NULL) {
			g_slist_foreach(gconf_notifiers,
					(GFunc)mce_gconf_notifier_remove, NULL);
			gconf_notifiers = NULL;
		}

		/* Unreference GConf client */
		g_object_unref(gconf_client);
	}

	return;
}
