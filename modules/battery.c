/**
 * @file battery.c
 * Battery module -- this implements battery and charger logic for MCE
 * <p>
 * Copyright © 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include <gmodule.h>
#include <bme-dbus-proxy/dbus-names.h>
#include "mce.h"
#include "mce-log.h"
#include "mce-dbus.h"
#include "datapipe.h"

/** Module name */
#define MODULE_NAME		"battery"

/** Functionality provided by this module */
static const gchar *const provides[] = { MODULE_NAME, NULL };

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 100
};

/**
 * D-Bus callback for the battery full signal
 *
 * @param msg The D-Bus message
 * @return TRUE on success, FALSE on failure
 */
static gboolean battery_full_dbus_cb(DBusMessage *const msg)
{
	gboolean status = FALSE;

	(void)msg;

	mce_log(LL_DEBUG,
		"Received battery full signal");

	execute_datapipe_output_triggers(&led_pattern_deactivate_pipe, MCE_LED_PATTERN_BATTERY_CHARGING, USE_INDATA);
	execute_datapipe_output_triggers(&led_pattern_activate_pipe, MCE_LED_PATTERN_BATTERY_FULL, USE_INDATA);

	execute_datapipe(&battery_status_pipe,
			 GINT_TO_POINTER(BATTERY_STATUS_FULL),
			 USE_INDATA, CACHE_INDATA);

	status = TRUE;

//EXIT:
	return status;
}

/**
 * D-Bus callback for the battery low signal
 *
 * @param msg The D-Bus message
 * @return TRUE on success, FALSE on failure
 */
static gboolean battery_low_dbus_cb(DBusMessage *const msg)
{
	gboolean status = FALSE;

	(void)msg;

	mce_log(LL_DEBUG,
		"Received battery low signal");

//	execute_datapipe_output_triggers(&led_pattern_activate_pipe, MCE_LED_PATTERN_BATTERY_LOW, USE_INDATA);

	execute_datapipe(&battery_status_pipe,
			 GINT_TO_POINTER(BATTERY_STATUS_LOW),
			 USE_INDATA, CACHE_INDATA);

	status = TRUE;

//EXIT:
	return status;
}

/**
 * D-Bus callback for the battery empty signal
 *
 * @param msg The D-Bus message
 * @return TRUE on success, FALSE on failure
 */
static gboolean battery_empty_dbus_cb(DBusMessage *const msg)
{
	gboolean status = FALSE;

	(void)msg;

	mce_log(LL_INFO,
		"Received battery empty signal");

	execute_datapipe(&battery_status_pipe,
			 GINT_TO_POINTER(BATTERY_STATUS_EMPTY),
			 USE_INDATA, CACHE_INDATA);

	status = TRUE;

//EXIT:
	return status;
}

/**
 * D-Bus callback for the charger_charging_on signal
 *
 * @param msg The D-Bus message
 * @return TRUE on success, FALSE on failure
 */
static gboolean charger_charging_on_dbus_cb(DBusMessage *const msg)
{
	gboolean old_charger_state = datapipe_get_gbool(charger_state_pipe);
	gboolean status = FALSE;

	(void)msg;

	mce_log(LL_DEBUG,
		"Received charger_charging_on signal");

	/* Only update the charger state if needed */
	if (old_charger_state == FALSE) {
		execute_datapipe(&charger_state_pipe, GINT_TO_POINTER(TRUE),
				 USE_INDATA, CACHE_INDATA);
	}

	/* In case these are active; there's no harm to call them anyway */
	execute_datapipe_output_triggers(&led_pattern_deactivate_pipe, MCE_LED_PATTERN_BATTERY_FULL, USE_INDATA);
//	execute_datapipe_output_triggers(&led_pattern_deactivate_pipe, MCE_LED_PATTERN_BATTERY_LOW, USE_INDATA);

	execute_datapipe_output_triggers(&led_pattern_activate_pipe, MCE_LED_PATTERN_BATTERY_CHARGING, USE_INDATA);

	status = TRUE;

//EXIT:
	return status;
}

