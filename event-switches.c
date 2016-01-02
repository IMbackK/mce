/**
 * @file event-switches.c
 * Switch event provider for the Mode Control Entity
 * <p>
 * Copyright © 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include <string.h>
#include "mce.h"
#include "event-switches.h"
#include "mce-io.h"
#include "datapipe.h"

/** ID for the lockkey I/O monitor */
static gconstpointer lockkey_iomon_id = NULL;

/** ID for the keyboard slide I/O monitor */
static gconstpointer kbd_slide_iomon_id = NULL;

/** ID for the cam focus I/O monitor */
static gconstpointer cam_focus_iomon_id = NULL;

/** ID for the cam launch I/O monitor */
static gconstpointer cam_launch_iomon_id = NULL;

/** ID for the lid cover I/O monitor */
static gconstpointer lid_cover_iomon_id = NULL;

/** ID for the proximity sensor I/O monitor */
static gconstpointer proximity_sensor_iomon_id = NULL;

static gconstpointer tahvo_usb_cable_iomon_id = NULL;

/** ID for the MUSB OMAP3 usb cable I/O monitor */
static gconstpointer musb_omap3_usb_cable_iomon_id = NULL;

/** ID for the mmc0 cover I/O monitor */
static gconstpointer mmc0_cover_iomon_id = NULL;

/** ID for the mmc cover I/O monitor */
static gconstpointer mmc_cover_iomon_id = NULL;

/** ID for the lens cover I/O monitor */
static gconstpointer lens_cover_iomon_id = NULL;

/** ID for the battery cover I/O monitor */
static gconstpointer bat_cover_iomon_id = NULL;

/** Cached call state */
static call_state_t call_state = CALL_STATE_INVALID;

/** Cached alarm UI state */
static alarm_ui_state_t alarm_ui_state = MCE_ALARM_UI_INVALID_INT32;

/** Does the device have a flicker key? */
gboolean has_flicker_key = FALSE;

/**
 * Generic I/O monitor callback that only generates activity
 *
 * @param data Unused
 * @param bytes_read Unused
 */
static void generic_activity_cb(gpointer data, gsize bytes_read)
{
	(void)data;
	(void)bytes_read;

	/* Generate activity */
	(void)execute_datapipe(&device_inactive_pipe, GINT_TO_POINTER(FALSE),
			       USE_INDATA, CACHE_INDATA);
}

/**
 * I/O monitor callback for the camera launch button
 *
 * @param data Unused
 * @param bytes_read Unused
 */
void camera_launch_button_cb(gpointer data, gsize bytes_read)
{
	camera_button_state_t camera_button_state;

	(void)bytes_read;

	if (!strncmp(data, MCE_CAM_LAUNCH_ACTIVE,
		     strlen(MCE_CAM_LAUNCH_ACTIVE))) {
		camera_button_state = CAMERA_BUTTON_LAUNCH;
	} else {
		camera_button_state = CAMERA_BUTTON_UNPRESSED;
	}

	/* Generate activity */
	(void)execute_datapipe(&device_inactive_pipe, GINT_TO_POINTER(FALSE),
			       USE_INDATA, CACHE_INDATA);

	/* Update camera button state */
	(void)execute_datapipe(&camera_button_pipe,
			       GINT_TO_POINTER(camera_button_state),
			       USE_INDATA, CACHE_INDATA);
}

/**
 * I/O monitor callback for the lock flicker key
 *
 * @param data The new data
 * @param bytes_read Unused
 */
void lockkey_cb(gpointer data, gsize bytes_read)
{
	gint lockkey_state;

	(void)bytes_read;

	if (!strncmp(data, MCE_FLICKER_KEY_ACTIVE,
		     strlen(MCE_FLICKER_KEY_ACTIVE))) {
		lockkey_state = 1;
	} else {
		lockkey_state = 0;
	}

	(void)execute_datapipe(&lockkey_pipe,
			       GINT_TO_POINTER(lockkey_state),
			       USE_INDATA, CACHE_INDATA);
}

/**
 * I/O monitor callback for the keyboard slide
 *
 * @param data The new data
 * @param bytes_read Unused
 */