/**
 * D-Bus callback for the charger_charging_off signal
 *
 * @param msg The D-Bus message
 * @return TRUE on success, FALSE on failure
 */
static gboolean charger_charging_off_dbus_cb(DBusMessage *const msg)
{
	gboolean old_charger_state = datapipe_get_gbool(charger_state_pipe);
	gboolean status = FALSE;

	(void)msg;

	mce_log(LL_DEBUG,
		"Received charger_charging_off signal");

	/* Only update the charger state if needed */
	if (old_charger_state == TRUE) {
		execute_datapipe(&charger_state_pipe, GINT_TO_POINTER(FALSE),
				 USE_INDATA, CACHE_INDATA);
	}

	/* In case these are active; there's no harm to call them anyway */
	execute_datapipe_output_triggers(&led_pattern_deactivate_pipe, MCE_LED_PATTERN_BATTERY_CHARGING, USE_INDATA);

	status = TRUE;

//EXIT:
	return status;
}

/**
 * D-Bus callback for the charger_charging_failed signal
 *
 * @param msg The D-Bus message
 * @return TRUE on success, FALSE on failure
 */
static gboolean charger_charging_failed_dbus_cb(DBusMessage *const msg)
{
	gboolean old_charger_state = datapipe_get_gbool(charger_state_pipe);
	gboolean status = FALSE;

	(void)msg;

	mce_log(LL_DEBUG,
		"Received charger_charging_failed signal");

	/* Only update the charger state if needed */
	if (old_charger_state == TRUE) {
		execute_datapipe(&charger_state_pipe, GINT_TO_POINTER(FALSE),
				 USE_INDATA, CACHE_INDATA);
	}

	/* In case these are active; there's no harm to call them anyway */
	execute_datapipe_output_triggers(&led_pattern_deactivate_pipe, MCE_LED_PATTERN_BATTERY_FULL, USE_INDATA);
	execute_datapipe_output_triggers(&led_pattern_deactivate_pipe, MCE_LED_PATTERN_BATTERY_CHARGING, USE_INDATA);

	/* Generate activity */
	(void)execute_datapipe(&device_inactive_pipe, GINT_TO_POINTER(FALSE),
			       USE_INDATA, CACHE_INDATA);

	status = TRUE;

//EXIT:
	return status;
}

/**
 * D-Bus callback for the charger_connected signal
 *
 * @param msg The D-Bus message
 * @return TRUE on success, FALSE on failure
 */
static gboolean charger_connected_dbus_cb(DBusMessage *const msg)
{
	gboolean status = FALSE;

	(void)msg;

	mce_log(LL_DEBUG,
		"Received charger_connected signal");

	/* Generate activity */
	(void)execute_datapipe(&device_inactive_pipe, GINT_TO_POINTER(FALSE),
			       USE_INDATA, CACHE_INDATA);

	status = TRUE;

//EXIT:
	return status;
}

/**
 * D-Bus callback for the charger_disconnected signal
 *
 * @param msg The D-Bus message
 * @return TRUE on success, FALSE on failure
 */
static gboolean charger_disconnected_dbus_cb(DBusMessage *const msg)
{
	gboolean old_charger_state = datapipe_get_gbool(charger_state_pipe);
	gboolean status = FALSE;

	(void)msg;

	mce_log(LL_DEBUG,
		"Received charger_disconnected signal");

	/* Only update the charger state if needed */
	if (old_charger_state == TRUE) {
		execute_datapipe(&charger_state_pipe, GINT_TO_POINTER(FALSE),
				 USE_INDATA, CACHE_INDATA);
	}

	/* In case these are active; there's no harm to call them anyway */
	execute_datapipe_output_triggers(&led_pattern_deactivate_pipe, MCE_LED_PATTERN_BATTERY_FULL, USE_INDATA);
	execute_datapipe_output_triggers(&led_pattern_deactivate_pipe, MCE_LED_PATTERN_BATTERY_CHARGING, USE_INDATA);

	/* Generate activity */
	(void)execute_datapipe(&device_inactive_pipe, GINT_TO_POINTER(FALSE),
			       USE_INDATA, CACHE_INDATA);

	status = TRUE;

//EXIT:
	return status;
}