void kbd_slide_cb(gpointer data, gsize bytes_read)
{
	cover_state_t slide_state;

	(void)bytes_read;

	if (!strncmp(data, MCE_KBD_SLIDE_OPEN, strlen(MCE_KBD_SLIDE_OPEN))) {
		slide_state = COVER_OPEN;

		/* Generate activity */
		if ((mce_get_submode_int32() & MCE_EVEATER_SUBMODE) == 0) {
			(void)execute_datapipe(&device_inactive_pipe,
					       GINT_TO_POINTER(FALSE),
					       USE_INDATA, CACHE_INDATA);
		}
	} else {
		slide_state = COVER_CLOSED;
	}

	(void)execute_datapipe(&keyboard_slide_pipe,
			       GINT_TO_POINTER(slide_state),
			       USE_INDATA, CACHE_INDATA);
}

/**
 * I/O monitor callback for the lid cover
 *
 * @param data The new data
 * @param bytes_read Unused
 */
static void lid_cover_cb(gpointer data, gsize bytes_read)
{
	cover_state_t lid_cover_state;

	(void)bytes_read;

	if (!strncmp(data, MCE_LID_COVER_OPEN, strlen(MCE_LID_COVER_OPEN))) {
		lid_cover_state = COVER_OPEN;

		/* Generate activity */
		(void)execute_datapipe(&device_inactive_pipe,
				       GINT_TO_POINTER(FALSE),
				       USE_INDATA, CACHE_INDATA);
	} else {
		lid_cover_state = COVER_CLOSED;
	}

	(void)execute_datapipe(&lid_cover_pipe,
			       GINT_TO_POINTER(lid_cover_state),
			       USE_INDATA, CACHE_INDATA);
}

/**
 * I/O monitor callback for the proximity sensor
 *
 * @param data The new data
 * @param bytes_read Unused
 */
void proximity_sensor_cb(gpointer data, gsize bytes_read)
{
	cover_state_t proximity_sensor_state;

	(void)bytes_read;

	if (!strncmp(data, MCE_PROXIMITY_SENSOR_OPEN,
		     strlen(MCE_PROXIMITY_SENSOR_OPEN))) {
		proximity_sensor_state = COVER_OPEN;
	} else {
		proximity_sensor_state = COVER_CLOSED;
	}

	(void)execute_datapipe(&proximity_sensor_pipe,
			       GINT_TO_POINTER(proximity_sensor_state),
			       USE_INDATA, CACHE_INDATA);
}

/**
 * I/O monitor callback for the USB cable
 *
 * @param data The new data
 * @param bytes_read Unused
 */
static void usb_cable_cb(gpointer data, gsize bytes_read)
{
	usb_cable_state_t cable_state;

	(void)bytes_read;

	if (!strncmp(data, MCE_TAHVO_USB_CABLE_CONNECTED,
		     strlen(MCE_TAHVO_USB_CABLE_CONNECTED)) ||
	    !strncmp(data, MCE_MUSB_USB_CABLE_CONNECTED,
		     strlen(MCE_MUSB_USB_CABLE_CONNECTED)) ||
	    !strncmp(data, MCE_MUSB_OMAP3_USB_CABLE_CONNECTED,
		     strlen(MCE_MUSB_OMAP3_USB_CABLE_CONNECTED))) {
		cable_state = USB_CABLE_CONNECTED;
	} else {
		cable_state = USB_CABLE_DISCONNECTED;
	}

	/* Generate activity */
	if ((mce_get_submode_int32() & MCE_EVEATER_SUBMODE) == 0) {
		(void)execute_datapipe(&device_inactive_pipe, GINT_TO_POINTER(FALSE),
				       USE_INDATA, CACHE_INDATA);
	}

	(void)execute_datapipe(&usb_cable_pipe,
			       GINT_TO_POINTER(cable_state),
			       USE_INDATA, CACHE_INDATA);
}

/**
 * I/O monitor callback for the lens cover
 *
 * @param data The new data
 * @param bytes_read Unused
 */
static void lens_cover_cb(gpointer data, gsize bytes_read)
{
	cover_state_t lens_cover_state;

	(void)bytes_read;

	if (!strncmp(data, MCE_LENS_COVER_OPEN, strlen(MCE_LENS_COVER_OPEN))) {
		lens_cover_state = COVER_OPEN;
	} else {
		lens_cover_state = COVER_CLOSED;
	}

	/* Generate activity */
	if ((mce_get_submode_int32() & MCE_EVEATER_SUBMODE) == 0) {
		(void)execute_datapipe(&device_inactive_pipe, GINT_TO_POINTER(FALSE),
				       USE_INDATA, CACHE_INDATA);
	}

	(void)execute_datapipe(&lens_cover_pipe,
			       GINT_TO_POINTER(lens_cover_state),
			       USE_INDATA, CACHE_INDATA);
}

/**
 * Update the proximity state
 *
 * @note Only gives reasonable readings when the proximity sensor is enabled
 * @return TRUE on success, FALSE on failure
 */
static gboolean update_proximity_sensor_state(void)
{
	cover_state_t proximity_sensor_state;
	gboolean status = FALSE;
	gchar *tmp = NULL;

	if (mce_read_string_from_file(MCE_PROXIMITY_SENSOR_STATE_PATH,
				      &tmp) == FALSE) {
		goto EXIT;
	}

	if (!strncmp(tmp, MCE_PROXIMITY_SENSOR_OPEN,
		     strlen(MCE_PROXIMITY_SENSOR_OPEN))) {
		proximity_sensor_state = COVER_OPEN;
	} else {
		proximity_sensor_state = COVER_CLOSED;
	}

	(void)execute_datapipe(&proximity_sensor_pipe,
			       GINT_TO_POINTER(proximity_sensor_state),
			       USE_INDATA, CACHE_INDATA);

	g_free(tmp);

EXIT:
	return status;
}

/**
 * Update the proximity monitoring
 */
static void update_proximity_monitor(void)
{
	if ((call_state == CALL_STATE_RINGING) ||
	    (call_state == CALL_STATE_ACTIVE) ||
	    (alarm_ui_state == MCE_ALARM_UI_VISIBLE_INT32) ||
	    (alarm_ui_state == MCE_ALARM_UI_RINGING_INT32)) {
		mce_write_string_to_file(MCE_PROXIMITY_SENSOR_DISABLE_PATH,
					 "0");
		(void)update_proximity_sensor_state();
	} else {
		mce_write_string_to_file(MCE_PROXIMITY_SENSOR_DISABLE_PATH,
					 "1");
	}
}

/**
 * Handle call state change
 *
 * @param data The call state stored in a pointer
 */
static void call_state_trigger(gconstpointer const data)
{
	call_state = GPOINTER_TO_INT(data);

	update_proximity_monitor();
}

/**
 * Handle alarm UI state change
 *
 * @param data The alarm state stored in a pointer
 */
static void alarm_ui_state_trigger(gconstpointer const data)
{
	alarm_ui_state = GPOINTER_TO_INT(data);

	update_proximity_monitor();
}

/**
 * Handle submode change
 *
 * @param data The submode stored in a pointer
 */
static void submode_trigger(gconstpointer data)
{
	static submode_t old_submode = MCE_NORMAL_SUBMODE;
	submode_t submode = GPOINTER_TO_INT(data);

	if ((submode & MCE_TKLOCK_SUBMODE) != 0) {
		if ((old_submode & MCE_TKLOCK_SUBMODE) == 0) {
			mce_write_string_to_file(MCE_CAM_FOCUS_DISABLE_PATH, "1");
			mce_write_string_to_file(MCE_CAM_LAUNCH_DISABLE_PATH, "1");
		}
	} else {
		if ((old_submode & MCE_TKLOCK_SUBMODE) != 0) {
			mce_write_string_to_file(MCE_CAM_LAUNCH_DISABLE_PATH, "0");
			mce_write_string_to_file(MCE_CAM_FOCUS_DISABLE_PATH, "0");
		}
	}

	old_submode = submode;
}

/**
 * Init function for the switches component
 *
 * @return TRUE on success, FALSE on failure
 */