/**
 * Request update of charger status
 *
 * @return TRUE on success, FALSE on failure
 */
static gboolean request_charger_status(void)
{
	return dbus_send(BME_SERVICE, BME_REQUEST_PATH, BME_REQUEST_IF,
			 BME_STATUS_INFO_REQ, NULL, DBUS_TYPE_INVALID);
}

/**
 * Init function for the battery and charger module
 *
 * @todo XXX status needs to be set on error!
 *
 * @param module Unused
 * @return NULL on success, a string with an error message on failure
 */
G_MODULE_EXPORT const gchar *g_module_check_init(GModule *module);
const gchar *g_module_check_init(GModule *module)
{
	(void)module;

	/* battery_full */
	if (mce_dbus_handler_add(BME_SIGNAL_IF,
				 BME_BATTERY_FULL,
				 NULL,
				 DBUS_MESSAGE_TYPE_SIGNAL,
				 battery_full_dbus_cb) == NULL)
		goto EXIT;

	/* battery_low */
	if (mce_dbus_handler_add(BME_SIGNAL_IF,
				 BME_BATTERY_LOW,
				 NULL,
				 DBUS_MESSAGE_TYPE_SIGNAL,
				 battery_low_dbus_cb) == NULL)
		goto EXIT;

	/* battery_empty */
	if (mce_dbus_handler_add(BME_SIGNAL_IF,
				 BME_BATTERY_EMPTY,
				 NULL,
				 DBUS_MESSAGE_TYPE_SIGNAL,
				 battery_empty_dbus_cb) == NULL)
		goto EXIT;

	/* charger_charging_on */
	if (mce_dbus_handler_add(BME_SIGNAL_IF,
				 BME_CHARGER_CHARGING_ON,
				 NULL,
				 DBUS_MESSAGE_TYPE_SIGNAL,
				 charger_charging_on_dbus_cb) == NULL)
		goto EXIT;

	/* charger_charging_off */
	if (mce_dbus_handler_add(BME_SIGNAL_IF,
				 BME_CHARGER_CHARGING_OFF,
				 NULL,
				 DBUS_MESSAGE_TYPE_SIGNAL,
				 charger_charging_off_dbus_cb) == NULL)
		goto EXIT;

	/* charger_charging_failed */
	if (mce_dbus_handler_add(BME_SIGNAL_IF,
				 BME_CHARGER_CHARGING_FAILED,
				 NULL,
				 DBUS_MESSAGE_TYPE_SIGNAL,
				 charger_charging_failed_dbus_cb) == NULL)
		goto EXIT;

	/* charger_connected */
	if (mce_dbus_handler_add(BME_SIGNAL_IF,
				 BME_CHARGER_CONNECTED,
				 NULL,
				 DBUS_MESSAGE_TYPE_SIGNAL,
				 charger_connected_dbus_cb) == NULL)
		goto EXIT;

	/* charger_disconnected */
	if (mce_dbus_handler_add(BME_SIGNAL_IF,
				 BME_CHARGER_DISCONNECTED,
				 NULL,
				 DBUS_MESSAGE_TYPE_SIGNAL,
				 charger_disconnected_dbus_cb) == NULL)
		goto EXIT;

	/* Update charger status */
	request_charger_status();

EXIT:
	return NULL;
}

/**
 * Exit function for the battery and charger module
 *
 * @todo D-Bus unregistration
 *
 * @param module Unused
 */
G_MODULE_EXPORT void g_module_unload(GModule *module);
void g_module_unload(GModule *module)
{
	(void)module;

	return;
}