gboolean mce_switches_init(void)
{
	gboolean status = FALSE;

	/* Append triggers/filters to datapipes */
	append_input_trigger_to_datapipe(&call_state_pipe,
					 call_state_trigger);
	append_input_trigger_to_datapipe(&alarm_ui_state_pipe,
					 alarm_ui_state_trigger);
	append_output_trigger_to_datapipe(&submode_pipe,
					  submode_trigger);

	/* Set default values, in case these are not available */
	(void)execute_datapipe(&lid_cover_pipe,
			       GINT_TO_POINTER(COVER_OPEN),
			       USE_INDATA, CACHE_INDATA);

	/* Register I/O monitors */
	// FIXME: error handling?
	lockkey_iomon_id =
		mce_register_io_monitor_string(-1,
					       MCE_FLICKER_KEY_STATE_PATH,
					       MCE_IO_ERROR_POLICY_IGNORE,
					       TRUE, lockkey_cb);
	kbd_slide_iomon_id =
		mce_register_io_monitor_string(-1,
					       MCE_KBD_SLIDE_STATE_PATH,
					       MCE_IO_ERROR_POLICY_IGNORE,
					       TRUE, kbd_slide_cb);
	cam_focus_iomon_id =
		mce_register_io_monitor_string(-1,
					       MCE_CAM_FOCUS_STATE_PATH,
					       MCE_IO_ERROR_POLICY_IGNORE,
					       TRUE, generic_activity_cb);
	cam_launch_iomon_id =
		mce_register_io_monitor_string(-1,
					       MCE_CAM_LAUNCH_STATE_PATH,
					       MCE_IO_ERROR_POLICY_IGNORE,
					       TRUE, camera_launch_button_cb);
	lid_cover_iomon_id =
		mce_register_io_monitor_string(-1,
					       MCE_LID_COVER_STATE_PATH,
					       MCE_IO_ERROR_POLICY_IGNORE,
					       TRUE, lid_cover_cb);
	proximity_sensor_iomon_id =
		mce_register_io_monitor_string(-1,
					       MCE_PROXIMITY_SENSOR_STATE_PATH,
					       MCE_IO_ERROR_POLICY_IGNORE,
					       TRUE, proximity_sensor_cb);
	musb_omap3_usb_cable_iomon_id =
		mce_register_io_monitor_string(-1,
					       MCE_MUSB_OMAP3_USB_CABLE_STATE_PATH,
					       MCE_IO_ERROR_POLICY_IGNORE,
					       TRUE, usb_cable_cb);

	tahvo_usb_cable_iomon_id =
		mce_register_io_monitor_string(-1,
					       MCE_TAHVO_USB_CABLE_STATE_PATH,
					       MCE_IO_ERROR_POLICY_IGNORE,
					       TRUE, usb_cable_cb);
	lens_cover_iomon_id =
		mce_register_io_monitor_string(-1,
					       MCE_LENS_COVER_STATE_PATH,
					       MCE_IO_ERROR_POLICY_IGNORE,
					       TRUE, lens_cover_cb);
	mmc0_cover_iomon_id =
		mce_register_io_monitor_string(-1,
					       MCE_MMC0_COVER_STATE_PATH,
					       MCE_IO_ERROR_POLICY_IGNORE,
					       TRUE, generic_activity_cb);
	mmc_cover_iomon_id =
		mce_register_io_monitor_string(-1,
					       MCE_MMC_COVER_STATE_PATH,
					       MCE_IO_ERROR_POLICY_IGNORE,
					       TRUE, generic_activity_cb);
	bat_cover_iomon_id =
		mce_register_io_monitor_string(-1,
					       MCE_BATTERY_COVER_STATE_PATH,
					       MCE_IO_ERROR_POLICY_IGNORE,
					       TRUE, generic_activity_cb);

	update_proximity_monitor();

	if (lockkey_iomon_id != NULL)
		has_flicker_key = TRUE;

	status = TRUE;

	return status;
}

/**
 * Exit function for the switches component
 */
void mce_switches_exit(void)
{
	/* Remove triggers/filters from datapipes */
	remove_output_trigger_from_datapipe(&submode_pipe,
					    submode_trigger);
	remove_input_trigger_from_datapipe(&alarm_ui_state_pipe,
					   alarm_ui_state_trigger);
	remove_input_trigger_from_datapipe(&call_state_pipe,
					   call_state_trigger);

	/* Unregister I/O monitors */
	mce_unregister_io_monitor(bat_cover_iomon_id);
	mce_unregister_io_monitor(mmc_cover_iomon_id);
	mce_unregister_io_monitor(mmc0_cover_iomon_id);
	mce_unregister_io_monitor(lens_cover_iomon_id);
	mce_unregister_io_monitor(tahvo_usb_cable_iomon_id);
	mce_unregister_io_monitor(proximity_sensor_iomon_id);
	mce_unregister_io_monitor(lid_cover_iomon_id);
	mce_unregister_io_monitor(cam_launch_iomon_id);
	mce_unregister_io_monitor(cam_focus_iomon_id);
	mce_unregister_io_monitor(kbd_slide_iomon_id);
	mce_unregister_io_monitor(lockkey_iomon_id);

	return;
}
